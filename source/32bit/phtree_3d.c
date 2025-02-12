#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "phtree_3d.h"

#ifndef _phtree_common_implementation_
#define _phtree_common_implementation_
/*
 * implementations of the common phtree functionalities
 * guarded so things dont break if you include multiple phtree dimensionalities
 */

/*
 * the maximum bit width we support
 * 	need this for anything involving counting leading zeroes
 * 		because we use the 64 bit version of those functions/builtins
 * !! changing this will break things !!
 */
#define PHTREE_BIT_WIDTH_MAX 64

// you can safely change this to any number <= 32 and >= 2
// keys will still be 32 bits in size but the tree will only have a depth of PHTREE_DEPTH
#define PHTREE_DEPTH 32

// KEY_ONE is an unsigned value of 1
#define PHTREE_KEY_ONE UINT32_C(1)
#define PHTREE_KEY_MAX UINT32_MAX

// hypercubes expect bit values of 0 to be less than bit values of 1
// 	the sign bit of signed integers breaks this
// 		a 1 bit means a number which is less than a 0 bit number
// 	to avoid having to specially handle negative numbers later
// 		we can correct the sign bit here
// because negative numbers are stored in 2s complement format
// 	we only have to flip the sign bit
// 	all other bits will be correct
// 		example with PHTREE_BIT_WIDTH = 4:
// 			before phtree_int_to_key
// 				 1 = 0001
// 				 0 = 0000
// 				-1 = 1111
// 				-2 = 1110
// 			after phtree_int_to_key
// 				 1 = 1001
// 				 0 = 1000
// 				-1 = 0111
// 				-2 = 0110
phtree_key_t phtree_int32_to_key (int32_t a)
{
	phtree_key_t b = 0;

	memcpy (&b, &a, sizeof (uint32_t));
	b ^= (PHTREE_KEY_ONE << (PHTREE_BIT_WIDTH - 1));  // flip sign bit

	return b;
}

// in a hypercube we expect bits set to 0 to be less than bits set to 1
// the sign bit in floating point does not work that way
// 	1 is negative
// the sign bit needs to be flipped
// negative numbers in general also do not work like this
// negative numbers are stored the same as positive numbers
// 	except with the sign bit set to 1
// this is a problem because when the sign bit is flipped
// 	negative numbers behave the same as positive numbers
// which is to say -3 should be less than -2
// but
// when the sign bit is flipped -3 will now be greater than -2
// 	since the numbers have just been changed to positive (3 > 2)
// this is easily fixed however
// we can just invert all of the bits of a negative number
// and it will be correctly set for hypercube functionality
//
// to fully support double keys
// 	your key type needs to have as many bits as a double
//
//	+infinity will be greater than all other numbers
// -infinity will be less than all other numbers
// +nan will be greater than +infinity
// -nan will be less than -infinity
// -0 is converted to +0
phtree_key_t phtree_float_to_key (float x)
{
	phtree_key_t bits;
	size_t size = sizeof (phtree_key_t) < sizeof (float) ? sizeof (phtree_key_t) : sizeof (float);

	memcpy (&bits, &x, size);
	// flip sign bit if value is positive
	if (x >= 0)
	{
		// handle negative zero by converting it to positive zero
		bits = bits & (PHTREE_KEY_MAX >> 1);
		bits ^= (PHTREE_KEY_ONE << (PHTREE_BIT_WIDTH - 1));
	}
	// invert everything if value is negative
	else
	{
		bits ^= PHTREE_KEY_MAX;
	}

	return bits;
}

phtree_key_t phtree_double_to_key (double x)
{
	phtree_key_t bits;
	size_t size = sizeof (phtree_key_t) < sizeof (double) ? sizeof (phtree_key_t) : sizeof (double);

	memcpy (&bits, &x, size);
	// flip sign bit if value is positive
	if (x >= 0)
	{
		// handle negative zero by converting it to positive zero
		bits = bits & (PHTREE_KEY_MAX >> 1);
		bits ^= (PHTREE_KEY_ONE << (PHTREE_BIT_WIDTH - 1));
	}
	// invert everything if value is negative
	else
	{
		bits ^= PHTREE_KEY_MAX;
	}

	return bits;
}

/*
 * count leading and trailing zeroes
 */

#if defined (_MSC_VER)
#include <intrin.h>
uint64_t msvc_count_leading_zeoes (uint64_t bit_string)
{
	unsigned long leading_zero = 0;
	return _BitScanReverse64 (&leading_zero, bit_string) ? 63 - leading_zero : 64U;
}

