/******************************************************************************
* Copyright 2013-2014 Espressif Systems (Wuxi)
*
* FileName: hw_timer.c
*
* Description: hw_timer driver
*
* Modification history:
*     2016/10/4, v1.1 Add wrapper methods (Jeff Hufford)
*     2014/5/1, v1.0 create this file.
*******************************************************************************/
#include "ets_sys.h"
#include "os_type.h"
#include "osapi.h"
#include "driver/hw_timer.h"

/* Espressif-provided macro to get ticks from us */
#define US_TO_RTC_TIMER_TICKS(t)    \
   ((t) ?                           \
   (((t) > 0x35A) ?                 \
   (((t)>>2) * ((APB_CLK_FREQ>>4)/250000) + ((t)&0x3) * ((APB_CLK_FREQ>>4)/1000000)) : \
   (((t) *(APB_CLK_FREQ>>4)) / 1000000)) : \
   0)

/* Keeps track of the current state of the HW timer. The wrapper methods      *
 * available through hw_timer.h use this to ensure everything is okay before  *
 * arming/disarming/changing callbacks. Otherwise we'd crash most likely.     */
static HW_TIMER_STATE_T hw_timer_state = HW_TIMER_DISABLED;
static int autoload_en = false;

/*******************************************************************************
 * INTERNAL USE ONLY!
*******************************************************************************/
/* Callback declaration */
static void (* user_hw_timer_cb)(uint32_t) = NULL;
static uint32_t callback_arg;

/******************************************************************************
* FunctionName : hw_timer_set_func_internal
* Description  : set the func, when trigger timer is up.
* Parameters   : void (* user_hw_timer_cb_set)(void):
                        timer callback function,
*******************************************************************************/
static void ICACHE_RAM_ATTR hw_timer_set_func_internal(void (* user_hw_timer_cb_set)(uint32_t), uint32_t arg)
{
   callback_arg = arg;
   user_hw_timer_cb = user_hw_timer_cb_set;
}

static void ICACHE_RAM_ATTR hw_timer_isr_cb(void *arg)
{
    if (user_hw_timer_cb != NULL) {
        (*(user_hw_timer_cb))(callback_arg);
    }
}

/******************************************************************************
* FunctionName : hw_timer_arm_internal
* Description  : set a trigger timer delay for this timer.
* Parameters   : uint32 val:
in autoload mode
                        50 ~ 0x7fffff;  for FRC1 source.
                        100 ~ 0x7fffff;  for NMI source.
in non autoload mode:
                        10 ~ 0x7fffff;
*******************************************************************************/
static void ICACHE_RAM_ATTR hw_timer_arm_internal(u32 val, bool reEnable)
{
   if (reEnable)
   {
      RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                                DIVIDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT);

      ETS_FRC_TIMER1_INTR_ATTACH(hw_timer_isr_cb, NULL);
      TM1_EDGE_INT_ENABLE();
      ETS_FRC1_INTR_ENABLE();
   }
   RTC_REG_WRITE(FRC1_LOAD_ADDRESS, US_TO_RTC_TIMER_TICKS(val));
}

static void ICACHE_RAM_ATTR hw_timer_disarm_internal()
{
   /* Set no reload mode */
   RTC_REG_WRITE(FRC1_CTRL_ADDRESS, DIVIDED_BY_16 | TM_EDGE_INT);

   TM1_EDGE_INT_DISABLE();
   ETS_FRC1_INTR_DISABLE();
}

/*******************************************************************************
 * Externally accessible wrapper methods
*******************************************************************************/
HW_TIMER_STATE_T hw_timer_get_state(void)
{
   return hw_timer_state;
}

void hw_timer_disarm()
{
   if (hw_timer_state == HW_TIMER_ACTIVE)
   {
      hw_timer_disarm_internal();
      hw_timer_state = HW_TIMER_STOPPED;
   }
}

