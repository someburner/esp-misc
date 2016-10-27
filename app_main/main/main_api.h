/* ========================================================================== *
 *                               Main API Header                              *
 * -------------------------------------------------------------------------- *
 *         Copyright (C) Jeff Hufford - All Rights Reserved. License:         *
 *                   "THE BEER-WARE LICENSE" (Revision 42):                   *
 * Jeff Hufford (jeffrey92<at>gmail.com) wrote this file. As long as you      *
 * retain this notice you can do whatever you want with this stuff. If we     *
 * meet some day, and you think this stuff is worth it, you can buy me a beer *
 * in return.                                                                 *
 * ========================================================================== */
#ifndef _MAIN_API_H
#define _MAIN_API_H

/*******************************************************************************
 * Initializers
*******************************************************************************/
bool main_app_init(uint8_t * orig_mac);
bool main_api_init();

/*******************************************************************************
 * Transport Setting APIs
*******************************************************************************/
/* ------------------------------- MQTT APIs -------------------------------- */
/* General */
void mqtt_setconn(uint8_t state);

/*******************************************************************************
 * General APIs
*******************************************************************************/
/* ------------------------------ OS Wrappers ------------------------------- */
bool main_api_timestamp_avail();
uint8_t getWiFiState();

void main_restart_sytem();
void main_restart_soon();
void rboot_arm_switch(uint32_t countdown);

/* ------------------------------ Messaging --------------------------------- */
void main_publish_identity();
bool main_log_msg(int iface, int level, const char * msg, int len);

/* ------------------------------ Utilities --------------------------------- */
uint32_t get_main_sys_flag();
void main_sys_flag(uint32_t flag, bool set, bool save);

int  main_api_get_conn_state();
void main_api_update_conn_state(uint8_t state);

uint16_t getSetupState();
void setSetupState(uint16_t flag, bool set);

char * gen_rand_num_string(int len);
char * gen_rand_frame_id(int len);
/* -------------------------------- JSON ------------------------------------ */
void jsmn_parse_input(uint8_t type, uint8_t flags, void *data);

/* -------------------------------- Test ------------------------------------ */
bool main_api_test(char * buf, int len);

/*******************************************************************************
 * MAIN DEFINES
*******************************************************************************/
/* ----------------------------- Global Vars -------------------------------- */
char MQTT_CLIENT_ID[16];

/* ----------------------------- System Flags ------------------------------- */
/* Check if a SYS_FLAG is set */
#define MAIN_SYS_FLAG_IS_SET(flag)      ( get_main_sys_flag() & (1UL<<flag) )
/* Set/Clear System Flag wrappers */
#define MAIN_SYS_FLAG_SET(flag, save)    main_sys_flag(flag, true, save)
#define MAIN_SYS_FLAG_CLEAR(flag, save)  main_sys_flag(flag, false, save)
void push_main_sys_flags();

#define MARK_STATE_FLAG(flag) setSetupState(flag, true)
#define CLEAR_STATE_FLAG(flag) setSetupState(flag, false)

/* --------------------------- Connection States ---------------------------- */
#define MAIN_CONN_INVALID             0
#define MAIN_CONN_DISCONNECTED        1
#define MAIN_CONN_GOT_IP              2
#define MAIN_CONN_MQTT                3
#define MAIN_CONN_HTTP                4
#define MAIN_CONN_MAX                 5

/* -------------------------- Main WiFi Defines ----------------------------- */
#define MAIN_WIFI_NOT_FOUND           0
#define MAIN_WIFI_FOUND               1
#define MAIN_WIFI_DISCONNECTED        2

/* ------------------------- Setup State Flags -------------------------- */
#define MAIN_INVALID_STATE            0
#define MAIN_DEFAULT_STATE            1
#define MAIN_COMMAND_ACTIVE           2
#define MAIN_RBOOT_SWITCH             3
#define MAIN_TIMED_OUT                4

/* ---------------------------- Interface Types ----------------------------- */
#define MSG_IFACE_UNKNOWN              0 // Invalid type, ignore/warn
#define MSG_IFACE_MQTT                 1 // MQTT transport
#define MSG_IFACE_UART                 2 // UART transport
#define MSG_IFACE_HTTP                 3 // HTTP transport
#define MSG_IFACE_INTERNAL             4
#define MSG_IFACE_ONE_WIRE             5
#define MSG_IFACE_MAX                  6

/* ----------------------------- Message Types ------------------------------ */
#define MSG_TYPE_UNKNOWN               0 // Invalid type, ignore/warn
#define MSG_TYPE_CRYPTO                1 // Crypto Message
#define MSG_TYPE_MAX                   2

/* ---------------------------- Direction Types ----------------------------- */
#define MSG_DIR_UNKNOWN                0 // Invalid type, ignore/warn
#define MSG_DIR_INPUT                  1 // Input from an interface
#define MSG_DIR_OUTPUT                 2 // Output to an interface
#define MSG_DIR_LOG                    3 // Log output to an interface
#define MSG_DIR_MAX                    4

/* --------------------------- Log Level Defines ---------------------------- */
#define LOG_LEVEL_INFO                 0
#define LOG_LEVEL_OK                   1
#define LOG_LEVEL_WARN                 2
#define LOG_LEVEL_SEVERE               3
#define LOG_LEVEL_CAL                  4
#define LOG_LEVEL_MAX                  5

#define LOG_BUF_MAX                    512

/* ------------------------------ Log Wrappers ------------------------------ */
#define LOG_INFO(iface, msg, len) MAIN_ROUTE_LOG(iface, LOG_LEVEL_INFO, msg, len)
#define LOG_OK(iface, msg, len)   MAIN_ROUTE_LOG(iface, LOG_LEVEL_OK, msg, len)
#define LOG_WARN(iface, msg, len) MAIN_ROUTE_LOG(iface, LOG_LEVEL_WARN, msg, len)
#define LOG_ERR(iface, msg, len)  MAIN_ROUTE_LOG(iface, LOG_LEVEL_SEVERE, msg, len)

/*******************************************************************************
 * Messages + Callbacks
*******************************************************************************/
typedef void (*main_cb) ();

typedef int (*main_int_cb) ();

typedef void (*SetupCB)(uint32_t *args);

typedef struct
{
   uint8_t     iface;
   uint8_t     type;
   uint16_t    len;

   uint32_t    stamp;
   void *      data;
} MAIN_IO_MSG_T;

/*******************************************************************************
 * Containers
*******************************************************************************/
/* ------------------------- MQTT-Related Structures ------------------------ */

typedef struct
{
   uint8_t        padding;
   uint8_t        sub_flags;

   uint8_t        uuid_sub_offset;


   uint8_t        sub_topic_offset;
   char *         bridge_resp_sub;
   char *         device_resp_sub;

   char *         universal_sub;

   char *         test_sub;
   char *         test_pub;

   char *         log_pub_topic;

   char *         mqtt_host_ip;

   main_int_cb   mqtt_buff_avail_cb;

} MAIN_MQTT_T;


/* ----------------------------- Main Structure ----------------------------- */
typedef struct
{
   os_timer_t     timer;
   uint32_t       timeout;
   SetupCB 	      timeout_cb;

   uint32_t       error_flags;

   uint8_t        conn_state;
   uint8_t        device_ct;
   uint16_t       state;

   MAIN_MQTT_T * mqtt;
   char *         uuid;
} MAIN_MON_T;


#endif
