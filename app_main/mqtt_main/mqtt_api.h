#ifndef __MQTT_API_H
#define __MQTT_API_H

#include "main/main_api.h"

/* ------------------------- Client Connection APIs ------------------------- */
void mqtt_app_init(MAIN_MQTT_T * main_ptr);
void mqtt_setconn(uint8_t state);
void mqtt_app_update_handle(MAIN_MQTT_T * main_mqtt_ptr);

/* ------------------------- Pointer/Callback APIs -------------------------- */
void mqtt_app_detach_cbs();

/* ------------------------------ Publish APIs ------------------------------ */
bool mqtt_api_pub_log(char * msg, int len);
bool mqtt_api_pub_temp(char * msg, int len);

/* ------------------------------- Utilities -------------------------------- */
int mqtt_api_buff_len_avail();


#endif
