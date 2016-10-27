/* ========================================================================== *
 *                        Main API Implementation                             *
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
#include "user_config.h"

#include "../app_common/libc/c_stdio.h"
#include "../app_common/platform/platform.h"
#include "../app_common/platform/flash_fs.h"
#include "../app_common/rboot/rboot-api.h"
#include "mqtt_main/mqtt_api.h"
#include "main_api.h"

/* Main structure pointer */
static MAIN_MON_T * main_mon = NULL;

/* Main Event extra Buffer */
char evt_extra_buf[128];

/* Timers */
static os_timer_t main_msg_timer;
static os_timer_t fw_switch_timer;

/* ---------------------------- Timer Callbacks ----------------------------- */
static void fw_switch_timer_cb(void * arg)
{
   // rboot_switch();
}

static void main_timer_cb(void *arg)
{
   NODE_DBG("main_timer_cb\n");
   os_timer_disarm(&main_mon->timer);
   main_mon->timeout_cb(arg);
}

static void main_msg_timer_cb(void *arg)
{

}


/* ------------------------------- Utilities -------------------------------- */
int main_api_get_conn_state()
{
   return main_mon->conn_state;
}

void main_api_update_conn_state(uint8_t state)
{
   if (state < MAIN_CONN_MAX)
      main_mon->conn_state = state;
}

uint16_t getSetupState()
{
   return main_mon->state;
}

void setSetupState(uint16_t flag, bool set)
{
   /* Set a flag */
   if (set)
   {
      /* Check if already set */
      if ((main_mon->state & (1U<<flag)) == (1U<<flag))
         { NODE_DBG("already set flag %u\n", flag); return;}
      main_mon->state |= (1U<<flag);
   }
   /* Clear a flag */
   else
   {
      main_mon->state &= ~(1U<<flag);
   }
}

void main_api_update_device_ct(bool addRemove)
{
   if (addRemove)
      main_mon->device_ct++;
   else
      main_mon->device_ct--;
}

char * gen_rand_frame_id(int len)
{
   if (!len) return NULL;
   char * buf = (char*)os_zalloc(sizeof(char)*len +1);
   uint32_t r1 = os_random();
   NODE_DBG("gen_rand_frame_id: ");
   if (ets_snprintf(buf, len, "%x\0", r1) != len) {
      if (buf) os_free(buf);
      return NULL;
   }
   NODE_DBG("ok?\n");
   return buf;
}


char * gen_rand_num_string(int len)
{
   char buf[32];
   if (!len)
      return NULL;
   char * bufOut = (char*)os_zalloc(sizeof(char)*len +1);
   int ix = 0;
   while ( len > 0 )
   {
      uint32_t r1 = os_random();
      os_sprintf(buf, "%08x", r1);
      memcpy(bufOut+ix, buf, (len%8) );
      ix+= (len%8);
      len -= 8;
   }
   bufOut[ix] = '\0';
   return bufOut;
}

bool main_api_test(char * buf, int len)
{
   return mqtt_api_pub_temp(buf, len);
}

/* ------------------------------- Messaging -------------------------------- */
bool main_log_msg(int iface, int level, const char * msg, int len)
{
   return true;
}

/* ------------------------------ OS Wrappers ------------------------------- */
void rboot_arm_switch(uint32_t countdown)
{
	// os_timer_disarm(&fw_switch_timer);
   // os_timer_arm(&fw_switch_timer, countdown, 0);
   // NODE_DBG("Attempting to switch firmware in %u ms\n", countdown);
}

void main_restart_sytem()
{
   system_restart();
}

void main_restart_soon()
{
   os_timer_disarm(&main_mon->timer);
   main_mon->timeout_cb = main_restart_sytem;
   os_timer_arm(&main_mon->timer, 1234UL, 0);
}


/* ------------------------------ Initializers ------------------------------ */
bool main_api_init(MAIN_MON_T * clientPtr)
{
   main_mon = clientPtr;

   os_timer_disarm(&main_mon->timer);
	os_timer_setfn(&main_mon->timer, (os_timer_func_t *)main_timer_cb, &main_mon->timeout_cb);

   os_timer_disarm(&fw_switch_timer);
   os_timer_setfn(&fw_switch_timer, (os_timer_func_t *)fw_switch_timer_cb, NULL);

   os_timer_disarm(&main_msg_timer);
   os_timer_setfn(&main_msg_timer, (os_timer_func_t *)main_msg_timer_cb, NULL);
   // os_timer_arm(&main_msg_timer, 3233UL, 1);

   return true;
}

/* End main_api.c */
