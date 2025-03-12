#include <string.h>

#include "phtree8_common.h"

// hypercubes expect bit values of 0 to be less than bit values of 1
// 	the sign bit of signed integers breaks this
// 		a 1 bit means a number which is less than a 0 bit number
// 	to avoid having to specially handle negative numbers later
// 		we can correct the sign bit here
// because negative numbers are stored in 2s complement format
// 	we only have to flip the sign bit
// 	all other bits will be correct
// 		example with PHTREE_BIT_WIDTH = 4:
// 			before phtree_int_to_key
// 				 1 = 0001
// 				 0 = 0000
// 				-1 = 1111
// 				-2 = 1110
// 			after phtree_int_to_key
// 				 1 = 1001
// 				 0 = 1000
// 				-1 = 0111
// 				-2 = 0110
phtree_key_t phtree_int8_to_key (void* input)
{
	phtree_key_t b = 0;

	memcpy (&b, input, sizeof (phtree_key_t));
	b ^= PHTREE_SIGN_BIT;  // flip sign bit

	return b;
}

// in a hypercube we expect bits set to 0 to be less than bits set to 1
// the sign bit in floating point does not work that way
// 	1 is negative
// the sign bit needs to be flipped
// in floating point negative numbers are stored the same as positive numbers
// 	except with the sign bit set to 1
// this is a problem because when the sign bit is flipped
// 	negative numbers behave the same as positive numbers
// which is to say -3 should be less than -2
// but
// when the sign bit is flipped -3 will now be greater than -2
// 	since the numbers have just been changed to positive (3 > 2)
// to fix this we convert the positive float value to a negative integer value
// 	using 2's complement
//
//	+infinity will be greater than all other numbers
// -infinity will be less than all other numbers
// +nan will be greater than +infinity
// -nan will be less than -infinity
// -0 is converted to +0

/*
 * count leading and trailing zeroes
 */

#if defined (_MSC_VER)
#include <intrin.h>
uint64_t msvc_count_leading_zeoes (uint64_t bit_string)
{
	unsigned long leading_zero = 0;
	return _BitScanReverse64 (&leading_zero, bit_string) ? 63 - leading_zero : 64U;
}

uint64_t msvc_count_trailing_zeroes (uint64_t bit_string)
{
	unsigned long trailing_zero = 0;
	return _BitScaneForward64 (&trailing_zero, bit_string) ? trailing_zero : 64U;
}
#endif

uint64_t phtree_count_leading_zeroes (uint64_t bit_string)
{
	if (bit_string == 0)
	{
		return 64;
	}

	uint64_t n = 1;
	uint32_t x = (bit_string >> 32);

	if (x == 0)
	{
		n += 32;
		x = (int) bit_string;
	}

	if (x >> 16 == 0)
	{
		n += 16;
		x <<= 16;
	}

	if (x >> 24 == 0)
	{
		n += 8;
		x <<= 8;
	}

	if (x >> 28 == 0)
	{
		n += 4;
		x <<= 4;
	}

	if (x >> 30 == 0)
	{
		n += 2;
		x <<= 2;
	}

	n -= x >> 31;

	return n;
}

uint64_t phtree_count_trailing_zeroes (uint64_t bit_string)
{
	if (bit_string == 0)
	{
		return 64;
	}

	uint32_t x = 0;
	uint32_t y = 0;
	uint16_t n = 63;

	y = (uint32_t) bit_string;

	if (y != 0)
	{
		n = n - 32;
		x = y;
	}
	else
	{
		x = (uint32_t) (bit_string >> 32);
	}

	y = x << 16;

	if (y != 0)
	{
		n = n - 16;
		x = y;
	}

	y = x << 8;

	if (y != 0)
	{
		n = n - 8;
		x = y;
	}

	y = x << 4;

	if (y != 0)
	{
		n = n - 4;
		x = y;
	}

	y = x << 2;

	if (y != 0)
	{
		n = n - 2;
		x = y;
	}

	return n - ((x << 1) >> 31);
}

/*
 * from: http://en.wikipedia.org/wiki/Hamming_weight#Efficient_implementation
 * This uses fewer arithmetic operations than any other known
 * implementation on machines with fast multiplication.
 * It uses 12 arithmetic operations, one of which is a multiply.
 */
uint64_t phtree_popcount (uint64_t bit_string)
{
	uint64_t m1 = 0x5555555555555555ull;  // binary: 0101...
	uint64_t m2 = 0x3333333333333333ull;  // binary: 00110011...
	uint64_t m4 = 0x0F0F0F0F0F0F0F0Full;  // binary: 00001111...
	uint64_t h01 = 0x0101010101010101ull;  // the sum of 256 to the power of 0, 1, 2, 3, ...

	bit_string -= (bit_string >> 1) & m1;  // put count of each 2 bits into those 2 bits
	bit_string = (bit_string & m2) + ((bit_string >> 2) & m2);  // put count of each 4 bits into those 4 bits
	bit_string = (bit_string + (bit_string >> 4)) & m4;  // put count of each 8 bits into those 8 bits

	// return left 8 bits of bit_string + (bit_string << 8) + (bit_string << 16) + (bit_string << 24) + ...
	return (bit_string * h01) >> 56;
}
