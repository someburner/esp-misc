#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

/* ========================================================================== *
 * Quick access defines
 * ========================================================================== */
// #define EN_WS8212_HPSI

// #define EN_TEMP_SENSOR

// #define TEST_CRYPTO_SIGN_OPEN
// #define TEST_CRYPTO_PK_ENCRYPT

/* ========================================================================== *
 * Hardware definitions
 * ========================================================================== */
#define LED_PIN_ONBOARD          4 // GPIO2
#define LED_PIN_WIFI             1 // GPIO5


/* ========================================================================== *
 * Compile-time switches
 * ========================================================================== */
#if defined(TEST_CRYPTO_SIGN_OPEN) || defined(TEST_CRYPTO_PK_ENCRYPT)
#define TEST_CRYPTO
#endif

#ifdef EN_TEMP_SENSOR
   #define ONEWIRE_PIN         5 // GPIO5
   #define ONEWIRE_NONBLOCKING 1
#else
   #define ONEWIRE_NONBLOCKING 0
#endif
/* ========================================================================== *
 * User-level config definitions
 * ========================================================================== */
/* Enable SmartConfig / ESP-Touch on setup rom */
// #define SETUP_SMART_ENABLE

/* Global enable/disable of debug prints */
#define DEVELOP_VERSION

/* Default UART buadrate */
#define BIT_RATE_DEFAULT         BIT_RATE_921600  // 115200, 921600

/* Defines a custom hostname */
#define USER_HOSTNAME            "misc-gw"

/* If free heap gets this low, reboot */
#define FREE_HEAP_CRITICAL       20000UL

/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! *
 * Don't change below here unless you know what you're doing (minus debugs)
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

/* Compile with JSMN parent links */
#define JSMN_PARENT_LINKS        1

/* Enable microsecond timer */
// #define USE_US_TIMER

/* SPI Flash size (MByte) */
#define FLASH_4M

/* Enable GPIO interrupts */
#define GPIO_INTERRUPT_ENABLE    1


/*******************************************************************************
 * Task Global Defines
*******************************************************************************/

#define DRIVER_TASK_PRIO         USER_TASK_PRIO_0
#define DRIVER_TASK_QUEUE_SIZE   10

#define EVENT_MON_TASK_PRIO      USER_TASK_PRIO_1
#define EVENT_MON_QUEUE_SIZE     10

#define MQTT_TASK_PRIO        	USER_TASK_PRIO_2
#define MQTT_TASK_QUEUE_SIZE    	1

/*******************************************************************************
 * Miscellaneous
*******************************************************************************/
#define DEBUG_UART 0 //debug on uart 0 or 1

#define BUILD_SPIFFS	1

/* ------------------------------ Linker/Memory ----------------------------- */
#define ICACHE_STORE_TYPEDEF_ATTR   __attribute__((aligned(4),packed))
#define ICACHE_STORE_ATTR           __attribute__((aligned(4)))
#define ICACHE_RAM_ATTR             __attribute__((section(".iram0.text")))

/*******************************************************************************
 * Debug Switches
*******************************************************************************/
/* ------------------------------- Makefile --------------------------------- */
#ifdef DEVELOP_VERSION
   #define NODE_DEBUG
   #define NODE_ERROR
#endif	/* DEVELOP_VERSION */

/* ----------------------------- Crypto Debugs ------------------------------ */
// #define CRYPTO_DEBUG

/* -------------------------- Espressif SDK Debugs -------------------------- */
// #define MEMLEAK_DEBUG

/* ------------------------- Filesystem API Debugs -------------------------- */
// #define FS_DEBUG

/* ------------------------------ JSON Debugs ------------------------------- */
// #define JSMN_PARSE_DEBUG
// #define JSONTREE_PRINT_DEBUG

/* ------------------------------- LED Debugs ------------------------------- */
// #define LED_DEBUG

/* ------------------------------ MQTT Debugs ------------------------------- */
// #define MQTT_DEBUG

/* ---------------------------- Net Util Debugs ----------------------------- */
// #define PING_DEBUG

/* ----------------------------- Onewire Debugs ----------------------------- */
#ifdef EN_TEMP_SENSOR
#define OW_DEBUG
#endif

/* --------------------------- SmartConfig Debugs --------------------------- */
// #define SMART_DEBUG

/* ------------------------------ Setup Debugs ------------------------------ */
#define SETUP_EVENT_DEBUG

/*******************************************************************************
 * Debug Definitions
*******************************************************************************/
/* ------------------------------- Makefile --------------------------------- */
#ifdef NODE_DEBUG
#define NODE_DBG              c_printf
#else
#define NODE_DBG(...)
#endif	/* NODE_DEBUG */

#ifdef NODE_ERROR
#define NODE_ERR              c_printf
#else
#define NODE_ERR(...)
#endif	/* NODE_ERROR */

/* ----------------------------- Crypto Debugs ------------------------------ */
#ifdef CRYPTO_DEBUG
#define CRYPTO_DBG            c_printf
#else
#define CRYPTO_DBG(...)
#endif	/* Crypto Debug */

/* ------------------------- Filesystem API Debugs -------------------------- */
#ifdef FS_DEBUG
#define FS_DBG                c_printf
#else
#define FS_DBG(...)
#endif /* FS_DEBUG */

/* ------------------------------ JSON Debugs ------------------------------- */
#ifdef JSMN_PARSE_DEBUG
#define JSMN_PARSE_DBG        c_printf
#else
#define JSMN_PARSE_DBG(...)
#endif /* JSMN_PARSE_DEBUG */

#ifdef JSONTREE_PRINT_DEBUG
#define JSONTREE_PRINT_DBG    c_printf
#else
#define JSONTREE_PRINT_DBG(...)
#endif /* JSONTREE_PRINT_DEBUG */

/* ------------------------------- LED Debugs ------------------------------- */
#ifdef LED_DEBUG
#define LED_DBG               c_printf
#else
#define LED_DBG(...)
#endif

/* ------------------------------ MQTT Debugs ------------------------------- */
#ifdef MQTT_ERROR
#define MQTT_ERR(a)           c_printf("MQTT ERR# %d\n", a)
#else
#define MQTT_ERR(...)
#endif

#ifdef MQTT_DEBUG
#define MQTT_DBG              c_printf
#else
#define MQTT_DBG(...)
#endif /* MQTT_DEBUG */

/* ---------------------------- Net Util Debugs ----------------------------- */
#ifdef PING_DEBUG
#define PING_DBG              c_printf
#else
#define PING_DBG(...)
#endif /* PING_DEBUG */

/* ----------------------------- Onewire Debugs ----------------------------- */
#ifdef OW_DEBUG
#define OW_DBG                c_printf
#else
#define OW_DBG(...)
#endif /* OW_DEBUG */

/* --------------------------- SmartConfig Debugs --------------------------- */
#ifdef SMART_DEBUG
#define SMART_DBG             c_printf
#else
#define SMART_DBG(...)
#endif	/* SMART_DEBUG */

/* ------------------------------ Setup Debugs ------------------------------ */
#ifdef SETUP_EVENT_DEBUG
#define SETUP_EVENT_DBG       c_printf
#else
#define SETUP_EVENT_DBG(...)
#endif	/* SETUP_EVENT_DEBUG */

#endif	/* __USER_CONFIG_H__ */
