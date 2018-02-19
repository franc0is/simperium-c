CC := gcc-6
LD := gcc-6
RM := rm
PROJECT := simple

ifneq ($(VERBOSE),)
  NO_ECHO=
else
  NO_ECHO=@
endif

IGNORE_ERRORS = >/dev/null 2>&1 || true

SOURCES := src/simple.c
INCLUDES := .
CFLAGS := -lcurl
LDFLAGS := -lcurl

# Include libs here
include lib/argtable3.mk

OBJS := $(patsubst %.c, %.o, $(SOURCES))
INC := $(addprefix -I,$(INCLUDES))

CFLAGS += $(INC)

#
# Rules
#

%.o: %.c
	@echo Compiling $(notdir $<)
	$(NO_ECHO)$(CC) $(CFLAGS) -c $< -o $@

#
# Targets
#

$(PROJECT): $(OBJS)
	@echo Linking $@
	$(NO_ECHO)$(LD) $(LDFLAGS) -o $@ $^

clean:
	$(NO_ECHO)$(RM) -r $(PROJECT) $(IGNORE_ERRORS)
	$(NO_ECHO)$(RM) -r $(OBJS) $(IGNORE_ERRORS)

.PHONY: clean
