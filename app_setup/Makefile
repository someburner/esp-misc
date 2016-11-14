#############################################################
# Required variables for each makefile
# Discard this section from all parent makefiles
# Expected variables (with automatic defaults):
#   CSRCS (all "C" files in the dir)
#   SUBDIRS (all subdirs with a Makefile)
#   GEN_LIBS - list of libs to be generated ()
#   GEN_IMAGES - list of object file images to be generated ()
#   GEN_BINS - list of binaries to be generated ()
#   COMPONENTS_xxx - a list of libs/objs in the form
#     subdir/lib to be extracted and rolled up into
#     a generated lib/image xxx.a ()
#
TARGET = misc

# FIRST_TARGET is exported as 0 from top-level Makefile on the first go around.
# It gets unset before the second make command call so we can assume its 1
ifndef FIRST_TARGET
APP_TARGET = 1
else
APP_TARGET = 0
endif

#FLAVOR = release
FLAVOR = debug

# DEBUGGER = 1d

# EXTRA_CCFLAGS += -u

ifndef PDIR # {
ifeq ($(APP_TARGET), 0)
   GEN_IMAGES= misc.app.setup0.out
   GEN_BINS= misc.app.setup0.bin
   SPECIAL_MKTARGETS=$(APP_MKTARGETS)
   SUBDIRS= \
		../app_common/jsmn      \
		../app_common/jsontree  \
		../app_common/libc      \
		../app_common/lwip      \
		../app_common/mqtt      \
		../app_common/newcrypto \
		../app_common/platform 	\
		../app_common/spiffs 	\
		../app_common/util 		\
		user 			\
		driver 		\
		mqtt_setup 	\
		setup
endif

ifeq ($(APP_TARGET), 1)
   GEN_IMAGES= misc.app.setup1.out
   GEN_BINS= misc.app.setup1.bin
   SPECIAL_MKTARGETS=$(APP_MKTARGETS)
   SUBDIRS= \
		../app_common/jsmn      \
		../app_common/jsontree  \
		../app_common/libc      \
		../app_common/lwip      \
		../app_common/mqtt      \
		../app_common/newcrypto \
		../app_common/platform 	\
		../app_common/spiffs 	\
		../app_common/util 		\
		user 			\
		driver 		\
		mqtt_setup 	\
		setup
endif
endif # } PDIR


APPDIR = .
LDDIR = $(PROJ_ROOT)/ld/

CCFLAGS += -Os

TARGET_LDFLAGS =		\
	-nostdlib			\
	-Wl,-EL 				\
	--longcalls			\
	--text-section-literals

ifeq ($(FLAVOR),debug)
    TARGET_LDFLAGS += -g -Os
endif

ifeq ($(FLAVOR),release)
    TARGET_LDFLAGS += -Os
endif

#############################################################
# App Target 0: esp-misc setup0 rom
#############################################################
ifeq ($(APP_TARGET), 0)
	LD_FILE = $(LDDIR)/misc.app.setup0.ld
endif

# NOTE: rboot.a must go last for proper linkage
ifeq ($(APP_TARGET), 0)
   COMPONENTS_misc.app.setup0 =   \
		../app_common/jsmn/libjsmn.a           \
		../app_common/jsontree/libjsontree.a   \
		../app_common/libc/liblibc.a           \
		../app_common/lwip/liblwip.a           \
		../app_common/mqtt/libmqtt.a           \
		../app_common/newcrypto/libnewcrypto.a \
		../app_common/platform/libplatform.a 	\
		../app_common/spiffs/spiffs.a 			\
		../app_common/util/util.a 					\
		user/libuser.a 		\
		mqtt_setup/mqtt.a		\
		setup/libsetup.a 		\
		driver/libdriver.a 	\
		../app_common/rboot/rboot.a
endif

