#
# Copyright 2016 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

include $(LIB_IP_DIR)/axi_fft/Makefile.inc
include $(LIB_IP_DIR)/axi_hb31/Makefile.inc
include $(LIB_IP_DIR)/axi_hb47/Makefile.inc
include $(LIB_IP_DIR)/complex_to_magphase/Makefile.inc
include $(LIB_IP_DIR)/complex_to_magphase_int16_int24/Makefile.inc
include $(LIB_IP_DIR)/complex_to_magphase_int32/Makefile.inc
include $(LIB_IP_DIR)/complex_multiplier/Makefile.inc
include $(LIB_IP_DIR)/complex_multiplier_dds/Makefile.inc
include $(LIB_IP_DIR)/cordic_rotator/Makefile.inc
include $(LIB_IP_DIR)/cordic_rotator24/Makefile.inc
include $(LIB_IP_DIR)/cordic_rotate_int24_int16/Makefile.inc
include $(LIB_IP_DIR)/cordic_rotate_int24/Makefile.inc
include $(LIB_IP_DIR)/divide_int16/Makefile.inc
include $(LIB_IP_DIR)/divide_int24/Makefile.inc
include $(LIB_IP_DIR)/divide_int32/Makefile.inc
include $(LIB_IP_DIR)/divide_uint32/Makefile.inc
include $(LIB_IP_DIR)/divide_int16_int32/Makefile.inc
include $(LIB_IP_DIR)/dds_sin_cos_lut_only/Makefile.inc

LIB_IP_XCI_SRCS = \
$(LIB_IP_AXI_FFT_SRCS) \
$(LIB_IP_AXI_HB31_SRCS) \
$(LIB_IP_AXI_HB47_SRCS) \
$(LIB_IP_COMPLEX_TO_MAGPHASE_SRCS) \
$(LIB_IP_COMPLEX_TO_MAGPHASE_INT16_INT24_SRCS) \
$(LIB_IP_COMPLEX_TO_MAGPHASE_INT32_SRCS) \
$(LIB_IP_COMPLEX_MULTIPLIER_SRCS) \
$(LIB_IP_COMPLEX_MULTIPLIER_DDS_SRCS) \
$(LIB_IP_CORDIC_ROTATOR_SRCS) \
$(LIB_IP_CORDIC_ROTATOR24_SRCS) \
$(LIB_IP_CORDIC_ROTATE_INT24_INT16_SRCS) \
$(LIB_IP_CORDIC_ROTATE_INT24_SRCS) \
$(LIB_IP_DIVIDE_INT16_SRCS) \
$(LIB_IP_DIVIDE_INT24_SRCS) \
$(LIB_IP_DIVIDE_INT32_SRCS) \
$(LIB_IP_DIVIDE_UINT32_SRCS) \
$(LIB_IP_DIVIDE_INT16_INT32_SRCS) \
$(LIB_IP_DDS_SIN_COS_LUT_ONLY_SRCS)

LIB_IP_SYNTH_OUTPUTS = \
$(LIB_IP_AXI_FFT_OUTS) \
$(LIB_IP_AXI_HB31_OUTS) \
$(LIB_IP_AXI_HB47_OUTS) \
$(LIB_IP_COMPLEX_TO_MAGPHASE_OUTS) \
$(LIB_IP_COMPLEX_TO_MAGPHASE_INT16_INT24_OUTS) \
$(LIB_IP_COMPLEX_TO_MAGPHASE_INT32_OUTS) \
$(LIB_IP_COMPLEX_MULTIPLIER_OUTS) \
$(LIB_IP_COMPLEX_MULTIPLIER_DDS_OUTS) \
$(LIB_IP_CORDIC_ROTATOR_OUTS) \
$(LIB_IP_CORDIC_ROTATOR24_OUTS) \
$(LIB_IP_CORDIC_ROTATE_INT24_INT16_OUTS) \
$(LIB_IP_CORDIC_ROTATE_INT24_OUTS) \
$(LIB_IP_DIVIDE_INT16_OUTS) \
$(LIB_IP_DIVIDE_INT24_OUTS) \
$(LIB_IP_DIVIDE_INT32_OUTS) \
$(LIB_IP_DIVIDE_UINT32_OUTS) \
$(LIB_IP_DIVIDE_INT16_INT32_OUTS) \
$(LIB_IP_COMPLEX_MULTIPLIER_DDS_OUTS) \
$(LIB_IP_DDS_SIN_COS_LUT_ONLY_OUTS)

lib_ip: $(LIB_IP_SYNTH_OUTPUTS)

.PHONY: lib_ip
