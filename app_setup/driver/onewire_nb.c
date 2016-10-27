/* onewire_nb.c                                                               *
 * Description: Non-blocking Onewire driver. Uses hw_timer to generate event  *
 * loop so that we don't wait for long times for onewire read/write/reset.    *
 * See onewire.md for more info on implementation.                            */
#include "user_interface.h"
#include "c_types.h"
#include "mem.h"
#include "gpio.h"
#include "osapi.h"
#include "ets_sys.h"
#include "os_type.h"
#include "user_config.h"

#if ONEWIRE_NONBLOCKING

#include "../app_common/libc/c_stdio.h"
#include "../app_common/util/crc.h"

#include "setup/setup_api.h"

#include "driver/hw_timer.h"
#include "driver/gpio16.h"
#include "driver/onewire_nb.h"
#include "driver/maxim28.h"

extern uint8_t pin_num[GPIO_PIN_NUM];

/* Timer declarations */
static os_timer_t onewire_timer;
/* use this to tell above timer which seq it was armed from */
static int onewire_timer_arg = DS_SEQ_INVALID;

/* Onewire driver declaration */
static onewire_driver_t onewire_driver;
static onewire_driver_t * one_driver = &onewire_driver;

/* Temperature data */
static Temperature latest_temp;

/* Onewire Reset state machine */
static void OW_doit_reset(void)
{
   switch (one_driver->cur_op)
   {
      case OW_RESET_OP_INIT:
      {
         one_driver->delay_count = 47;
         one_driver->cur_op = OW_RESET_OP_WAIT_480_LOW;

         // GPIO_DIS_OUTPUT(ONEWIRE_PIN); //line release
         GPIO_OUTPUT_SET(ONEWIRE_PIN, 0);
      } break;

      case OW_RESET_OP_WAIT_480_LOW:
      {
         if ( !(--one_driver->delay_count) )
         {
            GPIO_DIS_OUTPUT(ONEWIRE_PIN); // release
            one_driver->cur_op = OW_RESET_OP_WAIT_60_RESP; // give 60us for resp
            one_driver->delay_count = 6;
         }
      } break;

      case OW_RESET_OP_WAIT_60_RESP:
      {
         --one_driver->delay_count;

         /* Response Detected */
         if ( !GPIO_INPUT_GET(ONEWIRE_PIN) )
         {
            one_driver->delay_count = 47;
            one_driver->cur_op = OW_RESET_OP_WAIT_480;
            break;
         }

         /* If we get here we timed out */
         if  ( !( one_driver->delay_count) )
         {
            GPIO_DIS_OUTPUT(ONEWIRE_PIN);
            one_driver->error = OW_ERROR_RESET_RESP_TIMEOUT;
            one_driver->seq_state = OW_SEQ_STATE_DONE;
         }
      } break;

      case OW_RESET_OP_WAIT_480:
      {
         if  ( !( --one_driver->delay_count ) )
         {
            one_driver->cur_op = OW_RESET_OP_DONE;
         }
      } break;

      case OW_RESET_OP_DONE:
      {
         one_driver->seq_pos++;
         one_driver->seq_state = OW_SEQ_STATE_TRANSITION;
      } break;

      default: // invalid?
         one_driver->error = OW_ERROR_INVALID_OP;
         break;
   }
}

