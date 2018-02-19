CC := gcc-6
LD := gcc-6
RM := rm
PROJECT := simple

SRC := src/simple.c
OBJS := $(patsubst %.c, %.o, $(SRC))

CFLAGS := -lcurl
LDFLAGS := -lcurl

#
# Rules
#

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

#
# Targets
#

$(PROJECT): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

clean:
	$(RM) $(PROJECT)
	$(RM) $(OBJS)

.PHONY: clean
