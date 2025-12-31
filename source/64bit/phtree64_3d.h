#ifndef _ph3_h_
#define _ph3_h_

#include <stdbool.h>
#include <stdint.h>

/*
 * begin common section
 *
 * this common section contains functionality which is used in all 64 bit phtrees
 * 	regardless of their dimensionality
 */

typedef uint64_t phtree_key_t;

// use this for converting input into keys
// 	this should be the same as the bit width of your key type
#define PHTREE64_BIT_WIDTH 64

// PHTREE_KEY_ONE is an unsigned value of 1
#define PHTREE64_KEY_ONE UINT64_C(1)
#define PHTREE64_KEY_MAX UINT64_MAX

// if you need to flip the sign bit of phtree_key_t in a conversion function
#define PHTREE64_SIGN_BIT (PHTREE64_KEY_ONE << (PHTREE64_BIT_WIDTH - 1))

/*
 * functions to be run on elements when iterating the tree
 * data is any outside data you want to pass in to the function
 */
typedef void (*phtree_iteration_function_t) (void* element, void* data);

/*
 * end common section
 */

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
	 * the distance between a node and its parent, NOT inclusive
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
	 * required
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
	 * required
	 * element_destroy needs to free any memory allocated for an element
	 * 	including the element itself
	 */
	void (*element_destroy) (void* element);
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
 * !! the following 2 functions are REQUIRED for the tree to work !!
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
 *
 * the children_* functions are optional
 *
 * passing NULL to the children_* parameters
 * 	will cause the phtree to use the std library memory functions (malloc, realloc, free)
 *
 * if you want to do something like use a pool for tree nodes
 * 	the children_* functions are what you would use
 */
void ph3_initialize (
	ph3_t* tree,
	void* (*element_create) (void* input),
	void (*element_destroy) (void*),
	void* (*children_malloc) (size_t size),
	void* (*children_realloc) (void* pointer, size_t size),
	void (*children_free) (void* pointer));

/*
 * ph3_create
 * 	if you want to declare and initialize a phtree in one line
 */
ph3_t ph3_create (
	void* (*element_create) (void* input),
	void (*element_destroy) (void* element),
	void* (*children_malloc) (size_t size),
	void* (*children_realloc) (void* pointer, size_t size),
	void (*children_free) (void* pointer));

/*
 * clear all entries/elements from the tree
 */
void ph3_clear (ph3_t* tree);

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
void* ph3_insert (ph3_t* tree, ph3_point_t* point, void* element);
/*
 * find an element in the tree at index
 *
 * returns the element if it exists
 * returns NULL if the element does not exist
 */
void* ph3_find (ph3_t* tree, ph3_point_t* index);
/*
 * remove an element from the tree
 */
void ph3_remove (ph3_t* tree, ph3_point_t* point);
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
 * ph3_query_create
 * 	if you want to declare and initialize a query in one line
 */
ph3_query_t ph3_query_create (ph3_t* tree, void* min, void* max, phtree_iteration_function_t function);
void ph3_query_set (ph3_t* tree, ph3_query_t* query, ph3_point_t* min, ph3_point_t* max, phtree_iteration_function_t function);
void ph3_query_clear (ph3_query_t* query);

/*
 * convenience function for setting the values of a ph3_point_t
 */
void ph3_point_set (ph3_point_t* point, phtree_key_t a, phtree_key_t b, phtree_key_t c);

#endif
