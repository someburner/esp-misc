/* ========================================================================== *
 *                              Setup API Header                              *
 * -------------------------------------------------------------------------- *
 *         Copyright (C) Jeff Hufford - All Rights Reserved. License:         *
 *                   "THE BEER-WARE LICENSE" (Revision 42):                   *
 * Jeff Hufford (jeffrey92<at>gmail.com) wrote this file. As long as you      *
 * retain this notice you can do whatever you want with this stuff. If we     *
 * meet some day, and you think this stuff is worth it, you can buy me a beer *
 * in return.                                                                 *
 * ========================================================================== */
#ifndef _SETUP_API_H
#define _SETUP_API_H

/*******************************************************************************
 * Initializers
*******************************************************************************/
bool setup_app_init(uint8_t * orig_mac);
bool setup_api_init();

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
bool setup_api_timestamp_avail();
uint8_t getWiFiState();

void setup_restart_sytem();
void setup_restart_soon();
void rboot_arm_switch(uint32_t countdown);

/* ------------------------------ Messaging --------------------------------- */
void setup_publish_identity();
bool setup_log_msg(int iface, int level, const char * msg, int len);

/* ------------------------------ Utilities --------------------------------- */
uint32_t get_setup_sys_flag();
void setup_sys_flag(uint32_t flag, bool set, bool save);

int  setup_api_get_conn_state();
void setup_api_update_conn_state(uint8_t state);

uint16_t getSetupState();
void setSetupState(uint16_t flag, bool set);

char * gen_rand_num_string(int len);
char * gen_rand_frame_id(int len);
/* -------------------------------- JSON ------------------------------------ */
void jsmn_parse_input(uint8_t type, uint8_t flags, void *data);

/* -------------------------------- Test ------------------------------------ */
bool setup_api_test(char * buf, int len);

/*******************************************************************************
 * Setup Defines
*******************************************************************************/
/* ----------------------------- Global Vars -------------------------------- */
char MQTT_CLIENT_ID[16];

/* ----------------------------- System Flags ------------------------------- */
/* Check if a SYS_FLAG is set */
#define SETUP_SYS_FLAG_IS_SET(flag)      ( get_setup_sys_flag() & (1UL<<flag) )
/* Set/Clear System Flag wrappers */
#define SETUP_SYS_FLAG_SET(flag, save)    setup_sys_flag(flag, true, save)
#define SETUP_SYS_FLAG_CLEAR(flag, save)  setup_sys_flag(flag, false, save)
void push_setup_sys_flags();

#define MARK_STATE_FLAG(flag) setSetupState(flag, true)
#define CLEAR_STATE_FLAG(flag) setSetupState(flag, false)

/* --------------------------- Connection States ---------------------------- */
#define SETUP_CONN_INVALID             0
#define SETUP_CONN_DISCONNECTED        1
#define SETUP_CONN_GOT_IP              2
#define SETUP_CONN_MQTT                3
#define SETUP_CONN_HTTP                4
#define SETUP_CONN_MAX                 5

/* -------------------------- Setup WiFi Defines ---------------------------- */
#define SETUP_WIFI_NOT_FOUND           0
#define SETUP_WIFI_FOUND               1
#define SETUP_WIFI_DISCONNECTED        2

/* ------------------------- Setup State Flags -------------------------- */
#define SETUP_INVALID_STATE            0
#define SETUP_DEFAULT_STATE            1
#define SETUP_COMMAND_ACTIVE           2
#define SETUP_RBOOT_SWITCH             3
#define SETUP_TIMED_OUT                4

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
#define LOG_INFO(iface, msg, len) SETUP_ROUTE_LOG(iface, LOG_LEVEL_INFO, msg, len)
#define LOG_OK(iface, msg, len)   SETUP_ROUTE_LOG(iface, LOG_LEVEL_OK, msg, len)
#define LOG_WARN(iface, msg, len) SETUP_ROUTE_LOG(iface, LOG_LEVEL_WARN, msg, len)
#define LOG_ERR(iface, msg, len)  SETUP_ROUTE_LOG(iface, LOG_LEVEL_SEVERE, msg, len)

/*******************************************************************************
 * Messages + Callbacks
*******************************************************************************/
typedef void (*setup_cb) ();
typedef int (*setup_int_cb) ();

typedef void (*SetupCB)(uint32_t *args);

typedef struct
{
   uint8_t     iface;
   uint8_t     type;
   uint16_t    len;

   uint32_t    stamp;
   void *      data;
} SETUP_IO_MSG_T;

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

   setup_int_cb   mqtt_buff_avail_cb;

} SETUP_MQTT_T;


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

   SETUP_MQTT_T * mqtt;
   char *         uuid;
} SETUP_MON_T;


#endif
