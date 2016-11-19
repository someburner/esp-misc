#include "user_interface.h"
#include "mem.h"
#include "osapi.h"
#include "sntp.h"
#include "espconn.h"
#include "user_config.h"

#include "../app_common/include/server_config.h"

#include "../app_common/util/bitwise_utils.h"
#include "../app_common/libc/c_stdio.h"
#include "../app_common/platform/flash_fs.h"
#include "../app_common/mqtt/mqtt.h"
#include "../app_common/mqtt/mqtt_msg.h"

#include "main/main_api.h"

#include "mqtt_api.h"

static MQTT_Client mqtt_client;
static MAIN_MQTT_T * main_mqtt = NULL;

// static char * bridge_sub = NULL;
// static char * fs_sub = NULL;

static char * device_sub = NULL;

static long cur_fill_cnt = 0;

extern uint8_t wifiStatus_mq;
extern uint8_t lastwifiStatus_mq;

static int en_normal = false;

#ifndef BOXZEROBYTES
#define BOXZEROBYTES 16
#endif

#define COPY_DATABUF(msgBuf, inData, inDataLen) \
   (msgBuf) = (char*)os_zalloc(inDataLen+1);    \
   memcpy( (msgBuf), (inData), inDataLen);      \
   ((char*)(msgBuf))[inDataLen] = 0;

static void mqttNothingDataCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	NODE_DBG("mqttNothingDataCb\n");
}

static void mqttNothingConnectCb(uint32_t *args)
{
	NODE_DBG("mqttNothingConnectCb\n");
}

/* Simple method to set MQTT connection */
void mqtt_setconn(uint8_t state)
{
	// check if this is the first connection. Don't allow if FMON_DISCONN set
	if (state == 1) {
		MQTT_Connect(&mqtt_client);
	} else {
		NODE_DBG("MQTT: Detected wifi network down\n");
	}
}

static void mqttConnectedTestCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;

	NODE_DBG("MQTT: Connected (test: start timer)\n");
   if (MQTT_Subscribe(client, main_mqtt->test_sub, 1))
   {
      //mqtt_api_pub_test("Hello!", 6);
      main_api_update_conn_state(MAIN_CONN_MQTT);
   }
}

static void mqttDisconnectedCb(uint32_t *args)
{
	NODE_DBG("MQTT: Disconnected\n");
	main_api_update_conn_state(MAIN_CONN_DISCONNECTED);
}

static void mqttPublishedCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;

	/* Update app-level fill count variable */
	cur_fill_cnt = client->msgQueue.rb.fill_cnt;
	MQTT_DBG("mqttPublishedCb: outbuff (%d/%d)\n", client->msgQueue.rb.fill_cnt, client->msgQueue.rb.size);
}

static void mqttDataTestCb(uint32_t *args, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
	// MAIN_IO_MSG_T * newMsg = (MAIN_IO_MSG_T *)os_zalloc(sizeof(MAIN_IO_MSG_T));
	NODE_DBG("mqttDataTestCb: Receive topic = %s, data: %d bytes. Type = ", topic, data_len);
}

void mqtt_attach_test_cbs()
{
	MQTT_OnConnected(&mqtt_client, mqttConnectedTestCb);
	MQTT_OnData(&mqtt_client, mqttDataTestCb);
}

bool mqtt_api_pub_log(char * msg, int len)
{
   return true;
}

bool mqtt_api_pub_test(char * msg, int len)
{
   NODE_DBG("mqtt_pub_device to %s\n", main_mqtt->test_pub);
   return MQTT_Publish(&mqtt_client, main_mqtt->test_pub, msg, len, 1, 0);
}

void mqtt_app_detach_cbs()
{
	NODE_DBG("mqtt_app_detach_cbs\n");
	MQTT_OnData(&mqtt_client, mqttNothingDataCb);
	MQTT_OnConnected(&mqtt_client, mqttNothingConnectCb);
}

/*******************************************************************************
* MQTT General Utilties
*******************************************************************************/
void mqtt_app_update_handle(MAIN_MQTT_T * main_mqtt_ptr)
{
	main_mqtt = main_mqtt_ptr;
}

int mqtt_api_buff_len_avail()
{
	return (MQTT_BUF_USABLE - cur_fill_cnt);
}

static void mqttTimeoutCb(uint32_t *args)
{
	MQTT_Client* client = (MQTT_Client*)args;

	/* Update app-level fill count variable */
	cur_fill_cnt = client->msgQueue.rb.fill_cnt;

	NODE_DBG("mqttTimeoutCb: outbuff (%d/%d)\n", client->msgQueue.rb.fill_cnt, client->msgQueue.rb.size);

}

void mqtt_app_init(MAIN_MQTT_T * main_mqtt_ptr)
{
	/* Attach MQTT Monitor struct to MQTT Client */
	main_mqtt = main_mqtt_ptr;
   main_mqtt->mqtt_buff_avail_cb = mqtt_api_buff_len_avail;

	/* Init MQTT */
	MQTT_InitConnection(&mqtt_client, main_mqtt_ptr->mqtt_host_ip, 1883, 0);

	MQTT_InitClient(&mqtt_client, (uint8_t*)MQTT_CLIENT_ID, MAIN_MQTT_USER, MAIN_MQTT_PASS, MQTT_KEEPALIVE, 1); //last bit sets cleanSession flag
	MQTT_InitLWT(&mqtt_client, "/lwt", "offline-1235", 0, 0);

#ifdef MAIN_INC_TEST_TOPICS
   mqtt_attach_test_cbs();
#endif

	MQTT_OnDisconnected(&mqtt_client, mqttDisconnectedCb);
	MQTT_OnPublished(&mqtt_client, mqttPublishedCb);
	MQTT_OnTimeout(&mqtt_client, mqttTimeoutCb);

}
