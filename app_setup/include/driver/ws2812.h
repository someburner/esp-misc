#ifndef _WS2812_h
#define _WS2812_h

#define PIXEL_COUNT 32

#define WS2812_ANIM_NONE       0
#define WS2812_ANIM_FADE_INOUT 1
#define WS2812_ANIM_MAX        2

typedef void (*ws2812_cb_t)(void);

typedef struct
{
   uint8_t cur_m;
   uint8_t cur_pattern;
   uint8_t in_out; // bool
   uint8_t padding;
} ws2812_fade_inout_t;

typedef struct
{
   uint8_t r;
   uint8_t g;
   uint8_t b;
   uint8_t brightness;

   uint16_t cur_anim;
   uint16_t cur_pixel;

   os_timer_t timer;
   uint32_t timer_arg;

} ws2812_driver_t;


void ICACHE_RAM_ATTR ws2812_sendByte(uint8_t b);
// void ws2812_sendPixel(uint8_t r, uint8_t g, uint8_t b);
void ws2812_sendPixels(void);
void ws2812_showColor(uint16_t count, uint8_t r , uint8_t g , uint8_t b);

#if 0
void ws2812_colorTransition(  uint8_t from_r, uint8_t from_g, uint8_t from_b,
                              uint8_t to_r,   uint8_t to_g,   uint8_t to_b);
#endif

void ws2812_doit(void);

void ws2812_fade_init(void);
void ws2812_showit_fade(void);
void ws2812_fade_stop(void);

void ws2812_show(void);
void ws2812_clear(void);

bool ws2812_spi_init(void);
bool ws2812_init(void);




#endif
/* End */