uint64_t msvc_count_trailing_zeroes (uint64_t bit_string)
{
	unsigned long trailing_zero = 0;
	return _BitScaneForward64 (&trailing_zero, bit_string) ? trailing_zero : 64U;
}
#endif

uint64_t phtree_count_leading_zeroes (uint64_t bit_string)
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

uint64_t phtree_count_trailing_zeroes (uint64_t bit_string)
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
#define count_leading_zeroes(bit_string) count_leading_zeoes_msvc (bit_string)
#define count_trailing_zeroes(bit_string) count_trailing_zeroes_msvc (bit_string)
#else
#define count_leading_zeroes(bit_string) phtree_count_leading_zeroes (bit_string)
#define count_trailing_zeroes(bit_string) phtree_count_trailing_zeroes (bit_string)
#endif

#define phtree_node_is_leaf(node) ((node)->postfix_length == 0)
#define phtree_node_is_root(node) ((node)->parent == NULL)

#endif  // end _phtree_common_implementation_

#define DIMENSIONS 3
#define NODE_CHILD_COUNT (PHTREE_KEY_ONE << (DIMENSIONS))

typedef unsigned int hypercube_address_t;

/*
 * an index point in the tree
 */
typedef struct ph3_point_t
{
	phtree_key_t values[3];
} ph3_point_t;

typedef struct ph3_node_t ph3_node_t;
typedef struct ph3_node_t
{
	// parent is used when removing nodes
	ph3_node_t* parent;
	// children are either ph3_node_t* or ph3_entry_t*
	// if the node is a leaf
	// 	children are ph3_entry_t*
	// if the node is _not_ a leaf
	// 	children are ph3_node_t*
	void* children[PHTREE_KEY_ONE << 3];
	// how many active (not NULL) children a node has
	uint8_t child_count;

	// the distance between a node and its parent, not inclusive
	// example: 
	// 	if parent->postfix_length == 5
	// 	and child->postfix_length == 1
	// 	then  child->infix_length == 3  // not 4
	uint8_t infix_length;
	// counts how many nodes/layers are below this node
	uint8_t postfix_length;

	// only the bits of this point _before_ postfix_length + 1 are relevant
	// 	the bits at postfix_length are the children of this node
	// 	example:
	// 		point = 011010
	// 		postfix_length = 2
	// 		meaningful bits = 011|--
	// 			| is the children of this node
	// 			-- are the bits after this node
	ph3_point_t point;
} ph3_node_t;

typedef struct
{
	ph3_point_t point;
	ph3_node_t* parent;
	void* element;
} ph3_entry_t;

/*
 * the tree type
 */
typedef struct ph3_t
{
	ph3_node_t root;

	/*
	 * user defined functions for handling user defined elements
	 */
	/*
	 * element_create needs to allocate memory for an element
	 * 	and set any default values
	 *
	 * return a pointer to the newly created element
	 */
	void* (*element_create) ();
	/*
	 * element_destroy needs to free any memory allocated for an element
	 * 	including the element itself
	 */
	void (*element_destroy) (void* element);

	phtree_key_t (*convert_to_key) (void* input);
	/*
	 * convert_to_point converts your arbitrary data (input)
	 * 	to a spatial index which the phtree can use
	 */
	void (*convert_to_point) (ph3_t* tree, ph3_point_t* point_out, void* input);
} ph3_t;

typedef struct ph3_window_query_t
{
	ph3_point_t min;
	ph3_point_t max;
	/*
	 * function will be run on all elements inside of the query
	 * if you want to keep a collection of the elements which are inside of the query window
	 * 	pass your collection structure in as data
	 * 		and add the elements to the collection inside of your function
	 */
	phtree_iteration_function_t function;
} ph3_window_query_t;

/*
 * point_a >= point_b
 * 	_all_ of point_a's dimensions must be greater than or equal to point_b's dimensions
 * 		for point_a to be greater than or equal to point_b
 */
static bool point_greater_equal (ph3_point_t* point_a, ph3_point_t* point_b)
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

/*
 * point_a <= point_b
 * 	_all_ of point_a's dimensions must be less than or equal to point_b's dimensions
 * 		for point_a to be less than or equal to point_b
 */
static bool point_less_equal (ph3_point_t* point_a, ph3_point_t* point_b)
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


static void point_subtract (ph3_point_t* point_a, unsigned int amount)
{
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		// prevent integer wrapping
		if (point_a->values[dimension] < amount)
		{
			point_a->values[dimension] = 0;
		}
		else
		{
			point_a->values[dimension] -= amount;
		}
	}
}

