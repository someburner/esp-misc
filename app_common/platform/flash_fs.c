/* ========================================================================== *
 *                          Flash Filesystem APIs                             *
 *              App-level API for performing SPIFFS operations.               *
 * -------------------------------------------------------------------------- *
 *         Copyright (C) Jeff Hufford - All Rights Reserved. License:         *
 *                   "THE BEER-WARE LICENSE" (Revision 42):                   *
 * Jeff Hufford (jeffrey92<at>gmail.com) wrote this file. As long as you      *
 * retain this notice you can do whatever you want with this stuff. If we     *
 * meet some day, and you think this stuff is worth it, you can buy me a beer *
 * in return.                                                                 *
 * ========================================================================== */
#include "user_interface.h"
#include "user_config.h"
#include "mem.h"

#include "libc/c_stdio.h"
#include "jsontree/jsonparse.h"
#include "jsontree/jsontree.h"
#include "jsontree/jsonutil.h"
#include "util/crc.h"

#include "flash_fs.h"

/*******************************************************************************
 * Macros
*******************************************************************************/
#define DO_RETURN(val) \
   return val;

#define CLOSE_AND_RETURN(fs, fd, val) \
   if (val < 0) { \
      myfs_close(fs, fd); \
      return val; \
   }

#define DO_ERRNO(val) \
   NODE_DBG("fs_errno: %d\n", myfs_errno(FS0)); \
   DO_RETURN(val);

#define NULL_CHECK(ptr, on_err, err_arg) \
	if (!(ptr)) { \
		on_err(err_arg); \
	}

#define NEG_CHECK(input, on_err, err_arg) \
	if (input < 0) { \
		on_err(err_arg); \
	}

#define RDONLY_CHECK(fs_ptr, on_err, err_arg) \
   if (((fs_ptr)->flags) & MARK_RDONLY) { \
      NODE_ERR("This file is marked read only!\n"); \
      on_err(err_arg); \
   }

/* Update 'fsN' with either FS0 or FS1 given a fs_file_st * fs_ptr
 * If fs_ptr NULL or neither flag set, fsN will be -1 after this macro,
 * indicating an error to the caller
 */
#define GET_FS(fs_ptr, fsN) \
   fsN = -1; \
	if ((fs_ptr)) { \
		if ((fs_ptr)->flags & MARK_DYNFS) { fsN = FS1; } \
      else if ((fs_ptr)->flags & MARK_STATICFS) { fsN = FS0; } \
	}


/*******************************************************************************
 * Static declarations
*******************************************************************************/
/*---------------------------------------------------------------------------*/
static int items_cb(struct jsontree_context *js_ctx);


/*---------------------------------------------------------------------------*/
/* FS JSON Create Callback */
static struct jsontree_callback json_items_cb = JSONTREE_CALLBACK(items_cb, NULL);


/*---------------------------------------------------------------------------*/
/* JSONTREE Objects */
JSONTREE_ARRAY(items_arr,
   JSONTREE_PAIR_ARRAY(&json_items_cb)
);

JSONTREE_OBJECT(fs_tree,
      JSONTREE_PAIR("files", &items_arr)
);

/*******************************************************************************
 * JSON Methods
*******************************************************************************/
/* ------------------------ Callback implementations ------------------------ */
static int items_cb(struct jsontree_context *path)
{
   static struct fs_file_st *f_st;
   static unsigned i = 0;

   if (path->callback_state == 0) {
      f_st = STAILQ_FIRST(&fs_file_list);
   }

   while (f_st != NULL)
   {
      path->putchar('{');
      jsontree_write_string(path, "name");
      path->putchar(':');
      jsontree_write_string(path, f_st->name);
      path->putchar(',');

      jsontree_write_string(path, "size");
      path->putchar(':');
      jsontree_write_int(path, f_st->size);
      path->putchar(',');
      jsontree_write_string(path, "fs");
      path->putchar(':');
      int fsNo = -1;
      GET_FS(f_st, fsNo);
      jsontree_write_int(path, fsNo);
      path->putchar('}');

      i++;
      path->callback_state++;


      f_st = STAILQ_NEXT(f_st, next);
      if (f_st != NULL)
      {
         path->putchar(',');
      } else {
         return 0;
      }
   }
   return 1;
}

