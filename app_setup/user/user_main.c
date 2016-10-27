/******************************************************************************
 * FileName: user_main.c
 * Description: entry file of user application
 *
*******************************************************************************/
#include "../app_common/rboot/rboot-bigflash.c"

#include "user_interface.h"
#include "user_exceptions.h"
#include "user_config.h"
#include "ets_sys.h"
#include "mem.h"
#include "sntp.h"

#include "../app_common/include/hardware_defs.h"
#include "../app_common/include/fs_config.h"
#include "../app_common/include/server_config.h"
#include "../app_common/platform/platform.h"
#include "../app_common/platform/flash_fs.h"
#include "../app_common/platform/flash_api.h"
#include "../app_common/platform/led.h"

#include "../app_common/libc/c_stdio.h"
#include "../app_common/libc/c_stdlib.h"
#include "../app_common/util/netutil.h"

#ifdef USE_RTC_TIME
#include "rtc/rtctime.h"
#endif
#if FLASH_UNIQUE_ID_EN
#include "driver/spi.h"
#include "driver/spi_overlap.h"
#endif

#include "driver/uart.h"
#include "driver/gpio16.h"
#include "driver/ws2812.h"

#include "setup_api.h"
#include "setup_event.h"
#include "setup_cli.h"

#include "mqtt_api.h"

/* Must be defined after SDK stuff */
#include "../app_common/rboot/rboot-api.h"

/* Uncomment to ping an ip on timer */
// #define PING_TEST

/* Obtain using hspi overlap mode, if possible */
#if FLASH_UNIQUE_ID_EN
static uint32_t flash_dev_mfg_id;
static uint32_t flash_uid_arr[2];
#endif

/* Get from SDK API */
static uint32 flash_id;

static uint8_t orig_mac[6];

os_timer_t heapTimer;

// static char *rst_codes[] = {
//   "normal", "wdt reset", "exception", "soft wdt", "restart", "deep sleep", "external",
// };

uint32_t lastFreeHeap = 0;
uint32_t fsUsed = 0;
uint32_t fsTotal = 0;

#define VERS_STR_STR(V) #V
#define VERS_STR(V) VERS_STR_STR(V)
char *esp_misc_version = VERS_STR(VERSION);

void user_rf_pre_init(void) { }

/* Note: the trampoline *must* be explicitly put into the .text segment, since
 * by the time it is invoked the irom has not yet been mapped. This naturally
 * also goes for anything the trampoline itself calls.
 */
void TEXT_SECTION_ATTR user_start_trampoline (void)
{
   __real__xtos_set_exception_handler (
   EXCCAUSE_LOAD_STORE_ERROR, load_non_32_wide_handler);

   // Note: Keep this as close to call_user_start() as possible, since it
   // is where the cpu clock actually gets bumped to 80MHz.
#ifdef USE_RTC_TIME
   rtctime_early_startup();
#endif

   call_user_start();
}

static void heapTimerCb(void *arg)
{
   lastFreeHeap = system_get_free_heap_size();
   static uint16_t mod = 0;

   if (mod++%2 == 0)
   {
      NODE_DBG("FREE HEAP: %d\n",lastFreeHeap);
   }

#ifdef PING_TEST
   static PING_RESP_T ping_resp;

   if (ping_resp.state == PING_SEQ_DONE)
   {
      ping_resp.state = PING_SEQ_READY;
      NODE_DBG("Ping Response Stats:\n");
      NODE_DBG("%d/%d Succeeded, %d Failed. Average time = %dms\n",
         ping_resp.success, ping_resp.expected, ping_resp.error, ping_resp.avg_time);
   }

   if ((ping_resp.state <= PING_SEQ_READY) && (wifi_station_get_connect_status() == 5))
   {
      NODE_DBG("Init Ping\n");
      // netutil_ping_ip_string("10.128.128.128", (PING_RESP_T*)&ping_resp);
      netutil_ping_ip_string("52.8.59.67", (PING_RESP_T*)&ping_resp);
   }
#endif /* PING_TEST */
}

