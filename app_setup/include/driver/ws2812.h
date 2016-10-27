#ifndef _WS2812_h
#define _WS2812_h


void ICACHE_RAM_ATTR ws2812_sendByte(uint8_t b);
void ws2812_sendPixel(uint8_t r, uint8_t g, uint8_t b);
void ws2812_showColor(uint16_t count, uint8_t r , uint8_t g , uint8_t b);
void ws2812_colorTransition(  uint8_t from_r, uint8_t from_g, uint8_t from_b,
                              uint8_t to_r,   uint8_t to_g,   uint8_t to_b);


void ws2812_show();
bool ws2812_spi_init();
bool ws2812_init();




#endif
/* End */
