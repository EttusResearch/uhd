#
# Copyright 2024 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

# This subdir contains all the gateware
add_subdirectory(fpga/gain)
# Add one subdirectory for every type of RFNoC components you have YAML files
# for (e.g., blocks, transport_adapters, modules, ...)
add_subdirectory(blocks)
#add_subdirectory(modules)
#add_subdirectory(transport_adapters)
add_subdirectory(dts)