static void config_wifi()
{
#ifdef USER_HOSTNAME
   wifi_station_set_hostname(USER_HOSTNAME);
#endif /* USER_HOSTNAME */

   NODE_DBG("Configuring WiFi...\n");

#ifdef STA_SSID
   /* Assign SSID & Pass from Makefile if available */
   char sta_ssid[32] = STA_SSID;

   #if STA_PASS_EN && defined(STA_PASS)
      char sta_password[64] = VERS_STR(STA_PASS);
   #else
      char sta_password[64];
   #endif

   struct station_config stconf;
   wifi_station_get_config(&stconf);
   stconf.bssid_set = 0;

   memset(&stconf.ssid,0,32);
   memset(&stconf.password,0,64);

   os_memcpy(stconf.ssid, sta_ssid, strlen(sta_ssid));

   NODE_DBG("WiFi setting: \nap: %s ", (char*)stconf.ssid);
   #if STA_PASS_EN && defined(STA_PASS)
   os_memcpy(stconf.password, sta_password, strlen(sta_password));
   NODE_DBG("pw %s\n", (char*)stconf.password);
   #endif
   NODE_DBG("%s", wifi_station_get_hostname());
   wifi_station_set_config(&stconf);
#else
   NODE_ERR("No Makefil WiFi config available.\n");
   return;
#endif

   /* Start DHCP if not already */
   if (wifi_station_dhcpc_status() != DHCP_STARTED)
      { wifi_station_dhcpc_start(); }
   wifi_station_connect();
}

static void mount(int fs, bool force)
{
   myspiffs_mount(fs);
   if (!(myspiffs_mounted(fs)) && (force == true))
   {
      NODE_ERR("fs%d couldn't mount. Attempting format\n", fs);
      if (myspiffs_format(fs))
      {
         NODE_DBG("Format ok!\n");
      } else {
         NODE_DBG("Format failed\n");
      }
   } else {
      NODE_DBG("FS%d Mounted!\n", fs);
   }
}

void setup_init(void)
{
   /* Set up gpio/LED(s) */
#ifndef IRQ_SPAMMER
   // led_init();
   gpio_init();
#else
   gpio_init();
#endif

   uint32_t fs_size;
   fs_size = flash_safe_get_size_byte();
   NODE_DBG("\nflash safe sz = 0x%x\n", fs_size);
   fs_size = flash_rom_get_size_byte();
   NODE_DBG("\nflash rom sz = 0x%x\n", fs_size);
   NODE_DBG("platform_flash_get_first_free_block_address = 0x%08x\n", platform_flash_get_first_free_block_address(NULL));
   NODE_DBG("Flash_ID = %x\n", flash_id);

   /* Mount Filesystem(s) */
   mount(FS0, true);
   mount(FS1, true);

   /* Initialize file descriptors in memory
    * NOTE: Must not call until you're done mounting filesystem(s), but should
            be called immediately after to prevent concurrency issues
    */
   fs_init_info();

   /* Print rBoot info */
   rboot_print_config(true);

   /* Specify SNTP server(s) */
   sntp_setservername(0, NTP_SERVER_0);
   sntp_setservername(1, NTP_SERVER_1);
   sntp_set_timezone(0); // Use UTC for universal time
   sntp_init();

   setup_app_init(orig_mac);

   config_wifi();

#if FLASH_UNIQUE_ID_EN
   /* Manufacturer + Device ID (if available) */
   NODE_DBG("flash_dev_mfg_id: ");
   if (flash_dev_mfg_id)
      NODE_DBG("0x%04x\n", (uint16_t) (flash_dev_mfg_id & 0xFFFF) );
   else
      NODE_DBG("N/A\n");

   /* Unique ID (if available) */
   NODE_DBG("UUID: ");
   if (flash_uid_arr[0] != 0)
   {
      uint32_t i;
      for (i=0; i<4; i++)
         NODE_DBG("%02x:", (uint8_t) ( flash_uid_arr[0] >> (24-(8*i)) ) ) ;
      for (i=0; i<4; i++)
         NODE_DBG("%02x:", (uint8_t)( flash_uid_arr[1] >> (24-(8*i)) ) ) ;
      NODE_DBG("\n");
   }
   else
   {
      NODE_DBG("N/A\n");
   }
#endif

#if 0
   /* Initialize RFM69 last in case we immediately get a message */
   int res = init_rfm_handler();
   NODE_DBG("RFM69 Setup Init %s!\n", (res==1) ? "OK":"Error");
#endif

   ws2812_init();


#ifdef DEVELOP_VERSION
   os_memset(&heapTimer,0,sizeof(os_timer_t));
   os_timer_disarm(&heapTimer);
   os_timer_setfn(&heapTimer, (os_timer_func_t *)heapTimerCb, NULL);
   os_timer_arm(&heapTimer, 10000, 1);
#endif /* DEVELOP_VERSION */

#if 0
   struct rst_info *ri = system_get_rst_info();
   char rst_string[25];
   os_sprintf(rst_string, "Rst: %s", rst_codes[ri->reason]);
#endif
}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();

    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256: // 0
            rf_cal_sec = 128 - 5; // x80 * x1000 = x80000
            break;

        case FLASH_SIZE_8M_MAP_512_512: // 1
            rf_cal_sec = 256 - 5; // x100 * x1000 = x100000
            break;

        case FLASH_SIZE_16M_MAP_512_512: // 3
        case FLASH_SIZE_16M_MAP_1024_1024: // 5
            rf_cal_sec = 512 - 5; // x200 * x1000 = x200000
            break;

        case FLASH_SIZE_32M_MAP_512_512: // 4
        case FLASH_SIZE_32M_MAP_1024_1024: // 6
            rf_cal_sec = 1024 - 5; // x200 * x1000 = x400000
            break;

        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
