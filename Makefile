# object files
CC := gcc
SOURCES := $(wildcard *.c)
TARGETS := $(SOURCES:.c=.elf)
CFLAGS += -g
CFLAGS += -O2
CFLAGS += -Wall
CFLAGS += -lrt
CFLAGS += -pthread

all: $(TARGETS)

%.elf: %.c cacheutils.h helper.h
	$(CC) -D_FILE_OFFSET_BITS=64 $< $(CFLAGS) -o $@

clean:
	rm -f *.elf
