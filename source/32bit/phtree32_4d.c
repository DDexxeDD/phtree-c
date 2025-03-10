#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "phtree32_common.h"
#include "phtree32_4d.h"

/*
 * the maximum bit width we support
 * 	need this for counting leading zeroes
 * 		because we use the 64 bit version of those functions/builtins
 * !! changing this will break things !!
 */
#define PHTREE_BIT_WIDTH_MAX 64

// you can safely change this to any number <= 32 and >= 2
// keys will still be 32 bits in size but the tree will only have a depth of PHTREE_DEPTH
#define PHTREE_DEPTH 32

#define phtree_node_is_leaf(node) ((node)->postfix_length == 0)
#define phtree_node_is_root(node) ((node)->postfix_length == (PHTREE_DEPTH - 1))

#define DIMENSIONS 4
#define PHTREE_CHILD_FLAG UINT16_C(1)
#define NODE_CHILD_MAX (PHTREE_CHILD_FLAG << (DIMENSIONS))
#define CHILD_SHIFT (NODE_CHILD_MAX - 1)

// shifting active_children by (CHILD_SHIFT - address)
// 	puts the active child at the right most position
// 	and zeroes everything to the left of the 0th child
// 		8 bit example:
// 			DIMENSIONS = 3
// 			CHILD_SHIFT = 7
// 			address = 2
// 			active_children = 01101000
// 			                    ^ addressed child
// 			01101000 >> (CHILD_SHIFT - address) = 00000011
// popcounting the shifted active_children
// 	counts how many children there are before and including the child at address
// 		example:
// 			popcount (00000011) = 2
// subtracting 1 from popcount gives the index in the child array
// 	of the child we are looking for
#define child_index(node,address) (popcount ((node)->active_children >> (CHILD_SHIFT - (address))) - 1)
#define child_active(node,address) ((node)->active_children & (PHTREE_CHILD_FLAG << (CHILD_SHIFT - (address))))

typedef unsigned int hypercube_address_t;

/*
 * point_a >= point_b
 * 	_all_ of point_a's dimensions must be greater than or equal to point_b's dimensions
 * 		for point_a to be greater than or equal to point_b
 */
static bool point_greater_equal (ph4_point_t* point_a, ph4_point_t* point_b)
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
static bool point_less_equal (ph4_point_t* point_a, ph4_point_t* point_b)
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

static bool point_equal (ph4_point_t* point_a, ph4_point_t* point_b)
{
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		if (point_a->values[dimension] & ~(point_b->values[dimension]))
		{
			return false;
		}
	}

	return true;
}

static bool prefix_equal (ph4_point_t* point_a, ph4_point_t* point_b, int postfix_length)
{
	ph4_point_t local_a = *point_a;
	ph4_point_t local_b = *point_b;

	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		local_a.values[dimension] >>= postfix_length + 1;
		local_b.values[dimension] >>= postfix_length + 1;
	}

	return (point_equal (&local_a, &local_b));
}

/*
 * checks if all the bits before postfix_length are >=
 * 	used in window queries
 */
static bool prefix_greater_equal (ph4_point_t* point_a, ph4_point_t* point_b, int postfix_length)
{
	ph4_point_t local_a = *point_a;
	ph4_point_t local_b = *point_b;

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
static bool prefix_less_equal (ph4_point_t* point_a, ph4_point_t* point_b, int postfix_length)
{
	ph4_point_t local_a = *point_a;
	ph4_point_t local_b = *point_b;

	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		local_a.values[dimension] >>= postfix_length + 1;
		local_b.values[dimension] >>= postfix_length + 1;
	}

	return (point_less_equal (&local_a, &local_b));
}


static bool prefix_in_window (ph4_node_t* node, ph4_query_t* window)
{
	return (prefix_greater_equal (&node->point, &window->min, node->postfix_length) && prefix_less_equal (&node->point, &window->max, node->postfix_length));
}

static bool point_in_window (ph4_node_t* node, ph4_query_t* window)
{
	return (point_greater_equal (&node->point, &window->min) && point_less_equal (&node->point, &window->max));
}

