#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#
#   Common timing constraints for the X4xx's motherboard CPLD.
#

set_time_format -unit ns -decimal_places 3

#####################################################################
# General
#####################################################################
# For a couple of 3.3V interfaces the buffer SN74AVC4T774RSVR is used to
# increase the drive strength. For reuse we define the timings constants here.
# For direction A to B and B to A the maximum timing varies by 0.1 ns. Taking
# the maximum of both.
set buffer_prop_min 0.100
set buffer_prop_max 2.400

#####################################################################
# Main Clocks
#####################################################################
## Input clocks.
#    Reliable clock: 100.0 MHz
set CLK_100_period 10.000
create_clock -name CLK_100 -period $CLK_100_period [get_ports CLK_100]

# internal PLL derived clock
derive_pll_clocks
# provide name for derived clocks
set CLK_250 [get_clocks {*clk[1]}]

# PLL output pins of the generated 50 MHz clock for internal processing
set clk50_period 20.000
set pll_clk_out_pin [get_pins {pll_inst|altpll_component|auto_generated|pll1|clk[0]}]

set clk250_period 4.000

# PLL reference clock: 64 MHz (maximum)
set prc_clock_period 15.625
create_clock -name PLL_REF_CLK -period $prc_clock_period [get_ports PLL_REF_CLK]

#####################################################################
# Timing exceptions
#####################################################################
## SPI slaves
# Delay path for all synchronizers is based on the period of the
# faster clock domain (50 MHz derived by the PLL from 100 MHz reliable clock).
set clk50_period [expr {$CLK_100_period * 2}]

set_max_delay -to [get_registers *synchronizer_false_path\|value\[0\]\[*\]] \
              $clk50_period

# sclk data to CLK_100
set_max_delay -from [get_registers *spi_slave_async\|received_word\[*\]] \
              -to [get_registers *spi_slave_async\|data_out\[*\]] \
              $clk50_period
# PLL driven data to sclk
set_max_delay -from [get_clocks {pll_inst*}] \
              -to [get_registers *spi_slave_async\|transmit_bits\[*\]] \
              $clk50_period

#####################################################################
# Power supply clocks, LEDs, DIO direction
#####################################################################
# Change all output signals in this section within one clock period of the
# driving clocks.

# Power supply clocks
set power_supply_clocks_outputs [get_ports {PWR_SUPPLY_CLK_*}]
set_min_delay -to $power_supply_clocks_outputs 0
set_max_delay -to $power_supply_clocks_outputs $CLK_100_period

# LED signals in db specific sdc

# DIO direction
set dio_outputs [get_ports {DIO_DIRECTION_A[*] DIO_DIRECTION_B[*]}]
set_min_delay -to $dio_outputs 0
set_max_delay -to $dio_outputs $clk50_period

# Power control
set pwr_ctrl_outputs [get_ports {IPASS_POWER_DISABLE PWR_EN_5V_OSC_100 PWR_EN_5V_OSC_122_88}]
set_min_delay -to $pwr_ctrl_outputs 0
set_max_delay -to $pwr_ctrl_outputs $clk50_period

# Power fault inputs
# Virtual clocks for constraining inputs. Using an odd clock period to
# make sure any uncovered paths will result in timing errors due to short setup
# or hold path.
set power_fault_inputs [get_ports {IPASS_POWER_EN_FAULT[*]}]
create_clock -name virtual_async_in_clk -period 4.567
set_input_delay -clock virtual_async_in_clk 0 $power_fault_inputs

#####################################################################
# FPGA <-> MB CPLD PL SPI interface
#####################################################################
# Create clock for the PL's SPI interface.
# PRC at least divided by 2 by the SPI Master on FPGA
set pl_sclk_period [expr {2 * $prc_clock_period}]
create_clock -name pl_sclk -period $pl_sclk_period [get_registers mb_cpld_sclk]

# The SPI PL master (on the FPGA) is designed as a system synchronous
# interface using PLL_REF_CLK.
# The FPGA output constraints are required to calculate the windows
# at CPLD of valid data
# They are derived iteratively from the FPGA design ensuring a large
# valid data period.
set pl_spi_fpga_min_out  0.000
set pl_spi_fpga_max_out 11.000

# The longest trace on the PL SPI interface is (sssuming 170.0 ps/in)
#   Longest trace | Trace length | Trace delay
#       CS_0      |   7.143 in   |   1.215 ns
set pl_spi_board_delay 1.215

