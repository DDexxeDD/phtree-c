#include <stdbool.h>
#include <stdint.h>

#include "cvector.h"

#include "phtree.h"

/*
 * count leading and trailing zeroes
 */

#if defined (_MSC_VER)
// need this for _BitScanReverse and _BitScanForward64
#include <intrin.h>
#endif

#if defined (_MSC_VER)
inline uint64_t count_leading_zeoes_msvc (uint64_t bit_string)
{
	unsigned long leading_zero = 0;
	return _BitScanReverse64 (&leading_zero, bit_string) ? 63 - leading_zero : 64U;
}

inline uint64_t count_trailing_zeroes_msvc (uint64_t bit_string)
{
	unsigned long trailing_zero = 0;
	return _BitScaneForward64 (&trailing_zero, bit_string) ? trailing_zero : 64U;
}
#endif

inline uint64_t count_leading_zeroes_local (uint64_t bit_string)
{
	if (bit_string == 0)
	{
		return 64;
	}

	uint64_t n = 1;
	uint32_t x = (bit_string >> 32);

	if (x == 0)
	{
		n += 32;
		x = (int) bit_string;
	}

	if (x >> 16 == 0)
	{
		n += 16;
		x <<= 16;
	}

	if (x >> 24 == 0)
	{
		n += 8;
		x <<= 8;
	}

	if (x >> 28 == 0)
	{
		n += 4;
		x <<= 4;
	}

	if (x >> 30 == 0)
	{
		n += 2;
		x <<= 2;
	}

	n -= x >> 31;

	return n;
}

inline uint64_t count_trailing_zeroes_local (uint64_t bit_string)
{
	if (bit_string == 0)
	{
		return 64;
	}

	uint32_t x = 0;
	uint32_t y = 0;
	uint16_t n = 63;

	y = (uint32_t) bit_string;

	if (y != 0)
	{
		n = n - 32;
		x = y;
	}
	else
	{
		x = (uint32_t) (bit_string >> 32);
	}

	y = x << 16;

	if (y != 0)
	{
		n = n - 16;
		x = y;
	}

	y = x << 8;

	if (y != 0)
	{
		n = n - 8;
		x = y;
	}

	y = x << 4;

	if (y != 0)
	{
		n = n - 4;
		x = y;
	}

	y = x << 2;

	if (y != 0)
	{
		n = n - 2;
		x = y;
	}

	return n - ((x << 1) >> 31);
}

#if defined (__clang__) || defined (__GNUC__)
#define count_leading_zeroes(bit_string) (0 ? 64U : __builtin_clzll (bit_string))
#define count_trailing_zeroes(bit_string) (0 ? 64U : __builtin_ctzll (bit_string))
#elif defined (_MSC_VER)
inline uint64_t count_leading_zeoes_msvc (uint64_t bit_string);
inline uint64_t count_trailing_zeroes_msvc (uint64_t bit_string);
#define count_leading_zeroes(bit_string) count_leading_zeoes_msvc (bit_string)
#define count_trailing_zeroes(bit_string) count_trailing_zeroes_msvc (bit_string)
#else
inline uint64_t count_leading_zeroes_local (uint64_t bit_string);
inline uint64_t count_trailing_zeroes_local (uint64_t bit_string);
#define count_leading_zeroes(bit_string) count_leading_zeroes_local (bit_string)
#define count_trailing_zeroes(bit_string) count_trailing_zeroes_local (bit_string)
#endif

// this is only a part of this implementation to keep track of nodes when printing
char node_id = 'A';

#define node_is_leaf(node) ((node)->postfix_length == 0)
#define node_is_root(node) ((node)->parent == NULL)

