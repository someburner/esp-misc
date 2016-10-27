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

#FLAVOR = release
FLAVOR = debug

# DEBUGGER = 1d

# EXTRA_CCFLAGS += -u

ifndef PDIR # {
GEN_IMAGES= misc.app.main.out
GEN_BINS= misc.app.main.bin
SPECIAL_MKTARGETS=$(APP_MKTARGETS)
SUBDIRS=    \
   ../app_common/jsmn      \
   ../app_common/jsontree  \
   ../app_common/libc      \
   ../app_common/lwip      \
   ../app_common/mqtt      \
   ../app_common/newcrypto \
   ../app_common/platform  \
   ../app_common/spiffs    \
   ../app_common/util      \
   driver    \
   main      \
   mqtt_main \
   user
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
# App Target: esp-misc main rom
#############################################################
LD_FILE = $(LDDIR)/misc.app.main.ld

# NOTE: rboot.a must go last for proper linkage
COMPONENTS_misc.app.main = 	\
   ../app_common/jsmn/libjsmn.a           \
   ../app_common/jsontree/libjsontree.a   \
   ../app_common/libc/liblibc.a           \
   ../app_common/lwip/liblwip.a           \
   ../app_common/mqtt/libmqtt.a           \
   ../app_common/newcrypto/libnewcrypto.a \
   ../app_common/platform/libplatform.a   \
   ../app_common/spiffs/spiffs.a          \
   ../app_common/util/util.a              \
   driver/libdriver.a \
   main/libappmain.a  \
   mqtt_main/mqtt.a   \
   user/libuser.a     \
   ../app_common/rboot/rboot.a


LINKFLAGS_misc.app.main =	\
   -Wl,--gc-sections \
   -Wl,-Map=mapfile \
   -nostdlib \
   -T$(LD_FILE) \
   -Wl,@$(LDDIR)defsym.rom \
   -Wl,--no-check-sections	\
   -Wl,--wrap=_xtos_set_exception_handler	\
   -u user_start_trampoline \
   -u Cache_Read_Enable_New \
   -Wl,-static	\
   -Wl,--start-group	\
   $(DEP_LIBS_misc.app.main) \
   -lc \
   -lgcc \
   -lhal \
   -lphy \
   -lpp \
   -lnet80211 \
   -lwpa \
   -lwpa2 \
   -lmain2 \
   -lssl \
   -Wl,--end-group \
   -lm

DEPENDS_misc.app.main = \
                $(LD_FILE) \
                $(LDDIR)eagle.rom.addr.v6.ld

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
