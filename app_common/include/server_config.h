#ifndef __SERVER_CONFIG_H_
#define __SERVER_CONFIG_H_

/*******************************************************************************
 * NTP Servers
*******************************************************************************/
#define NTP_SERVER_0 "0.pool.ntp.org"
#define NTP_SERVER_1 "1.pool.ntp.org"

/*******************************************************************************
 * MQTT Servers (definitions)
*******************************************************************************/
#define PUBLIC_BROKER 1
#define MY_BROKER     2

/*******************************************************************************
 * MQTT Servers, Topics
*******************************************************************************/
/* ----------------------------- Choose Broker ------------------------------ */
// #define BROKER_SET PUBLIC_BROKER
#define BROKER_SET MY_BROKER

/* ------------------------- Choose Test Topic Set -------------------------- */
#define INC_TEST_TOPIC_SET

/* --------------------------------- Check ---------------------------------- */
#ifndef BROKER_SET
   #error "Must choose a broker!"
#endif

/*******************************************************************************
 * Brokers
*******************************************************************************/
/* ---------------------------- Host, User/Pass ----------------------------- */
/* Eclipse public MQTT Broker */
#if BROKER_SET==PUBLIC_BROKER
   /* Broker settings for "app_main" */
   #define MAIN_DEFAULT_MQTT_HOST   "iot.eclipse.org"
   #define MQTT_USER                NULL
   #define MAIN_MQTT_PASS           NULL

   /* Broker settings for "app_setup" */
   #define SETUP_DEFAULT_MQTT_HOST  "iot.eclipse.org"
   #define SETUP_MQTT_USER          NULL
   #define SETUP_MQTT_PASS          NULL

/* Your custom broker */
#elif BROKER_SET==MY_BROKER
   /* Broker settings for "app_main" */
   #define MAIN_DEFAULT_MQTT_HOST  "yourbroker.com"
   #define MAIN_MQTT_USER          "your_mqtt_user"
   #define MAIN_MQTT_PASS          "your_mqtt_pass"

   #define SETUP_DEFAULT_MQTT_HOST  "yourbroker.com"
   #define SETUP_MQTT_USER          "your_mqtt_user"
   #define SETUP_MQTT_PASS          "your_mqtt_pass"

/* At least one must be chosen */
#else
   #ifndef SPIFFY_SRC_MK
   #error "Invalid BROKER_SET"
   #endif
#endif


/*******************************************************************************
 * Topics (APP MAIN)
*******************************************************************************/
#define MAIN_MAX_TOPIC_LEN         64

/*******************************************************************************
 * Topics (APP SETUP)
*******************************************************************************/
/* ---------------------------- Dev/Test Topics ----------------------------- */
#ifdef INC_TEST_TOPIC_SET
   #define SETUP_TEST_PUB           "/temperature"
   #define SETUP_TEST_SUB           "/temp/response"
#endif







#endif /* __SERVER_CONFIG_H_ */
