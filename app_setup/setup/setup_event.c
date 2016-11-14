/* ========================================================================== *
 *                               Setup Events                                 *
 *                Handles various events, connects transports.                *
 * -------------------------------------------------------------------------- *
 *         Copyright (C) Jeff Hufford - All Rights Reserved. License:         *
 *                   "THE BEER-WARE LICENSE" (Revision 42):                   *
 * Jeff Hufford (jeffrey92<at>gmail.com) wrote this file. As long as you      *
 * retain this notice you can do whatever you want with this stuff. If we     *
 * meet some day, and you think this stuff is worth it, you can buy me a beer *
 * in return.                                                                 *
 * ========================================================================== */
#include "user_interface.h"
#include "c_types.h"
#include "mem.h"
#include "gpio.h"
#include "osapi.h"
#include "sntp.h"
#include "user_config.h"

#include "../app_common/libc/c_stdio.h"
#include "../app_common/include/server_config.h"
#include "../app_common/platform/flash_fs.h"
#include "../app_common/platform/led.h"
#include "../app_common/util/netutil.h"
#include "../app_common/rboot/rboot-api.h"

#include "mqtt_setup/mqtt_api.h"
#include "driver/uart.h"

#include "setup_api.h"
#include "setup_cli.h"
#include "setup_event.h"

#ifdef TEST_CRYPTO
#include "../app_common/newcrypto/crypto_test.h"
#endif
#ifdef EN_TEMP_SENSOR
   #include "driver/hw_timer.h"
   #include "driver/onewire_nb.h"
#endif

static bool gotSntpTs = false;

static uint32_t tmpSysFlag = 0;

#ifdef SETUP_SMART_ENABLE
#include "smartconfig.h"
static int wifi_smart_succeed = -2;
#endif

typedef enum {
   wifi_disconn,
   wifi_isconn,
   wifi_gotip
} wifi_config_type_t;

static SETUP_MON_T * setup_mon = NULL;

os_event_t EVENT_MON_QUEUE[EVENT_MON_QUEUE_SIZE];

char rxBuf[128];

// wifi status change callbacks
static WifiStateChangeCb wifi_state_change_cb[4];

uint8_t wifiStatus_mq = 0; 		//STATION_IDLE
uint8_t lastwifiStatus_mq = 0; 	//STATION_IDLE

// initial state
uint8_t wifiState = wifiIsDisconnected;

/* WiFi Failure reasons. See user_interface.h */
uint8_t wifiReason = 0;
static char *wifiReasons[] =
{
   "", "unspecified", "auth_expire", "auth_leave", "assoc_expire",            //4
   "assoc_toomany", "not_authed", "not_assoced", "assoc_leave",               //8
   "assoc_not_authed", "disassoc_pwrcap_bad", "disassoc_supchan_bad", "",     //12
   "ie_invalid", "mic_failure", "4way_handshake_timeout",                     //15
   "group_key_update_timeout", "ie_in_4way_differs", "group_cipher_invalid",  //18
   "pairwise_cipher_invalid", "akmp_invalid", "unsupp_rsn_ie_version",        //21
   "invalid_rsn_ie_cap", "802_1x_auth_failed", "cipher_suite_rejected",       //24
   "beacon_timeout", "no_ap_found"                                            //26
};

// callback when wifi status changes
// void (*wifiStatusCb)(uint8_t);

#ifdef SETUP_SMART_ENABLE
/* SmartConfig callback */
static void wifi_smart_succeed_cb(sc_status status, void *pdata)
{
   NODE_DBG("wifi_smart_succeed_cb:\n");

   // Got IP, connect to AP successfully
   if (status == SC_STATUS_LINK_OVER)
   {
      NODE_DBG("\tSmartConfig Success\n");
      smartconfig_stop();

      struct station_config sta_ok;
      wifi_station_get_config(&sta_ok);
      arm_reconnect_timer(1234UL);
      return;
   }

   if (status != SC_STATUS_LINK || !pdata)
   {
      NODE_DBG("\tSmartConfig result N/A\n");
      return;
   }


  struct station_config *sta_conf = pdata;
  wifi_station_set_config(sta_conf);
  wifi_station_disconnect();
  wifi_station_connect();

  if (wifi_smart_succeed != 1)
  {
     NODE_DBG("\twifi_smart_succeed code: %d\n", wifi_smart_succeed);
     wifi_smart_succeed = -2;
  }
}
#endif // SETUP_SMART_ENABLE

uint8_t getWiFiState()
{
   return wifiState;
}

