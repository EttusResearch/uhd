#
# Copyright 2013-2014 Ettus Research LLC
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

HEX_OUT = usrp_b200_fw.hex

all:$(HEX_OUT)

# Pull in the Cypress SDK files to build the firmware
FX3FWROOT=../..
FX3PFWROOT=../../u3p_firmware
include $(FX3FWROOT)/common/fx3_build_config.mak

ifndef OC
	OC	= arm-none-eabi-objcopy
endif

MODULE = b200_main

SOURCE += $(MODULE).c b200_i2c.c b200_usb_descriptors.c ../common/common_helpers.c ../common/common_descriptors.c

INCFLAGS = -I../common

LDLIBS  += \
	"$$ARMGCC_INSTALL_PATH"/arm-none-eabi/lib/libm.a

C_OBJECT=$(SOURCE:%.c=./%.o)
A_OBJECT=$(SOURCE_ASM:%.S=./%.o)

EXES = $(MODULE).$(EXEEXT)

$(MODULE).$(EXEEXT): $(A_OBJECT) $(C_OBJECT)
	$(LINK) $(LINKFLAGS)

$(C_OBJECT) : %.o : %.c
	$(COMPILE) $(INCFLAGS)

$(A_OBJECT) : %.o : %.S
	$(ASSEMBLE)

clean:
	rm -f ./$(MODULE).$(EXEEXT)
	rm -f ./$(MODULE).map
	rm -f ./*.o
	rm -f ../common/*.o

$(HEX_OUT): $(C_OBJECT) $(A_OBJECT) $(EXES)
	$(OC) -O ihex $(EXES) $@

#[]#