/* Returns JSON object containing array of all files */
char * fsJSON_get_list()
{
   /* Init JSON Structure */
   char * jsonStr;
   int sz = 0;
   sz = json_print((struct jsontree_value *)&fs_tree, "name", &jsonStr);

   if ( (jsonStr) && (sz) )
   {
      return jsonStr;
   }
   return NULL;
}

/*******************************************************************************
 * "Wrapper" Utilities
*******************************************************************************/
/* ----------------------------- Init Utilities ----------------------------- */
void fs_init_info(void)
{
   fs_DIR d, d2;

   struct fs_dirent e, e2;
   struct fs_dirent *pe = &e;
   struct fs_dirent *pe2 = &e2;

   fs_opendir("/", &d);
   while ((pe = fs_readdir(&d, pe)))
   {
      struct fs_file_st *f = NULL;
      f = (struct fs_file_st *)os_malloc(sizeof(struct fs_file_st));
      int size = strlen(pe->name);
      f->pix = pe->pix;
      f->size = pe->size;
      f->name = (char*)os_zalloc(sizeof(char)*size+1);
      strncpy(f->name, pe->name, size+1);
      f->flags = 0;
      f->crc = 0;

      /* set initial files to read only */
      // f->flags |= MARK_RDONLY;
      f->flags |= MARK_STATICFS;

      STAILQ_INSERT_HEAD(&fs_file_list, f, next);
   }
   fs_closedir(&d);

   /* Initialize dynamic fs files too */
   dynfs_opendir("/", &d2);
   while ((pe2 = dynfs_readdir(&d2, pe2)))
   {
      struct fs_file_st *f2 = NULL;
      f2 = (struct fs_file_st *)os_malloc(sizeof(struct fs_file_st));
      int size = strlen(pe2->name);
      f2->pix = pe2->pix;
      f2->size = pe2->size;
      f2->name = (char*)os_zalloc(sizeof(char)*size+1);
      strncpy(f2->name, pe2->name, size+1);

      f2->flags = 0;
      f2->crc = 0;

      /* Use MARK_BIG flag so that we know we need to update pix every so
       * often while doing r/w operations on the file. (Or only use filename)
       */
      if (pe2->size > 65000)
         f2->flags |= MARK_BIG;

      f2->flags |= MARK_DYNFS;
      FS_DBG("dynfs file: %s, flags = %d\n", f2->name, (int)f2->flags);

      STAILQ_INSERT_HEAD(&fs_file_list, f2, next);
   }
   fs_closedir(&d2);

   struct fs_file_st *f3;
   STAILQ_FOREACH(f3, &fs_file_list, next)
   {
      if (f3->size <= 255)
         f3->flags |= MARK_CRC8;
      else if (f3->size <= 65535)
         f3->flags |= MARK_CRC16;
      else
         f3->flags |= MARK_CRC32;

      fs_update_crc(f3);
   #ifdef FS_DEBUG
      FS_DBG("[%04x] %s: %d bytes | flags: %d\n", f3->pix, f3->name, f3->size, (int)f3->flags);
      FS_DBG("CRC of %s= 0x%x\n",f3->name, f3->crc);
   #endif
   }
   f3 = STAILQ_FIRST(&fs_file_list);

}

/*
 * Checks if file exists on any FS.
 * Returns 1 if the file given by 'name' was found
 * Returns -1 if it was not found
 */
int fs_exists(char * name, struct fs_file_st ** fs_st_ptr)
{
   NULL_CHECK(name, DO_RETURN, -1);

   struct fs_file_st *fs_st = STAILQ_FIRST(&fs_file_list);
   STAILQ_FOREACH(fs_st, &fs_file_list, next)
   {
      if ((strstr(fs_st->name,name)) != NULL)
      {
         *fs_st_ptr = fs_st;
         return 1;
      }
   }
   fs_st = STAILQ_FIRST(&fs_file_list);
   return -1;
}

