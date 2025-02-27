#ifndef _ph6_h_
#define _ph6_h_

#include <stdbool.h>
#include <stdint.h>

#include "phtree64_common.h"

/*
 * an index point in the tree
 */
typedef struct ph6_point_t
{
	phtree_key_t values[6];
} ph6_point_t;

typedef union ph6_dual_node_t ph6_dual_node_t;
typedef struct ph6_node_t
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
	ph6_point_t point;
	/*
	 * a child can be either a node or an element
	 * children is an ordered dynamic array
	 */
	ph6_dual_node_t* children;
	// bit flags for which children are active
	uint64_t active_children;
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
} ph6_node_t;

typedef struct
{
	ph6_point_t point;
	void* element;
} ph6_entry_t;

/*
 * children of a node can be either nodes or elements
 * we use this union so that we're always allocating enough space for the larger
 * 	(nodes should always be larger but lets be safe for the future)
 *
 * we trade the ugliness of managing casts for the ugliness of specifying node or entry
 * hopefully this keeps it clear that we are dealing with ambiguous node objects
 */
typedef union ph6_dual_node_t
{
	ph6_node_t node;
	ph6_entry_t entry;
} ph6_dual_node_t;

/*
 * the tree type
 */
typedef struct ph6_t ph6_t;
typedef struct ph6_t
{
	ph6_dual_node_t root;

	/*
	 * user defined functions for handling user defined elements
	 */
	/*
	 * element_create needs to allocate memory for an element
	 * 	and set any default values
	 *
	 * input is the user defined object being passed in to ph6_insert
	 * 	when the element is being created
	 * if more data is needed to initialize an element than that
	 * 	you will need to use the pointer returned by ph6_insert
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
	void (*convert_to_point) (ph6_t* tree, ph6_point_t* point_out, void* input);
	/*
	 * convert_to_box_point converts your arbitrary data (input)
	 * 	to a spatial index specifically useful in box queries
	 * see ph6_query_box_set in the header file for more information
	 */
	void (*convert_to_box_point) (ph6_t* tree, ph6_point_t* point_out, void* input);
} ph6_t;

typedef struct ph6_query_t
{
	ph6_point_t min;
	ph6_point_t max;
	/*
	 * function will be run on all elements inside of the query
	 * if you want to keep a collection of the elements which are inside of the query window
	 * 	pass your collection structure in as data
	 * 		and add the elements to the collection inside of your function
	 */
	phtree_iteration_function_t function;
} ph6_query_t;


/*
 * allocate and initialize a new tree
 */
/*
 * !! the following 4 functions are REQUIRED for the tree to work !!
 *
 * void* element_create ()
 * 	allocates and initializes your custom tree element object
 *
 * 	the input into this function will be the first thing you pass in to ph6_insert
 * 		at the index of the element being created
 * 		if it is not possible to completely initialize the element
 * 			using that first thing being inserted
 * 			you will need to finish initialization using the pointer returned from ph6_insert
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
 * void convert_to_point (ph6_t* tree, ph6_point_t* out, void* input)
 * 	convert your spatial data into a spatial index in the tree
 * 	likely you will be inputting an n-dimensional point into this
 * 		breaking up that point and passing it in to ph6_point_set
 *
 * convert_to_box_point is optional and can be set to NULL
 * if it is set to null
 * 	ph6_query_box_set will set the query to something unlikely to return any results
 *
 * void convert_to_box_point (ph6_t* tree, ph6_point_t* out, void* input)
 * 	convert input in to special points used for box queries
 * 	!! make sure to use ph6_point_box_set and not the regular ph6_point_set !!
 */
ph6_t* ph6_create (
	void* (*element_create) (void* input),
	void (*element_destroy) (void* element),
	phtree_key_t (*convert_to_key) (void* input),
	void (*convert_to_point) (ph6_t* tree, ph6_point_t* out, void* input),
	void (*convert_to_box_point) (ph6_t* tree, ph6_point_t* out, void* input));

int ph6_initialize (
	ph6_t* tree,
	void* (*element_create) (void* input),
	void (*element_destroy) (void*),
	phtree_key_t (*convert_to_key) (void* input),
	void (*convert_to_point) (ph6_t* tree, ph6_point_t* out, void* input),
	void (*convert_to_box_point) (ph6_t* tree, ph6_point_t* out, void* input));

/*
 * clear all entries/elements from the tree
 */
