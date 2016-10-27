#ifndef __MQTT_CONFIG_H__
#define __MQTT_CONFIG_H__

// #define CFG_HOLDER	0x00FF55A4	/* Change this value to load default configurations */
// #define CFG_LOCATION	0x3C	/* Please don't change or if you know what you doing */
// #define MQTT_SSL_ENABLE

/*DEFAULT CONFIGURATIONS*/

#define MQTT_PORT	         1883
#define MQTT_BUF_SIZE	     1792
// #define MQTT_BUF_SIZE		 	2048
// #define MQTT_BUF_SIZE		 	2048

#define MQTT_BUF_USABLE    1432  //guess?

#define MQTT_KEEPALIVE        20	 /*second*/
#define MQTT_SEND_TIMOUT		7

#define MQTT_RECONNECT_TIMEOUT 	10	/*second*/

#define DEFAULT_SECURITY         0

// #define QUEUE_BUFFER_SIZE		 	1440
#define QUEUE_BUFFER_SIZE		 	1792
// #define QUEUE_BUFFER_SIZE		 	2048

/* Number of elapsed milliseconds of no OTA dataCb until cancellation  */
#define MQTT_RETRY_TIMEOUT       5372
#define MQTT_OTA_TIMEOUT         17711UL

/* MQTT version 3.11 */
/* Compatible with https://eclipse.org/paho/clients/testing */
#if (MQTT_PROTOCOL_VER==311)
   #define PROTOCOL_NAMEv311
   #pragma message "Using MQTT v3.1.1"

/* MQTT version 3.1 */
/* Compatible with Mosquitto <= v0.15*/
#elif (MQTT_PROTOCOL_VER==31)
   #define PROTOCOL_NAMEv31
   #pragma message "Using MQTT v3.1"

#else
   #error "Must define MQTT Version!"
#endif


#endif // __MQTT_CONFIG_H__
