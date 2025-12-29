#ifndef _{{prefix}}_h_
#define _{{prefix}}_h_

#include <stdbool.h>
#include <stdint.h>

/*
 * begin common section
 *
 * this common section contains functionality which is used in all {{bit_width}} bit phtrees
 * 	regardless of their dimensionality
 */

typedef uint{{bit_width}}_t phtree_key_t;

// use this for converting input into keys
// 	this should be the same as the bit width of your key type
#define PHTREE{{bit_width}}_BIT_WIDTH {{bit_width}}

// PHTREE_KEY_ONE is an unsigned value of 1
#define PHTREE{{bit_width}}_KEY_ONE UINT{{bit_width}}_C(1)
#define PHTREE{{bit_width}}_KEY_MAX UINT{{bit_width}}_MAX

// if you need to flip the sign bit of phtree_key_t in a conversion function
#define PHTREE{{bit_width}}_SIGN_BIT (PHTREE{{bit_width}}_KEY_ONE << (PHTREE{{bit_width}}_BIT_WIDTH - 1))

/*
 * functions to be run on elements when iterating the tree
 * data is any outside data you want to pass in to the function
 */
typedef void (*phtree_iteration_function_t) (void* element, void* data);

/*
 * to use your own custom allocators
 * 	define these somewhere before here
 */
#ifndef phtree_calloc
#define phtree_calloc calloc
#endif
#ifndef phtree_free
#define phtree_free free
#endif
#ifndef phtree_realloc
#define phtree_realloc realloc
#endif

/*
 * end common section
 */

/*
 * an index point in the tree
 */
typedef struct {{prefix}}_point_t
{
	phtree_key_t values[{{dimensions}}];
} {{prefix}}_point_t;

typedef struct {{prefix}}_node_t {{prefix}}_node_t;
typedef struct {{prefix}}_node_t
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
	{{prefix}}_point_t point;
	/*
	 * a child can be either a node or an element
	 * 	we store everything as a node because most of the time we are working with nodes
	 * 	only in a few spots do we work with entries
	 * children is an ordered dynamic array
	 */
	{{prefix}}_node_t* children;
	// bit flags for which children are active
	uint{{max_children}}_t active_children;
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
} {{prefix}}_node_t;

/*
 * the tree type
 */
typedef struct {{prefix}}_t {{prefix}}_t;
typedef struct {{prefix}}_t
{
	{{prefix}}_node_t root;

	/*
	 * user defined functions for handling user defined elements
	 */
	/*
	 * required
	 * element_create needs to allocate memory for an element
	 * 	and set any default values
	 *
	 * input is the user defined object being passed in to {{prefix}}_insert
	 * 	when the element is being created
	 * if more data is needed to initialize an element than that
	 * 	you will need to use the pointer returned by {{prefix}}_insert
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
} {{prefix}}_t;

typedef struct {{prefix}}_query_t
{
	{{prefix}}_point_t min;
	{{prefix}}_point_t max;
	/*
	 * function will be run on all elements inside of the query
	 * if you want to keep a collection of the elements which are inside of the query window
	 * 	pass your collection structure in as data
	 * 		and add the elements to the collection inside of your function
	 */
	phtree_iteration_function_t function;
} {{prefix}}_query_t;


/*
 * {{prefix}}_create
 * 	if you want to declare and initialize a phtree in one line
 */
/*
 * !! the following 2 functions are REQUIRED for the tree to work !!
 *
 * void* element_create ()
 * 	allocates and initializes your custom tree element object
 *
 * 	the input into this function will be the first thing you pass in to {{prefix}}_insert
 * 		at the index of the element being created
 * 		if it is not possible to completely initialize the element
 * 			using that first thing being inserted
 * 			you will need to finish initialization using the pointer returned from {{prefix}}_insert
 *
 * 	return a pointer to the object you allocated
 *
 * void element_destory (void* element)
 * 	deallocates/frees whatever was allocated by element_create
 */
{{prefix}}_t {{prefix}}_create (
	void* (*element_create) (void* input),
	void (*element_destroy) (void* element));

