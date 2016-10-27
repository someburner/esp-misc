#include "user_interface.h"
#include "mem.h"
#include "c_stdio.h"
#include "user_config.h"

#include "jsmn_util.h"
/******************************************************************************
 * JSMN Helpers/Accessors
*******************************************************************************/
int jsmn_mem_move(const char *json, void * inBuf, jsmntok_t *tok)
{
   uint32_t len = tok->end-tok->start;
   if (!len) return -1;
   // char *s = (char*)os_zalloc(len+1);
   strncpy(inBuf, (char*)(json + tok->start), len);
   ((char*)inBuf)[len] = '\0';
   return 1;
}

char * jsmn_get_value_as_string(const char *json, jsmntok_t *tok)
{
   if (tok->type != JSMN_STRING)
      return NULL;

   uint32_t len = tok->end-tok->start;
   char *s = (char*)os_zalloc(len+1);
   strncpy(s, (char*)(json + tok->start), len);
   s[len] = '\0';
   return s;
}

char * jsmn_get_value_as_primitive(const char *json, jsmntok_t *tok)
{
   if (tok->type != JSMN_PRIMITIVE)
      return NULL;

   uint32_t len = tok->end-tok->start;
   char *s = (char*)os_zalloc(len+1);
   strncpy(s, (char*)(json + tok->start), len);
   s[len] = '\0';
   return s;
}

char * jsmn_get_object(const char *json, jsmntok_t *tok)
{
   if (tok->type != JSMN_OBJECT)
      return NULL;

   uint32_t len = tok->end-tok->start;
   char *s = (char*)os_zalloc(len+1);
   strncpy(s, (char*)(json + tok->start), len);
   s[len] = '\0';
   return s;
}

int8_t jsmneq(const char *json, jsmntok_t *tok, const char *s)
{
   if (tok->type == JSMN_STRING && (int) strlen(s) == tok->end - tok->start &&
                     strncmp(json + tok->start, s, tok->end - tok->start) == 0)
   {
      return 0;
   }
   return -1;
}
/*----------------------------------------------------------------------------*/
