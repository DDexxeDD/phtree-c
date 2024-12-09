#ifndef _phtree_h_
#define _phtree_h_

#include <stdbool.h>
#include <stdint.h>

#include "cvector.h"

// the maximum bit width we support
// 	need this for anything involving counting leading zeroes
// 		because we use the 64 bit version of those functions/builtins
// !! changing this will break things !!
#define BIT_WIDTH_MAX 64

// BIT_WIDTH needs to be 8, 16, 32, or 64
// 	BIT_WIDTH is used to choose one of the stdint.h integer types
// 		int8_t, int16_t, int32_t, int64_t
#define BIT_WIDTH 16

/*
 * all of these underscore prefixed defines are here
 * 	so we dont have to redefine these macros if we change BIT_WIDTH
 */

// KEY_ONE is an unsigned value of 1
// 	based on the BIT_WIDTH
#define __KEY_ONE(x) UINT##x##_C(1)
#define _KEY_ONE(x) __KEY_ONE(x)
#define KEY_ONE _KEY_ONE(BIT_WIDTH)

#define __KEY_ONE_SIGNED(x) INT##x##_C(1)
#define _KEY_ONE_SIGNED(x) __KEY_ONE_SIGNED(x)
#define KEY_ONE_SIGNED _KEY_ONE_SIGNED(BIT_WIDTH)

#define __KEY_MAX(x) UINT##x##_MAX
#define _KEY_MAX(x) __KEY_MAX(x)
#define KEY_MAX _KEY_MAX(BIT_WIDTH)

#define __KEY_MIN(x) UINT##x##_MIN
#define _KEY_MIN(x) __KEY_MIN(x)
#define KEY_MIN _KEY_MIN(BIT_WIDTH)

#define __INT_MAX(x) INT##x##_MAX
#define _INT_MAX(x) __INT_MAX(x)
#define INT_MAX _INT_MAX(BIT_WIDTH)

#define __INT_MIN(x) INT##x##_MIN
#define _INT_MIN(x) __INT_MIN(x)
#define INT_MIN _INT_MIN(BIT_WIDTH)

// key_type_t is an unsinged integer type from stdint
// 	uint8_t, uint16_t, uint32_t, or uint64_t
// 	defined by BIT_WIDTH
#define __key_type_t(x) uint##x##_t
#define _key_type_t(x) __key_type_t(x)
#define key_type_t _key_type_t(BIT_WIDTH)

// key_type_signed_t is used when converting signed integer input values to unsigned key values
#define __key_type_signed_t(x) int##x##_t
#define _key_type_signed_t(x) __key_type_signed_t(x)
#define key_type_signed_t _key_type_signed_t(BIT_WIDTH)

// DIMENSIONS should be 1, 2, or 3
// 	because we use plain arrays to store node children
// 		4+ dimenions would likely have a large amount of unused children (very memory inefficient)
// 	if you want 4+ dimensions
// 		you will need a different, more complex, way of managing node children
#define DIMENSIONS 2

// NODE_CHILD_COUNT is how many children a node can have
// 	based on the number of DIMENSIONS
// 	this is a power of 2: 2^DIMENSIONS
#define NODE_CHILD_COUNT (KEY_ONE << (DIMENSIONS))

typedef key_type_t phtree_key_t;

typedef struct
{
	phtree_key_t values[DIMENSIONS];
} phtree_point_t;

typedef struct phtree_node_t phtree_node_t;
typedef struct
{
	phtree_point_t point;
	phtree_node_t* parent;
	cvector (int) elements;
} phtree_entry_t;

typedef struct
{
	phtree_point_t min;
	phtree_point_t max;
	// the entries which are contained in this window
	// 	this is where we store the result when a query is run
	cvector (phtree_entry_t*) entries;
} phtree_window_query_t;

typedef struct phtree_node_t
{
	// parent is used when removing nodes
	phtree_node_t* parent;
	// children are either phtree_node_t* or phtree_entry_t*
	// if the node is a leaf
	// 	children are phtree_entry_t*
	// if the node is _not_ a leaf
	// 	children are phtree_node_t*
	void* children[NODE_CHILD_COUNT];
	// how many active (not NULL) children a node has
	uint8_t child_count;

	// the distance between a node and its parent, not inclusive
	// example: 
	// 	parent postfix_length == 5
	// 	child postfix_length == 1
	// 	child infix_length == 3   // not 4
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
	phtree_point_t point;
	// this is only here for keeping track of nodes in this implementation
	char id;
} phtree_node_t;

typedef struct
{
	phtree_node_t root;
	// dont have more than 2^64 entries :D
} phtree_t;

// we are _not_ supporting more than 3 dimensions
// 	so we wont ever need a number larger than 2^3 (8)
// 	uint8_t can support up to 8 dimensions
typedef uint8_t hypercube_address_t;

phtree_t* tree_create ();
void tree_initialize (phtree_t* tree);
void tree_clear (phtree_t* tree);
phtree_entry_t* tree_insert (phtree_t* tree, phtree_point_t* point, int value);
phtree_entry_t* tree_find (phtree_t* tree, phtree_point_t* point);
bool tree_point_exists (phtree_t* tree, phtree_point_t* point);
void tree_remove (phtree_t* tree, phtree_point_t* point);
void tree_remove_element (phtree_t* tree, phtree_point_t* point, int element);
bool tree_empty (phtree_t* tree);
void tree_query_window (phtree_t* tree, phtree_window_query_t* query);

phtree_window_query_t* window_query_create (phtree_point_t min, phtree_point_t max);
void window_query_clear (phtree_window_query_t* query);
void window_query_free (phtree_window_query_t* query);

bool point_set (phtree_point_t* point, key_type_signed_t a, key_type_signed_t b);
phtree_point_t* point_create (key_type_signed_t a, key_type_signed_t b);

void entry_free (phtree_entry_t* entry);

phtree_key_t value_to_key (key_type_signed_t a);
key_type_signed_t key_to_value (phtree_key_t a);

#endif
