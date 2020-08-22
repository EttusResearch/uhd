# 
# Copyright 2019 Ettus Research, a National Instruments Brand
# 
# SPDX-License-Identifier: GPL-3.0-or-later
# 

IMGOUT = usrp_b200_bl.img

all:$(IMGOUT)

ELF2IMG_SRC=$(FX3FWROOT)/util/elf2img/elf2img.c
ELF2IMG=elf2img

CYFXBUILD = gcc

include $(FX3FWROOT)/fw_build/boot_fw/fx3_build_config.mak

MODULE = cyfx_boot_app

ifeq ($(CYFXBUILD), arm)
	APP_ASM_SOURCE = 
else
	APP_ASM_SOURCE = cyfx_gcc_startup.S
endif

APP_SOURCE = main.c usb_boot.c usb_descriptors.c ../common/common_descriptors.c ../common/common_helpers.c

INCFLAGS = -I../common

APP_OBJECT=$(APP_SOURCE:%.c=./%.o)

APP_ASM_OBJECT=$(APP_ASM_SOURCE:%.S=./%.o)

EXES = $(MODULE).$(EXEEXT)

$(APP_ASM_OBJECT) : %.o : %.S 
	$(ASSEMBLE)


$(APP_OBJECT) : %.o : %.c
	$(COMPILE) $(INCFLAGS)

$(MODULE).$(EXEEXT): $(APP_OBJECT) $(APP_ASM_OBJECT) 
	$(LINK)

$(ELF2IMG):
	gcc -o $(ELF2IMG) $(ELF2IMG_SRC)

cyfx_gcc_startup.S:
	cp $(FX3FWROOT)/firmware/boot_fw/src/cyfx_gcc_startup.S .


clean:
	rm -f ./$(MODULE).$(EXEEXT)
	rm -f ./$(MODULE).map
	rm -f ./$(IMGOUT)
	rm -f ./cyfx_gcc_startup.S
	rm -f ./*.o
	rm -f ../common/*.o
	rm -f ./$(ELF2IMG)
ifeq ($(CYFXBUILD), gcc)
	rm -f ./gcceclipse_files/*.o
endif

compile: $(APP_OBJECT) $(APP_ASM_OBJECT) $(EXES)

$(IMGOUT): compile $(EXES) $(ELF2IMG)
	./$(ELF2IMG) -i $(EXES) -o $(IMGOUT) -i2cconf 0x1A

#[]#