/*
 * calculate the hypercube address of the point at the given node
 */
static hypercube_address_t calculate_hypercube_address (ph4_point_t* point, ph4_node_t* node)
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

static void* add_child (ph4_node_t* node, hypercube_address_t address)
{
	if (node->child_count >= node->child_capacity)
	{
		// add 4 slots
		// 	no performance testing/tuning was done on this, just adding 4
		// 		might be better to add some other number
		node->children = phtree_realloc (node->children, (node->child_capacity * sizeof (ph4_node_t)) + (sizeof (ph4_node_t) * 4));
		node->child_capacity += 4;
	}

	// need to set active_children before getting child index
	// 	so we get the correct index
	node->active_children |= (PHTREE_CHILD_FLAG << (CHILD_SHIFT - address));

	int index = child_index (node, address);
	// move the children which need to be to the right of the child we are adding
	memmove (node->children + index + 1, node->children + index, sizeof (ph4_node_t) * (node->child_count - index));
	// zero the child we are adding
	memset (node->children + index, 0, sizeof (ph4_node_t));

	node->child_count++;

	return node->children + index;
}

/*
 * insert a entry in a node
 * 	entries are nodes whose child pointer points to a user element
 */
static void node_add_entry (ph4_node_t* node, ph4_point_t* point)
{
	hypercube_address_t address = calculate_hypercube_address (point, node);

	// if there is already an entry at address
	// 	return
	// the entry we would add to will eventually be returned by ph4_insert
	if (child_active (node, address))
	{
		return;
	}

	// if there is _not_ an entry at address
	// 	create a new entry
	ph4_node_t* new_entry = add_child (node, address);

	new_entry->point = *point;
	new_entry->children = NULL;
}

static void node_initialize (ph4_node_t* node, uint16_t infix_length, uint16_t postfix_length, ph4_point_t* point)
{
	node->children = phtree_calloc (4, sizeof (ph4_node_t));
	node->child_capacity = 4;
	node->child_count = 0;
	node->active_children = 0;
	node->infix_length = infix_length;
	node->postfix_length = postfix_length;
	node->point = *point;

	phtree_key_t key_mask = PHTREE_KEY_MAX << (postfix_length + 1);

	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		// set the new node's postfix bits to 0
		node->point.values[dimension] &= key_mask;
		// set the bits at node to 1
		// 	this makes the node->point the center of the node
		// 	which is useful later in window queries
		node->point.values[dimension] |= PHTREE_KEY_ONE << postfix_length;
	}
}

/*
 * try to add a new child node to node
 * 	if the node already has a child at the address
 * 		return that existing node and set success to false
 */
static ph4_node_t* node_try_add (bool* added_new_node, ph4_node_t* node, hypercube_address_t address, ph4_point_t* point)
{
	ph4_node_t* node_out = NULL;

	// if the child is empty
	// 	create a new child
	if (!child_active (node, address))
	{
		// if we are creating an entirely new child node
		// 	because this is a patricia trie
		// 		the child is going to be all the way at the bottom of the tree
		// 			postfix = 0  // there will only be entries below this node, no other nodes
		node_out = add_child (node, address);
		node_initialize (node_out, node->postfix_length - 1, 0, point);
		node_add_entry (node_out, point);

		*added_new_node = true;
	}
	// if the child is not empty
	// 	return the child
	else
	{
		node_out = node->children + child_index (node, address);
		*added_new_node = false;
	}

	return node_out;
}

/*
 * return the bit at which the two points diverge
 */
