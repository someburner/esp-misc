// Module for RTC time keeping
// #ifdef RBOOT_INTEGRATION
// #include <rboot-integration.h>
// #endif
#include "rtc/rtctime_internal.h"
#include "rtc/rtctime.h"

// #ifdef RBOOT_INTEGRATION
// #include <rboot-integration.h>
// #endif

// extern Cache_Read_Enable_New(0, 0, 1)
// extern void Cache_Read_Enable_New(void);
// ******* C API functions *************
extern void Cache_Read_Enable_New();

void rtctime_early_startup(void)
{
  // Cache_Read_Enable (0, 0, 1);
  Cache_Read_Enable_New();
  rtc_time_register_bootup();
  rtc_time_switch_clocks();
  Cache_Read_Disable();
}

void rtctime_late_startup(void)
{
  rtc_time_switch_system();
}

void rtctime_gettimeofday(struct rtc_timeval *tv)
{
  rtc_time_gettimeofday(tv);
}

void rtctime_settimeofday(const struct rtc_timeval *tv)
{
  if (!rtc_time_check_magic())
    rtc_time_prepare();
  rtc_time_settimeofday(tv);
}

bool rtctime_have_time(void)
{
  return rtc_time_have_time();
}

void rtctime_deep_sleep_us(uint32_t us)
{
  rtc_time_deep_sleep_us(us);
}

void rtctime_deep_sleep_until_aligned_us(uint32_t align_us, uint32_t min_us)
{
  rtc_time_deep_sleep_until_aligned(align_us, min_us);
}


//  rtctime.set(sec, usec)
static int rtctime_set(uint32_t sec, uint32_t usec)
{
   if (!rtc_time_check_magic())
      rtc_time_prepare();

   struct rtc_timeval tv = { sec, usec };
      rtctime_settimeofday(&tv);
   return 0;
}


// sec, usec = rtctime.get()
static int rtctime_get(struct rtc_timeval tv)
{
  rtctime_gettimeofday(&tv);
  return 2;
}

static void do_sleep_opt(uint32_t opt, int idx)
{
   if (opt < 0 || opt > 4)
      return;
   // system_deep_sleep_set_option(opt);
}

static int rtctime_dsleep(uint32_t us)
{
  rtctime_deep_sleep_us(us); // does not return
  return 0;
}


// rtctime.dsleep_aligned(aligned_usec, min_usec, option)
static int rtctime_dsleep_aligned(uint32_t align_us, uint32_t min_us)
{
  if (!rtctime_have_time())
    return -2;

  rtctime_deep_sleep_until_aligned_us(align_us, min_us); // does not return
  return 0;
}
