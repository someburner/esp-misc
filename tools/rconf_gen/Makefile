VERBOSE=1

TARGET = genrconf
# .RECIPEPREFIX +=

#############
#
# Build tools
#
#############

CC = gcc $(COMPILEROPTIONS)
LD = ld
GDB = gdb
OBJCOPY = objcopy
OBJDUMP = objdump
MKDIR = mkdir -p

###############
#
# Files and libs
#
###############

CFILES   = main.c

COMPILEROPTIONS = -I.

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

############
#
# Tasks
#
############

OBJFILES = $(CFILES:%.c=%.o)

DEPFILES = $(CFILES:%.c=%.d)

ALLOBJFILES += $(OBJFILES)

DEPENDENCIES = $(DEPFILES)

# link object files, create binary
$(TARGET): $(ALLOBJFILES)
	@echo "... linking"
	@${CC} $(LINKEROPTIONS) -o $(TARGET) $(ALLOBJFILES)
	@${sh} ./$(TARGET)
	@rm -f main.o
	@rm -f main.d

-include $(DEPENDENCIES)

# compile c filesf
$(OBJFILES): %.o:%.c
		$(vecho) "... compile genrconf $@"
		@${CC} ${CFLAGS} ${FLAGS} -g -c -o $@ $<

# make dependencies
$(DEPFILES): %.d:%.c
		@echo "... depend $@"; \
		rm -f $@; \
		${CC} $(COMPILEROPTIONS) -M $< > $@.$$$$; \
		sed 's,\($*\)\.o[ :]*, /\1.o $@ : ,g' < $@.$$$$ > $@; \
		rm -f $@.$$$$

clean:
	@echo ... removing files
	@rm -f main.o
	@rm -f main.d
	@rm -f genrconf

all: $(TARGET) $(clean)