bool hw_timer_arm(u32 val)
{
   if ( hw_timer_state >= HW_TIMER_READY )
   {
      // if
      // {
      //    TM1_EDGE_INT_ENABLE();
      //    ETS_FRC1_INTR_ENABLE();
      // }

      hw_timer_arm_internal(val, (hw_timer_state != HW_TIMER_ACTIVE) ? true:false);
      hw_timer_state = HW_TIMER_ACTIVE;
      return true;
   }

   os_printf("Invalid hw timer state!\n");
   return false;
}

void hw_timer_set_func( void (* user_hw_timer_cb_set)(uint32_t), uint32_t arg )
{
   if (hw_timer_state < HW_TIMER_INIT)
      return;

   if (hw_timer_state == HW_TIMER_ACTIVE)
   {
      os_printf("Stop current timer first!\n");
      return;
   }

   hw_timer_state = HW_TIMER_READY;
   hw_timer_set_func_internal( user_hw_timer_cb_set, arg );
}

/******************************************************************************
* FunctionName : hw_timer_init
* Description  : initilize the hardware isr timer
* Parameters   :
FRC1_TIMER_SOURCE_TYPE source_type:
                        FRC1_SOURCE,    timer use frc1 isr as isr source.
                        NMI_SOURCE,     timer use nmi isr as isr source.
u8 req:
                        0,  not autoload,
                        1,  autoload mode,
* Returns      : NONE
*******************************************************************************/
void hw_timer_init(FRC1_TIMER_SOURCE_TYPE source_type, u8 req)
{
   if (hw_timer_state != HW_TIMER_DISABLED)
      return;

   if (req == 1)
   {
      autoload_en = true;
      RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
               FRC1_AUTO_LOAD | DIVIDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT);
   }
   else
   {
      autoload_en = false;
      RTC_REG_WRITE(FRC1_CTRL_ADDRESS,
                                DIVIDED_BY_16 | FRC1_ENABLE_TIMER | TM_EDGE_INT);
   }

   if (source_type == NMI_SOURCE)
   {
      ETS_FRC_TIMER1_NMI_INTR_ATTACH(hw_timer_isr_cb);
   }
   else
   {
      ETS_FRC_TIMER1_INTR_ATTACH(hw_timer_isr_cb, NULL);
   }
   hw_timer_state = HW_TIMER_INIT;
   os_printf("hw_timer: 10us = %u\n", US_TO_RTC_TIMER_TICKS(10U));
   // os_printf("hw_timer en: auto=%d\n", autoload_en);

   TM1_EDGE_INT_ENABLE();
   ETS_FRC1_INTR_ENABLE();

}

/*
NOTE:
1 if use nmi source, for autoload timer , the timer setting val can't be less than 100.
2 if use nmi source, this timer has highest priority, can interrupt other isr.
3 if use frc1 source, this timer can't interrupt other isr.
*/

// Can also use args?
// https://github.com/espressif/ESP8266_RTOS_ALINK_DEMO/commit/f8372541ef84821937a3e2190873581794b9a4df

//-------------------------------Test Code Below--------------------------------------
#if 0

#define REG_READ(_r) (*(volatile uint32 *)(_r))
#define WDEV_NOW() REG_READ(0x3ff20c00)

static void ICACHE_RAM_ATTR hw_test_timer_cb(uint32_t arg)
{
   // (void) arg;
   static uint32_t x = 0;
   static uint32 j = 0;
   j++;
   if (j == 10000)
   {
      // SETUP_IO_MSG_T * newMsg = (SETUP_IO_MSG_T *)os_zalloc(sizeof(SETUP_IO_MSG_T));
      // newMsg->iface = MSG_IFACE_HW_TIMER;
      system_os_post(EVENT_MON_TASK_PRIO, (os_signal_t)MSG_IFACE_ONE_WIRE, (os_param_t)arg);
      // x = WDEV_NOW();
      j = 0;
   }
}
void ICACHE_FLASH_ATTR user_init(void)
{
    hw_timer_init(FRC1_SOURCE, 1);
    hw_timer_set_func(hw_test_timer_cb);
    hw_timer_arm(100);
}
#endif
