#
# Copyright 2026 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#  Placement constraints for X4xx based devices.
#
#  Restricting the placement of the RF modules per DB ensures a better compile
#  success rate through the reduced routing congestion. The naming here matches
#  RFNoC block names used within UHD's default FPGA yml files.
#

# move RF core 0, radio 0, ddc 0, duc0 to lower part of the FPGA die
create_pblock rf_path_0
add_cells_to_pblock rf_path_0 [get_cells [list {gen_rf_cores[0]*}]]
add_cells_to_pblock rf_path_0 [get_cells [list x4xx_core_i/rfnoc_image_core_i/*ddc0*]]
add_cells_to_pblock rf_path_0 [get_cells [list x4xx_core_i/rfnoc_image_core_i/*duc0*]]
add_cells_to_pblock rf_path_0 [get_cells [list x4xx_core_i/rfnoc_image_core_i/*radio0*]]
resize_pblock rf_path_0 -add {CLOCKREGION_X0Y0:CLOCKREGION_X5Y3}

# move RF core 1, radio 1, ddc 1, duc1 to upper part of the FPGA die
create_pblock rf_path_1
add_cells_to_pblock rf_path_1 [get_cells [list {gen_rf_cores[1]*}]]
add_cells_to_pblock rf_path_1 [get_cells [list x4xx_core_i/rfnoc_image_core_i/*ddc1*]]
add_cells_to_pblock rf_path_1 [get_cells [list x4xx_core_i/rfnoc_image_core_i/*duc1*]]
add_cells_to_pblock rf_path_1 [get_cells [list x4xx_core_i/rfnoc_image_core_i/*radio1*]]
resize_pblock rf_path_1 -add {CLOCKREGION_X0Y4:CLOCKREGION_X5Y7}
