#include "../libc/c_stdio.h"
#include "../platform/platform.h"
#include "spiffs.h"
// #include "rboot-api.h"

extern void ets_wdt_enable();
extern void ets_wdt_disable();

// statfs == 0
// dynfs == 1
spiffs fs[2];

const char *fs_name[] = { "static", "dynamic" };

#define LOG_PAGE_SIZE       256

static u8_t spiffs_work_buf[LOG_PAGE_SIZE*2];
static u8_t dynfs_work_buf[LOG_PAGE_SIZE*2];

static u8_t spiffs_fds[32*4];
static u8_t dynfs_fds[32*4];
#if SPIFFS_CACHE
static u8_t spiffs_cache[(LOG_PAGE_SIZE+32)*2];
#endif
#if DYNFS_CACHE
static u8_t dynfs_cache[(LOG_PAGE_SIZE+32)*2];
#endif

static s32_t my_spiffs_read(u32_t addr, u32_t size, u8_t *dst) {
  platform_flash_read(dst, addr, size);
  return SPIFFS_OK;
}

static s32_t my_spiffs_write(u32_t addr, u32_t size, u8_t *src) {
  platform_flash_write(src, addr, size);
  return SPIFFS_OK;
}

static s32_t my_spiffs_erase(u32_t addr, u32_t size) {
  u32_t sect_first = platform_flash_get_sector_of_address(addr);
  u32_t sect_last = sect_first;
  while( sect_first <= sect_last )
    if( platform_flash_erase_sector( sect_first ++ ) == PLATFORM_ERR )
      return SPIFFS_ERR_INTERNAL;
  return SPIFFS_OK;
}

void myspiffs_check_callback(spiffs_check_type type, spiffs_check_report report, u32_t arg1, u32_t arg2){
  if(SPIFFS_CHECK_PROGRESS == report) return;
  NODE_ERR("type: %d, report: %d, arg1: %d, arg2: %d\n", type, report, arg1, arg2);
}

/*******************
The W25Q32BV array is organized into 16,384 programmable pages of 256-bytes each. Up to 256 bytes can be programmed at a time. 
Pages can be erased in groups of 16 (4KB sector erase), groups of 128 (32KB block erase), groups of 256 (64KB block erase) or 
the entire chip (chip erase). The W25Q32BV has 1,024 erasable sectors and 64 erasable blocks respectively. 
The small 4KB sectors allow for greater flexibility in applications that require data and parameter storage. 

********************/

void spiffs_mount_manual(fs_type t, u32_t phys_addr, u32_t phys_size)
{
   spiffs_config cfg;

   cfg.phys_addr = phys_addr;
   cfg.phys_size = phys_size;
   cfg.phys_erase_block = INTERNAL_FLASH_SECTOR_SIZE; // according to datasheet
   cfg.log_block_size = INTERNAL_FLASH_SECTOR_SIZE * 2; // Important to make large
   cfg.log_page_size = LOG_PAGE_SIZE; // as we said

   cfg.hal_read_f = my_spiffs_read;
   cfg.hal_write_f = my_spiffs_write;
   cfg.hal_erase_f = my_spiffs_erase;

   if (t == statfs)
   {
   #ifndef STATFS_FIXED_LOCATION
      #error "STATFS_FIXED_LOCATION must be defined"
   #else
      cfg.phys_addr = STATFS_FIXED_LOCATION;
   #endif

   /* Physical size provided */
   #ifndef STATFS_PHYS_SZ
      #error "STATFS_PHYS_SZ must be defined"
   #else
      cfg.phys_size = STATFS_PHYS_SZ;
   #endif

      cfg.phys_erase_block =  STATFS_ERASE_BLK_SZ;
      cfg.log_block_size = STATFS_LOG_BLK_SZ;

      int res = SPIFFS_mount(&fs[0], &cfg, spiffs_work_buf, spiffs_fds, sizeof(spiffs_fds),
   #if SPIFFS_CACHE
      spiffs_cache,
      sizeof(spiffs_cache),
   #else
      0, 0,
   #endif
      0);
   }

   else if (t == dynfs)
   {
   #ifndef DYNFS_FIXED_LOCATION
      #error "DYNFS_FIXED_LOCATION must be defined"
   #else
      cfg.phys_addr = DYNFS_FIXED_LOCATION;
   #endif

   /* Physical size provided */
   #ifndef DYNFS_PHYS_SZ
      #error "DYNFS_PHYS_SZ must be defined"
   #else
      cfg.phys_size = DYNFS_PHYS_SZ;
   #endif

      cfg.phys_erase_block =  DYNFS_ERASE_BLK_SZ;
      cfg.log_block_size = DYNFS_LOG_BLK_SZ;

      int res = SPIFFS_mount(&fs[1], &cfg, dynfs_work_buf, dynfs_fds, sizeof(dynfs_fds),
   #if DYNFS_CACHE
      dynfs_cache,
      sizeof(dynfs_cache),
   #else
      0, 0,
   #endif
      0);
   }
}