/* Onewire read state machine */
static void OW_doit_read(void)
{
   static uint8_t bitMask = 0x01;
   static int sample = 0;
   static uint8_t readsLeft = 0;
   static uint8_t readCount = 0;
   static uint8_t data = 0;

again:
{
   uint8_t cur_op = one_driver->cur_op;
   switch (cur_op)
   {
      case OW_READ_OP_INIT:
      {
         bitMask = 0x01;
         sample = 0;
         readCount = 0;
         data = 0;
         readsLeft = one_driver->arg_arr[one_driver->seq_pos];
         one_driver->cur_op = OW_READ_OP_LINE_LOW;
         goto again;
      } break;

      case OW_READ_OP_LINE_LOW:
      {
         GPIO_OUTPUT_SET(ONEWIRE_PIN, 0); // linelow
         os_delay_us(1);
         GPIO_DIS_OUTPUT(ONEWIRE_PIN); // line release
         one_driver->cur_op = OW_READ_OP_SAMPLE;
         // one_driver->readbit_count++; // uncomment for debugging
      } break;

      case OW_READ_OP_SAMPLE:
      {
         sample = GPIO_INPUT_GET(ONEWIRE_PIN); // sample
         if (sample)
            data |= bitMask;

         bitMask <<= 1;

         one_driver->delay_count = 4;
         one_driver->cur_op = OW_READ_OP_WAIT_50;
      } break;

      case OW_READ_OP_WAIT_50:
      {
         /* Done waiting. Perform checks. */
         if  ( !(--one_driver->delay_count))
         {
            /* We're done with *a* read sequence. */
            if (!bitMask)
            {
               /* Put data into buffer */
               one_driver->spad_buf[ readCount++ ] = data;

               /* Reset data and bitMask. Important! */
               data = 0;
               bitMask = 0x01;
               // one_driver->readbyte_count++; // uncomment for debugging

               /* No more read sequences left. We're completely done. */
               if ( !(--readsLeft) )
               {
                  one_driver->cur_op = OW_READ_OP_DONE;
                  break;
               }
            }

            /* If we get here we have more reads to do. Restart at line low. */
            one_driver->cur_op = OW_READ_OP_LINE_LOW;
            goto again;
         }
      } break;
   }
} /* End again block */


  /* Last OP. This means we're done with this set of ops.              *
   * Increment seq_pos. Main switch will determine if this means we're *
   * also done with the last op-set in the sequence. to finished so we */
   if (one_driver->cur_op == OW_READ_OP_DONE)
   {
      one_driver->seq_state = OW_SEQ_STATE_TRANSITION;
      one_driver->seq_pos++;
   }
}

/* Onewire Write state machine */
static void OW_doit_write(void)
{
   static uint8_t bitMask = 0x01;
   static uint8_t data = 0;

again:
{
   uint8_t cur_op = one_driver->cur_op;
   switch (cur_op)
   {
      case OW_WRITE_OP_INIT:
      {
         data = one_driver->arg_arr[one_driver->seq_pos];
         bitMask = 0x01;
         one_driver->cur_op = OW_WRITE_OP_LINE_LOW;
      } break;

      case OW_WRITE_OP_LINE_LOW:
      {
         if (bitMask & data)
         {
            one_driver->cur_op = OW_WRITE_OP_WRITE_FIRST; // "1" type
            one_driver->delay_count = 6;
         }
         else
         {
            one_driver->cur_op = OW_WRITE_OP_WAIT_FIRST; // "0" type
            one_driver->delay_count = 6;
         }

         GPIO_OUTPUT_SET(ONEWIRE_PIN, 0); // linelow
      } break;

      /* "1" type, write now. */
      case OW_WRITE_OP_WRITE_FIRST:
      {
         if  ( one_driver->delay_count == 6 )
         {
            GPIO_OUTPUT_SET(ONEWIRE_PIN, 1);
         }

         if  ( !(--one_driver->delay_count) )
         {
            one_driver->cur_op = OW_WRITE_OP_FINALLY;
         }
      } break;

      /* "0" type, wait to write. */
      case OW_WRITE_OP_WAIT_FIRST:
      {
         /* Done waiting (or no wait). Write bit. */
         if  ( !(--one_driver->delay_count) )
         {
            GPIO_OUTPUT_SET(ONEWIRE_PIN, 1);
            one_driver->cur_op = OW_WRITE_OP_FINALLY;
         }
      } break;

      case OW_WRITE_OP_FINALLY:
      {
         bitMask = bitMask << 1;

         /* We're done. Move on. */
         if (! (bitMask) )
         {
            if (USE_P_PWR)
            {
               GPIO_DIS_OUTPUT(ONEWIRE_PIN);
               GPIO_OUTPUT_SET(ONEWIRE_PIN, 0);
            }
            one_driver->cur_op = OW_WRITE_OP_DONE;
         }
         /* More bits to read. Go back to line low. */
         else
         {
            one_driver->cur_op = OW_WRITE_OP_LINE_LOW;
            goto again;
         }
      } break;
   }
} /* End again block */

   /*  Last OP. This means we're done with this set of ops.                   */
   if (one_driver->cur_op == OW_WRITE_OP_DONE)
   {
      one_driver->seq_state = OW_SEQ_STATE_TRANSITION;
      one_driver->seq_pos++;
   }
}