// XXX
// 	in a hypercube we expect bits set to 0 to be less than bits set to 1
// 	the sign bit in floating point does not work that way
// 		1 is negative
// 	the sign bit needs to be flipped
// 	negative numbers in general also do not work like this
// 	negative numbers are stored the same as positive numbers
// 		except with the sign bit set to 1
// 	this is a problem because when the sign bit is flipped
// 		negative numbers behave the same as positive numbers
// 	which is to say -3 should be less than -2
// 	but
// 	when the sign bit is flipped -3 will now be greater than -2
// 		since the numbers have just been changed to positive (3 > 2)
// 	this is easily fixed however
// 	we can just invert all of the bits of a negative number
// 	and it will be correctly set for hypercube functionality
//
// to fully support double keys
// 	use BIT_WIDTH = 64
//
//	+infinity will be greater than all other numbers
// -infinity will be less than all other numbers
// +nan will be greater than +infinity
// -nan will be less than -infinity
// -0 is converted to +0
static inline phtree_key_t double_to_key (double x)
{
	phtree_key_t bits;

	memcpy (&bits, &x, sizeof (bits));
	// flip sign bit if value is positive
	if (x >= 0)
	{
		// handle negative zero by converting it to positive zero
		bits = bits & (KEY_MAX >> 1);
		bits ^= (KEY_ONE << (BIT_WIDTH - 1));
	}
	// invert everything if value is negative
	else
	{
		bits ^= KEY_MAX;
	}

	return bits;
}

// point_a >= point_b
// 	_all_ of point_a's dimensions must be greater than or equal to point_b's dimensions
// 		for point_a to be greater than or equal to point_b
static inline bool point_greater_equal (phtree_point_t* point_a, phtree_point_t* point_b)
{
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		if (point_a->values[dimension] < point_b->values[dimension])
		{
			return false;
		}
	}

	return true;
}

// point_a <= point_b
// 	_all_ of point_a's dimensions must be less than or equal to point_b's dimensions
// 		for point_a to be less than or equal to point_b
static inline bool point_less_equal (phtree_point_t* point_a, phtree_point_t* point_b)
{
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		if (point_a->values[dimension] > point_b->values[dimension])
		{
			return false;
		}
	}

	return true;
}

static inline bool point_equal (phtree_point_t* point_a, phtree_point_t* point_b)
{
	phtree_key_t result = 0;
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		result |= point_a->values[dimension] - point_b->values[dimension];
	}

	return (result == 0);
}

static inline bool point_not_equal (phtree_point_t* point_a, phtree_point_t* point_b)
{
	return !point_equal (point_a, point_b);
}

// checks if all the bits before postfix_length are >=
// 	used in window queries
static inline bool prefix_greater_equal (phtree_point_t* point_a, phtree_point_t* point_b, int postfix_length)
{
	phtree_point_t local_a = *point_a;
	phtree_point_t local_b = *point_b;

	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		local_a.values[dimension] >>= postfix_length + 1;
		local_b.values[dimension] >>= postfix_length + 1;
	}

	return (point_greater_equal (&local_a, &local_b));
}

// checks if all the bits before postfix_length are <=
// 	used in window queries
static inline bool prefix_less_equal (phtree_point_t* point_a, phtree_point_t* point_b, int postfix_length)
{
	phtree_point_t local_a = *point_a;
	phtree_point_t local_b = *point_b;

	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		local_a.values[dimension] >>= postfix_length + 1;
		local_b.values[dimension] >>= postfix_length + 1;
	}

	return (point_less_equal (&local_a, &local_b));
}

static inline bool point_in_window (phtree_point_t* point, phtree_window_query_t* window, int postfix_length)
{
	return (point_greater_equal (point, &window->min) && point_less_equal (point, &window->max));
}

static inline bool node_in_window (phtree_node_t* node, phtree_window_query_t* window)
{
	return (prefix_greater_equal (&node->point, &window->min, node->postfix_length) && prefix_less_equal (&node->point, &window->max, node->postfix_length));
}

static inline bool entry_in_window (phtree_entry_t* entry, phtree_window_query_t* window)
{
	return (point_greater_equal (&entry->point, &window->min) && point_less_equal (&entry->point, &window->max));
}

/*
 * calculate the hypercube address of the point at the given node
 */
