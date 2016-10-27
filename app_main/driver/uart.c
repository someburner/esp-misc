/*
 * File	: uart.c
 * Copyright (C) 2013 - 2016, Espressif Systems
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_config.h"

#include "driver/uart.h"
#include "driver/uart_register.h"
// #include "driver/rfm69.h"

#include "main/main_api.h"
#include "../app_common/util/linked_list.h"

extern UartDevice UartDev;

static char buf[128];

LOCAL void ICACHE_RAM_ATTR uart0_rx_intr_handler(void *para);

/******************************************************************************
 * FunctionName : uart_config
 * Description  : Internal used function
 *                UART0 used for data TX/RX, RX buffer size is 0x100, interrupt enabled
 *                UART1 just used for debug output
 * Parameters   : uart_no, use UART0 or UART1 defined ahead
 * Returns      : NONE
*******************************************************************************/
LOCAL void uart_config(uint8 uart_no)
{
    if (uart_no == UART1)
    {
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO2_U, FUNC_U1TXD_BK);
    }
    else
    {
        /* rcv_buff size if 0x100 */
        ETS_UART_INTR_ATTACH(uart0_rx_intr_handler,  &(UartDev.rcv_buff));
        PIN_PULLUP_DIS(PERIPHS_IO_MUX_U0TXD_U);
        PIN_FUNC_SELECT(PERIPHS_IO_MUX_U0TXD_U, FUNC_U0TXD);
    }
    uart_div_modify(uart_no, UART_CLK_FREQ / (UartDev.baut_rate)); //SET BAUDRATE

    WRITE_PERI_REG(UART_CONF0(uart_no), ((UartDev.exist_parity & UART_PARITY_EN_M)  <<  UART_PARITY_EN_S) //SET BIT AND PARITY MODE
                                                                        | ((UartDev.parity & UART_PARITY_M)  <<UART_PARITY_S )
                                                                        | ((UartDev.stop_bits & UART_STOP_BIT_NUM) << UART_STOP_BIT_NUM_S)
                                                                        | ((UartDev.data_bits & UART_BIT_NUM) << UART_BIT_NUM_S));

    //clear rx and tx fifo,not ready
    SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);    //RESET FIFO
    CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);

   if (uart_no == UART0)
   {
      //set rx fifo trigger, interrupt timeout and full
      WRITE_PERI_REG(   UART_CONF1(uart_no),
                        ((127 & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S) |
                        (0x05 & UART_RX_TOUT_THRHD) << UART_RX_TOUT_THRHD_S |
                        UART_RX_TOUT_EN
         //| ((0 & UART_TXFIFO_EMPTY_THRHD)<<UART_TXFIFO_EMPTY_THRHD_S)
      );//wjl
      SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA | UART_RXFIFO_TOUT_INT_ENA);
   }
   else
   {
      WRITE_PERI_REG(UART_CONF1(uart_no),((UartDev.rcv_buff.TrigLvl & UART_RXFIFO_FULL_THRHD) << UART_RXFIFO_FULL_THRHD_S));//TrigLvl default val == 1
   }
   //clear all interrupt
   WRITE_PERI_REG(UART_INT_CLR(uart_no), 0xffff);
   //enable rx_interrupt
   SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_OVF_INT_ENA);
}

/******************************************************************************
 * FunctionName : uart1_tx_one_char
 * Description  : Internal used function
 *                Use uart1 interface to transfer one char
 * Parameters   : uint8 TxChar - character to tx
 * Returns      : OK
*******************************************************************************/
 STATUS uart_tx_one_char(uint8 uart, uint8 TxChar)
{
   while (true)
   {
      uint32 fifo_cnt = READ_PERI_REG(UART_STATUS(uart)) & (UART_TXFIFO_CNT<<UART_TXFIFO_CNT_S);
      if ((fifo_cnt >> UART_TXFIFO_CNT_S & UART_TXFIFO_CNT) < 126)
         { break; }
   }
   WRITE_PERI_REG(UART_FIFO(uart) , TxChar);
   return OK;
}

/******************************************************************************
 * FunctionName : uart1_write_char
 * Description  : Internal used function
 *                Do some special deal while tx char is '\r' or '\n'
 * Parameters   : char c - character to tx
 * Returns      : NONE
*******************************************************************************/
LOCAL void uart1_write_char(char c)
{
   if (c == '\n')
   {
      uart_tx_one_char(UART1, '\r');
      uart_tx_one_char(UART1, '\n');
   }
   else if (c == '\r')
   {  }
   else
   {
      uart_tx_one_char(UART1, c);
   }
}