# This path also contains a level translator which has a typical
# switching time of 2.7 ns. Let's add a margin of 1 ns as worst
# case estimation
set pl_level_trans_delay 3.700

# CPLD and FPGA both use PLL reference clock from a common clock chip.
# The traces from that clock chip to the ICs are not length matched
# Assume a worst case clock difference of 0.5 ns at the IC inputs.
# There is no direction defined. The clock can arrive faster or slower
# on one IC.
set pl_clock_diff 0.500

set pl_slave_inputs [get_ports {PL_CPLD_SCLK PL_CPLD_MOSI PL_CPLD_CS_N[*]}]
# calculate output delays back from capturing edge, add board delay, level translator and clock difference
set_input_delay -clock PLL_REF_CLK \
  -max [expr {$prc_clock_period - $pl_spi_fpga_max_out + $pl_spi_board_delay + $pl_level_trans_delay + $pl_clock_diff}] \
  $pl_slave_inputs
# Assuming data is going without any delay, clock is arriving early at CPLD.
# Negate minimum output delay as it is defined from the change to the start clock edge.
set_input_delay -clock PLL_REF_CLK \
  -min [expr {- $pl_spi_fpga_min_out - $pl_clock_diff}] \
  $pl_slave_inputs

# ensure large data valid window for the FPGA
# those values are used in the FPGA / DB CPLDs
# to calculate the input delay
# those values are maximum integer values to still meet timing
set pl_spi_cpld_min_out -1.000
set pl_spi_cpld_max_out  8.000

set pl_slave_outputs [get_ports {PL_CPLD_MISO}]
set_output_delay -clock PLL_REF_CLK -max $pl_spi_cpld_max_out $pl_slave_outputs
set_output_delay -clock PLL_REF_CLK -min $pl_spi_cpld_min_out $pl_slave_outputs

#####################################################################
# FPGA <-> MB CPLD PS SPI interface
#####################################################################
# Assume the PS SPI clock is maximum 5 MHz.
# It is driven from another source and provided with the data.
set ps_sclk_period 200.000
create_clock -name ps_sclk -period $ps_sclk_period [get_ports PS_CPLD_SCLK]

# The SPI PS master (on the FPGA) is wired through the MIO (Multiplexed I/O)
# pins, meaning that the timing characteristics of the interface come from
# the controller itself (i.e. no timed routing through PL).
# Based on the SPI master controller specification (DS925: Table 48),
# one may define the min/max input/output delay constraints.
set ps_spi_tco_min    -2.000
set ps_spi_tco_max     5.000
set ps_spi_miso_setup -2.000
set ps_spi_miso_hold  [expr {0.3 * $ps_sclk_period}]

# Use the worst-case board propagation delays.
# Assuming 170.0 ps/in.
#   Longest trace | Trace length | Trace delay
#    CS0_n        |  4.735 in    |  0.805 ns
#  --------------------------------------------
set ps_spi_board_delay 0.805

set ps_slave_inputs [get_ports {PS_CPLD_MOSI PS_CPLD_CS_N[*]}]
# clock is immediately available, data is taking maximum time
# SPI data in CPOL=CPHA=1 is driven on the falling sclk edge
set ps_sclk_max_in_delay [expr {$ps_spi_tco_max + $ps_spi_board_delay}]
set_input_delay -clock ps_sclk -clock_fall \
  -max $ps_sclk_max_in_delay \
  $ps_slave_inputs
# fast data and clock delayed (reducing data delay)
set_input_delay -clock ps_sclk -clock_fall \
  -min [expr {$ps_spi_tco_min - $ps_spi_board_delay}] \
  $ps_slave_inputs

set ps_slave_outputs [get_ports {PS_CPLD_MISO}]
# use only half the frequency because falling edge is driving data
set_output_delay -clock ps_sclk \
  -max [expr {$ps_spi_miso_setup + 2*$ps_spi_board_delay}] \
  $ps_slave_outputs
# use hold requirement only as clock and data propagation further
# delay the signal
set_output_delay -clock ps_sclk \
  -min [expr {-$ps_spi_miso_hold}] \
  $ps_slave_outputs

# Chip select signals are captured for binary decoding in 250 MHz clock domain.
# To be able to specify a maximum delay for the data path only a second set of
# input delays is added to the root clock of the 250 MHz domain.
set_input_delay -add_delay -clock CLK_100 0 [get_ports {PS_CPLD_CS_N[*]}]
# Declare paths between the 2 clock domains as false paths
set_false_path -from [get_clocks {CLK_100}] -to [get_ports {PS_CPLD_MISO}]
set_false_path -from [get_clocks {ps_sclk}] -to [get_registers {synchronizer:ps_spi_input_sync_inst*}]
# Specify maximum data path delay
set_max_delay -from [get_ports {PS_CPLD_CS_N[*]}] -to $CLK_250 $clk250_period