static void point_add (ph3_point_t* point_a, unsigned int amount)
{
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		// prevent integer wrapping
		if (PHTREE_KEY_MAX - amount < point_a->values[dimension])
		{
			point_a->values[dimension] = PHTREE_KEY_MAX;
		}
		else
		{
			point_a->values[dimension] += amount;
		}
	}
}


/*
 * checks if all the bits before postfix_length are >=
 * 	used in window queries
 */
static bool prefix_greater_equal (ph3_point_t* point_a, ph3_point_t* point_b, int postfix_length)
{
	ph3_point_t local_a = *point_a;
	ph3_point_t local_b = *point_b;

	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		local_a.values[dimension] >>= postfix_length + 1;
		local_b.values[dimension] >>= postfix_length + 1;
	}

	return (point_greater_equal (&local_a, &local_b));
}

/*
 * checks if all the bits before postfix_length are <=
 * 	used in window queries
 */
static bool prefix_less_equal (ph3_point_t* point_a, ph3_point_t* point_b, int postfix_length)
{
	ph3_point_t local_a = *point_a;
	ph3_point_t local_b = *point_b;

	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		local_a.values[dimension] >>= postfix_length + 1;
		local_b.values[dimension] >>= postfix_length + 1;
	}

	return (point_less_equal (&local_a, &local_b));
}


static bool node_in_window (ph3_node_t* node, ph3_window_query_t* window)
{
	return (prefix_greater_equal (&node->point, &window->min, node->postfix_length) && prefix_less_equal (&node->point, &window->max, node->postfix_length));
}

static bool entry_in_window (ph3_entry_t* entry, ph3_window_query_t* window)
{
	return (point_greater_equal (&entry->point, &window->min) && point_less_equal (&entry->point, &window->max));
}

/*
 * calculate the hypercube address of the point at the given node
 */
static hypercube_address_t calculate_hypercube_address (ph3_point_t* point, ph3_node_t* node)
{
	// which bit in the point->values we are interested in
	phtree_key_t bit_mask = PHTREE_KEY_ONE << node->postfix_length;
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

/*
 * insert a ph3_entry_t in a node
 */
static void node_add_entry (ph3_node_t* node, ph3_point_t* point)
{
	hypercube_address_t address = calculate_hypercube_address (point, node);

	// if there is already an entry at address
	// 	just return
	// 	the entry we would add to will eventually be returned by ph3_insert
	if (node->children[address])
	{
		return;
	}

	// if there is _not_ an entry at address
	// 	create a new entry
	ph3_entry_t* new_entry = phtree_calloc (1, sizeof (*new_entry));

	new_entry->point = *point;
	new_entry->parent = node;
	new_entry->element = NULL;
	node->children[address] = new_entry;
	node->child_count++;
}

/*
 * create a new node
 */
static ph3_node_t* node_create (ph3_node_t* parent, uint16_t infix_length, uint16_t postfix_length, ph3_point_t* point, hypercube_address_t address)
{
	ph3_node_t* new_node = phtree_calloc (1, sizeof (*new_node));

	if (!new_node)
	{
		return NULL;
	}

	new_node->parent = parent;
	new_node->child_count = 0;
	new_node->infix_length = infix_length;
	new_node->postfix_length = postfix_length;
	new_node->point = *point;

	phtree_key_t key_mask = PHTREE_KEY_MAX << (postfix_length + 1);

	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		// set the new node's postfix bits to 0
		new_node->point.values[dimension] &= key_mask;
		// set the bits at node to 1
		// 	this makes the node->point the center of the node
		// 	which is useful later in window queries
		new_node->point.values[dimension] |= PHTREE_KEY_ONE << postfix_length;
	}

	if (postfix_length == 0)
	{
		node_add_entry (new_node, point);
	}

	return new_node;
}

/*
 * try to add a new child node to node
 * 	if the node already has a child at the address
 * 		return that existing node and set success to false
 */
