#ifndef SPI_APP_H
#define SPI_APP_H

#include "spi_register.h"
#include "ets_sys.h"
#include "osapi.h"
#include "uart.h"
#include "os_type.h"

/*SPI number define*/
#define SPI 			0
#define HSPI			1

#define SPI_CLK_USE_DIV 0
#define SPI_CLK_80MHZ_NODIV 1

#define SPI_BYTE_ORDER_HIGH_TO_LOW 1
#define SPI_BYTE_ORDER_LOW_TO_HIGH 0

/* Should already be defined in eagle_soc.h */
#ifndef CPU_CLK_FREQ
#define CPU_CLK_FREQ 80*1000000
#endif

/* Define some default SPI clock settings */
#define SPI_CLK_PREDIV 10
#define SPI_CLK_CNTDIV 2
#define SPI_CLK_FREQ CPU_CLK_FREQ/(SPI_CLK_PREDIV*SPI_CLK_CNTDIV) // 80 / 20 = 4 MHz

/*******************************************************************************
 * spi_init:
 * Description: Wrapper to setup HSPI/SPI GPIO pins and default SPI clock
 * Parameters: spi_no - SPI (0) or HSPI (1)
*******************************************************************************/
void spi_init(uint8 spi_no);

/*******************************************************************************
 * spi_init_gpio:
 * Description: Initialises the GPIO pins for use as SPI pins.
 * Parameters:
 *    - spi_no: SPI (0) or HSPI (1)
 *    - sysclk_as_spiclk:
 *       - SPI_CLK_80MHZ_NODIV (1) if using 80MHz sysclock for SPI clock.
 *       - SPI_CLK_USE_DIV (0) if using divider to get lower SPI clock speed.
*******************************************************************************/
void spi_init_gpio(uint8 spi_no, uint8 sysclk_as_spiclk);

/*******************************************************************************
 * spi_clock:
 * Description: sets up the control registers for the SPI clock
 * Parameters:
 *    - spi_no: SPI (0) or HSPI (1)
 *    - prediv: predivider value (actual division value)
 *    - cntdiv: postdivider value (actual division value).
 * Set either divider to 0 to disable all division (80MHz sysclock)
*******************************************************************************/
void spi_clock(uint8 spi_no, uint16 prediv, uint8 cntdiv);

/*******************************************************************************
 * spi_tx_byte_order:
 * Description: Setup the byte order for shifting data out of buffer
 * Parameters:
 *    - spi_no: SPI (0) or HSPI (1)
 *    - byte_order:
 *       - SPI_BYTE_ORDER_HIGH_TO_LOW (1): Data is sent out starting with Bit31
 *         and down to Bit0
 *       - SPI_BYTE_ORDER_LOW_TO_HIGH (0): Data is sent out starting with the
 *         lowest BYTE, from MSB to LSB, followed by the second lowest BYTE,
 *         from MSB to LSB, followed by the second highest BYTE, from MSB to
 *         LSB, followed by the highest BYTE, from MSB to LSB 0xABCDEFGH would
 *         be sent as 0xGHEFCDAB
*******************************************************************************/
void spi_tx_byte_order(uint8 spi_no, uint8 byte_order);

/*******************************************************************************
 * spi_rx_byte_order:
 * Description: Setup the byte order for shifting data into buffer
 * Parameters:
 *    - spi_no: SPI (0) or HSPI (1)
 *    - byte_order:
 *       - SPI_BYTE_ORDER_HIGH_TO_LOW (1): Data is read in starting with Bit31
 *         and down to Bit0
 *       - SPI_BYTE_ORDER_LOW_TO_HIGH (0): Data is sent out starting with the
 *         lowest BYTE, from MSB to LSB, followed by the second lowest BYTE,
 *         from MSB to LSB, followed by the second highest BYTE, from MSB to
 *         LSB, followed by the highest BYTE, from MSB to LSB 0xABCDEFGH would
 *         be sent as 0xGHEFCDAB
*******************************************************************************/
void spi_rx_byte_order(uint8 spi_no, uint8 byte_order);

void spi_mode(uint8 spi_no, uint8 spi_cpha,uint8 spi_cpol);

/*******************************************************************************
 * spi_transaction:
 * Description: SPI transaction function
 * Parameters:
 *    - spi_no: SPI (0) or HSPI (1)
 *    - cmd_bits: actual number of command bits to transmit
 *    - cmd_data: command data to transmit
 *    - addr_bits: actual number of address bits to transmit
 *    - addr_data: address data to transmit
 *    - dout_bits: actual number of data bits to transfer out to slave
 *    - dout_data: data to send to slave
 *    - din_bits: actual number of data bits to transfer in from slave
 *    - din_data: data received from slave
 *    - dummy_bits: actual number of dummy bits to clock in to slave after
 *                  command and address have been clocked in
 * Returns:
 *    - read data: uint32 containing read in data only if RX was set
 *    - 0: something went wrong (or actual read data was 0)
 *    - 1: data sent ok (or actual read data is 1)
 *
 * Note: all data is assumed to be stored in the lower bits of the data
 * variables (for anything <32 bits).
*******************************************************************************/
// uint32 ICACHE_RAM_ATTR spi_transaction(uint8 spi_no, uint32 addr_bits, uint32 addr_data, uint32 dout_bits, uint32 dout_data, uint32 din_bits, uint32 dummy_bits);
// uint32 ICACHE_RAM_ATTR spi_transaction(uint8 spi_no, uint8 cmd_bits, uint16 cmd_data, uint32 addr_bits, uint32 addr_data, uint32 dout_bits, uint32 dout_data,uint32 din_bits, uint32 dummy_bits);
bool ICACHE_RAM_ATTR spi_transaction(uint8 spi_no,
   uint8 cmd_bits, uint16 cmd_data, uint32 addr_bits, uint32 addr_data,
   uint32 dout_bits, uint32 dout_data, uint32 din_bits, uint32 * din_data,
   uint32 dummy_bits
);

//Expansion Macros
#define spi_busy(spi_no) READ_PERI_REG(SPI_CMD(spi_no))&SPI_USR

#define spi_txd(spi_no, bits, data) spi_transaction(spi_no, 0, 0, bits, (uint32) data, 0, 0)
#define spi_tx8(spi_no, data)       spi_transaction(spi_no, 0, 0, 8,    (uint32) data, 0, 0)
#define spi_tx16(spi_no, data)      spi_transaction(spi_no, 0, 0, 16,   (uint32) data, 0, 0)
#define spi_tx32(spi_no, data)      spi_transaction(spi_no, 0, 0, 32,   (uint32) data, 0, 0)

#define spi_rxd(spi_no, bits) spi_transaction(spi_no, 0, 0, 0, 0, bits, 0)
#define spi_rx8(spi_no)       spi_transaction(spi_no, 0, 0, 0, 0, 8,    0)
#define spi_rx16(spi_no)      spi_transaction(spi_no, 0, 0, 0, 0, 16,   0)
#define spi_rx32(spi_no)      spi_transaction(spi_no, 0, 0, 0, 0, 32,   0)

#endif
