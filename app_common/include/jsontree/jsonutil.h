#ifndef JSONUTIL_H_
#define JSONUTIL_H_

#define JSON_BUFF_SZ             2*1024

/* Generating */
void json_debug_print(struct jsontree_value *tree, const char *path);
void json_ws_send(struct jsontree_value *tree, const char *path, char *pbuf);
int json_putchar(int c);
// char * json_print(struct jsontree_value *tree, const char *path);
int json_print(struct jsontree_value *tree, const char *path, char ** reqJsonBuf);

/* Parsing */
void json_parse(struct jsontree_context *json, const char *ptrJSONMessage);
struct jsontree_value * find_json_path(struct jsontree_context *json, const char *path);
int json_get_putchar_Size();
#endif
