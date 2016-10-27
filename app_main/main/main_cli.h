#ifndef _MAIN_CLI_H
#define _MAIN_CLI_H

void register_rx_buf(char * buf);

void main_cli_shift(char *str, int ndx);
int main_cli_parse(char *str);

void main_cli_get_cmd(char *cmd);
void main_cli_rboot_cmd(char *cmd, uint16_t len);
#endif
