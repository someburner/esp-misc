#ifndef CRC32_H_
#define CRC32_H_
/*----------------------------------------------------------------------------*\
 *  CRC-32 version 2.0.0 by Craig Bruce, 2006-04-29.
 *  Modified by Jeff Hufford 2016-07-08 for ESP8266.
 *  Obtained from https://github.com/ETrun/crc32
 *
 *  This program generates the CRC-32 values for the files named in the
 *  command-line arguments.  These are the same CRC-32 values used by GZIP,
 *  PKZIP, and ZMODEM.  The Crc32_ComputeBuf() can also be detached and
 *  used independently.
 *
 *  THIS PROGRAM IS PUBLIC-DOMAIN SOFTWARE.
 *
 *  Based on the byte-oriented implementation "File Verification Using CRC"
 *  by Mark R. Nelson in Dr. Dobb's Journal, May 1992, pp. 64-67.
 *
 *  v1.0.0: original release.
 *  v1.0.1: fixed printf formats.
 *  v1.0.2: fixed something else.
 *  v1.0.3: replaced CRC constant table by generator function.
 *  v1.0.4: reformatted code, made ANSI C.  1994-12-05.
 *  v2.0.0: rewrote to use memory buffer & static table, 2006-04-29.
 *  v2.1.0: modified by Nico, 2013-04-20
\*----------------------------------------------------------------------------*/
uint32_t Crc32_ComputeBuf(uint32_t inCrc32, const void *buf, size_t bufLen);

/*----------------------------------------------------------------------------*\
 * CRC16 implementation taken from Atmel AVR <util/crc16.h>                   *
 * Added Crc16_ComputeBuf to handle buffered input                            *
\*----------------------------------------------------------------------------*/
uint16_t Crc16_update8(uint16_t crc, uint8_t a);
uint16_t Crc16_ComputeBuf(uint16_t inCrc16, uint8_t *buf, uint16_t bufLen);

/*----------------------------------------------------------------------------*\
 * CRC8 implementation taken from some Chinese person on Github               *
\*----------------------------------------------------------------------------*/
uint8_t Crc8_ComputeBuf(uint8_t inCrc8, uint8_t *buf,uint16_t len);


uint8_t onewire_crc8(const uint8_t *addr, uint8_t len);

// uint16_t onewire_crc16(const uint16_t *data, const uint16_t  len);

#endif /* End crc32.h */