/* -------------------------------------------------------------------------- *
 * NOTE: Putting debugs in this method, or methods called herein, WILL result *
 *       in improper readings, or no readings at all. Use a results callback  *
 *       if you want to see what happened during the process.                 *
 * Sequence:                                                                  *
 *    -> Encompasses the entire set of reset,write,read (ops). 1 at a time.   *
 * Operation:                                                                 *
 *    -> An Op is defined for any part of a sequence that requires a delay    *
 *       that is >= 10us between. Eg.:                                        *
 *       -> A delay of 10us is an op. Another delay of 10us after a GPIO op   *
 *          is another *separate* op.                                         *
 *       -> A one-time delay of 500us is a single op that gets repeated.      *
 *       -> A GPIO read followed by a 5us delay and another write is 1 op.    *
 * -------------------------------------------------------------------------- */
void OW_doit(void)
{
   int type = one_driver->op_type;
   int state = one_driver->seq_state;
   switch (type)
   {
      case OW_OP_READ:
      {
         if (state == OW_SEQ_STATE_TRANSITION)
         {
            one_driver->seq_state = OW_SEQ_STATE_STARTED;
            one_driver->cur_op = OW_READ_OP_INIT;
         }
         /* Run the read state machine */
         OW_doit_read();
      } break;

      case OW_OP_WRITE:
      {
         if (state == OW_SEQ_STATE_TRANSITION)
         {
            one_driver->seq_state = OW_SEQ_STATE_STARTED;
            one_driver->cur_op = OW_WRITE_OP_INIT;
         }
         /* Run the write state machine */
         OW_doit_write();
      } break;

      case OW_OP_RESET:
      {
         if (state == OW_SEQ_STATE_TRANSITION)
         {
            one_driver->seq_state = OW_SEQ_STATE_STARTED;
            one_driver->cur_op = OW_RESET_OP_INIT;
         }
         /* Run the reset state machine */
         OW_doit_reset();
      } break;

      /* Last OP. This means we're done. Set seq state to finished so we   *
       * know to disable the hw_timer and call the callback.               */
      case OW_OP_END:
         one_driver->seq_state = OW_SEQ_STATE_DONE;
         break;

      /* Shouldn't occur. Treat as done anyways */
      case OW_OP_MAX:
         one_driver->seq_state = OW_SEQ_STATE_DONE;
         break;

      /* Invalid. Terminate */
      default:
         one_driver->seq_state = OW_SEQ_STATE_DONE;
         break;
   }

   state = one_driver->seq_state;
   switch (state)
   {
      case OW_SEQ_STATE_INIT:
      {
         /* Update seq_state and arm HW timer */
         one_driver->seq_state = OW_SEQ_STATE_STARTED;
         hw_timer_arm(10);
      } break;

      case OW_SEQ_STATE_STARTED:
      {
         /* Just arm HW timer */
         hw_timer_arm(10);
      } break;

      case OW_SEQ_STATE_TRANSITION:
      {
         /* If seq_pos is equal to, or has advanced beyond the total # of     *
          * sequences (seq_len) we're done. End it.                           */
         if (one_driver->seq_pos >= one_driver->seq_len)
         {
            one_driver->op_type = OW_OP_END;
            one_driver->seq_state = OW_SEQ_STATE_DONE;
         }
         /* Reset cur_op to 0 to trigger the seq init on the next doit call.  */
         else
         {
            one_driver->op_type = one_driver->seq_arr[one_driver->seq_pos];
            one_driver->cur_op = 0;
            hw_timer_arm(10);
         }
      } break;

      case OW_SEQ_STATE_DONE:
      {
         if (one_driver->callback)
         {
            one_driver->callback();
         }
         /* Disarm HW timer gets taken care of in callbacks. Leaving here as  *
         * a reference in case it's desired to disarm the hw_timer inside the *
         * state machine. Could be desired if using the hw timer configured   *
         * with autoload for reset or some other longer delay.                */
         // hw_timer_disarm();
      } break;
   }
}

