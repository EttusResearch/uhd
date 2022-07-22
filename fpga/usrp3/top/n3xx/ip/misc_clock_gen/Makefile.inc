#
# Copyright 2014 Ettus Research
#

include $(TOOLS_DIR)/make/viv_ip_builder.mak

IP_MISC_CLOCK_GEN_SRCS = $(IP_BUILD_DIR)/misc_clock_gen/misc_clock_gen.xci

IP_MISC_CLOCK_GEN_OUTS = $(addprefix $(IP_BUILD_DIR)/misc_clock_gen/, \
misc_clock_gen.xci.out \
misc_clock_gen.v \
)

# We have to patch the XDC file to remove constraints on the source clock for the module
# All timing constraints are handled in one place (timing.xdc)

.INTERMEDIATE: IP_MISC_CLOCK_GEN_TRGT
$(IP_MISC_CLOCK_GEN_SRCS) $(IP_MISC_CLOCK_GEN_OUTS) : IP_MISC_CLOCK_GEN_TRGT
	@:

IP_MISC_CLOCK_GEN_TRGT: $(IP_DIR)/misc_clock_gen/misc_clock_gen.xci
	$(call BUILD_VIVADO_IP,misc_clock_gen,$(ARCH),$(PART_ID),$(IP_DIR),$(IP_BUILD_DIR),0)
