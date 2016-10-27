#include "user_interface.h"
#include "user_config.h"

#include "../include/rom.h"
#include "randombytes.h"

void randombytes(uint8_t *buf, uint32_t xlen)
{
   uint32_t i = 0;
   uint32_t r = my_trng();

   while (xlen--)
   {
      if ( (i%4)  == 0)
      {
         r = my_trng();
      }

      buf[i] = (unsigned char) ( r >> (24 - ((i%4)*8)) );
      i++;
   }
}
