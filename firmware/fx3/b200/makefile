#
# Copyright 2013-2014 Ettus Research LLC
#

HEX_OUT = usrp_b200_fw.hex

all:$(HEX_OUT)

# Pull in the Cypress SDK files to build the firmware
FX3FWROOT=..
FX3PFWROOT=../u3p_firmware
include $(FX3FWROOT)/common/fx3_build_config.mak

ifndef OC
	OC	= arm-none-eabi-objcopy
endif

MODULE = b200_main

SOURCE += $(MODULE).c
SOURCE += b200_usb_descriptors.c
SOURCE += b200_i2c.c

INCLUDES = b200_main.h b200_gpifconfig.h b200_i2c.h

LDLIBS  += \
	"$$ARMGCC_INSTALL_PATH"/arm-none-eabi/lib/libm.a

C_OBJECT=$(SOURCE:%.c=./%.o)
A_OBJECT=$(SOURCE_ASM:%.S=./%.o)

EXES = $(MODULE).$(EXEEXT)

$(MODULE).$(EXEEXT): $(A_OBJECT) $(C_OBJECT)
	$(LINK) $(LINKFLAGS)

$(C_OBJECT) : %.o : %.c $(INCLUDES)
	$(COMPILE) $(INCFLAGS)

$(A_OBJECT) : %.o : %.S
	$(ASSEMBLE)

clean:
	rm -f ./$(MODULE).$(EXEEXT)
	rm -f ./$(MODULE).map
	rm -f ./*.o

$(HEX_OUT): $(C_OBJECT) $(A_OBJECT) $(EXES)
	$(OC) -O ihex $(EXES) $@

#[]#
