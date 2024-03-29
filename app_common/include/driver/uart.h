#ifndef UART_APP_H
#define UART_APP_H

#include "uart_register.h"
#include "eagle_soc.h"
#include "c_types.h"
#include "os_type.h"

typedef void (*uart0_data_received_callback_t)(uint8_t *data,int len);

#define UART0   0
#define UART1   1

#define UART_HW_RTS   0    // set 1: enable uart hw flow control RTS, PIN MTDO, FOR UART0
#define UART_HW_CTS   0    // set1: enable uart hw flow contrl CTS , PIN MTCK, FOR UART0

#define RX_BUFF_SIZE    0x100
#define TX_BUFF_SIZE    100
#define UART_FIFO_LEN   128  //define the tx fifo length

#define UART_TX_EMPTY_THRESH_VAL 0x10

typedef enum {
   FIVE_BITS   = 0x0,
   SIX_BITS    = 0x1,
   SEVEN_BITS  = 0x2,
   EIGHT_BITS  = 0x3
} UartBitsNum4Char;

typedef enum {
   ONE_STOP_BIT         = 0,
   ONE_HALF_STOP_BIT    = BIT2,
   TWO_STOP_BIT         = BIT2
} UartStopBitsNum;

typedef enum {
   NONE_BITS   = 0,
   ODD_BITS    = 0,
   EVEN_BITS   = BIT4
} UartParityMode;

typedef enum {
   STICK_PARITY_DIS   = 0,
   STICK_PARITY_EN    = BIT3 | BIT5
} UartExistParity;

typedef enum {
   BIT_RATE_300     = 300,
   BIT_RATE_600     = 600,
   BIT_RATE_1200    = 1200,
   BIT_RATE_2400    = 2400,
   BIT_RATE_4800    = 4800,
   BIT_RATE_9600    = 9600,
   BIT_RATE_19200   = 19200,
   BIT_RATE_38400   = 38400,
   BIT_RATE_57600   = 57600,
   BIT_RATE_74880   = 74880,
   BIT_RATE_115200  = 115200,
   BIT_RATE_230400  = 230400,
   BIT_RATE_256000  = 256000,
   BIT_RATE_460800  = 460800,
   BIT_RATE_921600  = 921600,
   BIT_RATE_1843200 = 1843200,
   BIT_RATE_3686400 = 3686400,
   BIT_RATE_4000000 = 4000000
} UartBautRate;

typedef enum {
   NONE_CTRL,
   HARDWARE_CTRL,
   XON_XOFF_CTRL
} UartFlowCtrl;

typedef enum {
   EMPTY,
   UNDER_WRITE,
   WRITE_OVER
} RcvMsgBuffState;

typedef struct {
   uint32   RcvBuffSize;
   uint8    *pRcvMsgBuff;
   uint8    *pWritePos;
   uint8    *pReadPos;
   uint8    TrigLvl; //JLU: may need to pad

   RcvMsgBuffState   BuffState;
} RcvMsgBuff;

typedef struct {
   uint32   TrxBuffSize;
   uint8    *pTrxBuff;
} TrxMsgBuff;

typedef enum {
   BAUD_RATE_DET,
   WAIT_SYNC_FRM,
   SRCH_MSG_HEAD,
   RCV_MSG_BODY,
   RCV_ESC_CHAR,
} RcvMsgState;

typedef struct {
   UartBautRate         baut_rate;
   UartBitsNum4Char     data_bits;
   UartExistParity      exist_parity;
   UartParityMode       parity;    // chip size in byte
   UartStopBitsNum      stop_bits;
   UartFlowCtrl         flow_ctrl;
   RcvMsgBuff           rcv_buff;
   TrxMsgBuff           trx_buff;
   RcvMsgState          rcv_state;
   int                  received;
   int                  buff_uart_no;  //indicate which uart use tx/rx buffer
} UartDevice;


void uart_config(uint8_t uart_no);
void uart_init(UartBautRate uart0_br, UartBautRate uart1_br);
void uart_write_string(uint8_t uart,const char *s);
void uart_write(uint8_t uart,uint8_t *data,int len);
void uart_write_char(uint8_t uart,char c);

void uart_clear_data_callback();
void uart_register_data_callback(uart0_data_received_callback_t callback);
#endif
