#
# Copyright 2017 Ettus Research, A National Instruments Company
# SPDX-License-Identifier: LGPL-3.0
#


# Declare the 100 MHz DRAM subsystem clock.
create_clock -name ddr3_ext_refclk -period 10.0 [get_ports sys_clk_p]

# Rename IODELAY reference clock outputs from the MMCM inside MIG.
# These are based off the 200 MHz bus_clk.
create_generated_clock -name ddr3_iodelay_refclk  \
  [get_pins {u_ddr3_32bit/u_ddr3_32bit_mig/u_iodelay_ctrl/clk_ref_mmcm_gen.mmcm_i/CLKOUT1}]
create_generated_clock -name ddr3_iodelay_refclk_fb \
  [get_pins {u_ddr3_32bit/u_ddr3_32bit_mig/u_iodelay_ctrl/clk_ref_mmcm_gen.mmcm_i/CLKFBOUT}]

# Rename the UI clock appropriately. This clocks the slave AXI4 ports of the MIG as well
# as the AXI logic/interconnects in the remainder of the FPGA design. Should be 200 MHz.
create_generated_clock -name ddr3_ui_clk \
  [get_pins {u_ddr3_32bit/u_ddr3_32bit_mig/u_ddr3_infrastructure/gen_mmcm.mmcm_i/CLKFBOUT}]

# Another UI clock at 300 MHz... not quite at 2x due to timing.
create_generated_clock -name ddr3_ui_clk_2x \
  [get_pins {u_ddr3_32bit/u_ddr3_32bit_mig/u_ddr3_infrastructure/gen_mmcm.mmcm_i/CLKOUT0}]

# Phase shifting clock for the MIG. Drives a few resets.
create_generated_clock -name ddr3_ps_clk \
  [get_pins {u_ddr3_32bit/u_ddr3_32bit_mig/u_ddr3_infrastructure/gen_mmcm.mmcm_i/CLKOUT5}]



# No need for any asynchronous clock groups between bus_clk and the MIG UI clocks,
# because bus_clk already has a blanket asynchronous constraint from the top level XDC.