hypercube_address_t calculate_hypercube_address (phtree_point_t* point, phtree_node_t* node)
{
	// which bit in the point->values we are interested in
	uint64_t bit_mask = KEY_ONE << node->postfix_length;
	hypercube_address_t address = 0;

	// for each dimension
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		// every time we process a dimension
		// 	we need to move the current value of address to make room for the new dimension
		// when address == 0
		// 	this does nothing
		address <<= 1;
		// calculate zero or 1 at the bit_mask position
		// then move that value to the bottom of the bits
		// add that value to the address
		// 	which we have already shifted to make room
		address |= (bit_mask & point->values[dimension]) >> node->postfix_length;
	}

	return address;
}

void entry_free (phtree_entry_t* entry)
{
	cvector_free (entry->elements);
	free (entry);
}

// insert a phtree_entry_t in a node
void node_add_entry (phtree_node_t* node, phtree_point_t* point, int value)
{
	hypercube_address_t address = calculate_hypercube_address (point, node);

	// if there is already an entry at address
	// 	push value to the entry elements
	if (node->children[address])
	{
		phtree_entry_t* child = node->children[address];
		cvector_push_back (child->elements, value);

		return;
	}

	// if there is _not_ an entry at address
	// 	create a new entry
	phtree_entry_t* new_entry = calloc (1, sizeof (*new_entry));

	new_entry->point = *point;
	new_entry->parent = node;
	new_entry->elements = NULL;
	cvector_init (new_entry->elements, 1, NULL);
	cvector_push_back (new_entry->elements, value);
	node->children[address] = new_entry;
	node->child_count++;
}

/*
 * create a new node
 */
phtree_node_t* node_create (phtree_node_t* parent, uint16_t infix_length, uint16_t postfix_length, phtree_point_t* point, hypercube_address_t address, int value)
{
	phtree_node_t* new_node = calloc (1, sizeof (*new_node));

	if (!new_node)
	{
		return NULL;
	}

	new_node->parent = parent;
	new_node->child_count = 0;
	new_node->infix_length = infix_length;
	new_node->postfix_length = postfix_length;
	new_node->point = *point;
	new_node->id = node_id;
	node_id++;

	phtree_key_t key_mask = KEY_MAX << (postfix_length + 1);

	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		// set the new node's postfix bits to 0
		new_node->point.values[dimension] &= key_mask;
		// set the bits at node to 1
		// 	this makes the node->point the center of the node
		// 	which is useful later in window queries
		new_node->point.values[dimension] |= KEY_ONE << postfix_length;
	}

	if (postfix_length == 0)
	{
		node_add_entry (new_node, point, value);
	}

	return new_node;
}

// try to add a new child node to node
// 	if the node already has a child at the address
// 		return that existing node and set success to false
phtree_node_t* node_try_add (phtree_node_t* node, bool* success, hypercube_address_t address, phtree_point_t* point, int value)
{
	bool added = false;

	if (!node->children[address])
	{
		// if we are creating an entirely new child node
		// 	because this is a patricia trie
		// 		the child is going to be all the way at the bottom of the tree
		// 			postfix = 0  // there will only be entries below this node, no other nodes
		node->children[address] = node_create (node, node->postfix_length - 1, 0, point, address, value);
		node->child_count++;
		added = true;
	}

	// report if we added a new child
	if (success)
	{
		*success = added;
	}

	return node->children[address];
}

/*
 * return the bit at which the two points diverge
 */
int number_of_diverging_bits (phtree_point_t* point_a, phtree_point_t* point_b)
{
	unsigned int difference = 0;

	for (size_t dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		difference |= (point_a->values[dimension] ^ point_b->values[dimension]);
	}

	// count_leading_zeroes always uses the 64 bit implementation
	// 	and will return a number based on a 64 bit input
	// 	so we use BIT_WIDTH_MAX instead of BIT_WIDTH
	return BIT_WIDTH_MAX - count_leading_zeroes (difference);
}

/*
 * insert a new node between existing nodes
 */
phtree_node_t* node_insert_split (phtree_node_t* node, phtree_node_t* sub_node, phtree_point_t* point, int max_conflicting_bits, int value)
{
	phtree_node_t* new_node = node_create (node, node->postfix_length - max_conflicting_bits, max_conflicting_bits - 1, point, 0, 0);

	node->children[calculate_hypercube_address (point, node)] = new_node;
	new_node->children[calculate_hypercube_address (&sub_node->point, new_node)] = sub_node;
	new_node->child_count++;

	sub_node->infix_length = (new_node->postfix_length - sub_node->postfix_length) - 1;

	return new_node;
}

