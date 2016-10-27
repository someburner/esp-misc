#ifndef _SETUP_CLI_H
#define _SETUP_CLI_H

void register_rx_buf(char * buf);

void setup_cli_shift(char *str, int ndx);
int setup_cli_parse(char *str);

void setup_cli_get_cmd(char *cmd);
void setup_cli_rboot_cmd(char *cmd, uint16_t len);
#endif