/* Timer to push state update information */
void setup_state_timeout_cb(uint32_t *arg)
{
   os_timer_disarm(&setup_mon->timer);

   if (wifi_station_get_connect_status() != STATION_GOT_IP)
   {
      arm_reconnect_timer(5000UL);
      return;
   }

   // static int swap = 1;
   // if (get_setup_sys_flag())
   // {
   //    push_setup_sys_flags();
   // }

   // swap = !swap;

   // if (!isRegistered())
   // {
   //    //switch to HTTP
   //    setup_mon->timeout_cb = setup_http_timeout_cb;
   //    os_timer_arm(&setup_mon->timer, 5000, 0);
   // } else {
   //    // save transport
   // }
}

/* API to see if we've gotten a timestamp yet. This is so we don't overload
 * the ESP with sntp calls while we're waiting for a timestamp. */
bool setup_api_timestamp_avail()
{
   return gotSntpTs;
}

/* setup_ts_timeout_cb:
 * Description: Timer to try to get SNTP timestamp. Attempts to get timestamp
 * on 500 ms interval. Once a timestamp is obtained, arms setup_state_timeout_cb
 * to begin pushing state info to server.
 */
static void setup_ts_timeout_cb(uint32_t *arg)
{
   os_timer_disarm(&setup_mon->timer);

   uint32_t ts = sntp_get_current_timestamp();
   if (ts)
   {
      gotSntpTs = true;
      NODE_DBG("got ts!\n");
      arm_state_timer(10012);
   }
   else
   {
      os_timer_arm(&setup_mon->timer, 1053, false);
   }
}

/* setup_reconnect_cb:
 * Description: Timer to re-attempt WiFi connect
 */
static void setup_reconnect_cb(uint32_t *arg)
{
   os_timer_disarm(&setup_mon->timer);
   SETUP_EVENT_DBG("setup_reconnect_cb:\n");
   NODE_DBG("setup_reconnect_cb:\n");

   /* Make sure MQTT isn't connected still */
   if (setup_api_get_conn_state() != SETUP_CONN_MQTT)
   {
      SETUP_EVENT_DBG("\tno mqtt\n");
      /* Try again */
      wifi_station_connect();
      os_timer_arm(&setup_mon->timer, RECONNECT_TIMOUT, 0);
   }

   /* MQTT connected, re-arm sntp timer */
   else
   {
      SETUP_EVENT_DBG("\tmqtt connected!\n");
      arm_sntp_timer(); // will arm state timer after it gets ts
   }
}

/* Arm state timer */
void arm_state_timer(int dt)
{
   /* Disarm */
   os_timer_disarm(&setup_mon->timer);

   /* Swap in the state callback pointer */
   setup_mon->timeout_cb = setup_state_timeout_cb;

   /* Arm timer w device specifc timeout */
   os_timer_arm(&setup_mon->timer, 15021U, 0);
}

/* Arm sntp timer */
void arm_sntp_timer()
{
   NODE_DBG("arm_sntp_timer:\n");
   /* Disarm */
   os_timer_disarm(&setup_mon->timer);

   /* Swap in the ts callback pointer */
   setup_mon->timeout_cb = setup_ts_timeout_cb;

   /* Arm timer for a bit over 3 seconds. SNTP timeout is 3 sec. */
   os_timer_arm(&setup_mon->timer, SNTP_TIMEOUT, 0);
}

/* Arm reconnect timer */
void arm_reconnect_timer(uint32_t delay)
{
   // SETUP_EVENT_DBG("arm_reconnect_timer\n");
   NODE_DBG("arm_reconnect_timer:\n");
   /* Disarm */
   os_timer_disarm(&setup_mon->timer);

   /* Swap in the ts callback pointer */
   setup_mon->timeout_cb = setup_reconnect_cb;

   /* Arm timer */
   os_timer_arm(&setup_mon->timer, delay, 0);
}

static char* wifiGetReason(void)
{
   if (wifiReason <= 24) return wifiReasons[wifiReason];
   if (wifiReason >= 200 && wifiReason <= 201) return wifiReasons[wifiReason-200+24];
   return wifiReasons[1];
}