// figure out what to do when trying to add a new node where a node already exists
phtree_node_t* node_handle_collision (bool* added, phtree_node_t* node, phtree_node_t* sub_node, phtree_point_t* point, int value)
{
	if (node_is_leaf (sub_node))
	{
		if (sub_node->infix_length > 0)
		{
			int max_conflicting_bits = number_of_diverging_bits (point, &sub_node->point);

			if (max_conflicting_bits > 1)
			{
				return node_insert_split (node, sub_node, point, max_conflicting_bits, value);
			}
		}

		// if we are in a leaf and we are _not_creating a split
		// 	create a new entry
		node_add_entry (sub_node, point, value);
		*added = true;
	}
	else
	{
		if (sub_node->infix_length > 0)
		{
			int max_conflicting_bits = number_of_diverging_bits (point, &sub_node->point);

			if (max_conflicting_bits > sub_node->postfix_length + 1)
			{
				return node_insert_split (node, sub_node, point, max_conflicting_bits, value);
			}
		}
	}

	return sub_node;
}

/*
 * add a new node to the tree
 */
phtree_node_t* node_add (bool* added, phtree_node_t* node, phtree_point_t* point, int value)
{
	hypercube_address_t address = calculate_hypercube_address (point, node);
	bool added_new_node = false;
	phtree_node_t* sub_node = node_try_add (node, &added_new_node, address, point, value);

	if (added_new_node)
	{
		*added = true;
		return sub_node;
	}

	return node_handle_collision (&added_new_node, node, sub_node, point, value);
}

/*
 * set default tree values
 */
void tree_initialize (phtree_t* tree)
{
	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		tree->root.children[iter] = NULL;
	}

	tree->root.parent = NULL;
	tree->root.postfix_length = BIT_WIDTH - 1;
	tree->root.id = node_id;
	node_id++;
}

/*
 * create a new tree
 */
phtree_t* tree_create ()
{
	phtree_t* tree = calloc (1, sizeof (*tree));

	if (!tree)
	{
		return NULL;
	}

	tree_initialize (tree);

	return tree;
}

// recursively free _ALL_ of the nodes under and including the argument node
// 	dont use this on the root node
void tree_free_nodes (phtree_node_t* node)
{
	if (!node)
	{
		return;
	}

	if (node_is_leaf (node))
	{
		for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
		{
			if (node->children[iter])
			{
				entry_free (node->children[iter]);
			}
		}

		free (node);

		return;
	}

	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		if (node->children[iter])
		{
			// do this recursively
			// worst case our stack is 64 deep
			tree_free_nodes (node->children[iter]);
		}
	}

	free (node);
}

/*
 * free all of the nodes and entries in the tree
 */
void tree_clear (phtree_t* tree)
{
	if (!tree)
	{
		return;
	}

	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		if (tree->root.children[iter])
		{
			tree_free_nodes (tree->root.children[iter]);
			tree->root.children[iter] = NULL;
		}
	}
}

/*
 * insert an entry in the tree at point
 * 
 * if an entry already exists at point
 * 	that entry will be returned
 */
phtree_entry_t* tree_insert (phtree_t* tree, phtree_point_t* point, int value)
{
	phtree_node_t* current_node = &tree->root;
	bool added_new_node = false;

	while (!node_is_leaf (current_node))
	{
		current_node = node_add (&added_new_node, current_node, point, value);
	}

	return current_node->children[calculate_hypercube_address (point, current_node)];
}

/*
 * find an entry at a specific point
 * returns NULL if there is no entry at the point
 */
phtree_entry_t* tree_find (phtree_t* tree, phtree_point_t* point)
{
	phtree_node_t* current_node = &tree->root;

	while (current_node && !node_is_leaf (current_node))
	{
		current_node = current_node->children[calculate_hypercube_address (point, current_node)];
	}

	if (!current_node)
	{
		return NULL;
	}

	return current_node->children[calculate_hypercube_address (point, current_node)];
}

