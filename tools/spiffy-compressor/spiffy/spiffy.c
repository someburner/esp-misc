#include <dirent.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <arpa/inet.h>

#define SPIFFY_SRC_MK
#include "../../../app_common/include/fs_config.h"
#include "../../../app_common/include/server_config.h"
#include "spiffs.h"

#define VERS_STR_STR(V) #V
#define VERS_STR(V) VERS_STR_STR(V)

//Spiffy
#define LOG_BLOCK_SIZE     	(8*1024)
#define ERASE_BLOCK_SIZE   	(4*1024)
#define SPI_FLASH_SEC_SIZE 	(4*1024)
#define LOG_PAGE_SIZE			256

#define DEFAULT_ROM_NAME "spiffy_rom.bin"
#define ROM_ERASE 0xFF

#define DEFAULT_ROM_SIZE 0x10000

static spiffs fs;
static u8_t spiffs_work_buf[LOG_PAGE_SIZE*2];
static u8_t spiffs_fds[32*4];
static u8_t spiffs_cache_buf[(LOG_PAGE_SIZE+32)*2];

#define S_DBG
// #define S_DBG printf

static FILE *rom = 0;

#ifdef SPIFFY_DEBUG
#define DEBUG(...) fprintf(stderr, __VA_ARGS__)
#else
#define DEBUG
#endif

#define PRINT_OUT(...) fprintf(stderr, __VA_ARGS__)


#ifndef MAGIC_NUM
#define MAGIC_NUM    0x1234
#endif

/*******************************************************************************
 * Populate Settings on SPI Flash
*******************************************************************************/
/* ------------------------------ Main Settings ----------------------------- */
FLAGCFG flagConf =
{
   0 // clear all flags
};


//Routines to convert host format to the endianness used in the xtensa
short htoxs(short in)
{
	char r[2];
	r[0]=in;
	r[1]=in>>8;
	return *((short *)r);
}

int htoxl(int in)
{
	unsigned char r[4];
	r[0]=in;
	r[1]=in>>8;
	r[2]=in>>16;
	r[3]=in>>24;
	return *((int *)r);
}

/* ############################################################ *
 * ##################         SPIFFS           ################ *
 * ############################################################ */
void hexdump_mem(u8_t *b, u32_t len)
{
	int i;
	for (i = 0; i < len; i++) {
		S_DBG("%02x", *b++);
		if ((i % 16) == 15) S_DBG("\n");
		else if ((i % 16) == 7) S_DBG(" ");
	}
	if ((i % 16) != 0) S_DBG("\n");
}


static s32_t my_spiffs_read(u32_t addr, u32_t size, u8_t *dst)
{
	int res;

	if (fseek(rom, addr, SEEK_SET))
	{
		DEBUG("Unable to seek to %d.\n", addr);
		return SPIFFS_ERR_END_OF_OBJECT;
	}

	res = fread(dst, 1, size, rom);
	if (res != size)
	{
		DEBUG("Unable to read - tried to get %d bytes only got %d.\n", size, res);
		return SPIFFS_ERR_NOT_READABLE;
	}

	S_DBG("Read %d bytes from offset %d.\n", size, addr);
	hexdump_mem(dst, size);
	return SPIFFS_OK;
}

static s32_t my_spiffs_write(u32_t addr, u32_t size, u8_t *src)
{
	if (fseek(rom, addr, SEEK_SET))
	{
		DEBUG("Unable to seek to %d.\n", addr);
		return SPIFFS_ERR_END_OF_OBJECT;
	}

	if (fwrite(src, 1, size, rom) != size)
	{
		DEBUG("Unable to write.\n");
		return SPIFFS_ERR_NOT_WRITABLE;
	}

	fflush(rom);
	S_DBG("Wrote %d bytes to offset %d.\n", size, addr);

	return SPIFFS_OK;
}


static s32_t my_spiffs_erase(u32_t addr, u32_t size)
{
	int i;

	if (fseek(rom, addr, SEEK_SET))
	{
		DEBUG("Unable to seek to %d.\n", addr);
		return SPIFFS_ERR_END_OF_OBJECT;
	}

	for (i = 0; i < size; i++)
	{
		if (fputc(ROM_ERASE, rom) == EOF)
		{
			DEBUG("Unable to write.\n");
			return SPIFFS_ERR_NOT_WRITABLE;
		}
	}

	fflush(rom);
	S_DBG("Erased %d bytes at offset 0x%06x.\n", (int)size, addr);

	return SPIFFS_OK;
}

