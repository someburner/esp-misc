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
SPECIAL_MKTARGETS=$(APP_MKTARGETS)
COMMON_MKTARGETS=$(CP_MKTARGETS)
SUBDIRS=     \
   libc		 \
   lwip		 \
   mqtt		 \
   jsmn		 \
   jsontree  \
   newcrypto \
   rboot     \
   spiffs	 \
   util
endif # } PDIR

APPDIR = .
LDDIR = $(PROJ_ROOT)/ld/

CCFLAGS += -Os

TARGET_LDFLAGS =		\
	-nostdlib			\
	-Wl,-EL 				\
	--longcalls			 \
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
LD_FILE = $(LDDIR)/misc.app.common.ld

COMPONENTS_misc.app.common = \
   newcrypto/libnewcrypto.a 	\
   lwip/liblwip.a 				\
   libc/liblibc.a 				\
   mqtt/libmqtt.a					\
   jsontree/libjsontree.a 		\
   jsmn/libjsmn.a					\
   rboot/rboot.a					\
   spiffs/spiffs.a				\
	util/util.a

LINKFLAGS_misc.app.common =	\
   -Wl,--gc-sections \
   -Wl,-Map=mapfile \
   -nostdlib \
   -T$(LD_FILE) \
   -Wl,@$(LDDIR)defsym.rom \
   -Wl,--no-check-sections	\
   -Wl,--wrap=_xtos_set_exception_handler	\
   -Wl,-static	\
   -Wl,--start-group	\
   $(DEP_LIBS_misc.app.common) \
   -lc \
   -lgcc \
   -lhal \
   -lphy \
   -lpp \
   -lnet80211 \
   -lwpa \
   -lwpa2 \
   -lmain2 \
   -lsmartconfig \
   -lssl \
   -Wl,--end-group \
   -lm

DEPENDS_misc.app.common = \
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