/*
 * check if there is an entry at the given point
 */
bool tree_point_exists (phtree_t* tree, phtree_point_t* point)
{
	phtree_entry_t* entry = tree_find (tree, point);

	// we could just return entry
	// 	but lets return an actual bool
	return (entry != NULL);
}

/*
 * remove the entry at the given point
 */
void tree_remove (phtree_t* tree, phtree_point_t* point)
{
	phtree_entry_t* entry = tree_find (tree, point);

	if (!entry)
	{
		return;
	}

	phtree_node_t* current_node = entry->parent;
	hypercube_address_t address = calculate_hypercube_address (point, current_node);

	entry_free (entry);

	current_node->children[address] = NULL;
	current_node->child_count--;

	// entries are all children of leaf nodes
	// 	if the leaf node child_count == 0
	// 	we can delete the leaf node
	if (current_node->child_count == 0)
	{
		phtree_node_t* parent = current_node->parent;

		parent->children[calculate_hypercube_address (&current_node->point, parent)] = NULL;
		parent->child_count--;
		free (current_node);

		// deleting the leaf node means we have to go up the tree
		// 	and possibly delete or merge other nodes
		current_node = parent;
		parent = current_node->parent;
		while (!node_is_root (current_node))
		{
			// if the current_node only has 1 child
			// 	we can get rid of current_node
			// 	and have current_node->parent point directly at current_node's 1 child
			if (current_node->child_count == 1)
			{
				phtree_node_t* child = NULL;

				for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
				{
					if (current_node->children[iter])
					{
						child = current_node->children[iter];
						break;
					}
				}

				parent->children[calculate_hypercube_address (&current_node->point, parent)] = child;
				free (current_node);

				current_node = parent;
				parent = current_node->parent;
			}
			// XXX
			// 	we dont need to check if current_node->child_count == 0
			// 		because that would imply that we had a split node which didnt split anything
			// 			and only had a single child
			// 		such a node shouldnt exist
			// 			it should have been removed before getting here

			// if current_node->child_count > 1
			// 	nothing further will change up the tree
			// 		so we can just break the loop
			else
			{
				break;
			}
		}
	}
}

/*
 * remove a specific element from the entry at point
 * if there are multiple copies of the element in the entry
 * 	only the first one will be removed
 */
void tree_remove_element (phtree_t* tree, phtree_point_t* point, int element)
{
	phtree_entry_t* entry = tree_find (tree, point);

	if (!entry)
	{
		return;
	}

	for (int iter = 0; iter < cvector_size (entry->elements); iter++)
	{
		if (entry->elements[iter] == element)
		{
			int last = cvector_size (entry->elements) - 1;

			// we dont care about the order of elements in the entry
			// 	when we remove an element
			// 		copy the last element into the removed element's place
			// 		so cvector doesnt do a big memcpy every time we remove an element
			if (iter < last)
			{
				entry->elements[iter] = entry->elements[last];
			}

			cvector_erase (entry->elements, last);

			break;
		}
	}
}

/*
 * check if the tree is empty
 */
bool tree_empty (phtree_t* tree)
{
	return (tree->root.child_count == 0);
}

/*
 * run a window query on a specific node
 */
void node_query_window (phtree_node_t* node, phtree_window_query_t* query)
{
	if (!node_in_window (node, query))
	{
		return;
	}

	/*
	 * these masks are used to accelerate queries
	 * 	when iterating children
	 * 		we can do a broad check if a child node overlaps the query window at all
	 * 		without needing to go to the child node and performing node_in_window
	 * 		
	 * 	if the child node does not overlap the query window
	 * 		we save a memory jump to that node
	 */
	phtree_key_t mask_lower = 0;
	phtree_key_t mask_upper = 0;

	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		/*
		 * for these >= to work properly
		 * 	node->point has to be set to the mid point of the node
		 * 	we set node->point to the mid point, during node creation
		 * 		so we dont have to calculate it here
		 */
		mask_lower <<= 1;
		mask_lower |= query->min.values[dimension] >= node->point.values[dimension];

		mask_upper <<= 1;
		mask_upper |= query->max.values[dimension] >= node->point.values[dimension];
	}

	if (node_is_leaf (node))
	{
		for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
		{
			phtree_entry_t* child = node->children[iter];

			if (child && ((iter | mask_lower) & mask_upper) == iter)
			{
				if (entry_in_window (child, query))
				{
					cvector_push_back (query->entries, child);
				}
			}
		}

		return;
	}

	// if the node _is_ in the window and _is not_ a leaf
	// 	recurse through the node's children
	for (unsigned int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		if (node->children[iter] && ((iter | mask_lower) & mask_upper) == iter)
		{
			node_query_window (node->children[iter], query);
		}
	}
}

