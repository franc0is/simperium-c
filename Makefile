CC := gcc-6
LD := gcc-6
RM := rm
PROJECT := simple

ifneq ($(VERBOSE),)
  NO_ECHO=
else
  NO_ECHO=@
endif

SRC := src/simple.c
OBJS := $(patsubst %.c, %.o, $(SRC))

CFLAGS := -lcurl
LDFLAGS := -lcurl

#
# Rules
#

%.o: %.c
	@echo Compiling $<
	$(NO_ECHO)$(CC) $(CFLAGS) -c $< -o $@

#
# Targets
#

$(PROJECT): $(OBJS)
	@echo Linking $@
	$(NO_ECHO)$(LD) $(LDFLAGS) -o $@ $^

clean:
	-$(NO_ECHO)$(RM) -r $(PROJECT)
	-$(NO_ECHO)$(RM) -r $(OBJS)

.PHONY: clean
