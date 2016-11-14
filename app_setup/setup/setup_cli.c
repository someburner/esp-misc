// Reads commands from UART buffer
#include "user_interface.h"
#include <ctype.h>
#include "osapi.h"
#include "user_config.h"

#include "../app_common/libc/c_stdio.h"
#include "../app_common/rboot/rboot-api.h"

#include "driver/gpio16.h"
#include "driver/uart.h"

#include "setup_cli.h"

static char * rxBuf = NULL;

// Shift rest of string left by ndx
void setup_cli_shift(char *str, int ndx)
{
   str -= ndx;

   while (*(str + ndx))
   {
      *str = *(str + ndx);
      str++;
   }
   *str = '\0';
}

// Remove unnecessary white space from uart command, return first command
int setup_cli_parse(char *str)
{
   int numWS = 0;
   int first = 1;
   char *head = str;

   int len = strlen(str);
   if (len <= 2)
   {
      // Too short
      memset(rxBuf, 0, len);
      return -1;
   }

   memset(rxBuf, 0, 128);
   if (str[len - 1] == '\n')
      str[len - 1] = '\0';

   while (*str)
   {
      if (*str == ' ')
      {
         numWS++;
         if (first)
         {
            memcpy(rxBuf, head, str - head);
            first = 0;
         }
      }

      else
      {
         if (numWS > 1)
         {
            setup_cli_shift(str, numWS - 1);
            str = str - (numWS - 1);
         }
         numWS = 0;
      }

      if (isupper((int)*str))
         *str = (char)tolower((int)*str);

      str++;
   }

   if (numWS)
      *(str - numWS) = '\0';

   if (first)
      memcpy(rxBuf, head, str - head);
}

void setup_cli_get_cmd(char *cmd)
{
   char * ptr = NULL;
   /* strstr(haystack, needle) */

   if ((ptr = strstr(cmd, "show id")) != NULL)
   {
      NODE_DBG("Chip ID: 0x%X\n", system_get_chip_id());
   }

   else if ((ptr = strstr(cmd, "show rboot")) != NULL)
   {
      rboot_print_config(true);
   }

   else if ((ptr = strstr(cmd, "show heap")) != NULL)
   {
      NODE_DBG("Free Heap: %d\n", system_get_free_heap_size());
   }

   else
   {
      NODE_DBG("invalid cmd\n");
   }
}

void setup_cli_rboot_cmd(char *cmd, uint16_t len)
{
   char * ptr = NULL;
   uint16_t i;
   /* strstr(haystack, needle) */
   NODE_DBG("setup_cli_rboot_cmd: len=%d\n", len);
   if ((ptr = strstr(cmd, "rboot switch")) != NULL)
   {
      for (i=11; i<len; i++)
      {
         if ((cmd[i] >= '0') && (cmd[i] <= '3'))
         {
            rboot_switch( ((uint8_t) (cmd[i]-48)), true );
         } else {
            NODE_DBG("?%c\n", cmd[i]);
         }
      }
   }
}


void register_rx_buf(char * buf)
{
   rxBuf = buf;
}