static int number_of_diverging_bits (ph4_point_t* point_a, ph4_point_t* point_b)
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
static ph4_node_t* node_insert_split (ph4_node_t* parent, ph4_node_t* child, ph4_point_t* point, int max_conflicting_bits)
{
	/*
	 * because child is already in the corrent array position we would want to put a new split node
	 * 	we copy everything out of child and re-initialize child into the new node 
	 * add two new children to the new child node
	 * copy the old child node into one of the new children
	 * then initialize the other child to a new node for the point we are inserting
	 */

	// store the values of the current child
	ph4_node_t old_child = *child;
	// clear and reset child
	node_initialize (child, parent->postfix_length - max_conflicting_bits, max_conflicting_bits - 1, point);
	// add a new child to child
	// 	which is going to be where the old_child goes
	ph4_node_t* new_child = add_child (child, calculate_hypercube_address (&old_child.point, child));
	// copy the values from old_child into the new_child
	*new_child = old_child;

	new_child->infix_length = (child->postfix_length - new_child->postfix_length) - 1;

	// add the new child that we created the split for
	new_child = add_child (child, calculate_hypercube_address (point, child));
	node_initialize (new_child, child->postfix_length - 1, 0, point);
	node_add_entry (new_child, point);

	return new_child;
}

/*
 * figure out what to do when trying to add a new node where a node already exists
 */
static ph4_node_t* node_handle_collision (ph4_node_t* node, ph4_node_t* sub_node, ph4_point_t* point)
{
	// if infix_length == 0
	// 	we can not insert a node between node and sub_node
	// 	point will be a child of sub_node
	if (sub_node->infix_length > 0)
	{
		int max_conflicting_bits = number_of_diverging_bits (point, &sub_node->point);

		/*
		 * max_conflicting_bits == sub_node->node.postfix_length
		 * 	means we are trying to insert a child of sub_node
		 *
		 * max_conflicting_bits == sub_node->node.postfix_length + 1
		 * 	means we would be inserting the same sub_node that already exists
		 *
		 * max_conflicting_bits > sub_node->node.postfix_length + 1
		 * 	we need to insert a node between node and sub_node
		 */
		if (max_conflicting_bits > sub_node->postfix_length + 1)
		{
			return node_insert_split (node, sub_node, point, max_conflicting_bits);
		}
	}

	if (phtree_node_is_leaf (sub_node))
	{
		node_add_entry (sub_node, point);
	}

	return sub_node;
}

/*
 * add a new node to the tree
 */
static ph4_node_t* node_add (ph4_node_t* node, ph4_point_t* point)
{
	hypercube_address_t address = calculate_hypercube_address (point, node);
	// because node_try_add will always return a node
	// 	we need to keep track of if node_try_add created the node
	// 		or if the node was already there
	bool added_new_node = false;
	ph4_node_t* sub_node = node_try_add (&added_new_node, node, address, point);

	// if there was not already a node at the point
	// 	we created one and can return it now
	if (added_new_node)
	{
		return sub_node;
	}

	// if there was already a node at the point
	return node_handle_collision (node, sub_node, point);
}

static void entry_free (ph4_t* tree, ph4_node_t* node)
{
	if (node->children)
	{
		if (tree->element_destroy)
		{
			tree->element_destroy (node->children);
		}

		node->children = NULL;
	}
}

int ph4_initialize (
	ph4_t* tree,
	void* (*element_create) (void* input),
	void (*element_destroy) (void*),
	phtree_key_t (*convert_to_key) (void* input),
	void (*convert_to_point) (ph4_t* tree, ph4_point_t* out, void* input),
	void (*convert_to_box_point) (ph4_t* tree, ph4_point_t* out, void* input))
{
	ph4_point_t empty_point = {{0, 0, 0, 0}};
	node_initialize (&tree->root, 0, PHTREE_DEPTH - 1, &empty_point);

	tree->element_create = element_create;
	tree->element_destroy = element_destroy;
	tree->convert_to_key = convert_to_key;
	tree->convert_to_point = convert_to_point;
	tree->convert_to_box_point = convert_to_box_point;

	return 0;
}

/*
 * create a new tree
 */
ph4_t* ph4_create (
	void* (*element_create) (void* input),
	void (*element_destroy) (void* element),
	phtree_key_t (*convert_to_key) (void* input),
	void (*convert_to_point) (ph4_t* tree, ph4_point_t* out, void* input),
	void (*convert_to_box_point) (ph4_t* tree, ph4_point_t* out, void* input))
{
	ph4_t* tree = phtree_calloc (1, sizeof (*tree));

	if (!tree)
	{
		return NULL;
	}

	if (ph4_initialize (tree, element_create, element_destroy, convert_to_key, convert_to_point, convert_to_box_point))
	{
		phtree_free (tree);
		return NULL;
	}

	return tree;
}

