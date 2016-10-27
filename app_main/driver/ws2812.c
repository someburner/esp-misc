#include "user_config.h"
#include "osapi.h"
#include "mem.h"
#include "c_types.h"

#include "../app_common/libc/c_stdio.h"
#include "../app_common/util/cbuff.h"

#include "driver/spi.h"
#include "driver/gpio16.h"
#include "driver/ws2812.h"

#define SPI_DEV HSPI

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

#define PIXELS 32

os_timer_t ws2812_timer;
static uint32_t ws2812_timer_arg = 0;

static uint8_t cur_m = 0;
static uint8_t cur_pattern = 0;
static bool fade_in_out = true;

float patterns [] [3] = {
  { 1, 0, 0 },  // red
  { 0, 1, 0 },  // green
  { 0, 0, 1 },  // blue
  { 1, 1, 0 },  // yellow
  { 1, 0, 1 },  // magenta
  { 0, 1, 1 },  // cyan
  { 1, 1, 1 },  // white
  { 160.0 / 256, 82.0 / 256, 45.0 / 256 },  // sienna
  { 46.0 / 256, 139.0 / 256, 87.0 / 256 },  // sea green
};

/* 40MHz:
 * 25ns per clock period
 * 2*25 = 50ns per spi bit
 * So for a zero-bit to have a 350ns pulse-width:
 *    -> do 7x1-bit = 7x50ns = 350ns
 *    -> do 16x0-bit = 16x50ns = 800ns
 *    -> Which means dout = b1111111 0000000000000000 = 0x3FFF000
 *       and total bits to write is 16+7 = 23
 *
 * So for a one-bit to have a 700ns pulse-width:
 *    -> do 14x1-bit = 14x50ns = 700ns
 *    -> do 12x0-bit = 12x50ns = 600ns
 *    -> Which means dout = b 11111111111111 000000000000 = 0x3FFF000
 *       and total bits to write is 14+12 = 26 bits
*/
void ICACHE_RAM_ATTR ws2812_sendByte(uint8_t b)
{
   static uint8_t bit;
   static uint32_t out;
   for ( bit = 0; bit < 8; bit++)
   {
      if (b & 0x80) // is high-order bit set?
      {
         out = 0x3FFF000;
         spi_transaction(SPI_DEV,0, 0, 0, 0, 26,  out, 0, NULL, 0);
      }
      else
      {
         out = 0x7F0000;
         spi_transaction(SPI_DEV, 0, 0, 0, 0, 23, out, 0, NULL, 0);
      }
      b <<= 1; // shift next bit into high-order position
      os_delay_us(1); // seems to be more stable if this is added.
   }
}

void ws2812_sendPixel(uint8_t r, uint8_t g, uint8_t b)
{
  ws2812_sendByte(g);        // NeoPixel wants colors in green-then-red-then-blue order
  ws2812_sendByte(r);
  ws2812_sendByte(b);
}

// Display a single color on the whole string
void ws2812_showColor(uint16_t count, uint8_t r , uint8_t g , uint8_t b)
{
   uint16_t pixel;
   ets_intr_lock(); /* These seem to be OK */
   for (pixel = 0; pixel < count; pixel++)
      ws2812_sendPixel (r, g, b);
   ets_intr_unlock(); /* These seem to be OK */
   ws2812_show();  // latch the colors
}

void ws2812_colorTransition(  uint8_t from_r, uint8_t from_g, uint8_t from_b,
                              uint8_t to_r,   uint8_t to_g,   uint8_t to_b)
{
   // draw one more of the desired colour, each time around the loop
   uint16_t iteration;
   for (iteration = 0; iteration < PIXELS; iteration++)
   {
      // ets_intr_lock(); /* will crash eventually */
      uint16_t pixel;
      for (pixel = 0; pixel < PIXELS; pixel++)
      {
         if (pixel < iteration)
            ws2812_sendPixel (to_r, to_g, to_b);
         else
            ws2812_sendPixel (from_r, from_g, from_b);
      }
      // ets_intr_unlock(); /* will crash eventually */
      ws2812_show();
   }
}

void ws2812_show()
{
   os_delay_us(9);
}

// #define FIRE_BRICK

static void ws2812_timer_cb(void* arg)
{

#ifdef FIRE_BRICK
   ws2812_showColor (PIXELS, 0xB2, 0x22, 0x22);  // firebrick
#else
   static uint8_t fadeInOut_count = 0;

   if (cur_pattern >= (ARRAY_SIZE(patterns)-1))
   {
      cur_pattern = 0;
   }

   if (fade_in_out && (cur_m == 100))
   {
      fade_in_out = false;
   }

   if ( (!fade_in_out) && (cur_m == 0))
   {
      cur_pattern++;
      fade_in_out = true;
   }

   if (fade_in_out)
   {
      ws2812_showColor(PIXELS, patterns [cur_pattern] [0] * cur_m, patterns [cur_pattern] [1] * cur_m, patterns [cur_pattern] [2] * cur_m);
      cur_m++;
   }
   else
   {
      ws2812_showColor(PIXELS, patterns [cur_pattern] [0] * cur_m, patterns [cur_pattern] [1] * cur_m, patterns [cur_pattern] [2] * cur_m);
      cur_m--;
   }

#endif
}


bool ws2812_spi_init()
{
   spi_init_gpio(SPI_DEV, SPI_CLK_USE_DIV);
   spi_clock(SPI_DEV, 2, 2); // prediv==2==40mhz, postdiv==1==40mhz
   spi_tx_byte_order(SPI_DEV, SPI_BYTE_ORDER_HIGH_TO_LOW);
   spi_rx_byte_order(SPI_DEV, SPI_BYTE_ORDER_HIGH_TO_LOW);

   /* Mode 1: CPOL=0, CPHA=1 */
   /* uint8 spi_no, uint8 spi_cpha,uint8 spi_cpol */
   spi_mode(SPI_DEV, 0, 0);

   SET_PERI_REG_MASK(SPI_USER(SPI_DEV), SPI_CS_SETUP|SPI_CS_HOLD);
   CLEAR_PERI_REG_MASK(SPI_USER(SPI_DEV), SPI_FLASH_MODE);

   return true;
}

bool ws2812_init()
{
   if (ws2812_spi_init())
   {
      NODE_DBG("ws2812_spi_init ok\n");
      ws2812_show();                       // in case MOSI went high, latch in whatever-we-sent
      ws2812_sendPixel (0, 0, 0);           // now change back to black
      ws2812_show();
   }

   os_timer_disarm(&ws2812_timer);
   os_timer_setfn(&ws2812_timer, (os_timer_func_t *) ws2812_timer_cb, &ws2812_timer_arg);
   os_timer_arm(&ws2812_timer, 10UL, true);

   // ws2812_seq_test();

   return true;
}





// end
