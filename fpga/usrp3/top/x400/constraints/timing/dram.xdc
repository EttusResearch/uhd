#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#   DRAM timing constraints for X410.
#

set DramSysClockPeriod 9.996
set DramSysClockWave   {0.0 4.998}
create_clock -name Dram0SysClock -period $DramSysClockPeriod -waveform $DramSysClockWave [get_ports {DRAM0_REFCLK_P}]
create_clock -name Dram1SysClock -period $DramSysClockPeriod -waveform $DramSysClockWave [get_ports {DRAM1_REFCLK_P}]

## This calibration signal is a static signal. Once asserted it will stay HIGH.
## The multi cycle path constraint is added to improve timing.
set_multicycle_path -setup 8 -from [get_pins */inst/u_ddr4_mem_intfc/u_ddr_cal_top/calDone*/C]
set_multicycle_path -end -hold 7 -from [get_pins */inst/u_ddr4_mem_intfc/u_ddr_cal_top/calDone*/C]

## These signals once asserted, stay asserted for multiple clock cycles.
## False path constraint is added to improve the HOLD timing.
set_false_path -hold -to [get_pins */inst/*/*/*/*/*/*.u_xiphy_control/xiphy_control/RIU_ADDR*]
set_false_path -hold -to [get_pins */inst/*/*/*/*/*/*.u_xiphy_control/xiphy_control/RIU_WR_DATA*]

## Multi-cycle path constraints for Fabric - RIU clock domain crossing signals
set_max_delay 5.0  -datapath_only -from [get_pins */inst/*/*/*/u_ddr_cal_addr_decode/io_ready_lvl_reg/C]    -to [get_pins */inst/*/u_io_ready_lvl_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 5.0  -datapath_only -from [get_pins */inst/*/*/*/u_ddr_cal_addr_decode/io_read_data_reg[*]/C] -to [get_pins */inst/*/u_io_read_data_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 3.0  -datapath_only -from [get_pins */inst/*/*/*/phy_ready_riuclk_reg/C]                      -to [get_pins */inst/*/u_phy2clb_phy_ready_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 3.0  -datapath_only -from [get_pins */inst/*/*/*/bisc_complete_riuclk_reg/C]                  -to [get_pins */inst/*/u_phy2clb_bisc_complete_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 3.0  -datapath_only -from [get_pins */inst/*/*/io_addr_strobe_lvl_riuclk_reg/C]               -to [get_pins */inst/*/u_io_addr_strobe_lvl_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 3.0  -datapath_only -from [get_pins */inst/*/*/io_write_strobe_riuclk_reg/C]                  -to [get_pins */inst/*/u_io_write_strobe_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 3.0  -datapath_only -from [get_pins */inst/*/*/io_address_riuclk_reg[*]/C]                    -to [get_pins */inst/*/u_io_addr_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 3.0  -datapath_only -from [get_pins */inst/*/*/io_write_data_riuclk_reg[*]/C]                 -to [get_pins */inst/*/u_io_write_data_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 10.0 -datapath_only -from [get_pins */inst/*/en_vtc_in_reg/C]                                 -to [get_pins */inst/*/u_en_vtc_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 10.0 -datapath_only -from [get_pins */inst/*/*/riu2clb_valid_r1_riuclk_reg[*]/C]              -to [get_pins */inst/*/u_riu2clb_valid_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 10.0 -datapath_only -from [get_pins */inst/*/*/*/phy2clb_fixdly_rdy_low_riuclk_int_reg[*]/C]  -to [get_pins */inst/*/u_phy2clb_fixdly_rdy_low/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 10.0 -datapath_only -from [get_pins */inst/*/*/*/phy2clb_fixdly_rdy_upp_riuclk_int_reg[*]/C]  -to [get_pins */inst/*/u_phy2clb_fixdly_rdy_upp/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 10.0 -datapath_only -from [get_pins */inst/*/*/*/phy2clb_phy_rdy_low_riuclk_int_reg[*]/C]     -to [get_pins */inst/*/u_phy2clb_phy_rdy_low/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 10.0 -datapath_only -from [get_pins */inst/*/*/*/phy2clb_phy_rdy_upp_riuclk_int_reg[*]/C]     -to [get_pins */inst/*/u_phy2clb_phy_rdy_upp/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 10.0 -datapath_only -from [get_pins */inst/*/rst_r1_reg/C]                                    -to [get_pins */inst/*/u_fab_rst_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 3.0  -datapath_only -from [get_pins */inst/*/*/*/clb2phy_t_b_addr_riuclk_reg/C]               -to [get_pins */inst/*/*/*/clb2phy_t_b_addr_i_reg[0]/D]
set_max_delay 3.0  -datapath_only -from [get_pins */inst/*/*/*/*/slave_en_lvl_reg/C]                        -to [get_pins */inst/*/*/*/*/u_slave_en_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 3.0  -datapath_only -from [get_pins */inst/*/*/*/*/slave_we_r_reg/C]                          -to [get_pins */inst/*/*/*/*/u_slave_we_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 3.0  -datapath_only -from [get_pins */inst/*/*/*/*/slave_addr_r_reg[*]/C]                     -to [get_pins */inst/*/*/*/*/u_slave_addr_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 3.0  -datapath_only -from [get_pins */inst/*/*/*/*/slave_di_r_reg[*]/C]                       -to [get_pins */inst/*/*/*/*/u_slave_di_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 3.0  -datapath_only -from [get_pins */inst/*/*/*/*/slave_rdy_cptd_sclk_reg/C]                 -to [get_pins */inst/*/*/*/*/u_slave_rdy_cptd_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 12.0 -datapath_only -from [get_pins */inst/*/*/*/*/slave_rdy_lvl_fclk_reg/C]                  -to [get_pins */inst/*/*/*/*/u_slave_rdy_sync/SYNC[*].sync_reg_reg[0]/D]
set_max_delay 12.0 -datapath_only -from [get_pins */inst/*/*/*/*/slave_do_fclk_reg[*]/C]                    -to [get_pins */inst/*/*/*/*/u_slave_do_sync/SYNC[*].sync_reg_reg[0]/D]
set_false_path -through [get_pins */inst/u_ddr4_infrastructure/sys_rst]
set_false_path -from    [get_pins  */inst/*/input_rst_design_reg/C]  -to [get_pins */inst/*/rst_div_sync_r_reg[0]/D]
set_false_path -from    [get_pins  */inst/*/input_rst_design_reg/C]  -to [get_pins */inst/*/rst_riu_sync_r_reg[0]/D]
set_false_path -from    [get_pins  */inst/*/input_rst_design_reg/C]  -to [get_pins */inst/*/rst_mb_sync_r_reg[0]/D]
set_false_path -from    [get_pins  */inst/*/rst_async_riu_div_reg/C] -to [get_pins */inst/*/rst_div_sync_r_reg[0]/D]
set_false_path -from    [get_pins  */inst/*/rst_async_mb_reg/C]      -to [get_pins */inst/*/rst_mb_sync_r_reg[0]/D]
set_false_path -from    [get_pins  */inst/*/rst_async_riu_div_reg/C] -to [get_pins */inst/*/rst_riu_sync_r_reg[0]/D]