/*
 * recursively free _ALL_ of the nodes under and including the argument node
 * !! do not call this on root !!
 */
static void free_nodes (ph4_t* tree, ph4_node_t* node)
{
	// this will free nodes recursively
	// 	worst case our stack is PHTREE_DEPTH deep
	void (*free_function) (ph4_t* tree, ph4_node_t* node) = free_nodes;

	if (phtree_node_is_leaf (node))
	{
		// if the node is a leaf we dont need to recurse any further
		// 	just free entries
		free_function = entry_free;
	}

	for (int iter = 0; iter < node->child_count; iter++)
	{
		free_function (tree, &node->children[iter]);
	}

	phtree_free (node->children);
}

/*
 * free all of the nodes and entries in the tree
 */
void ph4_clear (ph4_t* tree)
{
	if (!tree)
	{
		return;
	}

	for (int iter = 0; iter < tree->root.child_count; iter++)
	{
		free_nodes (tree, &tree->root.children[iter]);
	}

	tree->root.active_children = 0;
	tree->root.child_count = 0;
	tree->root.child_capacity = 0;

	phtree_free (tree->root.children);
}

/*
 * free a tree
 */
void ph4_free (ph4_t* tree)
{
	ph4_clear (tree);
	phtree_free (tree);
}

/*
 * internal for_each function
 * 	does not have safety check for tree, function, or node existence
 */
static void for_each (ph4_t* tree, ph4_node_t* node, void (*function) (void* element, void* data), void* data)
{
	if (phtree_node_is_leaf (node))
	{
		for (int iter = 0; iter < node->child_count; iter++)
		{
			ph4_node_t* entry = &node->children[iter];
			function (entry->children, data);
		}

		return;
	}

	for (int iter = 0; iter < node->child_count; iter++)
	{
		// do this recursively
		// worst case our stack is 32 deep
		for_each (tree, &node->children[iter], function, data);
	}
}

/*
 * run the iteration function on every element in the tree
 *
 * data is any external data the user wishes to pass to the iteration function
 */
void ph4_for_each (ph4_t* tree, phtree_iteration_function_t function, void* data)
{
	if (!tree || !function)
	{
		return;
	}

	for (int iter = 0; iter < tree->root.child_count; iter++)
	{
		for_each (tree, &tree->root.children[iter], function, data);
	}
}

void* ph4_insert (ph4_t* tree, void* index)
{
	ph4_point_t point;
	tree->convert_to_point (tree, &point, index);
	ph4_node_t* current_node = &tree->root;

	while (!phtree_node_is_leaf (current_node))
	{
		current_node = node_add (current_node, &point);
	}

	int offset = child_index (current_node, calculate_hypercube_address (&point, current_node));
	ph4_node_t* entry = current_node->children + offset;

	if (!entry->children)
	{
		entry->children = tree->element_create (index);
	}

	return entry->children;
}

/*
 * find an entry in the tree
 */
ph4_node_t* ph4_find_entry (ph4_t* tree, ph4_point_t* point)
{
	ph4_node_t* current_node = &tree->root.children[child_index (&tree->root, calculate_hypercube_address (point, &tree->root))];
	hypercube_address_t address;

	while (!phtree_node_is_leaf (current_node))
	{
		address = calculate_hypercube_address (point, current_node);

		if (!child_active (current_node, address)
			|| !prefix_equal (point, &current_node->point, current_node->postfix_length))
		{
			return NULL;
		}

		current_node = &current_node->children[child_index (current_node, address)];
	}

	address = calculate_hypercube_address (point, current_node);

	if (!child_active (current_node, address)
		|| !point_equal (point, &current_node->point))
	{
		return NULL;
	}

	return &current_node->children[child_index (current_node, address)];
}

/*
 * find an element at a specific index
 * returns NULL if there is no element at the index
 */
