# USRP E310 Firmware
# Copyright (C) 2014-2015 Ettus Research
# This file is part of the USRP E310 Firmware
# The USRP E310 Firmware is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
# The USRP E310 Firmware is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with the USRP E310 Firmware. If not, see <http://www.gnu.org/licenses/>.

##################################################
# Compiler
##################################################
CC = avr-gcc
OBJCOPY = avr-objcopy
STRIP = avr-strip
SIZE = avr-size
OBJDUMP = avr-objdump
SREC = srec_cat
CFLAGS = -Os -std=c11 -Wall -fshort-enums -pedantic -Wl,--gc-sections \
	-Wstrict-prototypes -Wcast-align -Wshadow

##################################################
# Files
##################################################
HDRS =
SRCS =  main.c adc.c bq2419x.c fpga.c i2c_twi.c interrupt.c io.c ltc294x.c ltc3675.c \
	pmu.c spi.c timer.c tps54478.c eeprom.c led.c
TARGET = main

##################################################
# Device
##################################################
MMCU = attiny88
PROGRAMMER = jtag3isp
PORT = usb
AVRDUDE = avrdude -p $(MMCU) -c $(PROGRAMMER) -P $(PORT) -V
FUSES = -U hfuse:w:hfuse.hex:i -U lfuse:w:lfuse.hex:i -U efuse:w:efuse.hex:i -u
EEPROM = -U eeprom:w:$(TARGET).eeprom.hex

##################################################
# Global Targets
##################################################
all: $(TARGET).hex $(TARGET).eeprom.hex size hfuse.hex lfuse.hex efuse.hex

clean:
	$(RM) *.o *.elf *.hex

install: all
	$(AVRDUDE) -U flash:w:$(TARGET).hex:i $(FUSES) $(EEPROM)

size: $(TARGET).hex
	@$(SIZE) --format=SysV --mcu=$(MMCU) $(TARGET).elf
	@$(SIZE) -C --mcu=$(MMCU) $(TARGET).elf

##################################################
# Dependency Targets
##################################################
fuses.hex: $(TARGET).elf
	$(OBJCOPY) -j .fuse -O ihex $< $@ --change-section-lma .fuse=0

lfuse.hex: fuses.hex
	$(SREC) $< -Intel -crop 0x00 0x01 -offset  0x00 -O $@ -Intel

hfuse.hex: fuses.hex
	$(SREC) $< -Intel -crop 0x01 0x02 -offset -0x01 -O $@ -Intel

efuse.hex: fuses.hex
	$(SREC) $< -Intel -crop 0x02 0x03 -offset -0x02 -O $@ -Intel

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -R .eeprom -R .fuse -O ihex $< $@

$(TARGET).eeprom.hex: $(TARGET).elf
	$(OBJCOPY) -j .eeprom --set-section-flags=.eeprom="alloc,load" --change-section-lma \
		.eeprom=0 -O ihex $(TARGET).elf $@

$(TARGET).elf: $(SRCS:.c=.o)
	$(CC) -mmcu=$(MMCU) $^ -o $@

%.o: %.c $(HDRS) Makefile
	$(CC) -mmcu=$(MMCU) -c $< -o $@ $(CFLAGS)
