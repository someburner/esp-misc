#include "c_types.h"

#include "bitwise_utils.h"

uint8_t hibit8(uint8_t input)
{
	uint8_t output = 0;
	while (input >>= 1) {
		output++;
	}
	return output;
}

uint32_t hibit32(uint32_t n)
{
    n |= (n >>  1);
    n |= (n >>  2);
    n |= (n >>  4);
    n |= (n >>  8);
    n |= (n >> 16);
    return n - (n >> 1);
}

uint32_t totalBitsSet32(uint32_t v)
{
	unsigned int c; // c accumulates the total bits set in v
	for (c = 0; v; c++)
	{
	  v &= v - 1; // clear the least significant bit set
	}
	return c;
}

uint32_t bigEndianULong(uint8_t * buf, int len)
{
   uint32_t outNum = 0;
   if (len > 4)
      len = 4;

   for (int i=0; i<len; i++)
   {
      outNum |= (((uint32_t) buf[i]) << (24-i*8));
   }
   return outNum;
}

uint32_t littleEndianULong(uint8_t * buf, int len)
{
   uint32_t outNum = 0;
   if (len > 4)
      len = 4;

   for (int i=0; i<len; i++)
   {
      outNum |= (((uint32_t) buf[i]) << (i*8));
   }
   return outNum;
}
