//////////////////////////////////////////////////
// rBoot config API for ESP8266.
// Copyright 2015 Richard A Burton
// richardaburton@gmail.com
// See license.txt for license terms.
//////////////////////////////////////////////////
#include <string.h>
#include <c_types.h>
#include <spi_flash.h>
#include "user_interface.h"

#include "../app_common/libc/c_stdio.h"

// detect rtos sdk (not ideal method!)
#ifdef IRAM_ATTR
#define os_free(s)   vPortFree(s)
#define os_malloc(s) pvPortMalloc(s)
#else
#include <mem.h>
#endif

#ifdef RBOOT_INTEGRATION
#include <rboot-integration.h>
#endif

#include "rboot-api.h"

const char * rboot_modes[] = {"STD", "GPIO", "TMP", "ERASE_SDK"};

#if defined(BOOT_CONFIG_CHKSUM) || defined(BOOT_RTC_ENABLED)
// calculate checksum for block of data
// from start up to (but excluding) end
static uint8 calc_chksum(uint8 *start, uint8 *end) {
	uint8 chksum = CHKSUM_INIT;
	while(start < end) {
		chksum ^= *start;
		start++;
	}
	return chksum;
}
#endif

void rboot_print_config(bool extended)
{
	rboot_config rConf = rboot_get_config();
	if (&rConf != NULL)
	{
		unsigned i;
      NODE_DBG("rBoot: v%hd\n\t", rConf.version);
		NODE_DBG("rom = %hd/%hd\n\t", rConf.current_rom, rConf.count);
		for (i=0; i< rConf.count; i++)
		{
			NODE_DBG("rom[%d] addr = 0x%x\n\t", i, rConf.roms[i]);
		}
      NODE_DBG("prev main: %hd\n\t", rConf.last_main_rom);
      NODE_DBG("prev setup: %hd\n\t", rConf.last_setup_rom);

      /* Print everything */
      if (extended)
      {
         NODE_DBG("magic: %hd\n\t", rConf.magic);
         NODE_DBG("mode = %s\n\t", rboot_modes[rConf.mode]);
         NODE_DBG("gpio_rom = %hd\n\t", rConf.gpio_rom);
      #ifdef BOOT_CONFIG_CHKSUM
         NODE_DBG("chksum = %hd\n\t", rConf.chksum);
   	#endif
      }
	}
}

void rboot_switch_to_main()
{
   rboot_config conf;
   conf = rboot_get_config();
   rboot_switch(conf.last_main_rom, true);
}

void rboot_switch_to_setup()
{
	rboot_config conf;
	conf = rboot_get_config();
   rboot_switch(conf.last_setup_rom, true);
}

void rboot_switch(uint8 slotNo, bool restart)
{
   static bool restartPending = false;
   uint8 after, before;

   if ((restartPending) && (restart == false))
   {
      NODE_ERR("Restart pending already!\n");
      return;
   }

   before = rboot_get_current_rom();
   if (slotNo >= 4)
   {
      return;
   }
   else if (before == slotNo)
   {
      if ((restartPending) && (restart == true))
         system_restart();
      else
         return;
   }
   else
   {
      after = slotNo;
      if (rboot_set_current_rom(after))
      {
         if (restart)
            system_restart();
         else
            restartPending = true;
      }

   }
   return;
}

rboot_config rboot_get_config(void) {
	rboot_config conf;
	spi_flash_read(BOOT_CONFIG_SECTOR * SECTOR_SIZE, (uint32*)&conf, sizeof(rboot_config));
	return conf;
}

// write the rboot config
// preserves the contents of the rest of the sector,
// so the rest of the sector can be used to store user data
// updates checksum automatically (if enabled)
bool rboot_set_config(rboot_config *conf) {
	uint8 *buffer;
	buffer = (uint8*)os_malloc(SECTOR_SIZE);
	if (!buffer) {
		//os_printf("No ram!\r\n");
		return false;
	}

#ifdef BOOT_CONFIG_CHKSUM
	conf->chksum = calc_chksum((uint8*)conf, (uint8*)&conf->chksum);
#endif

	spi_flash_read(BOOT_CONFIG_SECTOR * SECTOR_SIZE, (uint32*)((void*)buffer), SECTOR_SIZE);
	memcpy(buffer, conf, sizeof(rboot_config));
	spi_flash_erase_sector(BOOT_CONFIG_SECTOR);
	spi_flash_write(BOOT_CONFIG_SECTOR * SECTOR_SIZE, (uint32*)((void*)buffer), SECTOR_SIZE);
	os_free(buffer);
	return true;
}

// get current boot rom
uint8 rboot_get_current_rom(void) {
	rboot_config conf;
	conf = rboot_get_config();
	return conf.current_rom;
}

