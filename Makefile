#  copyright (c) 2010 Espressif System
#
.NOTPARALLEL:
ifndef PDIR

endif

TOP_DIR:=$(dir $(lastword $(MAKEFILE_LIST)))

HOST=$(shell hostname)

OPENSDK_ROOT ?= /your/path/to/esp-open-sdk-1.5.4.1

# Base directory for the compiler
XTENSA_TOOLS_ROOT ?= $(OPENSDK_ROOT)/xtensa-lx106-elf/bin/
SDK_ROOT ?= $(OPENSDK_ROOT)/sdk/
PROJ_ROOT = $(abspath $(dir $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))))/

LDFLAGS:= -L$(TOP_DIR)sdk-overrides/lib -L$(SDK_DIR)/lib -L$(SDK_DIR)/ld $(LDFLAGS)

#############################################################
# MQTT Options
#############################################################

# MQTT Protocol Version
# 311 = v3.1.1 - Compatible with paho-mqtt
MQTT_PROTOCOL_VER  ?= 311

#############################################################
# SPIFFS Options
#############################################################
# Location of FS0 start
# This is also the location esptool will flash with the
# created spiffy ROM if flash spiffy is set
SPIFFS0_ADDR       ?= 0x302000

# Size of Spiffy / FS0. Must be same as spiffy_rom.bin if flashing SPIFFS0_ADDR
SPIFFS0_SIZE       ?= 0x10000

# Location of FS1 start (aka "dynamic" fs)
# This is meant to be used for temporary storage of files
# Do not flash over this area of memory
SPIFFS1_ADDR       ?= 0x312000

# Size of DYNFS / FS1. Must be set correctly.
SPIFFS1_SIZE       ?= 0x50000

#############################################################
# Shortcuts to do everything
#############################################################
# Make everything?
MAKE_EVERYTHING  ?= no
# MAKE_EVERYTHING ?= yes

# Flash Everything?
FLASH_EVERYTHING ?= no
# FLASH_EVERYTHING ?= yes

# Compile as un-configured?
INIT_UNCONFIGURED  ?= no
# INIT_UNCONFIGURED  ?= yes

ifeq ("$(MAKE_EVERYTHING)","yes")
GEN_SPIFFY_ROM     ?= yes
MK_SPIFFY          ?= yes
MK_RCONF           ?= yes
endif

ifeq ("$(FLASH_EVERYTHING)","yes")
FLASH_CAL_SECT     ?= yes
FLASH_INIT_DEFAULT ?= yes
FLASH_BLANK        ?= yes
FLASH_RBOOT        ?= yes
FLASH_RCONF        ?= yes
FLASH_ROM0         ?= yes
FLASH_ROM1         ?= yes
FLASH_ROM2         ?= yes
FLASH_ROM3         ?= yes
FLASH_SPIFFY0      ?= yes
endif

# Start out in setup rom if unconfigured = true
ifeq ("$(INIT_UNCONFIGURED)", "yes")
   ROM_TO_BOOT ?= 0
endif
#############################################################
# Flash Options (SDK)
#############################################################
# Flash Cal sector?
# FLASH_CAL_SECT  ?= no
FLASH_CAL_SECT ?= yes
CAL_SECT_ADDR  ?= 0x3FB000

#Flash esp_init_data_default?
# FLASH_INIT_DEFAULT  ?= no
FLASH_INIT_DEFAULT ?= yes

#Flash blank.bin?
FLASH_BLANK    ?= no
# FLASH_BLANK    ?= yes

#############################################################
# Flash Options (Programs)
#############################################################
# Choose which rom to boot if flashing rconf
ROM_TO_BOOT        ?= 0

# Choose which roms to flash
FLASH_RBOOT        ?= yes
FLASH_RCONF        ?= yes
FLASH_ROM0         ?= yes
FLASH_ROM1         ?= no
FLASH_ROM2         ?= no
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

#############################################################
# Flash Options (SPIFFS)
#############################################################
# Choose which spiffs to flash
FLASH_SPIFFY0      ?= yes
# FLASH_SPIFFY0      ?= no
FLASH_SPIFFY1      ?= no

# Specify names of each spiffs image
SPIFFY0_NAME       ?= spiffy_rom.bin

# Format FS0/FS1 on boot?
FORMAT_FS          ?= 0
FORMAT_DYNFS       ?= 0