static void Event_Mon_Task(os_event_t *e)
{
   uint8_t iface = 0;
   uint8_t type = 0;
   uint16_t msgLen = 0;

   char * msgPtr = NULL;
   char * ptr = NULL;

   if (!e) return;

   if ( e->sig )
      iface = e->sig;

   if (iface == MSG_IFACE_ONE_WIRE)
   {
      goto onewire_skip;
   }

   SETUP_IO_MSG_T * msg = (SETUP_IO_MSG_T *)e->par;
   if (msg)
   {
      type = msg->type;
      msgLen = msg->len;
   } else {
      NODE_DBG("no par?\n");
      return;
   }

onewire_skip:

   switch (iface)
   {
      case MSG_IFACE_MQTT:
      {
         if (!msg->data)
            goto cleanup;
         // handle MQTT messages
         if ( (type == MSG_TYPE_CRYPTO) )
         {
            if (!msg->data)
               goto cleanup;
            // testDecryptSingle(msg->data, msg->len);
            // testDecryptMultiple(msg->data, msg->len);
         }
         else
         {
            SETUP_EVENT_DBG("Unkown type: %hd\n", type);
         }
      } break;

      case MSG_IFACE_INTERNAL:
      {
         if (!msg->data)
            goto cleanup;
      } break;

      // handle UART messages
      case MSG_IFACE_UART:
      {
         if (!msg->data)
            goto cleanup;
         msgPtr = (char *)msg->data;

         if (setup_cli_parse(msgPtr) < 0)
            break;

         if ((ptr = strstr(msgPtr, "blink")) != NULL)
         {
            SETUP_EVENT_DBG("It worked!\n");
            // toggle_led(LED_PIN_ONBOARD);
         }

         else if ((ptr = strstr(msgPtr, "rboot")) != NULL)
         {
            setup_cli_rboot_cmd(msgPtr, msgLen);
         }

         else if ((ptr = strstr(msgPtr, "show")) != NULL)
         {
            setup_cli_get_cmd(msgPtr);
         }

         else if ((ptr = strstr(msgPtr, "flush")) != NULL)
         {
            memset(rxBuf, 0, 128);
         }
         // else if ((ptr = strstr(msgPtr, "test")) != NULL)
         // {
         //    int fd = dynfs_open("my_file", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR);
         //    if (dynfs_write(fd, (u8_t *)"Hello world\0", 12) < 0)
         //       os_printf("errno %i\n", dynfs_errno());
         //    dynfs_close(fd);
         //
         //    fd = dynfs_open("my_file", SPIFFS_RDWR);
         //    if (dynfs_read(fd, (u8_t *)buf, 12) < 0) os_printf("errno %i\n", dynfs_errno());
         //    dynfs_close(fd);
         //    buf[11] = '\0';
         //    NODE_DBG("--> %s <--\n", buf);
         // }
         else
         {
            SETUP_EVENT_DBG("\r\ninvalid cmd: %s\n", msgPtr);
         }

      } break;

      case MSG_IFACE_HTTP:
      {
         // handle HTTP messages
      } break;

      case MSG_IFACE_ONE_WIRE:
      {
      #if defined(EN_TEMP_SENSOR) && (ONEWIRE_NONBLOCKING==1)
         onewire_driver_t * one_driver = (onewire_driver_t *)e->par;

         if (one_driver->seq_state == OW_SEQ_STATE_INIT)
         {
            one_driver->seq_state = OW_SEQ_STATE_TRANSITION;

            /* Assign first op (always 0) */
            one_driver->op_type = one_driver->seq_arr[0];
            /* Start with first valid operation (always 1) */
            one_driver->cur_op = 0;
            one_driver->seq_pos = 0;

            OW_DBG("1w seq init: type=%hd, op=%hd\n", one_driver->op_type, one_driver->cur_op);
         }

         OW_doit();
         return;
      #endif
      } break;

      default:
      {
         // unknown message type.
         NODE_DBG("Unkown msg type!\n");
      } break;

   }

cleanup:
   if (msg)
   {
      if (msg->data)
         os_free(msg->data);
      os_free(msg);
   }

   if (iface == MSG_IFACE_UART)
      uart0_send_EOT();
}

/* change the WiFi state indication */
void statusWifiUpdate(uint8_t state)
{
   wifiState = state;

   // /* Not connected. Check for reboot timeout */
   static uint32 disconnected_time = 0;
   if (wifiState != wifiGotIP)
   {
      if (!disconnected_time)
      {
         disconnected_time = system_get_time() / 1000UL;
      }

      else if (((system_get_time() / 1000UL) - disconnected_time) > DISCONNECTED_REBOOT_TIMOUT)
      {
         system_restart();
      }
   }
   /* Got IP. Clear disconnected time */
   else
   {
      NODE_DBG("Clear discon time\n");
      disconnected_time = 0;
   }
}

/* nslookup callback */
static void nslookup_cb(const char *name, ip_addr_t *ip, void *arg)
{
	if (!ip)
   {
      NODE_ERR("Dns Failed\n");
	}
   else
   {
      /* Max length of IP string = xxx.xxx.xxx.xxx = 16 */
      char * ipstr = os_zalloc(sizeof(char)*17);
      if (ipaddr_ntoa_r(ip, ipstr, sizeof(char)*17) != NULL)
      {
         NODE_DBG("Resolved Host: %s -> ip: %s\n", name, ipstr);
         /* Attach IP string to MQTT object */
         setup_mon->mqtt->mqtt_host_ip = ipstr;
         /* Init MQTT */
         mqtt_app_init(setup_mon->mqtt);
         mqtt_setconn(1);

      #ifdef EN_TEMP_SENSOR
         onewire_nb_init();
      #endif
      }
      else
      {
         NODE_ERR("IP Buffer too small\n");
         os_free(ipstr);
      }
   }
}