//os_printf output to fifo or to the tx buffer
LOCAL void uart0_write_char_no_wait(char c)
{
   if (c == '\n')
   {
      uart_tx_one_char_no_wait(UART0, '\r');
      uart_tx_one_char_no_wait(UART0, '\n');
   }
   else if (c == '\r')
   {  }
   else
   {
      uart_tx_one_char_no_wait(UART0, c);
   }
}

/******************************************************************************
 * FunctionName : uart0_tx_buffer
 * Description  : use uart0 to transfer buffer
 * Parameters   : uint8 *buf - point to send buffer
 *                uint16 len - buffer len
 * Returns      :
*******************************************************************************/
void uart0_tx_buffer(char *buf, uint16 len)
{
    uint16 i;
    for (i = 0; i < len; i++)
    {
        uart_tx_one_char(UART0, buf[i]);
    }
}

void uart0_send_EOT()
{
   uart_tx_one_char(UART0, 4);
}

/******************************************************************************
 * FunctionName : uart0_sendStr
 * Description  : use uart0 to transfer buffer
 * Parameters   : uint8 *buf - point to send buffer
 *                uint16 len - buffer len
 * Returns      :
*******************************************************************************/
void uart0_sendStr(const char *str)
{
   while (*str)
   {
      uart_tx_one_char(UART0, *str++);
   }
}
void at_port_print(const char *str) __attribute__((alias("uart0_sendStr")));
/******************************************************************************
 * FunctionName : uart0_rx_intr_handler
 * Description  : read a buffer-full from the uart
 *                UART0 interrupt handler, add self handle code inside
 * Parameters   : void *para - point to ETS_UART_INTR_ATTACH's arg
 * Returns      : NONE
*******************************************************************************/
LOCAL void uart_recvTask(os_event_t *events)
{
   static uint8 length = 0;
   // os_memset(buf, 0, 128);
   while (  (READ_PERI_REG(UART_STATUS(UART0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S)) &&
            (length < 128) )
   {
      buf[length++] = READ_PERI_REG(UART_FIFO(UART0)) & 0xFF;
   }

   if (buf[length-1] == '\n')
   {
      MAIN_IO_MSG_T * newMsg = (MAIN_IO_MSG_T *)os_zalloc(sizeof(MAIN_IO_MSG_T));
      newMsg->data = buf;
      newMsg->data = (char*)os_zalloc(length+1);
      memcpy(newMsg->data, buf, length);
      ((char*)newMsg->data)[length] = '\0';
      newMsg->len = length;
      length = 0;
      system_os_post(EVENT_MON_TASK_PRIO, (os_signal_t)MSG_IFACE_UART, (os_param_t)newMsg);
   }


   WRITE_PERI_REG(UART_INT_CLR(UART0), UART_RXFIFO_FULL_INT_CLR|UART_RXFIFO_TOUT_INT_CLR);
   ETS_UART_INTR_ENABLE();
}

LOCAL void ICACHE_RAM_ATTR uart0_rx_intr_handler(void *para)
{
   uint8 uart_no = UART0;//UartDev.buff_uart_no;

   if (UART_RXFIFO_FULL_INT_ST == (READ_PERI_REG(UART_INT_ST(uart_no)) & UART_RXFIFO_FULL_INT_ST) ||
      UART_RXFIFO_TOUT_INT_ST == (READ_PERI_REG(UART_INT_ST(uart_no)) & UART_RXFIFO_TOUT_INT_ST) )
   {
      ETS_UART_INTR_DISABLE();
      uart_recvTask(NULL);
   }
}

/******************************************************************************
 * FunctionName : uart_init
 * Description  : user interface for init uart
 * Parameters   : UartBautRate uart0_br - uart0 bautrate
 *                UartBautRate uart1_br - uart1 bautrate
 * Returns      : NONE
*******************************************************************************/
void uart_init(UartBautRate uart0_br, UartBautRate uart1_br)
{
   UartDev.baut_rate = uart0_br;
   uart_config(UART0);
   UartDev.baut_rate = uart1_br;
   uart_config(UART1);
   ETS_UART_INTR_ENABLE();

   os_install_putc1(uart0_write_char_no_wait);

   /*option 1: use default print, output from uart0 , will wait some time if fifo is full */
   //do nothing...

   /*option 2: output from uart1,uart1 output will not wait , just for output debug info */
   /*os_printf output uart data via uart1(GPIO2)*/
   //os_install_putc1((void *)uart1_write_char);    //use this one to output debug information via uart1 //

   /*option 3: output from uart0 will skip current byte if fifo is full now... */
   /*see uart0_write_char_no_wait:you can output via a buffer or output directly */
   /*os_printf output uart data via uart0 or uart buffer*/
   //os_install_putc1((void *)uart0_write_char_no_wait);  //use this to print via uart0
}

void uart_reattach()
{
   //  uart_init(BIT_RATE_115200, BIT_RATE_115200);
   uart_init(BIT_RATE_921600, BIT_RATE_921600);
}

/******************************************************************************
 * FunctionName : uart_tx_one_char_no_wait
 * Description  : uart tx a single char without waiting for fifo
 * Parameters   : uint8 uart - uart port
 *                uint8 TxChar - char to tx
 * Returns      : STATUS
*******************************************************************************/
STATUS ICACHE_RAM_ATTR uart_tx_one_char_no_wait(uint8 uart, uint8 TxChar)
{
   uint8 fifo_cnt = (( READ_PERI_REG(UART_STATUS(uart))>>UART_TXFIFO_CNT_S)& UART_TXFIFO_CNT);
   if (fifo_cnt < 126)
   {
      WRITE_PERI_REG(UART_FIFO(uart) , TxChar);
   }
   return OK;
}

STATUS ICACHE_RAM_ATTR uart0_tx_one_char_no_wait(uint8 TxChar)
{
   uint8 fifo_cnt = (( READ_PERI_REG(UART_STATUS(UART0))>>UART_TXFIFO_CNT_S)& UART_TXFIFO_CNT);
   if (fifo_cnt < 126)
   {
      WRITE_PERI_REG(UART_FIFO(UART0) , TxChar);
   }
   return OK;
}


/******************************************************************************
 * FunctionName : uart1_sendStr_no_wait
 * Description  : uart tx a string without waiting for every char, used for print debug info which can be lost
 * Parameters   : const char *str - string to be sent
 * Returns      : NONE
*******************************************************************************/
void uart1_sendStr_no_wait(const char *str)
{
   while (*str)
   {
      uart_tx_one_char_no_wait(UART1, *str++);
   }
}


// Interrupt Stuff
void ICACHE_RAM_ATTR uart_rx_intr_disable(uint8 uart_no)
{
#if 1
   CLEAR_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
#else
   ETS_UART_INTR_DISABLE();
#endif
}

void ICACHE_RAM_ATTR uart_rx_intr_enable(uint8 uart_no)
{
#if 1
   SET_PERI_REG_MASK(UART_INT_ENA(uart_no), UART_RXFIFO_FULL_INT_ENA|UART_RXFIFO_TOUT_INT_ENA);
#else
   ETS_UART_INTR_ENABLE();
#endif
}


//========================================================
LOCAL void uart0_write_char(char c)
{
   if (c == '\n')
   {
      uart_tx_one_char(UART0, '\r');
      uart_tx_one_char(UART0, '\n');
   }
   else if (c == '\r')
   {    }
   else
   {
      uart_tx_one_char(UART0, c);
   }
}

#if 0
void ICACHE_RAM_ATTR uart_write(uint8_t uart,uint8_t *data,int len)
{
	linked_list *list = &tx_list[uart];

	uint8_t fifo_cnt = UART_TX_FIFO_COUNT(uart);
   if (fifo_cnt > len && list->size==0)
   {
      //we can write on uart buffer
      while(len>0)
      {
         UART_WRITE_CHAR(uart,*data);
         data++;
         len--;
      }
      return;
   }

    //if fifo is full, buffer on our own
	while(len>0)
   {
   	struct tx_item *tx = (struct tx_item *)list_get_first(list);

   	if(tx==NULL || tx->len >= TX_ITEM_LEN)
      {
   		tx = (struct tx_item*)os_zalloc(sizeof(struct tx_item));
   		list_add_first(list,tx);
   	}

   	int remLen = TX_ITEM_LEN-tx->len;
   	if(len <= remLen)
      {
   		os_memcpy(tx->data+tx->len,data,len);
   		tx->len+=len;
   		len=0;
   	} else {
   		os_memcpy(tx->data+tx->len,data,remLen);
   		tx->len+=remLen;
   		len-=remLen;
   		data+=remLen;
   	}
	}

	os_timer_disarm(&tx_timer[uart]);
	os_timer_arm(&tx_timer[uart], 20, 0);
}
#endif

void ICACHE_RAM_ATTR uart_write_string(uint8_t uart, const char *s)
{
	// int str_len = os_strlen(s);
	// uart_write(uart, (uint8_t*)s,str_len);
   uart0_sendStr(s);
}


void UART_SetWordLength(uint8 uart_no, UartBitsNum4Char len)
{
   SET_PERI_REG_BITS(UART_CONF0(uart_no),UART_BIT_NUM,len,UART_BIT_NUM_S);
}

void UART_SetStopBits(uint8 uart_no, UartStopBitsNum bit_num)
{
   SET_PERI_REG_BITS(UART_CONF0(uart_no),UART_STOP_BIT_NUM,bit_num,UART_STOP_BIT_NUM_S);
}

void UART_SetLineInverse(uint8 uart_no, UART_LineLevelInverse inverse_mask)
{
   CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_LINE_INV_MASK);
   SET_PERI_REG_MASK(UART_CONF0(uart_no), inverse_mask);
}

