CC := gcc-6
LD := gcc-6
RM := rm
MKDIR := mkdir
AR := ar crv

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

SOURCES := src/simperium_auth.c \
		   src/simperium_bucket.c \
		   src/simperium_bucket_http.c \
		   src/simperium_bucket_ws.c

INCLUDES := . src
CFLAGS :=
LDFLAGS :=

# Include libs here

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

$(BUILD_DIR)/libsimperium.a: $(OBJS)
	@echo Packaging $@
	$(NO_ECHO)$(AR) $@ $(OBJS)

clean:
	$(NO_ECHO)$(RM) -r $(BUILD_DIR)

.PHONY: clean
