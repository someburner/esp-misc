#ifndef __HW_TIMER_h
#define __HW_TIMER_h

#define FRC1_ENABLE_TIMER  BIT7
#define FRC1_AUTO_LOAD  BIT6

//TIMER PREDIVED MODE
typedef enum {
    DIVIDED_BY_1 = 0,	//timer clock
    DIVIDED_BY_16 = 4,	//divided by 16
    DIVIDED_BY_256 = 8,	//divided by 256
} TIMER_PREDIVED_MODE;

typedef enum {			//timer interrupt mode
    TM_LEVEL_INT = 1,	// level interrupt
    TM_EDGE_INT   = 0,	//edge interrupt
} TIMER_INT_MODE;

typedef enum {
    FRC1_SOURCE = 0, // FRC1 source should be used to prevent the timer from interrupting other ISRs.
    NMI_SOURCE = 1,
} FRC1_TIMER_SOURCE_TYPE;

typedef enum {
   HW_TIMER_DISABLED = 0,
   HW_TIMER_INIT = 1,
   HW_TIMER_READY = 2,
   HW_TIMER_ACTIVE = 3,
   HW_TIMER_STOPPED = 4
} HW_TIMER_STATE_T;

typedef struct
{
   uint8_t        state;

} HW_TIMER_T;


void hw_timer_disarm();
void hw_timer_set_func(void (* user_hw_timer_cb_set)(uint32_t), uint32_t arg);
bool hw_timer_arm(u32 val);
HW_TIMER_STATE_T hw_timer_get_state(void);


void hw_timer_init(FRC1_TIMER_SOURCE_TYPE source_type, u8 req);

#endif