void OW_handle_error(uint8_t cb_type)
{
   NODE_ERR("OW errno %hd in seq_cb #%hd\n", one_driver->error, cb_type);
   one_driver->error = OW_ERROR_NONE;
   hw_timer_disarm();
}

void OW_print_temperature(void)
{
   if (latest_temp.available)
      NODE_DBG("temp = %c%d.%02d deg.C\n", latest_temp.sign, latest_temp.val, latest_temp.fract);
   else
      NODE_DBG("Temp. data N/A\n");
}

void OW_publish_temperature(void)
{
   static char pubBuf[64];
   int len = os_sprintf(pubBuf, "%c%d.%02d", latest_temp.sign, latest_temp.val, latest_temp.fract);
   setup_api_test(pubBuf, len);
}

/* Each time the hw timer fires, advance state machine */
static void ICACHE_RAM_ATTR ow_hw_timer_cb(uint32_t arg)
{
   // (void) arg;
   system_os_post(EVENT_MON_TASK_PRIO, (os_signal_t)MSG_IFACE_ONE_WIRE, (os_param_t)one_driver);
}

/* This callback is to be setup/armed from inside one of the xxx_done_cb      *
 * callbacks. The xxx_done_cb callbacks are called at the end of a sequence,  *
 * but we generally want to wait a bit inbetween sequences. This is mainly to *
 * be used for waiting between the conv_t sequence, which tells the sensor to *
 * start the conversion process, and the read_t sequence, where we actually   *
 * get the temperature from the scratchpad                                    */
static void onewire_seq_timer_cb(void * arg)
{
   /* de-reference */
   int seq_type = *(int*)arg;

   switch(seq_type)
   {
      /* UUID is done and we have waited a bit. Start conversion. */
      case DS_SEQ_UUID_T:
         OW_init_seq(DS_SEQ_CONV_T);
         break;

      /* Conversion is done and we have waited a bit. Start read. */
      case DS_SEQ_CONV_T:
         OW_init_seq(DS_SEQ_READ_T);
         break;

      /* Read is done. Do nothing. */
      case DS_SEQ_READ_T:
         OW_DBG("all done\n");
         break;

      case DS_SEQ_NEXT_READ_T:
         OW_init_seq(DS_SEQ_CONV_T);
         hw_timer_arm(10);
         break;

      default:
         OW_DBG("invalid seq_timer_cb arg\n");
         break;
   }
}

/* This is called immediately at the end of a read_t sequence */
static void read_temp_done_cb(void)
{
   hw_timer_disarm();
   uint16 reading;

   if (one_driver->error)
   {
      OW_handle_error(DS_SEQ_READ_T);
      return;
   }

   #if 0
   uint8_t i;
   for (i=0; i<9; i++)
      OW_DBG("r:%02x\n", one_driver->spad_buf[i]);
   #endif

   const uint8_t * checkCrc = one_driver->spad_buf;

   /* Verify CRC */
   uint8_t crc = onewire_crc8(checkCrc, 8);
   if (crc == one_driver->spad_buf[8])
   {
      /* CRC OK, populate latest_temp */
      latest_temp.sign = '+';
      reading = (one_driver->spad_buf[1] << 8) | one_driver->spad_buf[0];
      if (reading & 0x8000)
      {
         reading = (reading ^ 0xffff) + 1;				// 2's complement
         latest_temp.sign = '-';
      }

      latest_temp.val = reading >> 4;  // separate off the whole and fractional portions
      latest_temp.fract = (reading & 0xf) * 100 / 16;
      latest_temp.available = 1; // mark as available

   #ifdef OW_DEBUG
      OW_print_temperature();
   #endif
      OW_publish_temperature();
   }
   else
   {
      NODE_ERR("err: crc %02x(exp) != 0x%02x!\n", one_driver->spad_buf[8], crc);
   }

   memset(one_driver->spad_buf, 0, OW_SPAD_SIZE);

   os_timer_disarm(&onewire_timer);
   onewire_timer_arg = DS_SEQ_NEXT_READ_T;
   os_timer_arm(&onewire_timer, 2431UL, false);
   // hw_timer_arm(10);
   // OW_init_seq(DS_SEQ_UUID_T);
}

