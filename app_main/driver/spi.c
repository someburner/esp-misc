#include "driver/spi.h"

void spi_init(uint8 spi_no)
{
	if(spi_no > 1)
      return;

   spi_init_gpio(spi_no, SPI_CLK_USE_DIV);
   spi_clock(spi_no, SPI_CLK_PREDIV, SPI_CLK_CNTDIV);
   spi_tx_byte_order(spi_no, SPI_BYTE_ORDER_HIGH_TO_LOW);
   spi_rx_byte_order(spi_no, SPI_BYTE_ORDER_HIGH_TO_LOW);

   SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CS_SETUP|SPI_CS_HOLD);
   CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_FLASH_MODE);
}

void spi_init_gpio(uint8 spi_no, uint8 sysclk_as_spiclk)
{
   uint32 clock_div_flag = 0;
   if (sysclk_as_spiclk)
   {
      clock_div_flag = 0x0001;
   }

   if (spi_no==SPI)
   {

      WRITE_PERI_REG(PERIPHS_IO_MUX, 0x005|(clock_div_flag<<8)); // Set bit 8 if 80MHz sysclock required
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CLK_U, 1);
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_CMD_U, 1);
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA0_U, 1);
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA1_U, 1);
   }
   else if (spi_no==HSPI)
   {
      WRITE_PERI_REG(PERIPHS_IO_MUX, 0x105|(clock_div_flag<<9)); //Set bit 9 if 80MHz sysclock required
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U, 2); //GPIO12 is HSPI MISO pin (Master Data In)
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, 2); //GPIO13 is HSPI MOSI pin (Master Data Out)
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, 2); //GPIO14 is HSPI CLK pin (Clock)
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, 2); //GPIO15 is HSPI CS pin (Chip Select / Slave Select)
   }
}

void spi_clock(uint8 spi_no, uint16 prediv, uint8 cntdiv)
{
   if(spi_no > 1)
      return;

   if ( (prediv==0) | (cntdiv==0) )
   {
      WRITE_PERI_REG(SPI_CLOCK(spi_no), SPI_CLK_EQU_SYSCLK);
   }
   else
   {
      WRITE_PERI_REG(SPI_CLOCK(spi_no),
         ( ((prediv-1)&SPI_CLKDIV_PRE)<<SPI_CLKDIV_PRE_S ) |
         ( ((cntdiv-1)&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S )     |
         ( ( (cntdiv>>1)&SPI_CLKCNT_H )<<SPI_CLKCNT_H_S )  |
         ( ( 0&SPI_CLKCNT_L )<<SPI_CLKCNT_L_S )
      );
   }
}

void spi_tx_byte_order(uint8 spi_no, uint8 byte_order)
{
   if (spi_no > 1)
      return;

   if (byte_order)
   {
      SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_WR_BYTE_ORDER);
   }
   else
   {
      CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_WR_BYTE_ORDER);
   }
}

void spi_rx_byte_order(uint8 spi_no, uint8 byte_order)
{
   if (spi_no > 1)
      return;

   if (byte_order)
   {
      SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_RD_BYTE_ORDER);
   }
   else
   {
      CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_RD_BYTE_ORDER);
   }
}

// Function Name: spi_mode
 //   Description: Configures SPI mode parameters for clock edge and clock polarity.
 //    Parameters: spi_no - SPI (0) or HSPI (1)
 //				  spi_cpha - (0) Data is valid on clock leading edge
 //				             (1) Data is valid on clock trailing edge
 //				  spi_cpol - (0) Clock is low when inactive
 //				             (1) Clock is high when inactive
 //
 ////////////////////////////////////////////////////////////////////////////////
/* Mode 1: CPOL=0, CPHA=1 */
void spi_mode(uint8 spi_no, uint8 spi_cpha,uint8 spi_cpol)
{
   if (spi_cpha)
   {
      CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_CK_OUT_EDGE);
   }
   else
   {
      SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_CK_OUT_EDGE);
   }

   if (spi_cpol)
   {
      SET_PERI_REG_MASK(SPI_PIN(spi_no), SPI_IDLE_EDGE);
   }
   else
   {
      CLEAR_PERI_REG_MASK(SPI_PIN(spi_no), SPI_IDLE_EDGE);
   }
}


