#ifndef _ph3_h_
#define _ph3_h_

#include <stdbool.h>
#include <stdint.h>

#include "phtree32_common.h"

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
	/*
	 * this node layout combined with memory alignment will be less than 64 bytes in size
	 * 	for all combinations of supported dimensions and bit widths EXCEPT 64 bit, 6 dimensions
	 * 		64 bit, 6 dimensions will be 72 bytes
	 */
	/*
	 * only the bits of this point _before_ postfix_length + 1 are relevant
	 * 	the bits at postfix_length are the children of this node
	 * 	example:
	 * 		point = 011010
	 * 		postfix_length = 2
	 * 		meaningful bits = 011|--
	 * 			| is the children of this node
	 * 			-- are the bits after this node
	 */
	ph3_point_t point;
	/*
	 * a child can be either a node or an element
	 * 	we store everything as a node because most of the time we are working with nodes
	 * 	only in a few spots do we work with entries
	 * children is an ordered dynamic array
	 */
	ph3_node_t* children;
	// bit flags for which children are active
	uint8_t active_children;
	// curent capacity of the children array
	int8_t child_capacity;
	// how many active (not NULL) children a node has
	int8_t child_count;

	/*
	 * the distance between a node and its parent, not inclusive
	 * example: 
	 * 	if parent->postfix_length == 5
	 * 	and child->postfix_length == 1
	 * 	then  child->infix_length == 3  // not 4
	 */
	int8_t infix_length;
	// counts how many nodes/layers are below this node
	int8_t postfix_length;
} ph3_node_t;

/*
 * the tree type
 */
typedef struct ph3_t ph3_t;
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
	 * input is the user defined object being passed in to ph3_insert
	 * 	when the element is being created
	 * if more data is needed to initialize an element than that
	 * 	you will need to use the pointer returned by ph3_insert
	 * 		to finish element initialization
	 *
	 * return a pointer to the newly created element
	 */
	void* (*element_create) (void* input);
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

typedef struct ph3_query_t
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
} ph3_query_t;


/*
 * allocate and initialize a new tree
 */
/*
 * !! the following 4 functions are REQUIRED for the tree to work !!
 *
 * void* element_create ()
 * 	allocates and initializes your custom tree element object
 *
 * 	the input into this function will be the first thing you pass in to ph3_insert
 * 		at the index of the element being created
 * 		if it is not possible to completely initialize the element
 * 			using that first thing being inserted
 * 			you will need to finish initialization using the pointer returned from ph3_insert
 *
 * 	return a pointer to the object you allocated
 *
 * void element_destory (void* element)
 * 	deallocates/frees whatever was allocated by element_create
 *
 * phtree_key_t convert_to_key (void* input)
 * 	converts input into a single phtree_key_t value
 * 	likely you will use this to convert a single number into a key
 *
 * void convert_to_point (ph3_t* tree, ph3_point_t* out, void* input)
 * 	convert your spatial data into a spatial index in the tree
 * 	likely you will be inputting an n-dimensional point into this
 * 		breaking up that point and passing it in to ph3_point_set
 */
ph3_t* ph3_create (
	void* (*element_create) (void* input),
	void (*element_destroy) (void* element),
	phtree_key_t (*convert_to_key) (void* input),
	void (*convert_to_point) (ph3_t* tree, ph3_point_t* out, void* input));

int ph3_initialize (
	ph3_t* tree,
	void* (*element_create) (void* input),
	void (*element_destroy) (void*),
	phtree_key_t (*convert_to_key) (void* input),
	void (*convert_to_point) (ph3_t* tree, ph3_point_t* out, void* input));

/*
 * clear all entries/elements from the tree
 */
void ph3_clear (ph3_t* tree);
/*
 * free a tree
 */
void ph3_free (ph3_t* tree);

/*
 * run function on every element in the tree
 *
 * the data argument will be passed in to the iteration function when it is run
 */
void ph3_for_each (ph3_t* tree, phtree_iteration_function_t function, void* data);

/*
 * insert an element into the tree
 * if an element already exists at the specified point
 * 	the existing element will be returned
 *
 * index is whatever you are using to determine the spatial index of what you are inserting
 */
void* ph3_insert (ph3_t* tree, void* index);
/*
 * find an element in the tree at index
 *
 * returns the element if it exists
 * returns NULL if the element does not exist
 */
void* ph3_find (ph3_t* tree, void* index);
/*
 * remove an element from the tree
 */
void ph3_remove (ph3_t* tree, void* index);
/*
 * check if the tree is empty
 *
 * returns true if the tree is empty
 */
bool ph3_empty (ph3_t* tree);

/*
 * run a query on the tree
 *
 * the query's iteration function will be run on any element that is inside of the window
 *
 * data is any outside data that you want to pass in to the query function
 * 	if you want to store elements inside the window in a collection
 * 		pass the collection in as data
 * 			and store the elements in the collection inside your iteration function
 */
void ph3_query (ph3_t* tree, ph3_query_t* query, void* data);

/*
 * allocate a query
 */
ph3_query_t* ph3_query_create ();
void ph3_query_free (ph3_query_t* query);
void ph3_query_set (ph3_t* tree, ph3_query_t* query, void* min, void* max, phtree_iteration_function_t function);
void ph3_query_clear (ph3_query_t* query);
/*
 * if you need the center point of a window query
 */
void ph3_query_center (ph3_query_t* query, ph3_point_t* out);

/*
 * use this in your convert_to_point function
 * 	ph3_point_set calls tree->convert_to_key on each of the values passed in to it
 * 	so you should not call convert_to_key in your convert_to_point function
 */
void ph3_point_set (ph3_t* tree, ph3_point_t* point, void* a, void* b, void* c);

#endif
