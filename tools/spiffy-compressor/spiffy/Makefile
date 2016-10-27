#
# Makefile for spiffy
#

CC := gcc
LD := gcc

############
#
# Paths
#
############

curdir    := $(abspath $(basename $(CURDIR)))
spiffsdir := $(subst tools/spiffy-compressor/spiffy,app_common/spiffs, $(curdir))
maindir   := $(subst tools/spiffy-compressor/spiffy,app_main/main, $(curdir))
commondir := $(subst tools/spiffy-compressor/spiffy,app_common/include, $(curdir))

INCDIR := -I$(curdir)/ -I${spiffsdir} -I${commondir} -I${maindir}

CFLAGS := -O2
# CFLAGS := -O2 -Wall -Wno-unused-value

#Static gzipping is disabled by default.
SPIFFY_OUTPUT ?= 1
VERBOSE        = 0

.RECIPEPREFIX +=
###############
#
# Files and libs
#
###############

FLAGS  += -DCONFIG_BUILD_SPIFFS -I.. -std=gnu99

ifeq ($(SPIFFY_OUTPUT),1)
CCFLAGS ?= -DSPIFFY_DEBUG
endif

CCFLAGS += \
   $(SPIFFY_EXPORTS)

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

all: spiffy cleanup

# Use the app_common/spiffs c sources to ensure identical source
# Define SPIFFY_SRC_MK so source files know which headers to include
%.o: ${spiffsdir}/%.c
   $(vecho) "CC $<"
   $(Q) $(CC) $(CFLAGS) $(INCDIR) -DSPIFFY_SRC_MK -c $< -o $@

spiffy.o: spiffy.c
   $(vecho) "CC $<"
   $(Q) $(CC) $(CFLAGS) $(CCFLAGS) $(INCDIR) -c $< -o $@

spiffy: spiffy.o spiffs_cache.o spiffs_nucleus.o spiffs_hydrogen.o spiffs_gc.o spiffs_check.o
   $(vecho) "LD $@"
   $(Q) $(LD) -o $@ $^

cleanup:
   @echo "Removing objects"
   $(Q) rm -f *.o

clean:
   @echo "Removing spiffy"
   $(Q) rm -f spiffy
