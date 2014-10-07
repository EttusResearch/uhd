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
CFLAGS = -Os -std=gnu99 -Wall -fshort-enums -pedantic-errors -Wl,--gc-sections \
	-Wstrict-prototypes -Wmissing-prototypes -Wcast-align -Wshadow \
	-DENABLE_SERIAL -DCHARGER_TI -DLED_POLARITY -DDEBUG_VOID
# -DENABLE_SERIAL	: Output serial debug
# -DCHARGER_TI		: Use TI charger (rev B) instead of LTC (rev A)
# -DLED_POLARITY	: Dual-polarity LED on rev B
# -DDDR3L		: Lower DDR voltage (rev B R-divider changed, so disable this to get back into nominal range)
# -DDEBUG_VOID		: Use (void) debug function replacements instead of inline NOOP (use if .text overflows)
# -DI2C_REWORK		: Rev A only
# -DDEBUG		: Enable debug routines (LED blinks, etc)
# -DDEBUG_SAFETY	: Extra debug prints
# -DATTINY88_DIP	: ATTINY88 DIP testing on STK600
# -DHARDWIRE_ENABLE : LTC3675 dedicated enable lines - don't use

#-Werror
#-D IO_DEBUG

##################################################
# Files
##################################################
HDRS =
SRCS =  main.c io.c power.c ltc3675.c i2c.c debug.c bq24190.c
TARGET = main

##################################################
# Device
##################################################
MMCU = attiny88
#PROGRAMMER = avrisp2
PROGRAMMER = stk600
PORT = usb
AVRDUDE = avrdude -p $(MMCU) -c $(PROGRAMMER) -P $(PORT)

##################################################
# Global Targets
##################################################
all: $(TARGET).hex

clean:
	$(RM) *.o *.elf *.hex

install: all
	$(AVRDUDE) -U flash:w:$(TARGET).hex:i

##################################################
# Dependency Targets
##################################################
fuses.hex: $(TARGET).elf
	$(OBJCOPY) -j .fuse -O ihex $< $@ --change-section-lma .fuse=0

lfuse.hex: fuses.hex
	$(SREC) $< -Intel -crop 0x00 0x01 -offset  0x00 -O $@ -Intel

hfuse.hex: fuses.hex
	$(SREC) $< -Intel -crop 0x01 0x02 -offset -0x01 -O $@ -Intel

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -R .eeprom -R .fuse -O ihex $< $@

$(TARGET).elf: $(SRCS:.c=.o)
	$(CC) -mmcu=$(MMCU) $^ -o $@

%.o: %.c $(HDRS) Makefile
	$(CC) -mmcu=$(MMCU) -c $< -o $@ $(CFLAGS)
