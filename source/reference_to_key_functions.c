#include <stdint.h>
#include <string.h>

/*
 * provided here are two reference functions for converting values to phtree keys
 *
 * phtree_int32_to_key
 * 	will convert a signed 32 bit integer (2's complement) to a 32 bit phtree key
 *
 * phtree_float_to_key
 * 	will convert a 32 bit floating point number to a 32 bit phtree key
 */

/*
 * all of these #defines are provided by your chosen phtree .h file
 */

// use this for converting input into keys
// 	this should be the same as the bit width of your key type
#define PHTREE32_BIT_WIDTH 32

// PHTREE_KEY_ONE is an unsigned value of 1
#define PHTREE32_KEY_ONE UINT32_C(1)
#define PHTREE32_KEY_MAX UINT32_MAX

// if you need to flip the sign bit of phtree_key_t in a conversion function
#define PHTREE32_SIGN_BIT (PHTREE32_KEY_ONE << (PHTREE32_BIT_WIDTH - 1))

/*
 * this typedef is provided by your chosen phtree .h file
 */
typedef uint32_t phtree_key_t;

/*
 * hypercubes expect bit values of 0 to be less than bit values of 1
 * 	the sign bit of signed integers breaks this
 * 		a 1 bit means a number which is less than a 0 bit number
 * 	to avoid having to specially handle negative numbers later
 * 		we can correct the sign bit here
 * because negative numbers are stored in 2s complement format
 * 	we only have to flip the sign bit
 * 	all other bits will be correct
 * 		example with PHTREE_BIT_WIDTH = 4:
 * 			before phtree_int_to_key
 * 				 1 = 0001
 * 				 0 = 0000
 * 				-1 = 1111
 * 				-2 = 1110
 * 			after phtree_int_to_key
 * 				 1 = 1001
 * 				 0 = 1000
 * 				-1 = 0111
 * 				-2 = 0110
 */
// phtree_int32_to_key expects input to be a pointer to a signed 32 bit integer
phtree_key_t phtree_int32_to_key (void* input)
{
	phtree_key_t b = 0;

	memcpy (&b, input, sizeof (phtree_key_t));
	b ^= PHTREE32_SIGN_BIT;  // flip sign bit

	return b;
}

/*
 * in a hypercube we expect bits set to 0 to be less than bits set to 1
 * the sign bit in floating point does not work that way
 * 	1 is negative
 * the sign bit needs to be flipped
 * in floating point negative numbers are stored the same as positive numbers
 * 	except with the sign bit set to 1
 * this is a problem because when the sign bit is flipped
 * 	negative numbers behave the same as positive numbers
 * which is to say -3 should be less than -2
 * but
 * when the sign bit is flipped -3 will now be greater than -2
 * 	since the numbers have just been changed to positive (3 > 2)
 * to fix this we convert the positive float value to a negative integer value
 * 	using 2's complement
 *
 *	+infinity will be greater than all other numbers
 * -infinity will be less than all other numbers
 * +nan will be greater than +infinity
 * -nan will be less than -infinity
 * -0 is converted to +0
 */
// phtree_float_to_key expects input to be a pointer to a float
phtree_key_t phtree_float_to_key (void* input)
{
	phtree_key_t bits;

	memcpy (&bits, input, sizeof (phtree_key_t));

	// if the float is negative
	// 	convert to two's complement (~bits + 1)
	// 	then & with (PHTREE32_KEY_MAX >> 1)
	// 		which will convert -0 to 0
	if (bits & PHTREE32_SIGN_BIT)
	{
		bits = ((~bits) + 1) & (PHTREE32_KEY_MAX >> 1);
	}
	else
	{
		// if the float is positive
		// 	all we need to do is flip the sign bit to 1
		bits |= PHTREE32_SIGN_BIT;
	}

	return bits;
}