static ph3_node_t* node_try_add (ph3_node_t* node, bool* success, hypercube_address_t address, ph3_point_t* point)
{
	bool added = false;

	if (!node->children[address])
	{
		// because this is a patricia trie
		// 	if we are creating an entirely new child node
		// 		the child is going to be all the way at the bottom of the tree
		// 			postfix = 0  // there will only be entries below this node, no other nodes
		node->children[address] = node_create (node, node->postfix_length - 1, 0, point, address);
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
static int number_of_diverging_bits (ph3_point_t* point_a, ph3_point_t* point_b)
{
	unsigned int difference = 0;

	for (size_t dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		difference |= (point_a->values[dimension] ^ point_b->values[dimension]);
	}

	// count_leading_zeroes always uses the 64 bit implementation
	// 	and will return a number based on a 64 bit input
	// 	so we use PHTREE_BIT_WIDTH_MAX instead of PHTREE_BIT_WIDTH
	return PHTREE_BIT_WIDTH_MAX - count_leading_zeroes (difference);
}

/*
 * insert a new node between existing nodes
 */
static ph3_node_t* node_insert_split (ph3_node_t* node, ph3_node_t* sub_node, ph3_point_t* point, int max_conflicting_bits)
{
	ph3_node_t* new_node = node_create (node, node->postfix_length - max_conflicting_bits, max_conflicting_bits - 1, point, 0);

	node->children[calculate_hypercube_address (point, node)] = new_node;
	new_node->children[calculate_hypercube_address (&sub_node->point, new_node)] = sub_node;
	new_node->child_count++;

	sub_node->infix_length = (new_node->postfix_length - sub_node->postfix_length) - 1;
	sub_node->parent = new_node;

	return new_node;
}

/*
 * figure out what to do when trying to add a new node where a node already exists
 */
static ph3_node_t* node_handle_collision (bool* added, ph3_node_t* node, ph3_node_t* sub_node, ph3_point_t* point)
{
	if (phtree_node_is_leaf (sub_node))
	{
		if (sub_node->infix_length > 0)
		{
			int max_conflicting_bits = number_of_diverging_bits (point, &sub_node->point);

			if (max_conflicting_bits > 1)
			{
				return node_insert_split (node, sub_node, point, max_conflicting_bits);
			}
		}

		// if we are in a leaf and we are _not_creating a split
		// 	create a new entry
		node_add_entry (sub_node, point);
		*added = true;
	}
	else
	{
		if (sub_node->infix_length > 0)
		{
			int max_conflicting_bits = number_of_diverging_bits (point, &sub_node->point);

			if (max_conflicting_bits > sub_node->postfix_length + 1)
			{
				return node_insert_split (node, sub_node, point, max_conflicting_bits);
			}
		}
	}

	return sub_node;
}

/*
 * add a new node to the tree
 */
static ph3_node_t* node_add (bool* added, ph3_node_t* node, ph3_point_t* point)
{
	hypercube_address_t address = calculate_hypercube_address (point, node);
	bool added_new_node = false;
	ph3_node_t* sub_node = node_try_add (node, &added_new_node, address, point);

	if (added_new_node)
	{
		*added = true;
		return sub_node;
	}

	return node_handle_collision (&added_new_node, node, sub_node, point);
}

static void entry_free (ph3_t* tree, ph3_entry_t* entry)
{
	if (entry->element)
	{
		if (tree->element_destroy)
		{
			tree->element_destroy (entry->element);
		}

		entry->element = NULL;
	}

	phtree_free (entry);
}

/*
 * set default tree values
 */
int ph3_initialize (
	ph3_t* tree,
	void* (*element_create) (),
	void (*element_destroy) (void*),
	phtree_key_t (*convert_to_key) (void* input),
	void (*convert_to_point) (ph3_t* tree, ph3_point_t* out, void* input))
{
	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		tree->root.children[iter] = NULL;
	}

	tree->root.parent = NULL;
	tree->root.postfix_length = PHTREE_DEPTH - 1;
	tree->element_create = element_create;
	tree->element_destroy = element_destroy;
	tree->convert_to_key = convert_to_key;
	tree->convert_to_point = convert_to_point;

	return 0;
}

/*
 * create a new tree
 */
ph3_t* ph3_create (
	void* (*element_create) (),
	void (*element_destroy) (void* element),
	phtree_key_t (*convert_to_key) (void* input),
	void (*convert_to_point) (ph3_t* tree, ph3_point_t* out, void* input))
{
	ph3_t* tree = phtree_calloc (1, sizeof (*tree));

	if (!tree)
	{
		return NULL;
	}

	if (ph3_initialize (tree, element_create, element_destroy, convert_to_key, convert_to_point))
	{
		phtree_free (tree);
		return NULL;
	}

	return tree;
}

/*
 * recursively free _ALL_ of the nodes under and including the argument node
 * do not call this on root
 */
void ph3_free_nodes (ph3_t* tree, ph3_node_t* node)
{
	if (!node)
	{
		return;
	}

	if (phtree_node_is_leaf (node))
	{
		for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
		{
			if (node->children[iter])
			{
				entry_free (tree, node->children[iter]);
			}
		}

		phtree_free (node);

		return;
	}

	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		if (node->children[iter])
		{
			// do this recursively
			// worst case our stack is 32 deep
			ph3_free_nodes (tree, node->children[iter]);
		}
	}

	phtree_free (node);
}