// set current boot rom
bool rboot_set_current_rom(uint8 rom) {
	rboot_config conf;
	conf = rboot_get_config();
	if (rom >= conf.count)
      return false;
   if (conf.current_rom < 2)
      conf.last_setup_rom = conf.current_rom;
   else if (conf.current_rom < 4)
      conf.last_main_rom = conf.current_rom;
	conf.current_rom = rom;
	return rboot_set_config(&conf);
}

// create the write status struct, based on supplied start address
rboot_write_status rboot_write_init(uint32 start_addr) {
	rboot_write_status status = {0};
	status.start_addr = start_addr;
	status.start_sector = start_addr / SECTOR_SIZE;
	status.last_sector_erased = status.start_sector - 1;
	//status.max_sector_count = 200;
	//os_printf("init addr: 0x%08x\r\n", start_addr);
	return status;
}

bool rboot_write_end(rboot_write_status *status) {
   uint8 i;
   if (status->extra_count != 0) {
      for (i = status->extra_count; i < 4; i++) {
         status->extra_bytes[i] = 0xff;
      }
      return rboot_write_flash(status, status->extra_bytes, 4);
   }
   return true;
}

// function to do the actual writing to flash
// call repeatedly with more data (max len per write is the flash sector size (4k))
bool rboot_write_flash(rboot_write_status *status, uint8 *data, uint16 len) {

	bool ret = false;
	uint8 *buffer;
	int32 lastsect;

	if (data == NULL || len == 0) {
		return true;
	}

	// get a buffer
	buffer = (uint8 *)os_malloc(len + status->extra_count);
	if (!buffer) {
		//os_printf("No ram!\r\n");
		return false;
	}

	// copy in any remaining bytes from last chunk
	if (status->extra_count != 0)
   {
      memcpy(buffer, status->extra_bytes, status->extra_count);
   }
	// copy in new data
	memcpy(buffer + status->extra_count, data, len);

	// calculate length, must be multiple of 4
	// save any remaining bytes for next go
	len += status->extra_count;
	status->extra_count = len % 4;
	len -= status->extra_count;
	memcpy(status->extra_bytes, buffer + len, status->extra_count);

	// check data will fit
	//if (status->start_addr + len < (status->start_sector + status->max_sector_count) * SECTOR_SIZE) {

		// erase any additional sectors needed by this chunk
		lastsect = ((status->start_addr + len) - 1) / SECTOR_SIZE;
		while (lastsect > status->last_sector_erased)
		{
			status->last_sector_erased++;
			spi_flash_erase_sector(status->last_sector_erased);
		}

		// write current chunk
		//os_printf("write addr: 0x%08x, len: 0x%04x\r\n", status->start_addr, len);
		if (spi_flash_write(status->start_addr, (uint32 *)((void*)buffer), len) == SPI_FLASH_RESULT_OK)
		{
			ret = true;
			status->start_addr += len;
		}
	//}

	os_free(buffer);
	return ret;
}

#ifdef BOOT_RTC_ENABLED
bool rboot_get_rtc_data(rboot_rtc_data *rtc) {
	if (system_rtc_mem_read(RBOOT_RTC_ADDR, rtc, sizeof(rboot_rtc_data))) {
		return (rtc->chksum == calc_chksum((uint8*)rtc, (uint8*)&rtc->chksum));
	}
	return false;
}

bool rboot_set_rtc_data(rboot_rtc_data *rtc) {
	// calculate checksum
	rtc->chksum = calc_chksum((uint8*)rtc, (uint8*)&rtc->chksum);
	return system_rtc_mem_write(RBOOT_RTC_ADDR, rtc, sizeof(rboot_rtc_data));
}

bool rboot_set_temp_rom(uint8 rom) {
	rboot_rtc_data rtc;
	// invalid data in rtc?
	if (!rboot_get_rtc_data(&rtc)) {
		// set basics
		rtc.magic = RBOOT_RTC_MAGIC;
		rtc.last_mode = MODE_STANDARD;
		rtc.last_rom = 0;
	}
	// set next boot to temp mode with specified rom
	rtc.next_mode = MODE_TEMP_ROM;
	rtc.temp_rom = rom;

	return rboot_set_rtc_data(&rtc);
}

bool rboot_get_last_boot_rom(uint8 *rom) {
	rboot_rtc_data rtc;
	if (rboot_get_rtc_data(&rtc)) {
		*rom = rtc.last_rom;
		return true;
	}
	return false;
}

bool rboot_get_last_boot_mode(uint8 *mode) {
	rboot_rtc_data rtc;
	if (rboot_get_rtc_data(&rtc)) {
		*mode = rtc.last_mode;
		return true;
	}
	return false;
}
#endif
