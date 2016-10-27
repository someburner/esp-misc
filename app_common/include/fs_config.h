/*******************************************************************************
 * Filenames + Typedefs
 * Located in app_common as these should be accessible by all roms.
*******************************************************************************/
/* Change this value to load default configurations */
#define CFG_HOLDER	         0x00FF55A4

/* ----------------------------- System Flags ------------------------------- */
/* NOTE: Use SYS_FLAGs for any events/errors/debugs that are at risk of not
 * being pushed to the server due to reset or failure. For example, with OTA we
 * want to be able to know if it succeeded or failed, but we can't guarantee
 * the message will be sent before a reset, so we set a flag and quickly write
 * to FS so we can send the result after reset. Or, if we detect an MQTT error,
 * we will want to send the error after a clean reset, since the error probably
 * would prevent us from sending in the current state
 */
#define SYS_FLAG_FS_INIT_FAIL          1
#define SYS_FLAG_CONFIG_NOT_FOUND      2
#define SYS_FLAG_MQTT_INIT_FAIL        3
#define SYS_FLAG_MQTT_BUFF_OVERFLOW    4
#define SYS_FLAG_UNEXPECTED_RESET      5
#define SYS_FLAG_MAX                   6

/* -------------------------------- ROM Names ---------------------==-------- */
#define APP_MAIN_ROM_NAME      "main"    // Rom name for either slot 2 or 3
#define APP_SETUP_ROM0_NAME    "setup0"  // Rom name for slot 0 ONLY
#define APP_SETUP_ROM1_NAME    "setup1"  // Rom name for slot 1 ONLY
/* ------------------------ Shared Config filenames ------------------------- */
#define ID_CFGNAME             "id.cfg"
#define FLAG_CFGNAME           "flags.cfg"

typedef enum
{
   /* Config Types */
   CFG_INVALID,
   CFG_ID,
   CFG_FLAG,
   CFG_MAX
} CFG_FD_T;