/*
 * free all of the nodes and entries in the tree
 */
void ph3_clear (ph3_t* tree)
{
	if (!tree)
	{
		return;
	}

	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		if (tree->root.children[iter])
		{
			ph3_free_nodes (tree, tree->root.children[iter]);
			tree->root.children[iter] = NULL;
		}
	}

	tree->root.child_count = 0;
}

/*
 * free a tree
 */
void ph3_free (ph3_t* tree)
{
	ph3_clear (tree);
	phtree_free (tree);
}

/*
 * internal for_each function
 * 	does not have safety check for tree, function, or node existence
 */
static void for_each (ph3_t* tree, ph3_node_t* node, void (*function) (void* element, void* data), void* data)
{
	if (phtree_node_is_leaf (node))
	{
		for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
		{
			if (node->children[iter])
			{
				ph3_entry_t* entry = node->children[iter];
				function (entry->element, data);
			}
		}

		return;
	}

	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		if (node->children[iter])
		{
			// do this recursively
			// worst case our stack is 32 deep
			for_each (tree, node->children[iter], function, data);
		}
	}
}

/*
 * run the iteration function on every element in the tree
 *
 * data is any external data the user wishes to pass to the iteration function
 */
void ph3_for_each (ph3_t* tree, phtree_iteration_function_t function, void* data)
{
	if (!tree || !function)
	{
		return;
	}

	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		if (tree->root.children[iter])
		{
			for_each (tree, tree->root.children[iter], function, data);
		}
	}
}

void* ph3_insert (ph3_t* tree, void* index)
{
	ph3_point_t point;
	tree->convert_to_point (tree, &point, index);
	ph3_node_t* current_node = &tree->root;
	bool added_new_node = false;

	while (!phtree_node_is_leaf (current_node))
	{
		current_node = node_add (&added_new_node, current_node, &point);
	}

	ph3_entry_t* child = current_node->children[calculate_hypercube_address (&point, current_node)];

	if (child && !child->element)
	{
		child->element = tree->element_create ();
	}

	return child->element;
}

/*
 * find an entry in the tree
 */
ph3_entry_t* find_entry (ph3_t* tree, void* index)
{
	ph3_point_t point;
	tree->convert_to_point (tree, &point, index);
	ph3_node_t* current_node = &tree->root;

	while (current_node && !phtree_node_is_leaf (current_node))
	{
		current_node = current_node->children[calculate_hypercube_address (&point, current_node)];
	}

	if (!current_node)
	{
		return NULL;
	}

	return current_node->children[calculate_hypercube_address (&point, current_node)];
}

/*
 * find an element at a specific index
 * returns NULL if there is no element at the index
 */
void* ph3_find (ph3_t* tree, void* index)
{
	ph3_entry_t* entry = find_entry (tree, index);

	return entry->element;
}

/*
 * remove the entry and its element at the given index
 */