int fs_read_file(struct fs_file_st * fs_st, void * buff, uint16_t len, uint32_t pos)
{
   int fd = -1;
   int res = -1;
   NULL_CHECK(fs_st, DO_RETURN, -1);

   int fsNo;
   GET_FS(fs_st, fsNo);
   NEG_CHECK(fsNo, DO_RETURN, -1);

   if (!(fs_st->flags & MARK_BIG))
   {
      // fd = myfs_openpage(fsNo, fs_st->pix, FS_RDWR);
      fd = myfs_openname(fsNo, fs_st->name, FS_RDWR);
   } else {
      fd = myfs_openname(fsNo, fs_st->name, FS_RDWR);
   }
   NEG_CHECK(fd, DO_RETURN, -1);

   if (fs_st->size > 0)
   {
      res = myfs_seek(fsNo, fd, pos, FS_SEEK_SET);
      CLOSE_AND_RETURN(fsNo, fd, res);
   } else {
      NODE_ERR("Zero length file?\n");
      CLOSE_AND_RETURN(fsNo, fd, -1);
   }

   res = myfs_read(fsNo, fd, buff, len);
   CLOSE_AND_RETURN(fsNo, fd, res);
   // NEG_CHECK(res, DO_RETURN, -1);

   myfs_close(fsNo, fd);

   return res;
}


/* Creates a new file on FS == 'fs'
 * @PARAM1: fs - the filesystem to create new file on
 * @PARAM2: ** to fs_file_st:
 *       If the file exists on 'fs', the file's details (which are stored in a
 *    fs_file_st *) will be assigned to the pointer-to-pointer and accessible
 *    via *fs_file_st on sucess.
 *       If not, or if overwrite == TRUE, a new file with name 'filename' will
 *    be created and added to the linked list.
 *       File is created fresh, with no data written, and then closed
 */
int fs_new_file(int fs, struct fs_file_st ** fs_st_ptr, char * filename, bool overwrite)
{
   FS_DBG("fs_new_file\n");
   NULL_CHECK(filename, DO_RETURN, NULL);
   FS_DBG("filename %s\n", filename);

   /* Variables */
   struct fs_file_st * fs_st = NULL;
   int fd = -1;
   spiffs_stat s;
   int ret = -1;
   int queue_res = -1;
   int flags = (FS_RDWR | FS_CREAT);


   int fsNo = -1;

   /* See if it exists */
   ret = fs_exists(filename, &fs_st);
   FS_DBG("fs_exists ret = %d\n", ret);

   /* If it exists, see if it is on the same FS as the one passed in */
   if (ret == 1)
   {
      GET_FS(fs_st, fsNo);
      if (fsNo == fs)
      {
         FS_DBG("file exists, size = %d\n", fs_st->size);

         /* File exists but no overwrite requested */
         if (!(overwrite))
         {
            FS_DBG("But no overwrite. 0\n");
            return 0;
         }
      }

      /* If we made it this far, we can go ahead and truncate ( if needed) */
      if (fs_st->size >= 0)
      {
         flags |= FS_TRUNC;
         fs_st->size = 0;
         fs_st->crc = 0;
      }
      fd = myfs_openname(fsNo, filename, flags);
      // fd = myfs_openpage(fsNo, fs_st->pix, flags);
      /* Check that file is not read only */
      // RDONLY_CHECK(fs_st, DO_RETURN, ret);
   } else {
      fsNo = fs;
      NODE_ERR("file on fs%d DNE\n", fsNo);
      fd = myfs_openname(fsNo, filename, flags);
   }

   if (fd < 0)
   {
      if ( myfs_errno(fsNo) == SPIFFS_ERR_DELETED )
      {
         myfs_clearerr(fsNo);
         myfs_check(fsNo);
      }
   }
   FS_DBG("file create ok. fd = %d\n", fd);

   queue_res = myfs_fstat(fsNo, fd, &s);
   FS_DBG("queue_res = %d\n", queue_res);

   if (queue_res >= 0) { FS_DBG("stat size = %d, stat pix = %d\n", s.size, s.pix); }

   if (queue_res >= 0)
   {
      if (!fs_st)
      {
         fs_st = (struct fs_file_st *)os_malloc(sizeof(struct fs_file_st));
         int size = strlen(filename);
         fs_st->name = (char*)os_zalloc(sizeof(char)*size+1);
         strncpy(fs_st->name, filename, size);
         fs_st->name[size] = '\0';
         fs_st->flags = 0;

         // fs_st->flags |= MARK_STATICFS;

         /* Set flag for which FS */
         //fsNo 0 --> +3 = MARK_STATICFS
         //fsNo 1 --> +4 = MARK_DYNFS
         fs_st->flags = (1 << (fsNo+3));
         STAILQ_INSERT_HEAD(&fs_file_list, fs_st, next);
      }

      fs_st->pix = s.pix;
      fs_st->size = 0;
      FS_DBG("fs_st values updated: pix = %d\n", fs_st->pix);

      myfs_close(fsNo, fd);
      *fs_st_ptr = fs_st;
      return 1;
   } else {
      FS_DBG("error adding to fs_st queue\n");
      return -1;
   }

}

