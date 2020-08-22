#
# Copyright 2013-2014 Ettus Research LLC
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

HEX_OUT = usrp_b200_fw.hex

all:$(HEX_OUT)

# Pull in the Cypress SDK files to build the firmware
include $(FX3FWROOT)/firmware/common/fx3_build_config.mak

ifndef OC
	OC	= arm-none-eabi-objcopy
endif

MODULE = b200_main

SOURCE += $(MODULE).c b200_i2c.c b200_usb_descriptors.c ../common/common_helpers.c ../common/common_descriptors.c cyfxtx.c

INCFLAGS = -I../common

LDLIBS  += \
	"$$ARMGCC_INSTALL_PATH"/arm-none-eabi/lib/libm.a

ifeq ($(CYFXBUILD),arm)
	SOURCE_ASM=cyfx_startup.S
else
	SOURCE_ASM=cyfx_gcc_startup.S
endif

C_OBJECT=$(SOURCE:%.c=./%.o)
A_OBJECT=$(SOURCE_ASM:%.S=./%.o)

EXES = $(MODULE).$(EXEEXT)

$(MODULE).$(EXEEXT): $(A_OBJECT) $(C_OBJECT)
	$(LINK) $(LINKFLAGS)

cyfxtx.c:
	cp $(FX3FWROOT)/fw_build/fx3_fw/cyfxtx.c .

cyfx_startup.S:
	cp $(FX3FWROOT)/fw_build/fx3_fw/cyfx_startup.S .

cyfx_gcc_startup.S:
	cp $(FX3FWROOT)/fw_build/fx3_fw/cyfx_gcc_startup.S .

$(C_OBJECT) : %.o : %.c
	$(COMPILE) $(INCFLAGS)

$(A_OBJECT) : %.o : %.S
	$(ASSEMBLE)

clean:
	rm -f ./$(MODULE).$(EXEEXT)
	rm -f ./$(MODULE).map
	rm -f ./*.o
	rm -f ../common/*.o
	rm -f cyfxtx.c cyfx_startup.S cyfx_gcc_startup.S

$(HEX_OUT): $(C_OBJECT) $(A_OBJECT) $(EXES)
	$(OC) -O ihex $(EXES) $@

#[]#
