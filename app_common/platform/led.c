#include "user_interface.h"
#include "user_config.h"

#include "../app_common/libc/c_stdio.h"
#include "../app_common/include/hardware_defs.h"

#include "gpio.h"

#include "led.h"

void toggle_led(uint8_t pin)
{
   switch (pin)
   {
      case LED_PIN_ONBOARD:
      {
         if (GPIO_REG_READ(GPIO_OUT_ADDRESS) & BIT2)
            gpio_output_set(0, BIT2, BIT2, 0);
         else
            gpio_output_set(BIT2, 0, BIT2, 0);
      } break;
   }
}

void led_init(void)
{
   /* Pin 4 = GPIO2 - Onboard LED - Set to output */
   PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_GPIO2);
   //Set GPIO2 high initially
   gpio_output_set(BIT2, 0, BIT2, 0);
}