ifeq ($(APP_TARGET), 0)
   LINKFLAGS_misc.app.setup0 =	\
	   -Wl,--gc-sections 			\
	   -Wl,-Map=mapfile 			\
	   -nostdlib 				\
	   -T$(LD_FILE)   			\
	   -Wl,@$(LDDIR)defsym.rom			\
	   -Wl,--no-check-sections	\
	   -Wl,--wrap=_xtos_set_exception_handler	\
	   -u user_start_trampoline \
	   -u Cache_Read_Enable_New \
	   -Wl,-static	\
	   -Wl,--start-group	\
	   $(DEP_LIBS_misc.app.setup0)		\
	   -lc	\
	   -lgcc	\
	   -lhal	\
	   -lphy	\
	   -lpp	\
	   -lnet80211	\
	   -lwpa	\
	   -lwpa2					\
	   -lmain2	\
	   -lsmartconfig	\
	   -lssl \
	   -Wl,--end-group  \
	   -lm
endif

ifeq ($(APP_TARGET), 0)
   DEPENDS_misc.app.setup0 = \
                   $(LD_FILE) \
					    $(LDDIR)eagle.rom.addr.v6.ld
endif

#############################################################
# App Target 1: esp-misc setup1 rom
#############################################################
ifeq ($(APP_TARGET), 1)
	LD_FILE = $(LDDIR)/misc.app.setup1.ld
endif

# NOTE: rboot.a must go last for proper linkage
ifeq ($(APP_TARGET), 1)
   COMPONENTS_misc.app.setup1 =   \
		../app_common/jsmn/libjsmn.a           \
		../app_common/jsontree/libjsontree.a   \
		../app_common/libc/liblibc.a           \
		../app_common/lwip/liblwip.a           \
		../app_common/mqtt/libmqtt.a           \
		../app_common/newcrypto/libnewcrypto.a \
		../app_common/platform/libplatform.a 	\
		../app_common/spiffs/spiffs.a 			\
		../app_common/util/util.a 					\
		user/libuser.a 		\
		driver/libdriver.a 	\
		mqtt_setup/mqtt.a 	\
		setup/libsetup.a		\
		../app_common/rboot/rboot.a
endif

ifeq ($(APP_TARGET), 1)
   LINKFLAGS_misc.app.setup1 =	\
	   -Wl,--gc-sections 			\
	   -Wl,-Map=mapfile 			\
	   -nostdlib 				\
	   -T$(LD_FILE)   			\
	   -Wl,@$(LDDIR)defsym.rom			\
	   -Wl,--no-check-sections	\
	   -Wl,--wrap=_xtos_set_exception_handler	\
	   -u user_start_trampoline \
	   -u Cache_Read_Enable_New \
	   -Wl,-static	\
	   -Wl,--start-group	\
	   $(DEP_LIBS_misc.app.setup1)		\
	   -lc	\
	   -lgcc	\
	   -lhal	\
	   -lphy	\
	   -lpp	\
	   -lnet80211	\
	   -lwpa	\
	   -lwpa2					\
	   -lmain2	\
	   -lsmartconfig	\
	   -lssl \
	   -Wl,--end-group  \
	   -lm
endif

ifeq ($(APP_TARGET), 1)
   DEPENDS_misc.app.setup1 = \
                   $(LD_FILE) \
					    $(LDDIR)eagle.rom.addr.v6.ld
endif


#############################################################
# Configuration i.e. compile options etc.
# Target specific stuff (defines etc.) goes in here!
# Generally values applying to a tree are captured in the
#   makefile at its root level - these are then overridden
#   for a subtree within the makefile rooted therein
#

#UNIVERSAL_TARGET_DEFINES =		\

# Other potential configuration flags include:
#	-DTXRX_TXBUF_DEBUG
#	-DTXRX_RXBUF_DEBUG
#	-DWLAN_CONFIG_CCX
CONFIGURATION_DEFINES =	-D__ets__ \
						-DICACHE_FLASH \
						-DLWIP_OPEN_SRC	\
						-DPBUF_RSV_FOR_WLAN	\
						-DAPP_SETUP \
						-DEBUF_LWIP

DEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)

DDEFINES +=				\
	$(UNIVERSAL_TARGET_DEFINES)	\
	$(CONFIGURATION_DEFINES)


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
#

INCLUDES := $(INCLUDES) -I $(PDIR)include
INCLUDES += -I ./
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile

.PHONY: FORCE
FORCE:
