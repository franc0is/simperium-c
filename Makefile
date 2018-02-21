CC := gcc-6
LD := gcc-6
RM := rm
MKDIR := mkdir

PROJECT := simple
BUILD_DIR := build

ifneq ($(VERBOSE),)
  NO_ECHO=
else
  NO_ECHO=@
endif

ifneq ($(DEBUG),)
  DEFINES = DEBUG
endif

IGNORE_ERRORS = >/dev/null 2>&1 || true

SOURCES := src/simple.c
INCLUDES := .
CFLAGS := -lcurl -ljansson
LDFLAGS := -lcurl -ljansson

# Include libs here
include lib/argtable3.mk

OBJS := $(patsubst %.c, $(BUILD_DIR)/%.o, $(SOURCES))
INC := $(addprefix -I,$(INCLUDES))
DEFS := $(addprefix -D,$(DEFINES))

#
# Rules
#

$(BUILD_DIR)/%.o: %.c
	@echo Compiling $(notdir $<)
	$(NO_ECHO)$(MKDIR) -p $(dir $@)
	$(NO_ECHO)$(CC) $(CFLAGS) $(INC) $(DEFS) -c $< -o $@

#
# Targets
#

$(BUILD_DIR)/$(PROJECT): $(OBJS)
	@echo Linking $@
	$(NO_ECHO)$(LD) $(LDFLAGS) -o $@ $^

clean:
	$(NO_ECHO)$(RM) -r $(BUILD_DIR)

.PHONY: clean