#####################################################################
# MB CPLD PS SPI passthrough common
#####################################################################
###### Binary CS decoding ######
# The CS outputs for the external SPI slaves are driven from a 250 MHz clock to
# ensure glitch free switching after binary encoding. Additionally those signals
# have to meet the setup and hold requirements of the SPI slaves operating at
# ps_sclk (5 MHz). CS lines typically are asserted half a clock period of sclk
# before any active edge of sclk. The constraints below are using multi-cycle
# paths to provide the placer with information about the clock multiplier from
# ps_sclk to 250 MHz. Furthermore they incorporate the time required for
# decoding by lowering the clock multiplier as shown in the waveform below
# (multiplier is not shown correctly).
#
# ps_sclk          -\__________________________________________________/--------
# 250 MHz          _/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\_/-\-
# CS @ CPLD input  X>--------------- stable ------------------------------------
# CS @ CPLD output ---------<XXXXXXX>-------------- stable ---------------------
#                   |<----->| min decoding delay
#                           |<----->| change window
#           --------------->| SPI slave hold requirement
#       SPI slave setup requirement |<-------------------------------->|
#
# Get port to apply the multi-cycle constraint.
set binary_cs_ports [get_ports {LMK32_CS_N TPM_CS_N PHASE_DAC_CS_N CLK_DB_CS_N}]
# Determine number of full 250 MHz periods within half a period of ps_sclk.
set ps_spi_clock_divider [expr {int($ps_sclk_period/$clk250_period/2)}]
# Setup multi-cycle accounts for
# - one clock cycle data path delay from port to first register stage
# - one clock cycle to resolve meta-stability
# - up to 3 register stages internally (port to ps_cpld_cs_n_shift3)
# - one output register stage (registers on each $binary_cs_ports)
# Static timing analysis will take the data path from register to output port
# into account.
# The number of 250 MHz periods is reduced by a total of 7 clock cycles (listed
# above) to match the SPI slave setup requirement time shown in the waveform
# above. The slave's setup time in ps_sclk domain is specified below for each
# individual slave.
set ps_spi_setup_multicycle [expr {$ps_spi_clock_divider - 7}]
set_multicycle_path -setup -start -to $binary_cs_ports $ps_spi_setup_multicycle
# Hold multicycle accounts for
# - min 2 synchronization register stages internally (ps_cpld_cs_n_shift2)
#   (= min one clock cycle delay as data could arrive just before setup
#      requirement of first register stages assuming no data delay)
# - one output register stage
# Static timing analysis will take the data path from register to output port
# into account.
# As the clock edge for hold analysis is shifted with the setup edge the number
# of multi cycles has to be increased by this amount of cycles to get back to
# the falling edge of ps_sclk. Furthermore CS lines are released one half
# ps_sclk period after the last data transfer. So hold delay is increased by an
# additional half clock cycle.
set ps_spi_hold_multicycle [expr {$ps_spi_clock_divider + $ps_spi_setup_multicycle - 2}]
set_multicycle_path -hold  -start -to $binary_cs_ports $ps_spi_hold_multicycle
###### local SPI slave ######
# The chip select path for the MB CPLD itself is driven in the 250 MHz clock
# domain and captured by registers operating at ps_sclk. Therefore setting the
# path as false path preventing the placer from adding additional routing delay
# to ensure hold timing. The setup path is limited to a maximum extend of one
# clock period. As this path crosses clock domains clock propagation is included
# in this path during static timing analysis. The TCL analysis in
# scripts/ps_cs_analysis.tcl ensures a maximum value for data excluding the
# clocking network.
set_false_path -from [get_registers {ps_spi_cs_n_decoded[0]}] -hold
set_max_delay  -from [get_registers {ps_spi_cs_n_decoded[0]}] $clk250_period

###### LMK04832 ######
create_generated_clock -source [get_ports PS_CPLD_SCLK] \
  -name lmk_spi_sclk [get_ports LMK32_SCLK]

# Use the worst-case board propagation delays.
# Assuming 170.0 ps/in.
#   Longest trace | Trace length | Trace delay
#    LMK32_SCLK   |  8.259 in    |  1.404 ns
#  --------------------------------------------
set lmk_board_delay 1.404

