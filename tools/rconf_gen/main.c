#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "../../ld/locations.h"

#if defined ROM_TO_BOOT
   #if ROM_TO_BOOT >= 0 && ROM_TO_BOOT <= 3
      #define ROM_NUM ROM_TO_BOOT
   #else
      #error "Invalid rom num!"
   #endif
#else
   #define ROM_NUM 2
#endif
#define BOOT_CONFIG_CHKSUM
#define CHKSUM_INIT 0xef

typedef struct {
   uint8_t magic;           // our magic
   uint8_t version;         // config struct version
   uint8_t mode;            // boot loader mode
   uint8_t current_rom;     // currently selected rom
   uint8_t gpio_rom;        // rom to use for gpio boot
   uint8_t count;           // number of roms in use
   uint8_t last_main_rom;
   uint8_t last_setup_rom;
   uint32_t roms[ROM_COUNT]; // flash addresses of the roms
#ifdef BOOT_CONFIG_CHKSUM
   uint8_t chksum;          // boot config chksum
   uint8_t block[3];        // alignment
#endif
} rboot_config;

rboot_config testConfig = {
   0xE1, // magic - constant
   0x01, // version - constant
   0x00, // mode - standard (not gpio)
   ROM_NUM,    // current_rom (rom to boot)
   0,    // gpio_rom (unused)
   4,    // count
   2,    // last_main_rom
   0,    // last_setup_rom
   { 0x002000, 0x082000, 0x102000, 0x202000 } // roms[0-4]
   #ifdef BOOT_CONFIG_CHKSUM
   ,0x00
   ,{ 0x00, 0x00, 0x00 }
   #endif
};

static uint8_t calc_chksum(uint8_t *start, uint8_t *end) {
	uint8_t chksum = CHKSUM_INIT;
	while(start < end) {
		chksum ^= *start;
		start++;
	}
	return chksum;
}

int main() {
   FILE *fp = fopen("rconf.bin", "w+");
   if (!fp)
   {
      printf("couldn't open file\n");
      return -1;
   }
#ifdef BOOT_CONFIG_CHKSUM
   uint8_t csum = calc_chksum((uint8_t*)&testConfig, (uint8_t*)&testConfig.chksum);
   printf("rconf[%d] - checksum = %d\n", ROM_NUM, csum);
   testConfig.chksum = csum;
#endif

   size_t ret = fwrite(&testConfig, sizeof(uint8_t), sizeof(testConfig), fp);

   printf("fwrite returned %d\n", (int)ret);

   fclose(fp);

   return 0;
}
