#ifndef _DRIVER_EVENT_h
#define _DRIVER_EVENT_h

#include "ws2812.h"
#include "onewire_nb.h"

#define DRIVER_SRC_NONE    0
#define DRIVER_SRC_ONEWIRE 1
#define DRIVER_SRC_WS2812  2

#define MSG_IFACE_UNKNOWN              0 // Invalid type, ignore/warn
#define MSG_IFACE_MQTT                 1 // MQTT transport
#define MSG_IFACE_UART                 2 // UART transport
#define MSG_IFACE_HTTP                 3 // HTTP transport
#define MSG_IFACE_INTERNAL             4
#define MSG_IFACE_ONE_WIRE             5


void driver_event_register_ws(ws2812_driver_t * ws);
void driver_event_init();






















#endif /* _DRIVER_EVENT_h */