# setup and hold dominated by CS <-> SCK relationship
set lmk_setup   20.000
set lmk_hold    20.000
set lmk_tco_max 60.000

set lmk_outputs [get_ports {LMK32_MOSI LMK32_CS_N}]
set_output_delay -clock lmk_spi_sclk \
  -max [expr {$lmk_setup + $lmk_board_delay + $buffer_prop_max}] \
  $lmk_outputs
set_output_delay -clock lmk_spi_sclk \
  -min [expr {-$lmk_hold - $lmk_board_delay - $buffer_prop_min}] \
  $lmk_outputs

set lmk_inputs [get_ports {LMK32_MISO}]
set_input_delay -clock lmk_spi_sclk -clock_fall \
  -max [expr {$lmk_tco_max + 2*$lmk_board_delay + 2*$buffer_prop_max}] \
  $lmk_inputs
set_input_delay -clock lmk_spi_sclk -clock_fall \
  -min [expr {2*$buffer_prop_min}] \
  $lmk_inputs

###### Phase DAC ######
create_generated_clock -source [get_ports PS_CPLD_SCLK] \
  -name phase_dac_spi_sclk [get_ports PHASE_DAC_SCLK]

# Use the worst-case board propagation delays.
# Assuming 170.0 ps/in.
#   Longest trace | Trace length | Trace delay
#    SpiDCs3v3_n  |  8.322 in    |  1.415 ns
#  --------------------------------------------
set phase_dac_board_delay 1.415

#setup dominated by SYNC signal
set phase_dac_setup 13.000
set phase_dac_hold   5.000

# device captures data on falling clock edge (CPOL = 1)
# constraining it as it would be like all the other SPI modules
# PS SPI master is responsible for changing SPI mode when talking
# to this device
set phase_dac_outputs [get_ports {PHASE_DAC_MOSI PHASE_DAC_CS_N}]
set_output_delay -clock phase_dac_spi_sclk -clock_fall \
  -max [expr {$phase_dac_setup + $phase_dac_board_delay}] \
  $phase_dac_outputs
set_output_delay -clock phase_dac_spi_sclk -clock_fall \
  -min [expr {-$phase_dac_hold - $phase_dac_board_delay}] \
  $phase_dac_outputs

###### TPM ######
create_generated_clock -source [get_ports PS_CPLD_SCLK] \
  -name tpm_spi_sclk [get_ports TPM_SCLK]

# Use the worst-case board propagation delays.
# Assuming 170.0 ps/in.
#   Longest trace | Trace length | Trace delay
#    TPM_CS_n     |  1.128 in    |  0.196 ns
#  --------------------------------------------
set tpm_board_delay 0.196

#tco dominated by NSS signal
set tpm_setup    5.000
set tpm_hold     5.000
set tpm_tco_max 25.000

set tpm_outputs [get_ports {TPM_MOSI TPM_CS_N}]
set_output_delay -clock tpm_spi_sclk \
  -max [expr {$tpm_setup + $tpm_board_delay}] \
  $tpm_outputs
set_output_delay -clock tpm_spi_sclk \
  -min [expr {-$tpm_hold - $tpm_board_delay}] \
  $tpm_outputs

set tpm_inputs [get_ports {TPM_MISO}]
set_input_delay -clock tpm_spi_sclk -clock_fall \
  -max [expr {$tpm_tco_max + 2*$tpm_board_delay}] \
  $tpm_inputs
set_input_delay -clock tpm_spi_sclk -clock_fall \
  -min 0 \
  $tpm_inputs

#### Clocking AUX board SPI interface ####
# Rev B clocking aux board uses a LMK05318 connected to this interface
# Using its timing for this interface.
create_generated_clock -source [get_ports PS_CPLD_SCLK] \
  -name clk_db_clk_out [get_ports CLK_DB_SCLK]
set clk_db_setup       10.000
set clk_db_hold        10.000
set clk_db_tco_max     20.000
# Just a worst case assumption based on 2 times the MB trace length CLK_DB_MOSI.
# The multiplier 2 accounts for any traces on the CLK AUX board.
set clk_db_board_delay 4.000

set clk_db_outputs [get_ports {CLK_DB_CS_N CLK_DB_MOSI}]
# Output signals have to stable for max setup and propagation time. Clock delay
# to device is expected to be 0 in this equation.
set_output_delay -clock clk_db_clk_out \
  -max [expr {$clk_db_setup + $clk_db_board_delay + $buffer_prop_max}] $clk_db_outputs
