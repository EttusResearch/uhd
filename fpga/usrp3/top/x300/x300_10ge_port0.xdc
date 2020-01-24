#
# Copyright 2014 Ettus Research LLC
#

set_property LOC GTXE2_CHANNEL_X0Y0 [get_cells -hierarchical -filter {NAME =~ "*sfpp_io_i0/ten_gige_phy_i/*gtxe2_i*" && PRIMITIVE_TYPE == IO.gt.GTXE2_CHANNEL}]
set_property LOC GTXE2_COMMON_X0Y0  [get_cells -hierarchical -filter {NAME =~ "*sfpp_io_i0/ten_gige_phy_i/*gtxe2_common_0_i*" && PRIMITIVE_TYPE == IO.gt.GTXE2_COMMON}]
