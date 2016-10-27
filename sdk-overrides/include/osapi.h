#ifndef _SDK_OVERRIDE_OSAPI_H_
#define _SDK_OVERRIDE_OSAPI_H_

#include "../../app_common/include/rom.h"
void ets_timer_arm_new(ETSTimer *a, int b, int c, int isMstimer);

int os_printf(const char *format, ...) __attribute__ ((format (printf, 1, 2)));
int os_printf_plus(const char *format, ...)  __attribute__ ((format (printf, 1, 2)));

void call_user_start(void);

#include_next "osapi.h"

#endif