/*
 * run a window query on a tree
 */
void tree_query_window (phtree_t* tree, phtree_window_query_t* query)
{
	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		if (tree->root.children[iter])
		{
			node_query_window (tree->root.children[iter], query);
		}
	}
}

/*
 * create a new window query
 */
phtree_window_query_t* window_query_create (phtree_point_t min, phtree_point_t max)
{
	phtree_window_query_t* new_query = calloc (1, sizeof (*new_query));

	if (!new_query)
	{
		return NULL;
	}

	// make sure min and max are properly populated
	// 	all minimum values in min
	// 	all maximum values in max
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		if (max.values[dimension] < min.values[dimension])
		{
			phtree_key_t temp = min.values[dimension];
			min.values[dimension] = max.values[dimension];
			max.values[dimension] = temp;
		}
	}

	new_query->min = min;
	new_query->max = max;

	new_query->entries = NULL;
	cvector_init (new_query->entries, 10, NULL);

	return new_query;
}

/*
 * clear a window query
 * 	this is good for a window query you want to reuse
 */
void window_query_clear (phtree_window_query_t* query)
{
	cvector_clear (query->entries);

	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		query->min.values[dimension] = 0;
		query->max.values[dimension] = 0;
	}
}

/*
 * free a window query
 */
void window_query_free (phtree_window_query_t* query)
{
	cvector_free (query->entries);

	free (query);
}

// hypercubes expect bit values of 0 to be less than bit values of 1
// 	the sign bit of signed integers breaks this
// 		a 1 bit means a number which is less than a 0 bit number
// 	to avoid having to specially handle negative numbers later
// 		we can correct the sign bit here
// because negative numbers are stored in 2s complement format
// 	we only have to flip the sign bit
// 	all other bits will be correct
// 		example with BIT_WIDTH = 4:
// 			before value_to_key
// 				 1 = 0001
// 				 0 = 0000
// 				-1 = 1111
// 				-2 = 1110
// 			after value_to_key
// 				 1 = 1001
// 				 0 = 1000
// 				-1 = 0111
// 				-2 = 0110
phtree_key_t value_to_key (key_type_signed_t a)
{
	phtree_key_t b = 0;

	memcpy (&b, &a, sizeof (key_type_t));
	b ^= (KEY_ONE << (BIT_WIDTH - 1));  // flip sign bit

	return b;
}

/*
 * convert a phtree_key_t to its equivalent input value
 */
key_type_signed_t key_to_value (phtree_key_t a)
{
	a ^= (KEY_ONE << (BIT_WIDTH - 1));

	key_type_signed_t b;
	memcpy (&b, &a, sizeof (key_type_signed_t));

	return b;
}

/*
 * set the unsigned keys of a point using signed inputs
 */
bool point_set (phtree_point_t* point, key_type_signed_t a, key_type_signed_t b)
{
	// convert signed integer values to unsigned keys
	point->values[0] = value_to_key (a);
	point->values[1] = value_to_key (b);

	return true;
}

/*
 * create a new phtree_point_t
 */
phtree_point_t* point_create (key_type_signed_t a, key_type_signed_t b)
{
	phtree_point_t* new_point = calloc (1, sizeof (*new_point));

	if (!new_point)
	{
		return NULL;
	}

	if (!point_set (new_point, a, b))
	{
		free (new_point);

		return NULL;
	}

	return new_point;
}