#ifdef USE_RTC_TIME
   rtctime_late_startup ();
#endif

#ifdef USE_US_TIMER
   system_timer_reinit();
#endif

   flash_id = spi_flash_get_id();

#if FLASH_UNIQUE_ID_EN
   /* spi dev (0,1,2,3), SPI_CS0_FLASH (3) */
   hspi_overlap_init();
   hspi_dev_sel(SPI_CS0_FLASH);

/* spi_transaction quick ref:                                                                               *
 * 1,      8,        0x??,     0,         0,         0,         0,         xx,       uint32*   , xx         *
 * spi_no, cmd_bits, cmd_data, addr_bits, addr_data, dout_bits, dout_data, din_bits, din_data *, dummy_bits */

   uint8_t flash_mfg = (uint8_t)(flash_id & 0xFF);
   switch (flash_mfg)
   {
      case WINBOND:
         /* Read JEDEC - Disabled. Use SDK API. */
         // flash_dev_mfg_id = spi_transaction(1, 8, 0x9F, 0, 0x00, 0, 0, 24, 0);
         /* Read Manufacture ID + Device ID */
         spi_transaction(1, 8, 0x90, 24, 0x00000000, 0, 0, 16, &flash_dev_mfg_id, 0);
         /* Read Unique ID. 64 din/32 skip */
         spi_transaction(1, 8, 0x4B, 0, 0, 0, 0, 64, flash_uid_arr, 32);
         break;
      case BERGMICRO:
         /* Read JEDEC - Disabled. Use SDK API. */
         // flash_dev_mfg_id = spi_transaction(1, 8, 0x9F, 0, 0x00, 0, 0, 24, 0); // 0x1640e0
         /* Read Manufacture ID + Device ID */
         spi_transaction(1, 8, 0x90, 24, 0x00000000, 0, 0, 16, &flash_dev_mfg_id, 0); // 0xe015 x2
         /* Read Unique ID. 64 din/32 skip */
         spi_transaction(1, 8, 0x4B, 0, 0, 0, 0, 64, flash_uid_arr, 32);
         break;
      default:
         flash_dev_mfg_id = 0;
         memset(flash_uid_arr, 0, sizeof(flash_uid_arr));
   }

   hspi_overlap_deinit(); // Turn off overlap mode. Back to normal operation
#endif

   /* overclock :) */
   system_update_cpu_freq(160);

   /* Obtain UUID from station interface right away just in case */
   wifi_get_macaddr(STATION_IF, orig_mac);

   /* Setup UART */
   uart_init(BIT_RATE_921600,BIT_RATE_921600);
#ifndef NODE_DEBUG
    system_set_os_print(0);
#else
   system_set_os_print(1);
#endif

   /* Disable auto_connect on boot so we are in control of things */
   wifi_set_opmode(STATION_MODE);
   wifi_station_set_auto_connect(0);
   wifi_station_set_reconnect_policy(false);

   system_init_done_cb(setup_init);
}