#############################################################
# WiFi Options
#############################################################
# Build time Wifi Cfg
STA_PASS_EN          ?= 1

STA_SSID             ?= \"Your SSID\"
STA_PASS             ?= your_password

#############################################################
# Spiffy
# Usage: make spiffy_img.o
#############################################################
# Create a spiffy image?
# GEN_SPIFFY_ROM ?= yes
GEN_SPIFFY_ROM ?= no

# MK_SPIFFY ?= yes
MK_SPIFFY ?= no

VERBOSE = 1

RBOOT_SRC_DIR = $(PROJ_ROOT)rboot
ESPTOOL2_SRC_DIR = $(PROJ_ROOT)tools/esptool2_src/src

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
QECHO := @true
VECHO := @echo
vecho := @true
MAKEPDIR :=
else
Q := @
QECHO := @echo
VECHO := @true
vecho := @echo
MAKEPDIR := --no-print-directory -s
endif

#############################################################
# Esptool flash options:
# Configured for esp-12e (4MByte Windbond)
#############################################################
ESPBAUD   ?= 460800 # 115200, 230400, 460800
ET_FM     ?= qio  # qio, dio, qout, dout
ET_FS     ?= 32m  # 32Mbit flash size in esptool flash command
ET_FF     ?= 80m  # 80Mhz flash speed in esptool flash command
# ET_BLANK1 ?= 0x1FE000 # where to flash blank.bin to erase wireless settings
# ESP_INIT1 ?= 0x1FC000 # flash init data provided by espressif
ET_BLANK ?= 0x3FE000 # where to flash blank.bin to erase wireless settings --- system param area
ESP_INIT ?= 0x3FC000 # flash init data provided by espressif

#relative path of esptool script
ESPTOOL              ?= $(PROJ_ROOT)tools/esptool.py
ESPTOOL2             ?= ../tools/esptool2

#absolute path of sdk bin folder
ESPTOOLMAKE          := $(XTENSA_TOOLS_ROOT)

#rBoot Options
RBOOT_E2_SECTS       ?= .text .data .rodata
RBOOT_E2_USER_ARGS   ?= -bin -boot2 -iromchksum -4096 -$(ET_FM) -80

#rBoot Flags
CFLAGS               += -DRBOOT_INTEGRATION

#############################################################
# Version Info
#############################################################
DATE    := $(shell date '+%F %T')
BRANCH  := $(shell if git diff --quiet HEAD; then git describe --tags; \
                   else git symbolic-ref --short HEAD; fi)
SHA     := $(shell if git diff --quiet HEAD; then git rev-parse --short HEAD | cut -d"/" -f 3; \
                   else echo "development"; fi)
VERSION ?=esp-misc $(BRANCH) - $(DATE) - $(SHA)

#############################################################
# Select compile (Windows build removed)
#############################################################

# Can we use -fdata-sections?
ifndef COMPORT
   ESPPORT = /dev/ftdi_esp
else
   ESPPORT = $(COMPORT)
endif
CCFLAGS += -Os -ffunction-sections -fno-jump-tables -fdata-sections
AR       := $(XTENSA_TOOLS_ROOT)xtensa-lx106-elf-ar
CC       := $(XTENSA_TOOLS_ROOT)xtensa-lx106-elf-gcc -I$(PROJ_ROOT)sdk-overrides/include -I$(SDK_ROOT)include
NM       := $(XTENSA_TOOLS_ROOT)xtensa-lx106-elf-nm
CPP      := $(XTENSA_TOOLS_ROOT)xtensa-lx106-elf-cpp
OBJCOPY  := $(XTENSA_TOOLS_ROOT)xtensa-lx106-elf-objcopy
BINDIR    = ../bin
 UNAME_S := $(shell uname -s)
 ifeq ($(UNAME_S),Linux)
# LINUX
 endif
 UNAME_P := $(shell uname -p)
 ifeq ($(UNAME_P),x86_64)
# ->AMD64
 endif
 ifneq ($(filter %86,$(UNAME_P)),)
# ->IA32
 endif
 ifneq ($(filter arm%,$(UNAME_P)),)
# ->ARM
 endif
#############################################################

#############################################################
# Compiler flags
#############################################################
CSRCS    ?= $(wildcard *.c)
ASRCs    ?= $(wildcard *.s)
ASRCS    ?= $(wildcard *.S)
SUBDIRS  ?= $(filter-out %build,$(patsubst %/,%,$(dir $(wildcard */Makefile))))