void UART_SetParity(uint8 uart_no, UartParityMode Parity_mode)
{
   CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_PARITY |UART_PARITY_EN);

   if (Parity_mode!=NONE_BITS)
      SET_PERI_REG_MASK(UART_CONF0(uart_no), Parity_mode|UART_PARITY_EN);
}

void UART_SetBaudrate(uint8 uart_no,uint32 baud_rate)
{
   uart_div_modify(uart_no, UART_CLK_FREQ /baud_rate);
}

void UART_SetFlowCtrl(uint8 uart_no,UART_HwFlowCtrl flow_ctrl,uint8 rx_thresh)
{
   if (flow_ctrl&USART_HardwareFlowControl_RTS)
   {
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_U0RTS);
      SET_PERI_REG_BITS(UART_CONF1(uart_no),UART_RX_FLOW_THRHD,rx_thresh,UART_RX_FLOW_THRHD_S);
      SET_PERI_REG_MASK(UART_CONF1(uart_no), UART_RX_FLOW_EN);
   }
   else
   {
      CLEAR_PERI_REG_MASK(UART_CONF1(uart_no), UART_RX_FLOW_EN);
   }

   if (flow_ctrl&USART_HardwareFlowControl_CTS)
   {
      PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U, FUNC_UART0_CTS);
      SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_TX_FLOW_EN);
   }
   else
   {
      CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_TX_FLOW_EN);
   }
}