/* This is called immediately at the end of a conv_t sequence */
static void conv_t_done_cb(void)
{
   if (one_driver->error)
   {
      OW_handle_error(DS_SEQ_CONV_T);
      return;
   }
   /* Start read in 750ms */
   onewire_timer_arg = DS_SEQ_CONV_T;
   os_timer_arm(&onewire_timer, 750, false);
}

/* This is called immediately at the end of a read uuid sequence */
static void read_uuid_done_cb(void)
{
   if (one_driver->error)
   {
      OW_handle_error(DS_SEQ_UUID_T);
      return;
   }

   one_driver->UUID[0] = DS_FAMILY_CODE;
   memcpy(one_driver->UUID+1, one_driver->spad_buf, 7);

#ifdef OW_DEBUG
   OW_DBG("UUID: %02x", one_driver->UUID[0]);

   uint8_t i;
   for (i=1; i<8; i++)
   {
      OW_DBG(":%02x", one_driver->UUID[i]);
   }
   OW_DBG("\n");
#endif

   /* Start conversion soon */
   os_timer_disarm(&onewire_timer);
   onewire_timer_arg = DS_SEQ_UUID_T;
   os_timer_arm(&onewire_timer, 15, false);
}


void OW_init_seq(uint8_t seq)
{
   /* Assign seq array and args */
   one_driver->seq_arr = ds_seqs[seq];
   one_driver->arg_arr = ds_seq_args[seq];
   one_driver->seq_len = ds_seq_lens[seq];

   /* Set to init state */
   one_driver->seq_state = OW_SEQ_STATE_INIT;
   one_driver->error = OW_ERROR_NONE;
   onewire_timer_arg = (int)seq;

   switch (seq)
   {
      case DS_SEQ_UUID_T: one_driver->callback = read_uuid_done_cb; break;
      case DS_SEQ_CONV_T: one_driver->callback = conv_t_done_cb; break;
      case DS_SEQ_READ_T: one_driver->callback = read_temp_done_cb; break;
      default:
         NODE_ERR("inv init seq\n");
         onewire_timer_arg = 0;
         one_driver->callback = NULL;
         return;
   }

   /* Send init post */
   system_os_post(EVENT_MON_TASK_PRIO, (os_signal_t)MSG_IFACE_ONE_WIRE, (os_param_t)one_driver);
}

void onewire_nb_init()
{
#ifdef EN_TEMP_SENSOR
   /* Init all to 0 */
   memset(one_driver, 0, sizeof(onewire_driver));
   memset(&latest_temp, 0, sizeof(latest_temp));

   os_timer_disarm(&onewire_timer);
   os_timer_setfn(&onewire_timer, (os_timer_func_t *) onewire_seq_timer_cb, &onewire_timer_arg);

   /* Set pin 1 to input */
   if (set_gpio_mode(1, GPIO_INPUT, GPIO_PULLUP) ) // GPIO_PULLUP,GPIO_PULLDOWN.GPIO_FLOAT
   {
      NODE_DBG("GPIO%d set mode\r\n", pin_num[1]);
   }

   /* Init hw timer */
   hw_timer_init(FRC1_SOURCE, 0); // disable autoload
   hw_timer_set_func(ow_hw_timer_cb, NULL);

   OW_init_seq(DS_SEQ_UUID_T);
#else
   NODE_ERR("Temperature sensor not enabled!\n");
#endif
}

#endif
// end
