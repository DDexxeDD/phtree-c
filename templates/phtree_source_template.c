#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "phtree{{bit_width}}_common.h"
#include "phtree{{bit_width}}_{{dimensions}}d.h"

/*
 * the maximum bit width we support
 * 	need this for counting leading zeroes
 * 		because we use the 64 bit version of those functions/builtins
 * !! changing this will break things !!
 */
#define PHTREE_BIT_WIDTH_MAX 64

// you can safely change this to any number <= {{bit_width}} and >= 2
// keys will still be {{bit_width}} bits in size but the tree will only have a depth of PHTREE_DEPTH
#define PHTREE_DEPTH {{bit_width}}

#define phtree_node_is_leaf(node) ((node)->postfix_length == 0)
#define phtree_node_is_root(node) ((node)->postfix_length == (PHTREE_DEPTH - 1))

#define DIMENSIONS {{dimensions}}
#define PHTREE_CHILD_FLAG UINT{{max_children}}_C(1)
#define NODE_CHILD_MAX (PHTREE_CHILD_FLAG << (DIMENSIONS))
{{^3d}}
/*
 * because uint8_t is the smallest type we can store child flags in
 * CHILD_SHIFT needs to account for the unused bits
{{^2d}}
 * for 1 dimension we have 6 unused bits, so we add 6
{{/2d}}
{{#2d}}
 * for 2 dimensions we have 4 unused bits, so we add 4
{{/2d}}
 */
{{/3d}}
#define CHILD_SHIFT (NODE_CHILD_MAX - 1{{#child_padding}} + {{child_padding}}{{/child_padding}})

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
static bool point_greater_equal ({{prefix}}_point_t* point_a, {{prefix}}_point_t* point_b)
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
static bool point_less_equal ({{prefix}}_point_t* point_a, {{prefix}}_point_t* point_b)
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

static bool point_equal ({{prefix}}_point_t* point_a, {{prefix}}_point_t* point_b)
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

