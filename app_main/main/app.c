/* ========================================================================== *
 *                        Setup App Initializations                           *
 * -------------------------------------------------------------------------- *
 *         Copyright (C) Jeff Hufford - All Rights Reserved. License:         *
 *                   "THE BEER-WARE LICENSE" (Revision 42):                   *
 * Jeff Hufford (jeffrey92<at>gmail.com) wrote this file. As long as you      *
 * retain this notice you can do whatever you want with this stuff. If we     *
 * meet some day, and you think this stuff is worth it, you can buy me a beer *
 * in return.                                                                 *
 * ========================================================================== */
#include "user_interface.h"
#include "mem.h"
#include "osapi.h"
#include "sntp.h"
#include "queue.h"
#include "user_config.h"

#include "../app_common/include/server_config.h"
#include "../app_common/libc/c_stdio.h"
#include "../app_common/platform/platform.h"

#include "main_api.h"
#include "main_event.h"
// #include "main_json.h"

static MAIN_MON_T client;

bool main_app_init(uint8_t * orig_mac)
{
   char buffer[128];

   /* Create a pointer to Setup monitor and clear */
   MAIN_MON_T * clientPtr = &client;
   memset(clientPtr, 0, sizeof(client));

   clientPtr->conn_state = MAIN_CONN_DISCONNECTED;
   clientPtr->state = MAIN_INVALID_STATE;
   clientPtr->device_ct = 1;

   int len = os_sprintf(buffer, "%02x%02x%02x%02x%02x%02x",
                                 orig_mac[0], orig_mac[1], orig_mac[2],
                                 orig_mac[3], orig_mac[4], orig_mac[5] );
   buffer[len] = '\0';

   MQTT_CLIENT_ID[0] = 'e';
   MQTT_CLIENT_ID[1] = 's';
   MQTT_CLIENT_ID[2] = 'p';
   // memcpy(&MQTT_CLIENT_ID[3], buffer, 12);
   memcpy(MQTT_CLIENT_ID+3, buffer, 12);
   MQTT_CLIENT_ID[15] = '\0';
   NODE_DBG("UUID+MQTT Client ID: %s\n", MQTT_CLIENT_ID);

   clientPtr->uuid = (char *) os_zalloc(len+1);
   os_memcpy(clientPtr->uuid, buffer, len);

   /* Allocate Setup Monitor MQTT instance */
   clientPtr->mqtt = (MAIN_MQTT_T *) os_zalloc(sizeof(MAIN_MQTT_T));
   clientPtr->mqtt->mqtt_buff_avail_cb = NULL;

#ifdef MAIN_INC_TEST_TOPICS
   /* Allocate test pub topic */
   clientPtr->mqtt->test_pub = (char *) os_zalloc(os_strlen(MAIN_TEST_PUB)+1);
   os_memcpy(clientPtr->mqtt->test_pub, MAIN_TEST_PUB, os_strlen(MAIN_TEST_PUB));
   NODE_DBG("test_pub: %s\n", clientPtr->mqtt->test_pub);

   /* Allocate test sub topic */
   clientPtr->mqtt->test_sub = (char *) os_zalloc(os_strlen(MAIN_TEST_SUB)+1);
   os_memcpy(clientPtr->mqtt->test_sub, MAIN_TEST_SUB, os_strlen(MAIN_TEST_SUB));
   NODE_DBG("test_sub: %s\n", clientPtr->mqtt->test_sub);
#endif

   /* Initialize Main Timers */
   main_api_init(clientPtr);

   /* Initialize Setup Event Monitor */
   main_event_init(clientPtr);

   // main_jsmn_init();

	return true;
}
