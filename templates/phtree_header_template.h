#ifndef _{{prefix}}_h_
#define _{{prefix}}_h_

#include <stdbool.h>
#include <stdint.h>

#ifndef _phtree_common_h_
#define _phtree_common_h_
/*
 * this _phtree_common_h_ section contains functionality which is used in all phtrees
 * 	regardless of their dimensionality
 */

typedef uint{{bit_width}}_t phtree_key_t;

// use this for converting input into keys
// 	this should be the same as the bit width of your key type
#define PHTREE_BIT_WIDTH {{bit_width}}

/*
 * generic key converters
 */
phtree_key_t phtree_int{{bit_width}}_to_key (int{{bit_width}}_t a);
phtree_key_t phtree_float_to_key (float x);
/*
 * only use phtree_double_to_key if your PHTREE_BIT_WIDTH is 64
 * 	otherwise you will lose precision
 */
phtree_key_t phtree_double_to_key (double x);

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

#endif  // end _phtree_common_h_

typedef struct {{prefix}}_t {{prefix}}_t;
typedef struct {{prefix}}_point_t {{prefix}}_point_t;
typedef struct {{prefix}}_window_query_t {{prefix}}_window_query_t;

/*
 * allocate and initialize a new tree
 */
/*
 * getting the tree to work requires you to define 4 functions
 * these 4 functions will be passed in to either {{prefix}}_create or {{prefix}}_initialize
 *
 * void* element_create ()
 * 	allocates and initializes your custom tree element object
 * 	return a pointer to the object you allocated
 *
 * void element_destory (void* element)
 * 	deallocates/frees whatever was allocated by element_create
 *
 * phtree_key_t convert_to_key (void* input)
 * 	converts input into a single phtree_key_t value
 * 	likely you will use this to convert a single number into a key
 *
 * void convert_to_point ({{prefix}}_t* tree, {{prefix}}_point_t* out, void* index)
 * 	convert your spatial data into a spatial index in the tree
 * 	likely you will be inputting an n-dimensional point into this
 * 		breaking up that point and passing it in to {{prefix}}_point_set
 */
{{prefix}}_t* {{prefix}}_create (
	void* (*element_create) (),
	void (*element_destroy) (void* element),
	phtree_key_t (*convert_to_key) (void* input),
	void (*convert_to_point) ({{prefix}}_t* tree, {{prefix}}_point_t* out, void* index));

{{#unused}}
/*
 * initialize an existing tree
 */
int {{prefix}}_initialize (
	{{prefix}}_t* tree,
	void* (*element_create) (),
	void (*element_destroy) (void*),
	phtree_key_t (*convert_to_key) (void* input),
	void (*convert_to_point) ({{prefix}}_t* tree, {{prefix}}_point_t* out, void* index));
{{/unused}}

/*
 * clear all entries/elements from the tree
 */
void {{prefix}}_clear ({{prefix}}_t* tree);
/*
 * free a tree
 */
void {{prefix}}_free ({{prefix}}_t* tree);

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
void* {{prefix}}_insert ({{prefix}}_t* tree, void* index);
/*
 * find an element in the tree at index
 *
 * returns the element if it exists
 * returns NULL if the element does not exist
 */
void* {{prefix}}_find ({{prefix}}_t* tree, void* index);
/*
 * remove an element from the tree
 */
void {{prefix}}_remove ({{prefix}}_t* tree, void* index);
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
void {{prefix}}_query ({{prefix}}_t* tree, {{prefix}}_window_query_t* query, void* data);

/*
 * allocate a query
 */
{{prefix}}_window_query_t* {{prefix}}_query_create ();
void {{prefix}}_query_free ({{prefix}}_window_query_t* query);
void {{prefix}}_query_set ({{prefix}}_t* tree, {{prefix}}_window_query_t* query, void* min, void* max, phtree_iteration_function_t function);
/*
 * initialize a query window around a single point using chebyshev distance
 *
 * this is basically a square nearest neighbor window
 */
void {{prefix}}_query_chebyshev ({{prefix}}_t* tree, {{prefix}}_window_query_t* query, void* center, unsigned int distance, phtree_iteration_function_t function);
void {{prefix}}_query_clear ({{prefix}}_window_query_t* query);
/*
 * if you need the center point of a window query
 */
void {{prefix}}_query_center ({{prefix}}_window_query_t* query, {{prefix}}_point_t* out);

/*
 * use this in your convert_to_point function
 * 	{{prefix}}_point_set calls tree->convert_to_key on each of the values passed in to it
 * 	so you should not call convert_to_key in your convert_to_point function
 */
void {{prefix}}_point_set ({{prefix}}_t* tree, {{prefix}}_point_t* point, void* a{{#2d}}, void* b{{#3d}}, void* c{{/3d}}{{/2d}});

#endif
