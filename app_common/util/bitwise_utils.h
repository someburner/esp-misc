/*
 * Various utils related to bit manipulation.
 */
#ifndef BITWISE_UTILS_H
#define BITWISE_UTILS

/* Returns highest set '1' for the input byte */
uint8_t hibit8(uint8_t input);
uint32_t hibit32(uint32_t n);
uint32_t totalBitsSet32(uint32_t v);
uint32_t bigEndianULong(uint8_t * buf, int len);
uint32_t littleEndianULong(uint8_t * buf, int len);

#endif