void* ph4_find (ph4_t* tree, void* index)
{
	ph4_point_t point;
	tree->convert_to_point (tree, &point, index);
	ph4_node_t* entry = ph4_find_entry (tree, &point);

	if (!entry)
	{
		return NULL;
	}

	return entry->children;
}

void ph4_remove_child (ph4_node_t* node, hypercube_address_t address)
{
	int index = child_index (node, address);
	ph4_node_t* child = &node->children[index];

	phtree_free (child->children);

	memmove (node->children + index, node->children + index + 1, sizeof (ph4_node_t) * (node->child_count - index - 1));

	node->child_count--;
	node->active_children &= ~(PHTREE_CHILD_FLAG << (CHILD_SHIFT - address));
}

void ph4_remove_entry (ph4_t* tree, ph4_node_t* node, hypercube_address_t address)
{
	int index = child_index (node, address);
	ph4_node_t* entry = &node->children[index];

	if (entry->children)
	{
		if (tree->element_destroy)
		{
			tree->element_destroy (entry->children);
		}

		entry->children = NULL;
	}

	memmove (node->children + index, node->children + index + 1, sizeof (ph4_node_t) * (node->child_count - index - 1));

	node->child_count--;
	node->active_children &= ~(PHTREE_CHILD_FLAG << (CHILD_SHIFT - address));
}

void ph4_remove (ph4_t* tree, void* index)
{
	ph4_point_t point;
	tree->convert_to_point (tree, &point, index);
	int stack_index = 0;
	ph4_node_t* node_stack[PHTREE_DEPTH] = {0};
	ph4_node_t* current_node = &tree->root;
	hypercube_address_t address;

	while (!phtree_node_is_leaf (current_node))
	{
		address = calculate_hypercube_address (&point, current_node);

		if (child_active (current_node, address))
		{
			node_stack[stack_index] = current_node;
			stack_index++;
			current_node = &current_node->children[child_index (current_node, address)];
		}
		// if the point doesnt exist in the tree we dont need to remove it
		else
		{
			return;
		}
	}

	ph4_remove_entry (tree, current_node, calculate_hypercube_address (&point, current_node));

	if (current_node->child_count == 0)
	{
		// set stack_index to the last node in the stack
		// 	the parent of current_node
		stack_index--;

		ph4_node_t* parent = node_stack[stack_index];

		ph4_remove_child (parent, calculate_hypercube_address (&point, parent));
		stack_index--;

		// node_stack[0] is root
		// 	we dont need to run this on root
		while (stack_index > 0)
		{
			parent = node_stack[stack_index - 1];
			current_node = node_stack[stack_index];

			// XXX
			// 	we dont need to check if current_node->child_count == 0
			// 		because that would imply that we had a split node which didnt split anything
			// 			and only had a single child
			// 		such a node shouldnt exist
			// 			it should have been removed before getting here
			if (current_node->child_count > 1)
			{
				break;
			}

			int index = child_index (parent, calculate_hypercube_address (&point, parent));

			// current_node->children[0] is the only child
			parent->children[index] = current_node->children[0];
			parent->children[index].infix_length = parent->postfix_length - parent->children[index].postfix_length - 1;

			phtree_free (current_node->children);

			stack_index--;
		}
	}
}

/*
 * check if the tree is empty
 */
bool ph4_empty (ph4_t* tree)
{
	return (tree->root.child_count == 0);
}

/*
 * run a window query on a specific node
 */
static void node_query_window (ph4_node_t* node, ph4_query_t* query, void* data)
{
	if (!prefix_in_window (node, query))
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
		for (int iter = 0; iter < NODE_CHILD_MAX; iter++)
		{
			if (child_active (node, iter) && ((iter | mask_lower) & mask_upper) == iter)
			{
				ph4_node_t* child = &node->children[child_index (node, iter)];

				if (point_in_window (child, query))
				{
					query->function (child->children, data);
				}
			}
		}

		return;
	}

	// if the node _is_ in the window and _is not_ a leaf
	// 	recurse through the node's children
	for (unsigned int iter = 0; iter < NODE_CHILD_MAX; iter++)
	{
		if (child_active (node, iter) && ((iter | mask_lower) & mask_upper) == iter)
		{
			node_query_window (&node->children[child_index (node, iter)], query, data);
		}
	}
}

