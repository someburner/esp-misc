/* ========================================================================== *
 *                            Setup Events Header                             *
 * -------------------------------------------------------------------------- *
 *         Copyright (C) Jeff Hufford - All Rights Reserved. License:         *
 *                   "THE BEER-WARE LICENSE" (Revision 42):                   *
 * Jeff Hufford (jeffrey92<at>gmail.com) wrote this file. As long as you      *
 * retain this notice you can do whatever you want with this stuff. If we     *
 * meet some day, and you think this stuff is worth it, you can buy me a beer *
 * in return.                                                                 *
 * ========================================================================== */
#ifndef SETUP_EVENT_H
#define SETUP_EVENT_H

#include "setup_api.h"

#define SNTP_TIMEOUT       3523
#define RECONNECT_TIMOUT   4732

/* If we are disconnect for longer than this, reboot */
#define DISCONNECTED_REBOOT_TIMOUT  1800000UL   //30 minute default
// #define DISCONNECTED_REBOOT_TIMOUT  300000UL

uint8_t wifiStatus_mq;
uint8_t lastwifiStatus_mq;

enum { wifiIsDisconnected, wifiIsConnected, wifiGotIP };
enum { rfm_unknown, rfm_timed_out, rfm_connected };

typedef void(*WifiStateChangeCb)(uint8_t wifiStatus);

int wifi_start_smart();
int wifi_exit_smart();

void arm_state_timer(int dt);
void arm_sntp_timer();
void arm_reconnect_timer();

void statusWifiUpdate(uint8_t state);
void statusRfmUpdate(uint8_t state);

void setup_event_init(SETUP_MON_T * clientPtr);

#endif
