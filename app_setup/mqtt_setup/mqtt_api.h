#ifndef __MQTT_API_H
#define __MQTT_API_H

#include "setup/setup_api.h"

/* ------------------------- Client Connection APIs ------------------------- */
void mqtt_app_init(SETUP_MQTT_T * setup_ptr);
void mqtt_setconn(uint8_t state);
void mqtt_app_update_handle(SETUP_MQTT_T * setup_mqtt_ptr);

/* ------------------------- Pointer/Callback APIs -------------------------- */
void mqtt_app_detach_cbs();

/* ------------------------------ Publish APIs ------------------------------ */
bool mqtt_api_pub_log(char * msg, int len);
bool mqtt_api_pub_temp(char * msg, int len);

/* ------------------------------- Utilities -------------------------------- */
int mqtt_api_buff_len_avail();


#endif