/*
 * run a window query on a tree
 */
void ph4_query (ph4_t* tree, ph4_query_t* query, void* data)
{
	if (!tree || !query || !query->function)
	{
		return;
	}

	for (int iter = 0; iter < tree->root.child_count; iter++)
	{
		node_query_window (&tree->root.children[iter], query, data);
	}
}

/*
 * query_set_internal does not need to convert external values in to internal points/keys
 * so it needs to be its own function
 */
static void query_set_internal (ph4_t* tree, ph4_query_t* query, ph4_point_t* min, ph4_point_t* max, phtree_iteration_function_t function)
{
	ph4_query_clear (query);

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

void ph4_query_set (ph4_t* tree, ph4_query_t* query, void* min_in, void* max_in, phtree_iteration_function_t function)
{
	if (!query)
	{
		return;
	}

	ph4_point_t min = {0};
	ph4_point_t max = {0};

	tree->convert_to_point (tree, &min, min_in);
	tree->convert_to_point (tree, &max, max_in);

	query_set_internal (tree, query, &min, &max, function);
}

void ph4_query_box_set (ph4_t* tree, ph4_query_t* query, bool intersect, void* min_in, void* max_in, phtree_iteration_function_t function)
{
	if (!query)
	{
		return;
	}

	ph4_point_t min = {0};
	ph4_point_t max = {0};

	if (!tree->convert_to_box_point)
	{
		query->min = min;
		query->max = max;
		query->function = function;

		return;
	}

	tree->convert_to_box_point (tree, &min, min_in);
	tree->convert_to_box_point (tree, &max, max_in);

	if (intersect)
	{
		for (int iter = 0; iter < DIMENSIONS / 2; iter++)
		{
			min.values[iter] = 0;
		}

		for (int iter = DIMENSIONS / 2; iter < DIMENSIONS; iter++)
		{
			max.values[iter] = PHTREE_KEY_MAX;
		}
	}

	query_set_internal (tree, query, &min, &max, function);
}

void ph4_query_box_point_set (ph4_t* tree, ph4_query_t* query, void* point, phtree_iteration_function_t function)
{
	ph4_query_box_set (tree, query, true, point, point, function);
}

/*
 * create a new window query
 */
ph4_query_t* ph4_query_create ()
{
	ph4_query_t* new_query = phtree_calloc (1, sizeof (*new_query));

	if (!new_query)
	{
		return NULL;
	}

	return new_query;
}

void ph4_query_free (ph4_query_t* query)
{
	phtree_free (query);
}

/*
 * clear a window query
 */
void ph4_query_clear (ph4_query_t* query)
{
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		query->min.values[dimension] = 0;
		query->max.values[dimension] = 0;
	}

	query->function = NULL;
}

void ph4_query_center (ph4_query_t* query, ph4_point_t* out)
{
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		out->values[dimension] = (query->max.values[dimension] - query->min.values[dimension]) / 2;
	}
}

/*
 * convert input values to tree keys and set the point's values accordingly
 */
void ph4_point_set (ph4_t* tree, ph4_point_t* point, void* a, void* b, void* c, void* d)
{
	point->values[0] = tree->convert_to_key (a);
	point->values[1] = tree->convert_to_key (b);
	point->values[2] = tree->convert_to_key (c);
	point->values[3] = tree->convert_to_key (d);
}

void ph4_point_box_set (ph4_t* tree, ph4_point_t* point, void* a, void* b)
{
	point->values[0] = tree->convert_to_key (a);
	point->values[1] = tree->convert_to_key (b);

	// this could be cleaner
	// 	but we're doing it this way to work with the current template generation system
	point->values[DIMENSIONS / 2] = point->values[0];
	point->values[(DIMENSIONS / 2) + 1] = point->values[1];
}

#undef child_index
#undef child_active

#undef DIMENSIONS
#undef NODE_CHILD_MAX
#undef CHILD_SHIFT