void {{prefix}}_initialize (
	{{prefix}}_t* tree,
	void* (*element_create) (void* input),
	void (*element_destroy) (void*));

/*
 * clear all entries/elements from the tree
 */
void {{prefix}}_clear ({{prefix}}_t* tree);

/*
 * run function on every element in the tree
 *
 * the data argument will be passed in to the iteration function when it is run
 */
void {{prefix}}_for_each ({{prefix}}_t* tree, phtree_iteration_function_t function, void* data);

/*
 * insert an element into the tree
 * if an element already exists at the specified point
 * 	the existing element will be returned
 *
 * index is whatever you are using to determine the spatial index of what you are inserting
 */
void* {{prefix}}_insert ({{prefix}}_t* tree, {{prefix}}_point_t* point, void* element);
/*
 * find an element in the tree at index
 *
 * returns the element if it exists
 * returns NULL if the element does not exist
 */
void* {{prefix}}_find ({{prefix}}_t* tree, {{prefix}}_point_t* index);
/*
 * remove an element from the tree
 */
void {{prefix}}_remove ({{prefix}}_t* tree, {{prefix}}_point_t* point);
/*
 * check if the tree is empty
 *
 * returns true if the tree is empty
 */
bool {{prefix}}_empty ({{prefix}}_t* tree);

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
void {{prefix}}_query ({{prefix}}_t* tree, {{prefix}}_query_t* query, void* data);

/*
 * {{prefix}}_query_create
 * 	if you want to declare and initialize a query in one line
 */
{{prefix}}_query_t {{prefix}}_query_create ({{prefix}}_t* tree, void* min, void* max, phtree_iteration_function_t function);
void {{prefix}}_query_set ({{prefix}}_t* tree, {{prefix}}_query_t* query, {{prefix}}_point_t* min, {{prefix}}_point_t* max, phtree_iteration_function_t function);
{{#even}}
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
 * use {{prefix}}_query_box_set to properly set up a query for boxes
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
 * the {{prefix}}_query_box_point_set function is a wrapper which does this for you
 *
 * set intersect to 'true' to include intersecting boxes
 * set intersect to 'false' to only include boxes entirely contained in the query box
 */
void {{prefix}}_query_box_set ({{prefix}}_t* tree, {{prefix}}_query_t* query, bool intersect, {{prefix}}_point_t* min, {{prefix}}_point_t* max, phtree_iteration_function_t function);

/*
 * {{prefix}}_query_box_point_set is a convenience function
 * 	for querying a single lower dimensional point against higher dimensional boxes
 * you can do the same with regular {{prefix}}_query_box_set
 */
void {{prefix}}_query_box_point_set ({{prefix}}_t* tree, {{prefix}}_query_t* query, {{prefix}}_point_t* point, phtree_iteration_function_t function);
{{/even}}
void {{prefix}}_query_clear ({{prefix}}_query_t* query);

/*
 * convenience function for setting the values of a {{prefix}}_point_t
 */
void {{prefix}}_point_set ({{prefix}}_point_t* point, phtree_key_t a{{#2d}}, phtree_key_t b{{#3d}}, phtree_key_t c{{#4d}}, phtree_key_t d{{#5d}}, phtree_key_t e{{#6d}}, phtree_key_t f{{/6d}}{{/5d}}{{/4d}}{{/3d}}{{/2d}});
{{#even}}
/*
 * when treating a higher dimensional space as representing boxes
 * 	a lower dimensional point will repeat itself
 * 	for example:
 * 		in a 4d space which represents boxes
 * 			the 2d point (1, 2) would be represented as (1, 2, 1, 2) in the 4d space
 *
 * this function is a convenience to set a higher dimensional point
 * 	using a lower dimensional set of keys
 */
void {{prefix}}_point_box_set ({{prefix}}_point_t* point, phtree_key_t a{{#4d}}, phtree_key_t b{{#6d}}, phtree_key_t c{{/6d}}{{/4d}});
{{/even}}

#endif
