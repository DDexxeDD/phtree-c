#ifndef _ph3_h_
#define _ph3_h_

#include <stdbool.h>
#include <stdint.h>

#ifndef _phtree_common_h_
#define _phtree_common_h_
/*
 * this _phtree_common_h_ section contains functionality which is used in all phtrees
 * 	regardless of their dimensionality
 */

typedef uint32_t phtree_key_t;

// use this for converting input into keys
// 	this should be the same as the bit width of your key type
#define PHTREE_BIT_WIDTH 32

/*
 * generic key converters
 */
phtree_key_t phtree_int32_to_key (int32_t a);
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

typedef struct ph3_t ph3_t;
typedef struct ph3_point_t ph3_point_t;
typedef struct ph3_window_query_t ph3_window_query_t;

/*
 * allocate and initialize a new tree
 */
/*
 * getting the tree to work requires you to define 4 functions
 * these 4 functions will be passed in to either ph3_create or ph3_initialize
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
 * void convert_to_point (ph3_t* tree, ph3_point_t* out, void* index)
 * 	convert your spatial data into a spatial index in the tree
 * 	likely you will be inputting an n-dimensional point into this
 * 		breaking up that point and passing it in to ph3_point_set
 */
ph3_t* ph3_create (
	void* (*element_create) (),
	void (*element_destroy) (void* element),
	phtree_key_t (*convert_to_key) (void* input),
	void (*convert_to_point) (ph3_t* tree, ph3_point_t* out, void* index));


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
void ph3_query (ph3_t* tree, ph3_window_query_t* query, void* data);

/*
 * allocate a query
 */
ph3_window_query_t* ph3_query_create ();
void ph3_query_free (ph3_window_query_t* query);
void ph3_query_set (ph3_t* tree, ph3_window_query_t* query, void* min, void* max, phtree_iteration_function_t function);
/*
 * initialize a query window around a single point using chebyshev distance
 *
 * this is basically a square nearest neighbor window
 */
void ph3_query_chebyshev (ph3_t* tree, ph3_window_query_t* query, void* center, unsigned int distance, phtree_iteration_function_t function);
void ph3_query_clear (ph3_window_query_t* query);
/*
 * if you need the center point of a window query
 */
void ph3_query_center (ph3_window_query_t* query, ph3_point_t* out);

/*
 * use this in your convert_to_point function
 * 	ph3_point_set calls tree->convert_to_key on each of the values passed in to it
 * 	so you should not call convert_to_key in your convert_to_point function
 */
void ph3_point_set (ph3_t* tree, ph3_point_t* point, void* a, void* b, void* c);

#endif