# The min output delay is comprised of:
# - device required hold time ($clk_db_hold)
# - max clock propagation delay ($clk_db_board_delay)
# - min data propagation time (0)
# All terms have to be negated as min output delay is defined in opposite
# direction (positive into the past).
set_output_delay -clock clk_db_clk_out \
  -min [expr {-$clk_db_hold - $clk_db_board_delay - $buffer_prop_min}] $clk_db_outputs

set clk_db_inputs [get_ports {CLK_DB_MISO}]
# Max delay calculated is based on
# - max clock delay ($clk_db_board_delay)
# - max clock to out LMK ($clk_db_tco_max)
# - max data path delay ($clk_db_board_delay)
set_input_delay -clock clk_db_clk_out -clock_fall \
  -max [expr {$clk_db_tco_max + $clk_db_board_delay*2 + 2*$buffer_prop_max}] $clk_db_inputs
# Min delay assumes clock propagates to device and data propagates to CPLD
# without any delays.
set_input_delay -clock clk_db_clk_out -clock_fall \
  -min [expr {2*$buffer_prop_min}] $clk_db_inputs


#####################################################################
# PCIe signals
#####################################################################
# I²C bus is operated at 100kHz. Constraints would not improve timing
# significantly (typically in the order of nanoseconds, which is negligible
# given the SCL period of 10 us).
# PCI-Express reset signal is not timing critical as it is received
# asynchronously by the FPGA.
set_false_path -to [get_ports {IPASS_SDA[0] IPASS_SCL[0] PCIE_RESET}]
# I²C inputs are only consumed by synchronizers.
# Add exceptions for all known consumers.
set_false_path -to [get_registers {PcieCmiWrapper:pcie_cmi_inst|PcieCmi:PcieCmix|UsfCablePort:UsfCablePortx|CablePort:CablePortx|I2cTop:CableI2cx|I2cMonitor:I2cMonitorx|I2cFilter:I2cFilterx|I2cSigFilter:SclFilterx|fSig_ms}]
set_false_path -to [get_registers {PcieCmiWrapper:pcie_cmi_inst|PcieCmi:PcieCmix|UsfCablePort:UsfCablePortx|CablePort:CablePortx|I2cTop:CableI2cx|I2cMonitor:I2cMonitorx|I2cFilter:I2cFilterx|I2cSigFilter:SdaFilterx|fSig_ms}]
set_false_path -to [get_registers {PcieCmiWrapper:pcie_cmi_inst|PcieCmi:PcieCmix|UsfCablePort:UsfCablePortx|CablePort:CablePortx|StuckBusFixer:StuckBusFixerx|DoubleSyncSlAsyncIn:DoubleSclkx|DoubleSyncAsyncInBase:DoubleSyncAsyncInBasex|DFlopAsync:oSig_msx|lpm_ff:LPM_FFx|dffs[0]}]
set_false_path -to [get_registers {PcieCmiWrapper:pcie_cmi_inst|PcieCmi:PcieCmix|UsfCablePort:UsfCablePortx|CablePort:CablePortx|StuckBusFixer:StuckBusFixerx|DoubleSyncSlAsyncIn:DoubleSdax|DoubleSyncAsyncInBase:DoubleSyncAsyncInBasex|DFlopAsync:oSig_msx|lpm_ff:LPM_FFx|dffs[0]}]

#####################################################################
# Known Issue of On-Chip Flash
#####################################################################
# see https://www.intel.com/content/www/us/en/programmable/support/support-resources/knowledge-base/tools/2016/warning--332060---node---alteraonchipflash-onchipflash-alteraonc.html
create_generated_clock -name flash_se_neg_reg \
 -source [get_pins { on_chip_flash:flash_inst|altera_onchip_flash:onchip_flash_0|altera_onchip_flash_avmm_data_controller:avmm_data_controller|flash_se_neg_reg|clk }] \
 -divide_by 2 [get_pins { on_chip_flash:flash_inst|altera_onchip_flash:onchip_flash_0|altera_onchip_flash_avmm_data_controller:avmm_data_controller|flash_se_neg_reg|q } ]

#####################################################################
# Clock uncertainty
#####################################################################
# Assign some uncertainty to all clocks
set clock_uncertainty 0.150
set_clock_uncertainty -to [get_clocks *] $clock_uncertainty
derive_clock_uncertainty
