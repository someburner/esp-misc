#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
/* ========================================================================== *
 * User-level config definitions
 * ========================================================================== */
/* Global enable/disable of debug prints */
#define DEVELOP_VERSION

/* Default UART buadrate */
#define BIT_RATE_DEFAULT         BIT_RATE_921600 //115200, 921600

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

/* -------------------------- Espressif SDK Debugs -------------------------- */
// #define MEMLEAK_DEBUG

/* ------------------------------ MQTT Debugs ------------------------------- */
// #define MQTT_DEBUG

/* ----------------------------- Crypto Debugs ------------------------------ */
#define CRYPTO_DEBUG

/* ------------------------- Filesystem API Debugs -------------------------- */
// #define FS_DEBUG

/* ------------------------------ JSON Debugs ------------------------------- */
// #define JSMN_PARSE_DEBUG
// #define JSONTREE_PRINT_DEBUG

/* ---------------------------- Net Util Debugs ----------------------------- */
// #define PING_DEBUG

/* --------------------------- SmartConfig Debugs --------------------------- */
// #define SMART_DEBUG

/* ------------------------------- LED Debugs ------------------------------- */
// #define LED_DEBUG

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

/* ---------------------------- Net Util Debugs ----------------------------- */
#ifdef PING_DEBUG
#define PING_DBG              c_printf
#else
#define PING_DBG(...)
#endif /* PING_DEBUG */

/* --------------------------- SmartConfig Debugs --------------------------- */
#ifdef SMART_DEBUG
#define SMART_DBG             c_printf
#else
#define SMART_DBG(...)
#endif	/* SMART_DEBUG */

/* ------------------------------- LED Debugs ------------------------------- */
#ifdef LED_DEBUG
#define LED_DBG               c_printf
#else
#define LED_DBG(...)
#endif


#endif	/* __USER_CONFIG_H__ */