void ph3_remove (ph3_t* tree, void* index)
{
	ph3_entry_t* entry = find_entry (tree, index);

	if (!entry)
	{
		return;
	}

	ph3_point_t point;
	tree->convert_to_point (tree, &point, index);
	ph3_node_t* current_node = entry->parent;
	hypercube_address_t address = calculate_hypercube_address (&point, current_node);

	entry_free (tree, entry);

	current_node->children[address] = NULL;
	current_node->child_count--;

	// entries are all children of leaf nodes
	// 	if the leaf node child_count == 0
	// 	we can delete the leaf node
	if (current_node->child_count == 0)
	{
		ph3_node_t* parent = current_node->parent;

		parent->children[calculate_hypercube_address (&current_node->point, parent)] = NULL;
		parent->child_count--;
		phtree_free (current_node);

		// deleting the leaf node means we have to go up the tree
		// 	and possibly delete or merge other nodes
		current_node = parent;
		parent = current_node->parent;
		while (!phtree_node_is_root (current_node))
		{
			// if the current_node only has 1 child
			// 	we can get rid of current_node
			// 	and have current_node->parent point directly at current_node's 1 child
			if (current_node->child_count == 1)
			{
				ph3_node_t* child = NULL;

				// find the child that is populated
				for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
				{
					if (current_node->children[iter])
					{
						child = current_node->children[iter];
						break;
					}
				}

				parent->children[calculate_hypercube_address (&current_node->point, parent)] = child;
				child->parent = parent;
				child->infix_length = child->parent->postfix_length - child->postfix_length - 1;

				phtree_free (current_node);

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
 * check if the tree is empty
 */
bool ph3_empty (ph3_t* tree)
{
	return (tree->root.child_count == 0);
}

/*
 * run a window query on a specific node
 */
static void node_query_window (ph3_node_t* node, ph3_window_query_t* query, void* data)
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

	if (phtree_node_is_leaf (node))
	{
		for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
		{
			ph3_entry_t* child = node->children[iter];

			if (child && ((iter | mask_lower) & mask_upper) == iter)
			{
				if (entry_in_window (child, query))
				{
					query->function (child->element, data);
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
			node_query_window (node->children[iter], query, data);
		}
	}
}

/*
 * run a window query on a tree
 */
void ph3_query (ph3_t* tree, ph3_window_query_t* query, void* data)
{
	if (!tree || !query || !query->function)
	{
		return;
	}

	for (int iter = 0; iter < NODE_CHILD_COUNT; iter++)
	{
		if (tree->root.children[iter])
		{
			node_query_window (tree->root.children[iter], query, data);
		}
	}
}

/*
 * query_set_internal does not need to convert external values in to internal points/keys
 * so it needs to be its own function
 */
static void query_set_internal (ph3_t* tree, ph3_window_query_t* query, ph3_point_t* min, ph3_point_t* max, phtree_iteration_function_t function)
{
	ph3_query_clear (query);

	query->min = *min;
	query->max = *max;

	// make sure min and max are properly populated
	// 	all minimum values in min
	// 	all maximum values in max
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		if (query->max.values[dimension] < query->min.values[dimension])
		{
			phtree_key_t temp = query->min.values[dimension];
			query->min.values[dimension] = query->max.values[dimension];
			query->max.values[dimension] = temp;
		}
	}

	query->function = function;
}

void ph3_query_set (ph3_t* tree, ph3_window_query_t* query, void* min_in, void* max_in, phtree_iteration_function_t function)
{
	if (!query)
	{
		return;
	}

	ph3_point_t min = {0};
	ph3_point_t max = {0};

	tree->convert_to_point (tree, &min, min_in);
	tree->convert_to_point (tree, &max, max_in);

	query_set_internal (tree, query, &min, &max, function);
}

/*
 * create a square/cube query centered on a point
 * distance is how many steps to take outward from the center point
 * 	(chebyshev distance)
 *
 * example: X is the center point, distance = 2
 *
 *    2 2 2 2 2
 *    2 1 1 1 2
 *    2 1 X 1 2
 *    2 1 1 1 2
 *    2 2 2 2 2
 */
void ph3_query_chebyshev (ph3_t* tree, ph3_window_query_t* query, void* center, unsigned int distance, phtree_iteration_function_t function)
{
	if (!query)
	{
		return;
	}

	ph3_point_t min;
	tree->convert_to_point (tree, &min, center);
	ph3_point_t max = min;

	point_subtract (&min, distance);
	point_add (&max, distance);

	query_set_internal (tree, query, &min, &max, function);
}

/*
 * create a new window query
 */
ph3_window_query_t* ph3_query_create ()
{
	ph3_window_query_t* new_query = phtree_calloc (1, sizeof (*new_query));

	if (!new_query)
	{
		return NULL;
	}

	return new_query;
}

void ph3_query_free (ph3_window_query_t* query)
{
	phtree_free (query);
}

/*
 * clear a window query
 */
void ph3_query_clear (ph3_window_query_t* query)
{
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		query->min.values[dimension] = 0;
		query->max.values[dimension] = 0;
	}

	query->function = NULL;
}

void ph3_query_center (ph3_window_query_t* query, ph3_point_t* out)
{
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		out->values[dimension] = (query->max.values[dimension] - query->min.values[dimension]) / 2;
	}
}

/*
 * convert input values to tree keys and set the point's values accordingly
 */
void ph3_point_set (ph3_t* tree, ph3_point_t* point, void* a, void* b, void* c)
{
	point->values[0] = tree->convert_to_key (a);
	point->values[1] = tree->convert_to_key (b);
	point->values[2] = tree->convert_to_key (c);
}

#undef DIMENSIONS
#undef NODE_CHILD_COUNT
