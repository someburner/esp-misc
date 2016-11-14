#
# config.mk
#
# User-defined config definitions. Everything here is fair game.
#

#############################################################
# SDK Path Setup
#############################################################
# Base directory for the esp-open-sdk, absolute
OPENSDK_ROOT ?= /home/jeffrey/esp-open-sdk-1.5.4.1

# xtena bin tools dir
XTENSA_TOOLS_ROOT ?= $(OPENSDK_ROOT)/xtensa-lx106-elf/bin/
# Root SDK dir (used for includes)
SDK_ROOT ?= $(OPENSDK_ROOT)/sdk/

# COM Port for esptool.py
ESPPORT = /dev/node_mcu

#############################################################
# Shortcuts to do everything
#############################################################
# Make everything?
MAKE_EVERYTHING     ?= no
# MAKE_EVERYTHING     ?= yes

# Flash Everything?
FLASH_EVERYTHING    ?= no
# FLASH_EVERYTHING    ?= yes

# Compile as un-configured?
INIT_UNCONFIGURED   ?= no
# INIT_UNCONFIGURED   ?= yes

# Hard-code Device IDs?
# HARDCODE_DEVICE_IDS ?= no
HARDCODE_DEVICE_IDS ?= yes

ifeq ("$(MAKE_EVERYTHING)","yes")
GEN_SPIFFY_ROM      ?= yes
MK_SPIFFY           ?= yes
MK_RCONF            ?= yes
endif

ifeq ("$(FLASH_EVERYTHING)","yes")
FLASH_CAL_SECT      ?= yes
FLASH_INIT_DATA     ?= yes
FLASH_BLANK         ?= yes
FLASH_RBOOT         ?= yes
FLASH_RCONF         ?= yes
FLASH_ROM0          ?= yes
FLASH_ROM1          ?= yes
FLASH_ROM2          ?= yes
FLASH_ROM3          ?= yes
FLASH_SPIFFY0       ?= yes
endif

# Start out in setup rom if unconfigured = true
ifeq ("$(INIT_UNCONFIGURED)", "yes")
ROM_TO_BOOT         ?= 0
endif

# Set IDs to 0 if not hardcoding to trigger provision requests
ifeq ("$(HARDCODE_DEVICE_IDS)","no")
BRIDGE_ID_DEFAULT       ?= 0
WATER_SENSOR_ID_DEFAULT ?= 0
endif

#############################################################
# Flash Options (SDK)
#############################################################
# Calibration data sector
# Must agree with user_rf_cal_sector_set() in user_main.c
CAL_SECT_ADDR      ?= 0x3FB000

# Flash Cal sector?
# FLASH_CAL_SECT     ?= no
FLASH_CAL_SECT     ?= yes

#Flash esp_init_data_default?
# FLASH_INIT_DATA    ?= no
FLASH_INIT_DATA    ?= yes

#Flash blank.bin?
# FLASH_BLANK        ?= no
FLASH_BLANK        ?= yes

#############################################################
# Flash Options (User)
#############################################################
# Choose which rom to boot if flashing rconf
ROM_TO_BOOT        ?= 0

# Choose which roms to flash
FLASH_RBOOT        ?= yes
FLASH_RCONF        ?= yes
FLASH_ROM0         ?= yes
FLASH_ROM1         ?= no
FLASH_ROM2         ?= yes
FLASH_ROM3         ?= no

# Choose to flash a blank to sector 1.
# Causes rBoot to recreate conf w defaults.
FLASH_RCONF_BLANK  ?= no

# Specify names of each rom
RBOOT_NAME         ?= rboot.bin
RCONF_NAME         ?= rconf.bin
ROM0_NAME          ?= misc.app.setup0.bin
ROM1_NAME          ?= misc.app.setup1.bin
ROM2_NAME          ?= misc.app.main.bin
ROM3_NAME          ?= misc.app.main.bin

# Specify rom flash locations
RBOOT_ADDR         ?= 0x000000
RCONF_ADDR         ?= 0x001000
ROM0_ADDR          ?= 0x002000
ROM1_ADDR          ?= 0x082000
ROM2_ADDR          ?= 0x102000
ROM3_ADDR          ?= 0x202000

# Choose to flash generated spiffy roms
# FS0
FLASH_SPIFFY0      ?= yes
# FLASH_SPIFFY0      ?= no

# FS1
FLASH_SPIFFY1      ?= no

#############################################################
# SPIFFS Addresses
#############################################################
# Location of FS0 start
# This is also the location esptool will flash with the
# created spiffy ROM if flash spiffy is set
SPIFFS0_ADDR       ?= 0x302000

# Size of FS0 aka fs0. Must be same as spiffy_rom.bin if flashing SPIFFS0_ADDR
SPIFFS0_SIZE       ?= 0x10000

# Location of FS1 start (aka "dynamic" fs)
# This is meant to be used for temporary storage of files
# Do not flash over this area of memory
SPIFFS1_ADDR       ?= 0x312000

# Size of FS1 aka dynfs. Must be set correctly.
SPIFFS1_SIZE       ?= 0x50000

# Set to 1 to format FS0/FS1 on boot
FORMAT_FS          ?= 0
FORMAT_DYNFS       ?= 0

#############################################################
# Spiffy options
# Usage: make spiffy_img.o
#############################################################
# Choose to re-compile SPIFFY (or not)
# Must be compiled at least once to generate a rom
# MK_SPIFFY          ?= yes
MK_SPIFFY          ?= no

# Create a spiffy rom?
# GEN_SPIFFY_ROM     ?= yes
GEN_SPIFFY_ROM     ?= no

# Specify names of each spiffs image
SPIFFY0_NAME       ?= spiffy_rom.bin

#############################################################
# WiFi Options
#############################################################
# Build time Wifi Cfg
STA_PASS_EN        ?= 1

WHITELIST_MAC      ?= 0
STA_SSID           ?= \"Your SSID\"
STA_PASS           ?= yourpassword

#############################################################
# MQTT Options
#############################################################
# MQTT Protocol Version
# 311 = v3.1.1 - Compatible with paho-mqtt
MQTT_PROTOCOL_VER  ?= 311

#############################################################
# Makefile assignemnt cheatsheet
#############################################################
# VARIABLE = value (Lazy Set)
# Normal setting of a variable - values are recursively
# expanded when the variable is used, not when it's declared
#
# VARIABLE := value (Immediate Set)
# Setting of a variable with simple expansion of the values.
# Values within are expanded at declaration time
#
# VARIABLE ?= value (Set If Absent)
# Setting of a variable only if it doesn't have a value
#
# VARIABLE += value (Append)
# Appending the supplied value to the existing value
#




# END #
