#ifndef __ONEWIRE_NB_h
#define __ONEWIRE_NB_h

#include "user_config.h"
#if ONEWIRE_NONBLOCKING

#define OW_SPAD_SIZE 9
#define OW_UUID_LEN 8

void OW_print_temperature(void);
void OW_publish_temperature(void);
void OW_handle_error(uint8_t cb_type);

void OW_init_seq(uint8_t seq);
void onewire_nb_init(void);
void OW_doit(void);

typedef enum {
   OW_SEQ_STATE_INVALID = 0,
   OW_SEQ_STATE_INIT,
   OW_SEQ_STATE_STARTED,
   OW_SEQ_STATE_TRANSITION,
   OW_SEQ_STATE_DONE,
   OW_SEQ_STATE_MAX
} OW_SEQ_STATE_T;

typedef enum {
   OW_OP_INVALID = 0,
   OW_OP_RESET,
   OW_OP_READ,
   OW_OP_WRITE,
   OW_OP_END,
   OW_OP_MAX
} OW_OP_T;

typedef enum {
   OW_READ_OP_INVALID = 0,
   OW_READ_OP_INIT,
   OW_READ_OP_LINE_LOW,
   OW_READ_OP_SAMPLE,
   OW_READ_OP_WAIT_50,
   OW_READ_OP_DONE
} OW_READ_OP_T;

typedef enum {
   OW_WRITE_OP_INVALID = 0,
   OW_WRITE_OP_INIT,
   OW_WRITE_OP_LINE_LOW,
   OW_WRITE_OP_WRITE_FIRST,
   OW_WRITE_OP_WAIT_FIRST,
   OW_WRITE_OP_FINALLY,
   OW_WRITE_OP_DONE
} OW_WRITE_OP_T;

typedef enum {
   OW_RESET_OP_INVALID = 0,
   OW_RESET_OP_INIT,
   OW_RESET_OP_WAIT_480_LOW,
   OW_RESET_OP_WAIT_60_RESP,
   OW_RESET_OP_WAIT_480,
   OW_RESET_OP_DONE
} OW_RESET_OP_T;

typedef enum {
   OW_ERROR_NONE = 0,
   OW_ERROR_INVALID_SEQ,
   OW_ERROR_INVALID_OP,
   OW_ERROR_RESET_RESP_TIMEOUT
} OW_ERROR_T;


typedef void (*OW_cb_t)(void);

typedef struct
{
   uint8_t * seq_arr;   // read temp, read rom, etc
   uint8_t * arg_arr;   // read temp, read rom, etc

   uint8_t   seq_len;

   uint8_t seq_state;   // init, started, stopped
   uint8_t seq_pos;     // reset, skip rom, conv, etc

   uint8_t op_type;     // read, write, reset, etc (OW_OP_T)
   uint8_t cur_op;      // where are we in the operation
   uint8_t delay_count; // 1 count per 10us

   uint8_t spad_buf[OW_SPAD_SIZE]; // scratchpad buffer for temperature data
   uint8_t UUID[OW_UUID_LEN];     // unique ID for this device

   uint8_t error;
   OW_cb_t callback;

   /* Useful for debugging */
   // uint16_t readbyte_count;
   // uint16_t readbit_count;

} onewire_driver_t;


#endif /* End ONEWIRE_NONBLOCKING */

#endif
// end