ODIR     := .output
OBJODIR  := $(ODIR)/$(TARGET)/$(FLAVOR)/obj

OBJS := $(CSRCS:%.c=$(OBJODIR)/%.o) \
        $(ASRCs:%.s=$(OBJODIR)/%.o) \
        $(ASRCS:%.S=$(OBJODIR)/%.o)

DEPS := $(CSRCS:%.c=$(OBJODIR)/%.d) \
        $(ASRCs:%.s=$(OBJODIR)/%.d) \
        $(ASRCS:%.S=$(OBJODIR)/%.d)

LIBODIR := $(ODIR)/$(TARGET)/$(FLAVOR)/lib
OLIBS := $(GEN_LIBS:%=$(LIBODIR)/%)

IMAGEODIR := $(ODIR)/$(TARGET)/$(FLAVOR)/image
OIMAGES := $(GEN_IMAGES:%=$(IMAGEODIR)/%)

BINODIR := $(PROJ_ROOT)bin
OBINS := $(GEN_BINS:%=$(BINODIR)/%)

#
# Note:
# https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
# If you add global optimize options like "-O2" here
# they will override "-Os" defined above.
# "-Os" should be used to reduce code size
#
CCFLAGS += \
   -g \
   -Wpointer-arith \
   -Wundef \
   -Werror \
   -Wl,-EL \
   -fno-inline-functions \
   -nostdlib \
   -mlongcalls \
   -mtext-section-literals
#   -Wall

CFLAGS = $(CCFLAGS) $(DEFINES) $(EXTRA_CCFLAGS) $(STD_CFLAGS) $(INCLUDES)
DFLAGS = $(CCFLAGS) $(DDEFINES) $(EXTRA_CCFLAGS) $(STD_CFLAGS) $(INCLUDES)


#############################################################
# Generate Flash Command
#############################################################
FLASH_BINS ?=

################################### Program ####################################
ifeq ("$(FLASH_RBOOT)","yes")
FLASH_BINS += $(RBOOT_ADDR) $(BINDIR)/$(RBOOT_NAME)
endif
ifeq ("$(FLASH_RCONF)","yes")
FLASH_BINS += $(RCONF_ADDR) $(BINDIR)/$(RCONF_NAME)
endif
ifeq ("$(FLASH_RCONF_BLANK)","yes")
FLASH_BINS += $(RCONF_ADDR) $(BINDIR)/blank.bin
endif
ifeq ("$(FLASH_ROM0)","yes")
FLASH_BINS += $(ROM0_ADDR) $(BINDIR)/$(ROM0_NAME)
endif
ifeq ("$(FLASH_ROM1)","yes")
FLASH_BINS += $(ROM1_ADDR) $(BINDIR)/$(ROM1_NAME)
endif
ifeq ("$(FLASH_ROM2)","yes")
FLASH_BINS += $(ROM2_ADDR) $(BINDIR)/$(ROM2_NAME)
endif
ifeq ("$(FLASH_ROM3)","yes")
FLASH_BINS += $(ROM3_ADDR) $(BINDIR)/$(ROM3_NAME)
endif
################################### SPIFFS #####################################
ifeq ("$(FLASH_SPIFFY0)","yes")
FLASH_BINS += $(SPIFFS0_ADDR) $(BINDIR)/$(SPIFFY0_NAME)
endif
##################################### SDK ######################################
ifeq ("$(FLASH_CAL_SECT)","yes")
FLASH_BINS += $(CAL_SECT_ADDR) $(BINDIR)/blank.bin
endif
ifeq ("$(FLASH_INIT_DEFAULT)","yes")
FLASH_BINS += $(ESP_INIT) $(BINDIR)/esp_init_data_default.bin
endif
ifeq ("$(FLASH_BLANK)","yes")
FLASH_BINS += $(ET_BLANK) $(BINDIR)/blank.bin
endif


#############################################################
# Functions
#############################################################
# Recipe Reset:
# .RECIPEPREFIX allows you to reset the recipe introduction character from the
# default (TAB) to something else. The first character of this variable value is
# the new recipe introduction character. If the variable is set to the empty
# string, TAB is used again. It can be set and reset at will; recipes will use
# the value active when they were first parsed. To detect this feature check the
# value of $(.RECIPEPREFIX).
.RECIPEPREFIX +=
define ShortcutRule
$(1): .subdirs $(2)/$(1)
endef

