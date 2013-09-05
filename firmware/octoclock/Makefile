#
# Copyright 2009 Ettus Research LLC
#

##################################################
# Compiler
##################################################
CC = avr-gcc
OBJCOPY = avr-objcopy
STRIP = avr-strip
OBJDUMP = avr-objdump
SREC = srec_cat
#CFLAGS = -O2 -std=gnu99 -fshort-enums -pedantic-errors -Wall -Werror \
#	-Wstrict-prototypes -Wmissing-prototypes -Wcast-align -Wshadow
CFLAGS = -std=gnu99 -O2

#-D IO_DEBUG

##################################################
# Files
##################################################
HDRS = OctoClock-io.h
SRCS = OctoClock.c
TARGET = octoclock_fw

##################################################
# Device
##################################################
MMCU = atmega128
PROGRAMMER = avrisp2
PORT = usb
AVRDUDE = avrdude -p $(MMCU) -c $(PROGRAMMER) -P $(PORT)

##################################################
# Global Targets
##################################################
all: $(TARGET).hex

clean:
	$(RM) *.o *.elf *.hex

install: all
	$(AVRDUDE) -U efuse:w:0xFF:m -U hfuse:w:0x89:m -U lfuse:w:0xFF:m -U flash:w:$(TARGET).hex:i

##################################################
# Dependency Targets
##################################################

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex $< $@

$(TARGET).elf: $(SRCS:.c=.o)
	$(CC) -mmcu=$(MMCU) $^ -o $@

%.o: %.c $(HDRS) Makefile
	$(CC) -mmcu=$(MMCU) -c $< -o $@ $(CFLAGS)
