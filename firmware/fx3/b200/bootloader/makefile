# 
# Copyright 2019 Ettus Research, a National Instruments Brand
# 
# SPDX-License-Identifier: GPL-3.0-or-later
# 

IMGOUT = usrp_b200_bl.img

all:$(IMGOUT)

BL_ROOT=../../boot_fw
BL_INC=$(BL_ROOT)/include
BL_LIB=$(BL_ROOT)/lib
BL_CYP_SRC=$(BL_ROOT)/src

ELF2IMG_SRC=../../elf2img/elf2img.c
ELF2IMG=elf2img

CYFXBUILD = gcc

include fx3_build_config.mak

MODULE = cyfx_boot_app

APP_SOURCE = main.c usb_boot.c usb_descriptors.c ../common/common_descriptors.c ../common/common_helpers.c

INCFLAGS = -I$(BL_INC) -I../common

APP_OBJECT=$(APP_SOURCE:%.c=./%.o)

APP_ASM_OBJECT=$(APP_ASM_SOURCE:%.S=./%.o)

EXES = $(MODULE).$(EXEEXT)

ifeq ($(CYFXBUILD), gcc)

APP_ASM_SOURCE = cyfx_gcc_startup.S

else

APP_ASM_SOURCE = 

endif

$(APP_ASM_OBJECT) : %.o : %.S 
	$(ASSEMBLE)

$(APP_OBJECT) : %.o : %.c  ../lib/cyfx3_boot.a cyfx3.ld
	$(COMPILE) $(INCFLAGS)

$(MODULE).$(EXEEXT): $(APP_OBJECT) $(APP_ASM_OBJECT) 
	$(LINK)

$(ELF2IMG):
	gcc -o $(ELF2IMG) $(ELF2IMG_SRC)

fx3_build_config.mak:
	cp $(BL_CYP_SRC)/fx3_build_config.mak .
fx3_armgcc_config.mak:
	cp $(BL_CYP_SRC)/fx3_armgcc_config.mak .
cyfx_gcc_startup.S:
	cp $(BL_CYP_SRC)/cyfx_gcc_startup.S .
cyfx3.ld:
	cp $(BL_CYP_SRC)/cyfx3.ld .
../lib/cyfx3_boot.a:
	mkdir -p ../lib
	cp $(BL_LIB)/cyfx3_boot.a ../lib


clean:
	rm -f ./$(MODULE).$(EXEEXT)
	rm -f ./$(MODULE).map
	rm -f ./$(IMGOUT)
	rm -f ./fx3_build_config.mak
	rm -f ./fx3_armgcc_config.mak
	rm -f ./cyfx_gcc_startup.S
	rm -f ./cyfx3.ld
	rm -f ../lib/cyfx3_boot.a
	rmdir ../lib --ignore-fail-on-non-empty
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