/* Handler for wifi status change callback coming in from espressif SDK */
static void wifiHandleEventCb(System_Event_t *evt)
{
   lastwifiStatus_mq = wifiStatus_mq;
   switch (evt->event)
   {
      case EVENT_STAMODE_CONNECTED:
      {
         wifiState = wifiIsConnected;
         wifiReason = 0;
         NODE_DBG("Wifi connected to ssid %s, ch %d\n", evt->event_info.connected.ssid,
         evt->event_info.connected.channel);
         statusWifiUpdate(wifiState);
      } break;
      case EVENT_STAMODE_DISCONNECTED:
      {
         /* First disconnect CB */
         if (wifiState != wifiIsDisconnected)
         {
            wifiState = wifiIsDisconnected;
            setup_mon->conn_state = SETUP_CONN_DISCONNECTED;

            wifiReason = evt->event_info.disconnected.reason;
            NODE_DBG("Wifi disconnected from ssid %s, reason %s (%d)\n",
            evt->event_info.disconnected.ssid, wifiGetReason(), evt->event_info.disconnected.reason);
            wifiStatus_mq = wifiIsDisconnected;
            mqtt_setconn(0);

            arm_reconnect_timer(5000UL);
         }

         statusWifiUpdate(wifiState);
      } break;
      case EVENT_STAMODE_AUTHMODE_CHANGE:
      {
         NODE_DBG("Wifi auth mode: %d -> %d\n",
         evt->event_info.auth_change.old_mode, evt->event_info.auth_change.new_mode);
      } break;
      case EVENT_STAMODE_GOT_IP:
      {
      #ifdef SETUP_SMART_ENABLE
         // if (wifi_smart_succeed != -2)
         //    wifi_smart_succeed = 1;
      #endif
         wifiState = wifiGotIP;
         wifiReason = 0;
         NODE_DBG("Wifi got ip:" IPSTR ",mask:" IPSTR ",gw:" IPSTR "\n",
         IP2STR(&evt->event_info.got_ip.ip), IP2STR(&evt->event_info.got_ip.mask),
         IP2STR(&evt->event_info.got_ip.gw));
         statusWifiUpdate(wifiState);
         wifiStatus_mq = 2;

         netutil_nslookup(SETUP_DEFAULT_MQTT_HOST, nslookup_cb);
         arm_reconnect_timer(1234UL);

      } break;
      case EVENT_SOFTAPMODE_STACONNECTED:
      {
         NODE_DBG("Wifi AP: station " MACSTR " joined, AID = %d\n",
         MAC2STR(evt->event_info.sta_connected.mac), evt->event_info.sta_connected.aid);
      } break;
      case EVENT_SOFTAPMODE_STADISCONNECTED:
      {
         NODE_DBG("Wifi AP: station " MACSTR " left, AID = %d\n",
         MAC2STR(evt->event_info.sta_disconnected.mac), evt->event_info.sta_disconnected.aid);
      } break;
      default: return;
   }

   int i;
   for (i = 0; i < 4; i++)
   {
      if (wifi_state_change_cb[i] != NULL) (wifi_state_change_cb[i])(wifiState);
   }
}

#ifdef SETUP_SMART_ENABLE
/* Initialize SmartConfig */
int wifi_start_smart()
{

   if (wifi_get_opmode() != STATION_MODE)
   {
      return -1;
   }

   if (wifi_smart_succeed != -2)
   {
      return -1;
   }
   uint8_t smart_type = SC_TYPE_ESPTOUCH;

   if ( smart_type > 1 )
      return -1;

   NODE_DBG("start smart: %d\n", smart_type);
   smartconfig_set_type(smart_type);
   smartconfig_start(wifi_smart_succeed_cb);
   wifi_smart_succeed = -1;

   return 0;
}

int wifi_exit_smart()
{
  smartconfig_stop();
  return 0;
}
#endif /* SETUP_SMART_ENABLE */

void setup_event_init(SETUP_MON_T * clientPtr)
{
   setup_mon = clientPtr;

   register_rx_buf(rxBuf);

   setup_mon->timeout_cb = setup_ts_timeout_cb;
	// os_timer_arm(&setup_mon->timer, 2000, 0);

   /* Setup System Task for Events */
   system_os_task(Event_Mon_Task, EVENT_MON_TASK_PRIO, EVENT_MON_QUEUE, EVENT_MON_QUEUE_SIZE);

   /* Attach WiFi Event Handler */
   wifi_set_event_handler_cb(wifiHandleEventCb);

#ifdef TEST_CRYPTO_SIGN_OPEN
   testSignOpen();
#endif
#ifdef TEST_CRYPTO_PK_ENCRYPT
   testEncrypt();
#endif

}



/* End event.c */
