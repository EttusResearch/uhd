#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   Shared constants for the X410 FPGA and the ZBX CPLD.
#

# These output delays are more or less arbitrarily chosen, but they dictate the
# delays at the input of the DB CPLD.
# Since the CPLD is built in Quartus, the file extension is .sdc.
set synth_sync_hold_requirement 1.0
set synth_sync_setup_requirement 8.5

# A MC100EPT23 clock translator is placed on the daughterboard to convert
# pll_ref_clk clock IO Standard to 3.3V. The minimum and maximum delays across
# this part are 1.1 ns and 1.8 ns, respectively.
# https://www.onsemi.com/pub/Collateral/MC100EPT23-D.PDF
set clock_translate_min 1.1
set clock_translate_max 1.8

## DB GPIO interface.
# The following values have been adjusted after verifying the slack on input
# and output path for the Post-synthesized designs for FPGA (Vivado) and CPLD
# (Quartus). Values are chosen as those that provide the most slack for FPGA
# timing, while still meeting timing on the CPLD.

# FPGA output constraints
set db_gpio_fpga_min_out  0.000
set db_gpio_fpga_max_out  3.250

# CPLD output constraints
set db_gpio_cpld_min_out -2.000
set db_gpio_cpld_max_out  8.500

# The longest traces on the DB GPIO interface.
# In order to compute the db_gpio_board_max_delay, the longest GPIO trace from
# the FPGA to any of the DB connector was simulated, and its worst-case value
# was added to the worst-case value obtained while simulating the longest GPIO
# trace on the DB. Both directions (FPGA <-> CPLD) were considered when
# computing the longest propagation delays. Propagation through the Board to
# board connector is assumed shorter than the effect of having the edge
# propagation time twice, as well as adding the two longest traces, instead of
# the longest corresponding pair.
#   Longest trace     | Trace length | Worst-case delay
# MB: DB0_GPIO[19]    |    4.25 in   |   912 ps
# DB: MB_FPGA_GPIO_A8 |    3.83 in   |   862 ps
set db_gpio_board_max_delay 1.774

# The shortest traces on the DB GPIO interface are (assuming 170.0 ps/in).
# A similar consideration was made as for db_gpio_board_max_delay, but using
# the shortest traces in each CCA.
# In this case, edge propagation was not taken into account, as the trace delay
# can act as the worst-case minimum.
#   Shorted trace     | Trace length | Trace delay
# MB: DB1_GPIO[8]     |    2.23 in   |   379 ps
# DB: MB_FPGA_GPIO_B12|    2.57 in   |   436 ps
set db_gpio_board_min_delay 0.815

# DB and FPGA both use PLL reference clock from a common clock chip.
# The common chip that drives these clocks is a LMK04832.
# There exist two additional clock buffers within the trace taking the clock to
# the daughterboard CPLD, a ADCLK944 located in the DB, and a MC100EPT23 near
# the CPLD which takes care of level translation.
# The largest delay that can exist between clocks exists when the LMK04832
# presents its max skew between outputs and when all clock buffers present
# their maximum delay.
# MC100EPT23 propagation delays will be handled separately in constraints due
# to their significant impact in timing closure, as well as to enable support
# for various corner-cases.
# Board delays were simulated in HyperLynx. Minimum delays considered time
# between start of edge at the source to the start of edge at destination.
# Maximum delays considered time between the start of an edge at the source and
# the settling of the edge at destination. Corner cases were considered for IC
# modeling.
#
# The clock path to the FPGA is composed by a single trace:
# - Minimum clock propagation delay to FPGA = 815 ps.
# - Maximum clock propagation delay to FPGA = 1.329 ps.
#
# The clock path to the CPLD (w/o MC100EPT23) is comprised by multiple traces:
# - Path from MB LMK04832 to DB connector
#       - Minimum delay : 1636 ps
# - Path from DB Connector to DB ADCLK944
#       - Minimum delay : 299 ps
# - Path from DB ADCLK944 to CPLD:
#       - Minimum delay : 197 ps
#
# The clock path to the CPLD (w/ MC100EPT23) is comprised of multiple traces:
# - Path from MB LMK04832 to DB connector
#       - Minimum delay : 1636 ps
#       - Maximum delay : 1890 ps
# - Path from DB Connector to DB ADCLK944
#       - Minimum delay : 299 ps
#       - Maximum delay : 397 ps
# - Path from DB ADCLK944 to MC100EPT23:
#       - Minimum delay : 120 ps
#       - Maximum delay : 411 ps
# - Path from MC100EPT23 to CPLD:
#       - Minimum delay : 365 ps
#       - Maximum delay : 847 ps
#
# The minimum and maximum delays for all traces can be added to account for the
# total board delay to the CPLD:
#
#  Max total Board delay = 1890 + 397 + 411 + 847 = 3545 ps
#
#  Min total Board delay = 1636 + 299 + 197       = 2132 ps

# This leaves the worst-case clock arrival values for the different ICs as:

#   Max total Board delay + (Max LMK Skew) + (Max DB ADCLK Prop)
#         3.545           +       .1       +      .130        = 3.775 ns,
set db_cpld_prc_clock_prop_max  3.775

#   Min total Board delay - (Max LMK Skew) + (Min DB ADCLK Prop)
#         2.132           -       .1       +      .07         = 2.102 ns,
set db_cpld_prc_clock_prop_min  2.102

#   Board delay   + (Max LMK Skew)
#       1.329     +      .1      =   1.429 ns
set fpga_prc_clock_prop_max     1.429

#   Board delay   - (Max LMK Skew)
#       .815      -      .1      =   0.715 ns
set fpga_prc_clock_prop_min     0.715

# When combining these values, the LMK Skew will be accounted for twice,
# which will just result in more conservative constraints.
