#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#
#   Timing constants for the MB CPLD <-> DB CPLD SPI interface
#

# Delays are rounded to integer values which leave a slack of >1ns on each setup
# and hold path without requirement for adding hold delays (as reported
# by Quartus fitter report).
# The signal might change before the SCLK edge as the internal
# registers are driven by PLL reference clock rather than the SPI clock used
# for the port timing constaints.
set db_cpld_spi_max_out  14.000
set db_cpld_spi_min_out   2.000
set db_cpld_spi_max_in    2.000
set db_cpld_spi_min_in   -2.000