void myspiffs_mount(fs_type t)
{
   spiffs_config cfg;

   cfg.hal_read_f = my_spiffs_read;
   cfg.hal_write_f = my_spiffs_write;
   cfg.hal_erase_f = my_spiffs_erase;
   cfg.log_page_size = LOG_PAGE_SIZE; // as we said

   int res;

   /* Semi-static (Small) */
   if (t == statfs)
   {
   /* Fixed location provided by user*/
   #ifndef STATFS_FIXED_LOCATION
      #error "STATFS_FIXED_LOCATION must be defined"
   #else
      cfg.phys_addr = STATFS_FIXED_LOCATION;
   #endif

   /* Physical size provided */
   #ifndef STATFS_PHYS_SZ
      #error "STATFS_PHYS_SZ must be defined"
   #else
      cfg.phys_size = STATFS_PHYS_SZ;
   #endif

      cfg.phys_erase_block =  STATFS_ERASE_BLK_SZ;
      cfg.log_block_size = STATFS_LOG_BLK_SZ;

      res = SPIFFS_mount(&fs[0], &cfg, spiffs_work_buf, spiffs_fds, sizeof(spiffs_fds),
   #if SPIFFS_CACHE
      spiffs_cache,
      sizeof(spiffs_cache),
   #else
      0, 0,
   #endif
      0);
   }

   /* Dynfs (Big) */
   else if (t == dynfs)
   {
      NODE_DBG("mounting dynfs");
   /* Fixed location provided by user*/
   #ifndef DYNFS_FIXED_LOCATION
      #error "DYNFS_FIXED_LOCATION must be defined"
   #else
      cfg.phys_addr = DYNFS_FIXED_LOCATION;
   #endif
      NODE_DBG(" @ %x\n", cfg.phys_addr);

      /* Physical size provided */
   #ifndef DYNFS_PHYS_SZ
      #error "DYNFS_PHYS_SZ must be defined"
   #else
      cfg.phys_size = DYNFS_PHYS_SZ;
   #endif

      cfg.phys_erase_block =  DYNFS_ERASE_BLK_SZ;
      cfg.log_block_size = DYNFS_LOG_BLK_SZ;

      res = SPIFFS_mount(&fs[1], &cfg, dynfs_work_buf, dynfs_fds, sizeof(dynfs_fds),
      #if DYNFS_CACHE
         dynfs_cache,
         sizeof(dynfs_cache),
      #else
         0, 0,
      #endif
      0);
   }

   NODE_DBG("mount returned %i\n", res);

   if (res == 0)
   {
      u32_t total, used;
      NODE_DBG("\n%s start:0x%x, size:0x%x\n",fs_name[t], cfg.phys_addr,cfg.phys_size);

      res = SPIFFS_info(&fs[t], &total, &used);
      // NODE_DBG("Used  (KB) = %d KB\n", (used/1000));
      // NODE_DBG("Total (KB) = %d KB\n", (total/1000));
      // NODE_DBG("Free  (KB) = %d KB\n", ((total-used)/1000));
   }
}

int myspiffs_mounted(fs_type t)
{
   return (int) SPIFFS_mounted(&fs[t]);
}

void myspiffs_unmount(fs_type t)
{
  SPIFFS_unmount(&fs[t]);
}


// FS formatting function
// Returns 1 if OK, 0 for error
int myspiffs_format(fs_type t)
{
   SPIFFS_unmount(&fs[t]);
   u32_t sect_first, sect_last;

   /* Semi-static (Small) */
   if (t == statfs)
   {
   #ifndef STATFS_FIXED_LOCATION
      #error "STATFS_FIXED_LOCATION must be defined"
   #else
      sect_first = STATFS_FIXED_LOCATION;
      sect_first = platform_flash_get_sector_of_address(sect_first);
   #endif

   #ifndef STATFS_PHYS_SZ
      #error "STATFS_PHYS_SZ must be defined"
   #else
      sect_last = (sect_first + platform_flash_get_sector_of_address(STATFS_PHYS_SZ)) - 1;
   #endif
   }


   /* Dynfs (Big) */
   else if (t == dynfs)
   {
   #ifndef DYNFS_FIXED_LOCATION
      #error "DYNFS_FIXED_LOCATION must be defined"
   #else
      sect_first = DYNFS_FIXED_LOCATION;
   #endif
      sect_first = platform_flash_get_sector_of_address(sect_first);

   #ifndef DYNFS_PHYS_SZ
      #error "DYNFS_PHYS_SZ must be defined"
   #else
      sect_last = (sect_first + platform_flash_get_sector_of_address(DYNFS_PHYS_SZ)) - 1;
   #endif
   }
   else
   {
      NODE_ERR("unkown fs type??\n");
      return 0;
   }

   NODE_DBG("format: sect_first: %x, sect_last: %x\n", sect_first, sect_last);
   while( sect_first <= sect_last )
      if( platform_flash_erase_sector( sect_first ++ ) == PLATFORM_ERR )
         return 0;
   myspiffs_mount(t);

   return 1;
}