bool ICACHE_RAM_ATTR spi_transaction(  uint8 spi_no,
                                       uint8 cmd_bits, uint16 cmd_data,
                                       uint32 addr_bits, uint32 addr_data,
                                       uint32 dout_bits, uint32 dout_data,
                                       uint32 din_bits, uint32 * din_data,
                                       uint32 dummy_bits)
{
   if (spi_no > 1)
      return false;

   /* code for custom Chip Select as GPIO PIN here */
   while ( spi_busy(spi_no) ); //wait for SPI to be ready

   /* Enable SPI Functions */
   //disable MOSI, MISO, ADDR, COMMAND, DUMMY in case previously set.
   CLEAR_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI|SPI_USR_MISO|SPI_USR_COMMAND|SPI_USR_ADDR|SPI_USR_DUMMY);

   // Enable functions based on number of bits. 0 bits = disabled.
   // This is rather inefficient but allows for a very generic function.
   // CMD ADDR and MOSI are set below to save on an extra if statement.
   // if (cmd_bits) {SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_COMMAND);}
   // if (addr_bits) {SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_ADDR);}
   if (din_bits)
      { SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MISO); }

   if (dummy_bits)
      { SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_DUMMY); }

   /* Setup Bitlengths */
   WRITE_PERI_REG(SPI_USER1(spi_no),
      ( (addr_bits-1) & SPI_USR_ADDR_BITLEN ) << SPI_USR_ADDR_BITLEN_S | //Number of bits in Address
      ( (dout_bits-1) & SPI_USR_MOSI_BITLEN ) << SPI_USR_MOSI_BITLEN_S | //Number of bits to Send
      ( (din_bits-1)  & SPI_USR_MISO_BITLEN ) << SPI_USR_MISO_BITLEN_S |  //Number of bits to receive
      ( (dummy_bits-1) & SPI_USR_DUMMY_CYCLELEN ) << SPI_USR_DUMMY_CYCLELEN_S  //Number of Dummy bits to insert
   );

   /* Setup Command Data */
   if (cmd_bits)
   {
      /* enable COMMAND function in SPI module */
      SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_COMMAND);
      /* align command data to high bits */
      uint16 command = cmd_data << (16-cmd_bits);
      /* swap byte order */
      command = ( (command>>8) & 0xff ) | ( (command<<8) & 0xff00 );

      WRITE_PERI_REG(SPI_USER2(spi_no), (
         ( ((cmd_bits-1) & SPI_USR_COMMAND_BITLEN)<<SPI_USR_COMMAND_BITLEN_S ) |
         command & SPI_USR_COMMAND_VALUE )
      );
   }

   /* Setup Address Data */
   if (addr_bits)
   {
      /* Enable Address function in SPI module */
      SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_ADDR);

      /* Align address data to high bits */
      WRITE_PERI_REG( SPI_ADDR(spi_no), addr_data<<(32-addr_bits) );
   }

   /* Setup DOUT data */
   if (dout_bits)
   {
      /* enable MOSI function in SPI module */
      SET_PERI_REG_MASK(SPI_USER(spi_no), SPI_USR_MOSI);

      /* copy data to W0 */
      if ( READ_PERI_REG(SPI_USER(spi_no)) & SPI_WR_BYTE_ORDER )
      {
         WRITE_PERI_REG(SPI_W0(spi_no), dout_data<<(32-dout_bits));
      }
      else
      {
         uint8 dout_extra_bits = dout_bits%8;

        /* If your data isn't a byte multiple (8/16/24/32 bits)and you don't  *
         * have SPI_WR_BYTE_ORDER set, you need this to move the non-8bit     *
         * remainder to the MSBs. Not sure if there's even a use case for     *
         * this, but it's here if you need it.                                *
         * For example, 0xDA4 12 bits without SPI_WR_BYTE_ORDER would usually *
         * be output as if it were 0x0DA4, of which 0xA4, and then 0x0 would  *
         * be shifted out (first 8 bits of low byte, then 4 MSB bits of high  *
         * byte - ie reverse byte order). The code below shifts it out as     *
         * 0xA4 followed by 0xD as you might require.                         */
         if (dout_extra_bits)
         {
            WRITE_PERI_REG(SPI_W0(spi_no), (
               (0xFFFFFFFF<<(dout_bits - dout_extra_bits)&dout_data)<<(8-dout_extra_bits) |
               (0xFFFFFFFF>>(32-(dout_bits - dout_extra_bits)))&dout_data)
            );
         }
         else
         {
            WRITE_PERI_REG(SPI_W0(spi_no), dout_data);
         }
      }
   }

   /* Begin SPI Transaction */
   SET_PERI_REG_MASK(SPI_CMD(spi_no), SPI_USR);

   /* Return DIN data */
   if (din_bits)
   {
      char idx = 0;
      /* Wait for SPI transaction to complete */
      while ( spi_busy(spi_no) );

      do
      {
         /* Select reg in reverse order */
         if ( READ_PERI_REG(SPI_USER(spi_no) ) & SPI_RD_BYTE_ORDER )
         {
            din_data[ idx ] = READ_PERI_REG(SPI_W0(spi_no)) >> (32-(din_bits%32));
         }
         /* Select reg in normal order */
         else
         {
            din_data[ idx ]  = READ_PERI_REG(SPI_W0(spi_no) + (idx << 2));
         }
      }
      while ( ++idx < (din_bits / 32) );
   }

   /* Transaction completed. Return success. */
   return true;
}





/* End File */
