#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Module: ps_cs_analysis
#
# Description:
#
#   Analyze false path in PS SPI logic to ensure an upper delay boundary.
#

# get project to a working state
project_open -force "mb_cpld.qpf"
create_timing_netlist
update_timing_netlist

# Determine data path delay from MB CPLD chip select signal to MB CPLD internal
# SPI slave
set paths [report_path -from [get_registers {ps_spi_cs_n_decoded[0]}] -multi_corner]
set spiSlaveCsPathDelay [lindex $paths 1]

# clock period at 250 MHz (clock driving the decoding registers)
set maxDelay 4

# compare path from above with maximum delay
if ([expr {$maxDelay < $spiSlaveCsPathDelay}]) {
  puts "MB CPLD SPI CS line longer than expected."
  exit 1
}

exit 0