int myspiffs_check(fs_type t)
{
  ets_wdt_disable();
  int res = (int)SPIFFS_check(&fs[t]);
  ets_wdt_enable();
  return res;
}

int myspiffs_info(fs_type t, uint32_t * total, uint32_t * used)
{
   return (int)SPIFFS_info(&fs[t], (u32_t *)total, (u32_t *)used);
}

int myspiffs_creat(fs_type t, const char *name)
{
   return (int)SPIFFS_creat(&fs[t], (char *)name, 0);
}

int myspiffs_open(fs_type t, const char *name, int flags)
{
  return (int)SPIFFS_open(&fs[t], (char *)name, (spiffs_flags)flags, 0);
}

spiffs_DIR *myspiffs_opendir(fs_type t, const char *name, spiffs_DIR *d)
{
   return SPIFFS_opendir(&fs[t], (char *)name, d);
}

int myspiffs_open_by_dirent(fs_type t, struct spiffs_dirent *e, int flags)
{
   return (int)SPIFFS_open_by_dirent(&fs[t], e,  (spiffs_flags)flags, 0);
}

int myspiffs_openpage(fs_type t, u16_t page_ix, int flags)
{
   (int) SPIFFS_open_by_page(&fs[t], (spiffs_page_ix)page_ix, (spiffs_flags)flags, 0);
}

struct spiffs_dirent *myspiffs_readdir(fs_type t, spiffs_DIR *d, struct spiffs_dirent *e)
{
   return SPIFFS_readdir(d, e);
}

int myspiffs_closedir(fs_type t, spiffs_DIR *d)
{
   return (int) SPIFFS_closedir(d);
}

int myspiffs_fstat(fs_type t, int fd, spiffs_stat *s)
{
   return (int)SPIFFS_fstat(&fs[t], (spiffs_file)fd, s);
}

int myspiffs_stat(fs_type t, const char *name, spiffs_stat *s)
{
   return (int)SPIFFS_stat(&fs[t], (char *)name, s);
}

int myspiffs_fmatch(fs_type t, int fd, const char *name)
{
   spiffs_stat s;
   u16_t saveid = 0;
   int res = 0;
   res = (int)SPIFFS_fstat(&fs[t], (spiffs_file)fd, &s);

   if (res < 0)
   {
      NODE_ERR("errno %i\n", SPIFFS_errno(&fs[t]));
      return -2;
   }

   if (strstr(s.name, name)==NULL) {
      NODE_ERR("errno %i\n", SPIFFS_errno(&fs[t]));
      return -1;
   }
   saveid = s.obj_id;

   res = SPIFFS_stat(&fs[t], (char *)name, &s);
   if (res < 0)
   {
      NODE_ERR("match errno %i\n", SPIFFS_errno(&fs[t]));
      return -2;
   }

   if ((s.obj_id) != saveid)
      { return -2; }

   return 1; //match!
}

int myspiffs_close(fs_type t, int fd)
{
  SPIFFS_close(&fs[t], (spiffs_file)fd);
  return 0;
}

size_t myspiffs_write(fs_type t, int fd, const void* ptr, size_t len)
{
#if 0
  if(fd==c_stdout || fd==c_stderr){
    uart0_tx_buffer((u8_t*)ptr, len);
    return len;
  }
#endif
  int res = SPIFFS_write(&fs[t], (spiffs_file)fd, (void *)ptr, len);
  if (res < 0) {
    NODE_DBG("write errno %i\n", SPIFFS_errno(&fs[t]));
    return 0;
  }
  return res;
}

size_t myspiffs_read(fs_type t, int fd, void* ptr, size_t len)
{
  int res = SPIFFS_read(&fs[t], (spiffs_file)fd, (void *)ptr, len);
  if (res < 0) {
    NODE_DBG("read errno %i\n", SPIFFS_errno(&fs[t]));
    return -1;
  }
  return res;
}

size_t myspiffs_remove(fs_type t, const char *path)
{
   int res = SPIFFS_remove(&fs[t], (char *)path);
   myspiffs_check(t);
   int delete_flag = SPIFFS_errno(&fs[t]);
   if (delete_flag == SPIFFS_ERR_NOT_FOUND)
      { SPIFFS_clearerr(&fs[t]); return 1; }
   if (res == SPIFFS_ERR_NOT_FOUND)
      { return 1; }
   if (res < 0)
   {
      NODE_DBG("remove res %i\n", res);
      return -1;
   }
   return res;
}

