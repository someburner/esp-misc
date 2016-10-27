#ifndef __HARDWARE_DEFS_H__
#define __HARDWARE_DEFS_H__

/* ========================================================================== *
 * SPI Flash Chips
 * ========================================================================== */
/* Attempt to read  64-bit Unique ID if available */
// #define FLASH_UNIQUE_ID_ENABLE

#define WINBOND   0xEF
#define BERGMICRO 0x0E

#ifdef FLASH_UNIQUE_ID_ENABLE
   #define FLASH_UNIQUE_ID_EN 1
#else
   #define FLASH_UNIQUE_ID_EN 0
#endif
/* ========================================================================== *
 * LEDs
 * ========================================================================== */
#define LED_PIN_ONBOARD          4 // GPIO2
#define LED_PIN_WIFI             1 // GPIO5

/* ========================================================================== *
 * Onewire/Temperature
 * ========================================================================== */
// #define USE_PARASITE_POWER
// #define IS_DS18B20
#define IS_MAX31820


#endif