/* Append to file.
 * Takes a pointer to fs_file_st (get with fs_exists()).
 * If the file exists, *data will be written for len bytes. Returns # of bytes
 * written.                                                                   */
int fs_append_to_file(struct fs_file_st * fs_st,  void * data, uint16_t len)
{
   int fd = -1;
   int res = -1;

   NULL_CHECK(len, DO_RETURN, res);
   NULL_CHECK(data, DO_RETURN, res);
   NULL_CHECK(fs_st, DO_RETURN, res);
   // RDONLY_CHECK(fs_st, DO_RETURN, res);

   if (fs_st->size + len > 65535)
   {
      fs_st->flags |= MARK_BIG;
   }

   int fsNo;
   GET_FS(fs_st, fsNo);
   NEG_CHECK(fsNo, DO_RETURN, -1);
   // FS_DBG("fs_append: st->pix = %d\n", fs_st->pix);
   // fd = myfs_openpage(fsNo, fs_st->pix, (FS_RDWR | FS_APPEND));
   fd = myfs_openname(fsNo, fs_st->name, (FS_RDWR | FS_APPEND));
   if (fd < 0)
   {
      NODE_ERR("open err: ");
      DO_ERRNO(-1);
   }
   // NEG_CHECK(fd, DO_ERRNO, -1);

   res = myfs_write(fsNo, fd, data, len);


   myfs_close(fsNo, fd);
   if (res > 0) { fs_st->size += res; }
   else { NODE_ERR("write err: "); DO_ERRNO(-1); }

   return res;
}

int fs_rename_file(int fs, char * old, char * new)
{
   struct fs_file_st * fs_st = NULL;
   int ret;
   NULL_CHECK(old, DO_RETURN, -1);
   NULL_CHECK(new, DO_RETURN, -1);

   ret = fs_exists(old, &fs_st);
   NULL_CHECK(fs_st, DO_RETURN, -1);

   int fsNo;
   GET_FS(fs_st, fsNo);
   NEG_CHECK(fsNo, DO_RETURN, -1);

   ret = myfs_rename(fsNo, old, new);
   NEG_CHECK(ret, DO_RETURN, -1);

   os_free(fs_st->name);
   ret = strlen(new);

   fs_st->name = (char*)os_zalloc(sizeof(char)*ret+1);
   memcpy(fs_st->name, new, ret+1);

   return 1;
}

int fs_remove_by_name(char * filename)
{
   int res = -1;
   int fd = -1;
   NULL_CHECK(filename, DO_RETURN, res);
   struct fs_file_st * fs_st = NULL;

   res = fs_exists(filename, &fs_st);
   FS_DBG("remove exists res=  %d\n", res);
   NULL_CHECK(fs_st, DO_RETURN, res);
   // struct fs_file_st *fs_st = fs_exists(filename);
   // NULL_CHECK(fs_st, DO_RETURN, res);
   // RDONLY_CHECK(fs_st, DO_RETURN, res);

   int fsNo;
   GET_FS(fs_st, fsNo);
   NEG_CHECK(fsNo, DO_RETURN, res);

   fd = myfs_openname(fsNo, fs_st->name, FS_RDWR);
   NEG_CHECK(fd, DO_RETURN, -1);
   FS_DBG("remove exists fd =  %d\n", fd);

   res = myfs_fremove(fsNo, fd);
   FS_DBG("remove res=  %d\n", res);
   // NEG_CHECK(fd, DO_RETURN, myfs_errno(fsNo));

   if ((res >= 0) || (res == SPIFFS_ERR_NOT_FOUND) || (res == SPIFFS_ERR_DELETED))
   {
      STAILQ_REMOVE(&fs_file_list, fs_st, fs_file_st, next);
      os_free(fs_st->name);
      os_free(fs_st);
      res = 1;
   }
   return res;
}