size_t myspiffs_fremove(fs_type t, int fd)
{
   int res = SPIFFS_fremove(&fs[t], (spiffs_file)fd);
   if (res >= 0)
   {
      NODE_DBG("frem ok\n");
      res = SPIFFS_close(&fs[t], (spiffs_file)fd);
      if (res >= 0) return 1;
   }
   NODE_DBG("frem er->chck\n");
   // myspiffs_check(t);

   int delete_flag = SPIFFS_errno(&fs[t]);
   if ( (delete_flag == SPIFFS_ERR_NOT_FOUND) || (delete_flag == SPIFFS_ERR_DELETED) )
      { SPIFFS_clearerr(&fs[t]); return 1; }
   if ( (res == SPIFFS_ERR_NOT_FOUND)  || (res == SPIFFS_ERR_DELETED) )
      { SPIFFS_clearerr(&fs[t]); return 1; }
   if (res < 0)
   {
      NODE_DBG("remove res %i\n", res);
      return -1;
   }
   return res;
}

int myspiffs_lseek(fs_type t, int fd, int off, int whence)
{
  return SPIFFS_lseek(&fs[t], (spiffs_file)fd, off, whence);
}

int myspiffs_eof(fs_type t, int fd)
{
  return SPIFFS_eof(&fs[t], (spiffs_file)fd);
}

int myspiffs_tell(fs_type t, int fd)
{
  return SPIFFS_tell(&fs[t], (spiffs_file)fd);
}
int myspiffs_getc(fs_type t, int fd)
{
  unsigned char c = 0xFF;
  int res;
  if(!myspiffs_eof(t, fd)){
    res = SPIFFS_read(&fs[t], (spiffs_file)fd, &c, 1);
    if (res != 1) {
      NODE_DBG("getc errno %i\n", SPIFFS_errno(&fs[t]));
      return (int)EOF;
    } else {
      return (int)c;
    }
  }
  return (int)EOF;
}

int myspiffs_ungetc(fs_type t, int c, int fd)
{
  return SPIFFS_lseek(&fs[t], (spiffs_file)fd, -1, SEEK_CUR);
}

int myspiffs_flush(fs_type t, int fd)
{
  return SPIFFS_fflush(&fs[t], (spiffs_file)fd);
}

int myspiffs_errno(fs_type t)
{
  return SPIFFS_errno(&fs[t]);
}

void myspiffs_clearerr(fs_type t)
{
  SPIFFS_clearerr(&fs[t]);
}

int myspiffs_rename(fs_type t, const char *old, const char *newname)
{
  return SPIFFS_rename(&fs[t], (char *)old, (char *)newname);
}

size_t myspiffs_size(fs_type t, int fd)
{
  return SPIFFS_size(&fs[t], (spiffs_file)fd);
}

int myspiffs_listall(fs_type t)
{
   spiffs_DIR d;
   struct spiffs_dirent e;
   struct spiffs_dirent *pe = &e;
   int res;

   u32_t total, used;
   res = SPIFFS_info(&fs[t], &total, &used);
   if (res < 0) {
      NODE_ERR("errno %i\n", SPIFFS_errno(&fs[t]));
      return -1;
   }
   NODE_DBG("SPIFFS: Used = %08x, Total = %08x\n", used, total);

   res = 0;
   SPIFFS_opendir(&fs[t], "/", &d);
   while ((pe = SPIFFS_readdir(&d, pe)))
   {
      res++;
      NODE_DBG("%s [%04x] size:%i\n", pe->name, pe->obj_id, pe->size);
   }
   SPIFFS_closedir(&d);
   return res;
}

#if 0
void test_spiffs()
{
  char buf[12];
  // Surely, I've mounted spiffs before entering here

  spiffs_file fd = SPIFFS_open(&fs[t], "test.txt", SPIFFS_CREAT | SPIFFS_TRUNC | SPIFFS_RDWR, 0);
  if (SPIFFS_write(&fs[t], fd, (u8_t *)"Hello world", 12) < 0) NODE_DBG("errno %i\n", SPIFFS_errno(&fs[t]));
  SPIFFS_close(&fs[t], fd);
  fd = SPIFFS_open(&fs[t], "test.txt", SPIFFS_RDWR, 0);
  if (SPIFFS_read(&fs[t], fd, (u8_t *)buf, 12) < 0) NODE_DBG("errno %i\n", SPIFFS_errno(&fs[t]));
  SPIFFS_close(&fs[t], fd);
  NODE_DBG("--> %s <--\n", buf);
}
#endif