void UART_WaitTxFifoEmpty(uint8 uart_no , uint32 time_out_us) //do not use if tx flow control enabled
{
   uint32 t_s = system_get_time();
   while (READ_PERI_REG(UART_STATUS(uart_no)) & (UART_TXFIFO_CNT << UART_TXFIFO_CNT_S))
   {
      if(( system_get_time() - t_s )> time_out_us)
         break;
   WRITE_PERI_REG(0X60000914, 0X73);//WTD
   }
}

/*
bool UART_CheckOutputFinished(uint8 uart_no, uint32 time_out_us)
{
    uint32 t_start = system_get_time();
    uint8 tx_fifo_len;
    uint32 tx_buff_len;
    while(1){
        tx_fifo_len =( (READ_PERI_REG(UART_STATUS(uart_no))>>UART_TXFIFO_CNT_S)&UART_TXFIFO_CNT);
        if(pTxBuffer){
            tx_buff_len = ((pTxBuffer->UartBuffSize)-(pTxBuffer->Space));
        }else{
            tx_buff_len = 0;
        }

        if( tx_fifo_len==0 && tx_buff_len==0){
            return TRUE;
        }
        if( system_get_time() - t_start > time_out_us){
            return FALSE;
        }
	WRITE_PERI_REG(0X60000914, 0X73);//WTD
    }
}*/


void UART_ResetFifo(uint8 uart_no)
{
   SET_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
   CLEAR_PERI_REG_MASK(UART_CONF0(uart_no), UART_RXFIFO_RST | UART_TXFIFO_RST);
}

void UART_ClearIntrStatus(uint8 uart_no,uint32 clr_mask)
{
   WRITE_PERI_REG(UART_INT_CLR(uart_no), clr_mask);
}

void UART_SetIntrEna(uint8 uart_no,uint32 ena_mask)
{
   SET_PERI_REG_MASK(UART_INT_ENA(uart_no), ena_mask);
}


void UART_SetPrintPort(uint8 uart_no)
{
   if(uart_no==1)
   {
      os_install_putc1(uart1_write_char);
   }
   else
   {
      /*option 1: do not wait if uart fifo is full,drop current character*/
      os_install_putc1(uart0_write_char_no_wait);
      /*option 2: wait for a while if uart fifo is full*/
      os_install_putc1(uart0_write_char);
   }
}