static bool prefix_equal ({{prefix}}_point_t* point_a, {{prefix}}_point_t* point_b, int postfix_length)
{
	{{prefix}}_point_t local_a = *point_a;
	{{prefix}}_point_t local_b = *point_b;

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
static bool prefix_greater_equal ({{prefix}}_point_t* point_a, {{prefix}}_point_t* point_b, int postfix_length)
{
	{{prefix}}_point_t local_a = *point_a;
	{{prefix}}_point_t local_b = *point_b;

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
static bool prefix_less_equal ({{prefix}}_point_t* point_a, {{prefix}}_point_t* point_b, int postfix_length)
{
	{{prefix}}_point_t local_a = *point_a;
	{{prefix}}_point_t local_b = *point_b;

	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		local_a.values[dimension] >>= postfix_length + 1;
		local_b.values[dimension] >>= postfix_length + 1;
	}

	return (point_less_equal (&local_a, &local_b));
}

{{#unused}}
static bool point_in_window ({{prefix}}_point_t* point, {{prefix}}_query_t* window, int postfix_length)
{
	return (point_greater_equal (point, &window->min) && point_less_equal (point, &window->max));
}
{{/unused}}

static bool prefix_in_window ({{prefix}}_node_t* node, {{prefix}}_query_t* window)
{
	return (prefix_greater_equal (&node->point, &window->min, node->postfix_length) && prefix_less_equal (&node->point, &window->max, node->postfix_length));
}

static bool point_in_window ({{prefix}}_node_t* node, {{prefix}}_query_t* window)
{
	return (point_greater_equal (&node->point, &window->min) && point_less_equal (&node->point, &window->max));
}

/*
 * calculate the hypercube address of the point at the given node
 */
static hypercube_address_t calculate_hypercube_address ({{prefix}}_point_t* point, {{prefix}}_node_t* node)
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

static void* add_child ({{prefix}}_node_t* node, hypercube_address_t address)
{
	if (node->child_count >= node->child_capacity)
	{
		// add 4 slots
		// 	no performance testing/tuning was done on this, just adding 4
		// 		might be better to add some other number
		node->children = phtree_realloc (node->children, (node->child_capacity * sizeof ({{prefix}}_node_t)) + (sizeof ({{prefix}}_node_t) * 4));
		node->child_capacity += 4;
	}

	// need to set active_children before getting child index
	// 	so we get the correct index
	node->active_children |= (PHTREE_CHILD_FLAG << (CHILD_SHIFT - address));

	int index = child_index (node, address);
	// move the children which need to be to the right of the child we are adding
	memmove (node->children + index + 1, node->children + index, sizeof ({{prefix}}_node_t) * (node->child_count - index));
	// zero the child we are adding
	memset (node->children + index, 0, sizeof ({{prefix}}_node_t));

	node->child_count++;

	return node->children + index;
}

/*
 * insert a entry in a node
 * 	entries are nodes whose child pointer points to a user element
 */
static void node_add_entry ({{prefix}}_node_t* node, {{prefix}}_point_t* point)
{
	hypercube_address_t address = calculate_hypercube_address (point, node);

	// if there is already an entry at address
	// 	return
	// the entry we would add to will eventually be returned by {{prefix}}_insert
	if (child_active (node, address))
	{
		return;
	}

	// if there is _not_ an entry at address
	// 	create a new entry
	{{prefix}}_node_t* new_entry = add_child (node, address);

	new_entry->point = *point;
	new_entry->children = NULL;
}

static void node_initialize ({{prefix}}_node_t* node, uint16_t infix_length, uint16_t postfix_length, {{prefix}}_point_t* point)
{
	node->children = phtree_calloc (4, sizeof ({{prefix}}_node_t));
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
static {{prefix}}_node_t* node_try_add (bool* added_new_node, {{prefix}}_node_t* node, hypercube_address_t address, {{prefix}}_point_t* point)
{
	{{prefix}}_node_t* node_out = NULL;

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
static int number_of_diverging_bits ({{prefix}}_point_t* point_a, {{prefix}}_point_t* point_b)
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
static {{prefix}}_node_t* node_insert_split ({{prefix}}_node_t* parent, {{prefix}}_node_t* child, {{prefix}}_point_t* point, int max_conflicting_bits)
{
	/*
	 * because child is already in the corrent array position we would want to put a new split node
	 * 	we copy everything out of child and re-initialize child into the new node 
	 * add two new children to the new child node
	 * copy the old child node into one of the new children
	 * then initialize the other child to a new node for the point we are inserting
	 */

	// store the values of the current child
	{{prefix}}_node_t old_child = *child;
	// clear and reset child
	node_initialize (child, parent->postfix_length - max_conflicting_bits, max_conflicting_bits - 1, point);
	// add a new child to child
	// 	which is going to be where the old_child goes
	{{prefix}}_node_t* new_child = add_child (child, calculate_hypercube_address (&old_child.point, child));
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
static {{prefix}}_node_t* node_handle_collision ({{prefix}}_node_t* node, {{prefix}}_node_t* sub_node, {{prefix}}_point_t* point)
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
static {{prefix}}_node_t* node_add ({{prefix}}_node_t* node, {{prefix}}_point_t* point)
{
	hypercube_address_t address = calculate_hypercube_address (point, node);
	// because node_try_add will always return a node
	// 	we need to keep track of if node_try_add created the node
	// 		or if the node was already there
	bool added_new_node = false;
	{{prefix}}_node_t* sub_node = node_try_add (&added_new_node, node, address, point);

	// if there was not already a node at the point
	// 	we created one and can return it now
	if (added_new_node)
	{
		return sub_node;
	}

	// if there was already a node at the point
	return node_handle_collision (node, sub_node, point);
}

static void entry_free ({{prefix}}_t* tree, {{prefix}}_node_t* node)
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

void {{prefix}}_initialize (
	{{prefix}}_t* tree,
	void* (*element_create) (void* input),
	void (*element_destroy) (void*),
	phtree_key_t (*convert_to_key) (void* input),
{{^even}}
	void (*convert_to_point) ({{prefix}}_t* tree, {{prefix}}_point_t* out, void* input))
{{/even}}
{{#even}}
	void (*convert_to_point) ({{prefix}}_t* tree, {{prefix}}_point_t* out, void* input),
	void (*convert_to_box_point) ({{prefix}}_t* tree, {{prefix}}_point_t* out, void* input))
{{/even}}
{
	{{! change delimiters because of double braces in declaration }}
	{{=<% %>=}}
	<%prefix%>_point_t empty_point = {{0<%#2d%>, 0<%#3d%>, 0<%#4d%>, 0<%#5d%>, 0<%#6d%>, 0<%/6d%><%/5d%><%/4d%><%/3d%><%/2d%>}};
	<%={{ }}=%>
	node_initialize (&tree->root, 0, PHTREE_DEPTH - 1, &empty_point);

	tree->element_create = element_create;
	tree->element_destroy = element_destroy;
	tree->convert_to_key = convert_to_key;
	tree->convert_to_point = convert_to_point;
{{#even}}
	tree->convert_to_box_point = convert_to_box_point;
{{/even}}
}

/*
 * create a new tree
 */
{{prefix}}_t* {{prefix}}_create (
	void* (*element_create) (void* input),
	void (*element_destroy) (void* element),
	phtree_key_t (*convert_to_key) (void* input),
{{^even}}
	void (*convert_to_point) ({{prefix}}_t* tree, {{prefix}}_point_t* out, void* input))
{{/even}}
{{#even}}
	void (*convert_to_point) ({{prefix}}_t* tree, {{prefix}}_point_t* out, void* input),
	void (*convert_to_box_point) ({{prefix}}_t* tree, {{prefix}}_point_t* out, void* input))
{{/even}}
{
	{{prefix}}_t* tree = phtree_calloc (1, sizeof (*tree));

	if (!tree)
	{
		return NULL;
	}

	{{prefix}}_initialize (tree, element_create, element_destroy, convert_to_key, convert_to_point{{#even}}, convert_to_box_point{{/even}});
	if (tree->root.children == NULL)
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
static void free_nodes ({{prefix}}_t* tree, {{prefix}}_node_t* node)
{
	// this will free nodes recursively
	// 	worst case our stack is PHTREE_DEPTH deep
	void (*free_function) ({{prefix}}_t* tree, {{prefix}}_node_t* node) = free_nodes;

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
void {{prefix}}_clear ({{prefix}}_t* tree)
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
void {{prefix}}_free ({{prefix}}_t* tree)
{
	{{prefix}}_clear (tree);
	phtree_free (tree);
}

/*
 * internal for_each function
 * 	does not have safety check for tree, function, or node existence
 */
static void for_each ({{prefix}}_t* tree, {{prefix}}_node_t* node, void (*function) (void* element, void* data), void* data)
{
	if (phtree_node_is_leaf (node))
	{
		for (int iter = 0; iter < node->child_count; iter++)
		{
			{{prefix}}_node_t* entry = &node->children[iter];
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
void {{prefix}}_for_each ({{prefix}}_t* tree, phtree_iteration_function_t function, void* data)
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

void* {{prefix}}_insert ({{prefix}}_t* tree, void* index)
{
	{{prefix}}_point_t point;
	tree->convert_to_point (tree, &point, index);
	{{prefix}}_node_t* current_node = &tree->root;

	while (!phtree_node_is_leaf (current_node))
	{
		current_node = node_add (current_node, &point);
	}

	int offset = child_index (current_node, calculate_hypercube_address (&point, current_node));
	{{prefix}}_node_t* entry = current_node->children + offset;

	if (!entry->children)
	{
		entry->children = tree->element_create (index);
	}

	return entry->children;
}

/*
 * find an entry in the tree
 */
{{prefix}}_node_t* {{prefix}}_find_entry ({{prefix}}_t* tree, {{prefix}}_point_t* point)
{
	{{prefix}}_node_t* current_node = &tree->root.children[child_index (&tree->root, calculate_hypercube_address (point, &tree->root))];
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
void* {{prefix}}_find ({{prefix}}_t* tree, void* index)
{
	{{prefix}}_point_t point;
	tree->convert_to_point (tree, &point, index);
	{{prefix}}_node_t* entry = {{prefix}}_find_entry (tree, &point);

	if (!entry)
	{
		return NULL;
	}

	return entry->children;
}

void {{prefix}}_remove_child ({{prefix}}_node_t* node, hypercube_address_t address)
{
	int index = child_index (node, address);
	{{prefix}}_node_t* child = &node->children[index];

	phtree_free (child->children);

	memmove (node->children + index, node->children + index + 1, sizeof ({{prefix}}_node_t) * (node->child_count - index - 1));

	node->child_count--;
	node->active_children &= ~(PHTREE_CHILD_FLAG << (CHILD_SHIFT - address));
}

void {{prefix}}_remove_entry ({{prefix}}_t* tree, {{prefix}}_node_t* node, hypercube_address_t address)
{
	int index = child_index (node, address);
	{{prefix}}_node_t* entry = &node->children[index];

	if (entry->children)
	{
		if (tree->element_destroy)
		{
			tree->element_destroy (entry->children);
		}

		entry->children = NULL;
	}

	memmove (node->children + index, node->children + index + 1, sizeof ({{prefix}}_node_t) * (node->child_count - index - 1));

	node->child_count--;
	node->active_children &= ~(PHTREE_CHILD_FLAG << (CHILD_SHIFT - address));
}

void {{prefix}}_remove ({{prefix}}_t* tree, void* index)
{
	{{prefix}}_point_t point;
	tree->convert_to_point (tree, &point, index);
	int stack_index = 0;
	{{prefix}}_node_t* node_stack[PHTREE_DEPTH] = {0};
	{{prefix}}_node_t* current_node = &tree->root;
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

	{{prefix}}_remove_entry (tree, current_node, calculate_hypercube_address (&point, current_node));

	if (current_node->child_count == 0)
	{
		// set stack_index to the last node in the stack
		// 	the parent of current_node
		stack_index--;

		{{prefix}}_node_t* parent = node_stack[stack_index];

		{{prefix}}_remove_child (parent, calculate_hypercube_address (&point, parent));
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
bool {{prefix}}_empty ({{prefix}}_t* tree)
{
	return (tree->root.child_count == 0);
}

/*
 * run a window query on a specific node
 */
static void node_query_window ({{prefix}}_node_t* node, {{prefix}}_query_t* query, void* data)
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
				{{prefix}}_node_t* child = &node->children[child_index (node, iter)];

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
void {{prefix}}_query ({{prefix}}_t* tree, {{prefix}}_query_t* query, void* data)
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
static void query_set_internal ({{prefix}}_t* tree, {{prefix}}_query_t* query, {{prefix}}_point_t* min, {{prefix}}_point_t* max, phtree_iteration_function_t function)
{
	{{prefix}}_query_clear (query);

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

void {{prefix}}_query_set ({{prefix}}_t* tree, {{prefix}}_query_t* query, void* min_in, void* max_in, phtree_iteration_function_t function)
{
	if (!query)
	{
		return;
	}

	{{prefix}}_point_t min = {0};
	{{prefix}}_point_t max = {0};

	tree->convert_to_point (tree, &min, min_in);
	tree->convert_to_point (tree, &max, max_in);

	query_set_internal (tree, query, &min, &max, function);
}

{{#even}}
void {{prefix}}_query_box_set ({{prefix}}_t* tree, {{prefix}}_query_t* query, bool intersect, void* min_in, void* max_in, phtree_iteration_function_t function)
{
	if (!query)
	{
		return;
	}

	{{prefix}}_point_t min = {0};
	{{prefix}}_point_t max = {0};

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

void {{prefix}}_query_box_point_set ({{prefix}}_t* tree, {{prefix}}_query_t* query, void* point, phtree_iteration_function_t function)
{
	{{prefix}}_query_box_set (tree, query, true, point, point, function);
}
{{/even}}

void {{prefix}}_query_free ({{prefix}}_query_t* query)
{
	phtree_free (query);
}

/*
 * clear a window query
 */
void {{prefix}}_query_clear ({{prefix}}_query_t* query)
{
	for (int dimension = 0; dimension < DIMENSIONS; dimension++)
	{
		query->min.values[dimension] = 0;
		query->max.values[dimension] = 0;
	}

	query->function = NULL;
}

/*
 * convert input values to tree keys and set the point's values accordingly
 */
void {{prefix}}_point_set ({{prefix}}_t* tree, {{prefix}}_point_t* point, void* a{{#2d}}, void* b{{#3d}}, void* c{{#4d}}, void* d{{#5d}}, void* e{{#6d}}, void* f{{/6d}}{{/5d}}{{/4d}}{{/3d}}{{/2d}})
{
	point->values[0] = tree->convert_to_key (a);
	{{#2d}}
	point->values[1] = tree->convert_to_key (b);
	{{#3d}}
	point->values[2] = tree->convert_to_key (c);
	{{#4d}}
	point->values[3] = tree->convert_to_key (d);
	{{#5d}}
	point->values[4] = tree->convert_to_key (e);
	{{#6d}}
	point->values[5] = tree->convert_to_key (f);
	{{/6d}}
	{{/5d}}
	{{/4d}}
	{{/3d}}
	{{/2d}}
}

{{#even}}
void {{prefix}}_point_box_set ({{prefix}}_t* tree, {{prefix}}_point_t* point, void* a{{#4d}}, void* b{{#6d}}, void* c{{/6d}}{{/4d}})
{
	point->values[0] = tree->convert_to_key (a);
	{{#4d}}
	point->values[1] = tree->convert_to_key (b);
	{{#6d}}
	point->values[2] = tree->convert_to_key (c);
	{{/6d}}
	{{/4d}}

	// this could be cleaner
	// 	but we're doing it this way to work with the current template generation system
	point->values[DIMENSIONS / 2] = point->values[0];
	{{#4d}}
	point->values[(DIMENSIONS / 2) + 1] = point->values[1];
	{{#6d}}
	point->values[(DIMENSIONS / 2) + 2] = point->values[2];
	{{/6d}}
	{{/4d}}
}
{{/even}}

#undef child_index
#undef child_active

#undef DIMENSIONS
#undef NODE_CHILD_MAX
#undef CHILD_SHIFT