void ph6_clear (ph6_t* tree);
/*
 * free a tree
 */
void ph6_free (ph6_t* tree);

/*
 * run function on every element in the tree
 *
 * the data argument will be passed in to the iteration function when it is run
 */
void ph6_for_each (ph6_t* tree, phtree_iteration_function_t function, void* data);

/*
 * insert an element into the tree
 * if an element already exists at the specified point
 * 	the existing element will be returned
 *
 * index is whatever you are using to determine the spatial index of what you are inserting
 */
void* ph6_insert (ph6_t* tree, void* index);
/*
 * find an element in the tree at index
 *
 * returns the element if it exists
 * returns NULL if the element does not exist
 */
void* ph6_find (ph6_t* tree, void* index);
/*
 * remove an element from the tree
 */
void ph6_remove (ph6_t* tree, void* index);
/*
 * check if the tree is empty
 *
 * returns true if the tree is empty
 */
bool ph6_empty (ph6_t* tree);

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
void ph6_query (ph6_t* tree, ph6_query_t* query, void* data);

/*
 * allocate a query
 */
ph6_query_t* ph6_query_create ();
void ph6_query_free (ph6_query_t* query);
void ph6_query_set (ph6_t* tree, ph6_query_t* query, void* min, void* max, phtree_iteration_function_t function);
/*
 * box queries are only relevant in trees with an even number of dimensions
 * in a phtree of DIMENSIONS you can represent axis aligned boxes of (DIMENSIONS / 2)
 * 	as single points composed of the min and max points of the box
 * 	examples:
 * 		1d line segment from points 1 to 5
 * 			in 2d is a point (1, 5)
 * 		2d box with min = (1, 2) and max = (3, 4)
 * 			in 4d is a point (1, 2, 3, 4)
 * 		3d cube with min = (1, 2, 3) and max = (4, 5, 6)
 * 			in 6d is a point (1, 2, 3, 4, 5, 6)
 *
 * querying these lower dimensional boxes stored as higher dimensional points
 * 	requires setting up query->min and query->max in special ways
 * use ph6_query_box_set to properly set up a query for boxes
 *
 * for more information check: https://tzaeschke.github.io/phtree-site/#rectangles--boxes-as-key
 *
 * by default, queries only include points which are entirely within the query
 * when dealing with boxes however
 * 	you may want to include boxes which intersect the query
 * 		but are not _entirely_ contained in the query
 *
 * to query if a lower dimensional point intersects higher dimensional boxes
 * 	pass in a higher dimensional point which repeats the lower dimensional point
 * 		as both min and max
 * 	example:
 * 		2d point = (1, 2)
 * 			4d query point = (1, 2, 1, 2) as both min and max
 * 		3d point = (1, 2, 3)
 * 			6d query point = (1, 2, 3, 1, 2, 3) as both min and max
 * 	set intersect to 'true'
 * the ph6_query_box_point_set function is a wrapper which does this for you
 *
 * set intersect to 'true' to include intersecting boxes
 * set intersect to 'false' to only include boxes entirely contained in the query box
 */
void ph6_query_box_set (ph6_t* tree, ph6_query_t* query, bool intersect, void* min, void* max, phtree_iteration_function_t function);

/*
 * ph6_query_box_point_set is a convenience function
 * 	for querying a single lower dimensional point against higher dimensional boxes
 * you can do the same with regular ph6_query_box_set
 */
void ph6_query_box_point_set (ph6_t* tree, ph6_query_t* query, void* point, phtree_iteration_function_t function);
void ph6_query_clear (ph6_query_t* query);
/*
 * if you need the center point of a window query
 */
void ph6_query_center (ph6_query_t* query, ph6_point_t* out);

/*
 * use this in your convert_to_point function
 * 	ph6_point_set calls tree->convert_to_key on each of the values passed in to it
 * 	so you should not call convert_to_key in your convert_to_point function
 */
void ph6_point_set (ph6_t* tree, ph6_point_t* point, void* a, void* b, void* c, void* d, void* e, void* f);
/*
 * use this in your convert_to_box_point function
 * 
 * ph6_point_box_set only takes (DIMENSIONS / 2) inputs
 * 	as it is specifically used for querying lower dimensional boxes
 */
void ph6_point_box_set (ph6_t* tree, ph6_point_t* point, void* a, void* b, void* c);

#endif
