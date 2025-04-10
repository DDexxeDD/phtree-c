#ifndef _phtree16_common_h_
#define _phtree16_common_h_
/*
 * this _phtree_common_h_ section contains functionality which is used in all phtrees
 * 	regardless of their dimensionality
 */

#include <stdbool.h>
#include <stdint.h>

typedef uint16_t phtree_key_t;

// use this for converting input into keys
// 	this should be the same as the bit width of your key type
#define PHTREE_BIT_WIDTH 16

// KEY_ONE is an unsigned value of 1
#define PHTREE_KEY_ONE UINT16_C(1)
#define PHTREE_KEY_MAX UINT16_MAX

// if you need to flip the sign bit of phtree_key_t in a conversion function
#define PHTREE_SIGN_BIT (PHTREE_KEY_ONE << (PHTREE_BIT_WIDTH - 1))

/*
 * generic key converters
 */
// phtree_int16_to_key expects input to be a pointer to a 16 integer
phtree_key_t phtree_int16_to_key (void* input);

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

uint64_t phtree_count_leading_zeroes (uint64_t bit_string);
uint64_t phtree_count_trailing_zeroes (uint64_t bit_string);
uint64_t phtree_popcount (uint64_t bit_string);

#if defined (_MSC_VER)
#include <intrin.h>
uint64_t msvc_count_leading_zeoes (uint64_t bit_string);
uint64_t msvc_count_trailing_zeroes (uint64_t bit_string);
#endif

#if defined (__clang__) || defined (__GNUC__)
#define count_leading_zeroes(bit_string) (0 ? 64U : __builtin_clzll (bit_string))
#define count_trailing_zeroes(bit_string) (0 ? 64U : __builtin_ctzll (bit_string))
#define popcount __builtin_popcountll
#elif defined (_MSC_VER)
#define count_leading_zeroes(bit_string) count_leading_zeoes_msvc (bit_string)
#define count_trailing_zeroes(bit_string) count_trailing_zeroes_msvc (bit_string)
#define popcount __popcnt64
#else
#define count_leading_zeroes(bit_string) phtree_count_leading_zeroes (bit_string)
#define count_trailing_zeroes(bit_string) phtree_count_trailing_zeroes (bit_string)
#define popcount phtree_popcount
#endif

#endif  // end _phtree16_common_h_