void fs_update_list(void)
{
   struct fs_file_st *fs_st = STAILQ_FIRST(&fs_file_list);
   STAILQ_FOREACH(fs_st, &fs_file_list, next)
   {
      if (fs_st->flags & MARK_DELETE)
      {
         STAILQ_REMOVE(&fs_file_list, fs_st, fs_file_st, next);
         os_free(fs_st->name);
         os_free(fs_st);
         break;
      }
   }
   fs_st = STAILQ_FIRST(&fs_file_list);
}

int fs_update_pix(struct fs_file_st * fs_st)
{
   spiffs_stat s;
   int stat_res = -1;

   int fsNo;
   GET_FS(fs_st, fsNo);
   NEG_CHECK(fsNo, DO_RETURN, -1);

   int fd = myfs_openname(fsNo, fs_st->name, FS_RDWR);
   NEG_CHECK(fd, DO_ERRNO, -1);

   stat_res = myfs_fstat(fsNo, fd, &s);
   FS_DBG("append fstat res = %d\n", stat_res);
   if (stat_res >= 0)
   {
      FS_DBG("stat size = %d, stat pix = %d\n", s.size, s.pix);
      fs_st->size = s.size;
      fs_st->pix = s.pix;
   }
   myfs_close(fsNo, fd);
   return stat_res;
}

uint32_t fs_get_avail(int fs)
{
   int res = -1;
   uint32_t used, total;
   res = myfs_info(fs, &total, &used);
   NEG_CHECK(res, DO_RETURN, -1);
   return (total - used);
}

bool fs_update_crc(struct fs_file_st * fs_st)
{
   uint8_t crcBuf[CRCBUF_LEN];
   static uint32_t crcOut = 0;
   uint32_t bytesRem = 0;

   int res = -1;
   int fd = -1;
   NULL_CHECK(fs_st, DO_RETURN, -1);

   int fsNo;
   GET_FS(fs_st, fsNo);
   NEG_CHECK(fsNo, DO_RETURN, -1);

   fd = myfs_openname(fsNo, fs_st->name, FS_RDWR);
   NEG_CHECK(fd, DO_RETURN, -1);

   FS_DBG("fs_update_crc:\n");

   if (fs_st->size > 0)
   {
      crcOut = 0;
      res = myfs_seek(fsNo, fd, 0, FS_SEEK_SET);
      CLOSE_AND_RETURN(fsNo, fd, res);

      bytesRem = fs_st->size;

      while (bytesRem)
      {
         uint16_t toRead = CRCBUF_LEN;
         if (bytesRem < CRCBUF_LEN)
            toRead = bytesRem;

         res = myfs_read(fsNo, fd, crcBuf, toRead);
         if (res > 0)
         {
            bytesRem -= res;
            if (fs_st->flags & MARK_CRC32)
               crcOut = Crc32_ComputeBuf(crcOut, crcBuf, res);
            else if (fs_st->flags & MARK_CRC16)
               crcOut = (uint16_t) Crc16_ComputeBuf((uint16_t)crcOut, crcBuf, res);
            else if (fs_st->flags & MARK_CRC8)
            {
               crcOut = (uint8_t) Crc8_ComputeBuf((uint8_t)crcOut, crcBuf, res);
            }
         }
         else if (res == 0)
            break;
      }
      /* Update crc value */
      fs_st->crc = crcOut;
      FS_DBG("%s crc: %u\n", fs_st->name, fs_st->crc);
   }
   else
   {
      NODE_ERR("Zero length file?\n");
      CLOSE_AND_RETURN(fsNo, fd, false);
   }

   res = myfs_seek(fsNo, fd, 0, FS_SEEK_SET);

   myfs_close(fsNo, fd);

   return true;
}