int my_spiffs_mount(u32_t msize)
{
	spiffs_config cfg;
#if (SPIFFS_SINGLETON == 0)
	cfg.phys_size = msize;
	cfg.phys_addr = 0;

	cfg.phys_erase_block =  ERASE_BLOCK_SIZE;
	cfg.log_block_size =  LOG_BLOCK_SIZE;
	cfg.log_page_size = LOG_PAGE_SIZE;
#endif

	cfg.hal_read_f = my_spiffs_read;
	cfg.hal_write_f = my_spiffs_write;
	cfg.hal_erase_f = my_spiffs_erase;

	int res = SPIFFS_mount(&fs,
			&cfg,
			spiffs_work_buf,
			spiffs_fds,
			sizeof(spiffs_fds),
			spiffs_cache_buf,
			sizeof(spiffs_cache_buf),
			0);
	PRINT_OUT("Mount result: %d.\n", res);

	return res;
}

void my_spiffs_unmount()
{
	SPIFFS_unmount(&fs);
}

int my_spiffs_format()
{
	int res  = SPIFFS_format(&fs);
	DEBUG("Format result: %d.\n", res);
	return res;
}

int write_to_spiffs(char *fname, u8_t *data, int size)
{
	int ret = 0;
	spiffs_file fd = -1;

	fd = SPIFFS_open(&fs, fname, SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);

	if (fd < 0) { PRINT_OUT("Unable to open spiffs file '%s', error %d.\n", fname, fd); }
	else
	{
		if (SPIFFS_write(&fs, fd, (u8_t *)data, size) < SPIFFS_OK)
		{
			DEBUG("Unable to write to spiffs file '%s', errno %d.\n", fname, SPIFFS_errno(&fs));
		}
		else { ret = 1; }
	}

	if (fd >= 0)
	{
		SPIFFS_close(&fs, fd);
		PRINT_OUT("Closed spiffs file '%s'.\n", fname);
	}
	return ret;
}

int get_rom_size (const char *str)
{
	long val;

	// accept decimal or hex, but not octal
	if ((strlen(str) > 2) && (str[0] == '0') && (((str[1] == 'x')) || ((str[1] == 'X'))))
	{
		val = strtol(str, NULL, 16);
	}
	else { val = strtol(str, NULL, 10); }

	return (int)val;
}

int main(int argc, char **argv)
{
#ifdef STA_SSID
   char xstr[] = VERS_STR(STA_SSID);
   PRINT_OUT("%s\n", xstr);
#endif
#if STA_PASS_EN==1
   PRINT_OUT("PW Enabled\n");
#endif
   int sz;
   uint8_t *buf;

	struct stat statBuf;
	int serr;
	int res, ret = EXIT_SUCCESS;

	const char *romfile  = DEFAULT_ROM_NAME;

	int romsize;
	romsize = DEFAULT_ROM_SIZE;
	PRINT_OUT("\nSpiffy main.c begin\n");
   PRINT_OUT("Using Bridge Version: %u\n", BRIDGE_FW_VERSION);
	PRINT_OUT("Creating rom '%s' of size 0x%x (%d) bytes.\n", romfile, romsize, romsize);
	DEBUG(" > %s opened for writing...\n", DEFAULT_ROM_NAME);

	rom = fopen(romfile, "wb+");
	if (!rom)
	{
		PRINT_OUT("Unable to open file '%s' for writing.\n", romfile);
		exit(0);
	}

	DEBUG("Writing blanks...\n");
	int i;
	for (i = 0; i < romsize; i++)
	{
		fputc(ROM_ERASE, rom);
	}
	fflush(rom);

	if (!my_spiffs_mount(romsize))
	{
		my_spiffs_unmount();
	}

	if ((res =  my_spiffs_format()))
	{
		PRINT_OUT("Failed to format spiffs, error %d.\n", res);
		ret = EXIT_FAILURE;
	} else if ((res = my_spiffs_mount(romsize))) {
		PRINT_OUT("Failed to mount spiffs, error %d.\n", res);
		ret = EXIT_FAILURE;
	} else {
      write_to_spiffs(FLAG_CFGNAME,   (uint8_t*)&flagConf,   sizeof(flagConf)   );
      // more files here
	} /*end my_spiffs_mount*/

	if (rom) fclose(rom);
	if (ret == EXIT_FAILURE) unlink(romfile);
	exit(EXIT_SUCCESS);
return 0;
}
