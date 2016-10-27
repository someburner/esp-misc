#ifndef __JSMN_UTIL_H_
#define __JSMN_UTIL_H_

#include "jsmn.h"

int jsmn_mem_move(const char *json, void * inBuf, jsmntok_t *tok);
char * jsmn_get_value_as_string(const char *json, jsmntok_t *tok);
char * jsmn_get_value_as_primitive(const char *json, jsmntok_t *tok);
char * jsmn_get_object(const char *json, jsmntok_t *tok);
int8_t jsmneq(const char *json, jsmntok_t *tok, const char *s);

#endif /* __JSMN_UTIL_H_ */