define MakeLibrary
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %.a,$$(COMPONENTS_$(1))),$$(dir $$(lib))$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(dir $$(obj))$$(OBJODIR)/$$(notdir $$(obj)))
$$(LIBODIR)/$(1).a: $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
   $(Q) mkdir -p $$(LIBODIR)
   $(Q) $$(if $$(filter %.a,$$?),mkdir -p $$(EXTRACT_DIR)_$(1))
   $(Q) $$(if $$(filter %.a,$$?),cd $$(EXTRACT_DIR)_$(1); $$(foreach lib,$$(filter %.a,$$?),$$(AR) xo $$(UP_EXTRACT_DIR)/$$(lib);))
   $(QECHO) AR $$@
   $(Q) $$(AR) ru $$@ $$(filter %.o,$$?) $$(if $$(filter %.a,$$?),$$(EXTRACT_DIR)_$(1)/*.o)
   $(Q) $$(if $$(filter %.a,$$?),$$(RM) -r $$(EXTRACT_DIR)_$(1))
endef

define MakeImage
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %.a,$$(COMPONENTS_$(1))),$$(dir $$(lib))$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(dir $$(obj))$$(OBJODIR)/$$(notdir $$(obj)))
$$(IMAGEODIR)/$(1).out: $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
   $(Q) mkdir -p $$(IMAGEODIR)
   $(QECHO) CC $$@
   $(Q) $$(CC) $$(LDFLAGS) $$(if $$(LINKFLAGS_$(1)),$$(LINKFLAGS_$(1)),$$(LINKFLAGS_DEFAULT) $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1))) -o $$@
endef

# esptool.py elf creation
# $(BINODIR)/%.bin: $(IMAGEODIR)/%.out
#    @mkdir -p $(BINODIR)
#    $(ESPTOOL) elf2image $< -o $(BINDIR) --path $(ESPTOOLMAKE) -fs $(ET_FS) -ff $(ET_FF)
# @echo "Build Complete!"

# esptool2 elf creation
$(BINODIR)/%.bin: $(IMAGEODIR)/%.out
   $(Q) mkdir -p $(BINODIR)
   $(QECHO) E2 $@
   $(Q) $(ESPTOOL2) $(RBOOT_E2_USER_ARGS) $< $@ $(RBOOT_E2_SECTS)

#############################################################
# Rules base
# Should be done in top-level makefile only
#############################################################
.PHONY: all .subdirs spiffy spiffy_img.o rboot rconf

ifndef PDIR
# targets for top level only

# Define SPIFFY_EXPORTS - Makefile settings that should be pased to spiffy when
# creating spiffy image.
ifeq ("$(MK_SPIFFY)","yes")
# Define SPIFFY_EXPORTS global
SPIFFY_EXPORTS=-g
# Add to make command
OSPIFFY = spiffy

# Pass WiFi and RFM69 parameters to spiffy
SPIFFY_EXPORTS = -DSTA_SSID="$(STA_SSID)" -DSTA_PASS_EN=$(STA_PASS_EN) \
   -DSTA_PASS="$(STA_PASS)"

ifeq ("$(INIT_UNCONFIGURED)", "yes")
   SPIFFY_EXPORTS += -DINIT_UNCONFIGURED=1
else
   SPIFFY_EXPORTS += -DINIT_UNCONFIGURED=0
endif
export SPIFFY_EXPORTS
endif
ifeq ("$(GEN_SPIFFY_ROM)","yes")
OSPIFFY += spiffy_img.o
endif
ifeq ("$(MK_RCONF)","yes")
ORCONF = rconf
endif

# rBoot setup
RBOOT=bin/rboot.bin
COMMON=liblibc.a libjsmn.a libjsontree.a liblwip.a libmqtt.a libspiffs.a libutil.a \
   libnewcrypto.a libplatform.a librboot.a
LIBMAIN_SRC = $(SDK_ROOT)lib/libmain.a
LIBMAIN_DST = $(PROJ_ROOT)sdk-overrides/lib/libmain2.a
endif

all: $(COMMON) .subdirs $(ORCONF) $(OSPIFFY) $(LIBMAIN_DST) $(RBOOT) $(OBJS) $(OLIBS) $(OIMAGES) $(OBINS) $(SPECIAL_MKTARGETS)

#############################################################
# SPIFFY targets
#############################################################
spiffy: clspiffy
   @echo "Making spiffy utility $(SPIFFY_EXPORTS)"
   $(Q) $(MAKE) $$(SPIFFY_EXPORTS) --no-print-directory -C tools/spiffy-compressor/spiffy
   @echo "Done"

spiffy_img.o:
   $(shell exec tools/spiffy-compressor/spiffy/spiffy)
   $(Q) mv spiffy_rom.bin ${PROJ_ROOT}bin/spiffy_rom.bin

#############################################################
# rBoot targets
#############################################################
$(LIBMAIN_DST): $(LIBMAIN_SRC)
   @echo "OC $@"
   @$(OBJCOPY) -W Cache_Read_Enable_New $^ $@

bin/rboot.bin: rboot/firmware/rboot.bin
   $(Q) cp $^ $@

#############################################################
# Common lib targets
# Compiled separately from main/setup since they do not
# change between app_main and app_setup
#############################################################
libnewcrypto.a:
   @$(MAKE) --no-print-directory -C app_common/newcrypto
   cp -f $(PROJ_ROOT)app_common/newcrypto/.output/misc/debug/lib/libnewcrypto.a $(PROJ_ROOT)sdk-overrides/lib/libnewcrypto.a

liblibc.a:
   @$(MAKE) --no-print-directory -C app_common/libc
   cp -f $(PROJ_ROOT)app_common/libc/.output/misc/debug/lib/liblibc.a $(PROJ_ROOT)sdk-overrides/lib/liblibc.a

liblwip.a:
   @$(MAKE) --no-print-directory -C app_common/lwip
   cp -f $(PROJ_ROOT)app_common/lwip/.output/misc/debug/lib/liblwip.a $(PROJ_ROOT)sdk-overrides/lib/liblwip.a

libmqtt.a:
   @$(MAKE) --no-print-directory -C app_common/mqtt
   cp -f $(PROJ_ROOT)app_common/mqtt/.output/misc/debug/lib/libmqtt.a $(PROJ_ROOT)sdk-overrides/lib/libmqtt.a

libjsmn.a:
   @$(MAKE) --no-print-directory -C app_common/jsmn
   cp -f $(PROJ_ROOT)app_common/jsmn/.output/misc/debug/lib/libjsmn.a $(PROJ_ROOT)sdk-overrides/lib/libjsmn.a

libjsontree.a:
   @$(MAKE) --no-print-directory -C app_common/jsontree
   cp -f $(PROJ_ROOT)app_common/jsontree/.output/misc/debug/lib/libjsontree.a $(PROJ_ROOT)sdk-overrides/lib/libjsontree.a

libplatform.a:
   @$(MAKE) --no-print-directory -C app_common/platform
   cp -f $(PROJ_ROOT)app_common/platform/.output/misc/debug/lib/libplatform.a $(PROJ_ROOT)sdk-overrides/lib/libplatform.a

librboot.a:
   @$(MAKE) --no-print-directory -C app_common/rboot
   cp -f $(PROJ_ROOT)app_common/rboot/.output/misc/debug/lib/rboot.a $(PROJ_ROOT)sdk-overrides/lib/librboot.a

libspiffs.a:
   @$(MAKE) --no-print-directory -C app_common/spiffs
   cp -f $(PROJ_ROOT)app_common/spiffs/.output/misc/debug/lib/spiffs.a $(PROJ_ROOT)sdk-overrides/lib/libspiffs.a

libutil.a:
   @$(MAKE) --no-print-directory -C app_common/util
   cp -f $(PROJ_ROOT)app_common/util/.output/misc/debug/lib/util.a $(PROJ_ROOT)sdk-overrides/lib/libutil.a
#############################################################
# Utilities
#############################################################
clean:
   $(Q) $(foreach d, $(SUBDIRS), $(MAKE) $(MAKEPDIR) -C $(d) clean;)
   $(Q) $(RM) -r $(ODIR)/$(TARGET)/$(FLAVOR)

clobber: $(SPECIAL_CLOBBER)
   $(Q) $(foreach d, $(SUBDIRS), $(MAKE) $(MAKEPDIR) -C $(d) clobber;)
   $(Q) $(RM) -r $(ODIR)

flash:
ifndef PDIR
   $(Q) $(MAKE) $(MAKEPDIR) -C ./app_main flash
else
   $(Q) $(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) write_flash -fs $(ET_FS) -fm $(ET_FM) -ff $(ET_FF) \
   $(FLASH_BINS)
endif

erase:
   $(Q) $(ESPTOOL) --port $(ESPPORT) --baud $(ESPBAUD) erase_flash

.subdirs:
   $(Q) set -e; $(foreach d, $(SUBDIRS), $(MAKE) $(MAKEPDIR) -C $(d);)

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),clobber)
ifdef DEPS
sinclude $(DEPS)
endif
endif
endif

$(OBJODIR)/%.o: %.c
   $(Q) mkdir -p $(OBJODIR);
   $(QECHO) CC $<
   $(Q) $(CC) $(if $(findstring $<,$(DSRCS)),$(DFLAGS),$(CFLAGS)) $(COPTS_$(*F)) -o $@ -c $<

$(OBJODIR)/%.d: %.c
   $(Q) mkdir -p $(OBJODIR);
   $(VECHO) DEPEND: $(CC) -M $(CFLAGS) $<
   $(Q) set -e; rm -f $@; \
   $(CC) -M $(CFLAGS) $< > $@.$$$$; \
   sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
   rm -f $@.$$$$

$(OBJODIR)/%.o: %.s
   $(Q) mkdir -p $(OBJODIR);
   $(QECHO) CC $<
   $(Q) $(CC) $(CFLAGS) -o $@ -c $<

$(OBJODIR)/%.d: %.s
   $(Q) mkdir -p $(OBJODIR);
   $(Q) set -e; rm -f $@; \
   $(CC) -M $(CFLAGS) $< > $@.$$$$; \
   sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
   rm -f $@.$$$$

$(OBJODIR)/%.o: %.S
   $(Q) mkdir -p $(OBJODIR);
   $(QECHO) CC $<
   $(Q) $(CC) $(CFLAGS) -D__ASSEMBLER__ -o $@ -c $<

$(OBJODIR)/%.d: %.S
   $(Q) mkdir -p $(OBJODIR);
   $(QECHO) CC $<
   $(Q) set -e; rm -f $@; \
   $(CC) -M $(CFLAGS) $< > $@.$$$$; \
   sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
   rm -f $@.$$$$


$(foreach lib,$(GEN_LIBS),$(eval $(call ShortcutRule,$(lib),$(LIBODIR))))
$(foreach image,$(GEN_IMAGES),$(eval $(call ShortcutRule,$(image),$(IMAGEODIR))))
$(foreach bin,$(GEN_BINS),$(eval $(call ShortcutRule,$(bin),$(BINODIR))))
$(foreach lib,$(GEN_LIBS),$(eval $(call MakeLibrary,$(basename $(lib)))))
$(foreach image,$(GEN_IMAGES),$(eval $(call MakeImage,$(basename $(image)))))

mkesptool2:
   $(Q) $(MAKE) -C ${ESPTOOL2_SRC_DIR}
   $(Q) cp -f ${ESPTOOL2_SRC_DIR}/esptool2 $(PROJ_ROOT)tools/esptool2;
   @rm -f ${ESPTOOL2_SRC_DIR}/esptool2

rconf:
   $(Q) echo "$(HOST)"
   $(Q) $(MAKE) CFLAGS=-DROM_TO_BOOT=$(ROM_TO_BOOT) -C $(PROJ_ROOT)tools/rconf_gen
   $(Q) cp -f ${PROJ_ROOT}tools/rconf_gen/rconf.bin $(PROJ_ROOT)bin/rconf.bin;

rboot:
   $(Q) mkdir -p ${PROJ_ROOT}/sdk-overrides/lib;
   $(Q) rm -f ${PROJ_ROOT}/sdk-overrides/lib/libmain2.a;
   $(OBJCOPY) -W Cache_Read_Enable_New ${SDK_ROOT}lib/libmain.a ${LIBMAIN_DST}
   $(Q) $(MAKE) -C ${RBOOT_SRC_DIR}

clspiffy:
   @echo "clear spiffy"
   @rm -f tools/spiffy-compressor/spiffy/*.o
   @rm -f tools/spiffy-compressor/spiffy/spiffy

clean-esptool2:
   @rm -f ${ESPTOOL2_SRC_DIR}/*.o
   @rm -f ${ESPTOOL2_SRC_DIR}/../esptool2
#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#############################################################

INCLUDES := $(INCLUDES) -I $(PDIR)include -I $(PDIR)include/$(TARGET) -I $(PDIR)app/libc
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile
