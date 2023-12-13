#
# Copyright 2021 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#
# Description:
#
#   Timing constraints for the x410's motherboard CPLD.
#


#####################################################################
# JTAG to daughterboards
#####################################################################
# Use the worst-case board propagation delays.
# Assuming 170.0 ps/in and usage of X410 DB.
#   Longest trace | Trace length | Trace delay
#    TDI to DB 0  |  7.625 in    |  1.296 ns
#  --------------------------------------------

# JTAG parameters
# see https://www.intel.com/content/www/us/en/programmable/documentation/mcn1397700832153.html#mcn1399899915639
set db_jtag_board_delay  1.296
set db_jtag_setup        3.000
set db_jtag_hold        10.000
set db_jtag_clk_to_out  20.000

set db0_jtag_outputs [get_ports {DB_JTAG_TDI[0] DB_JTAG_TMS[0]}]
set db0_jtag_inputs  [get_ports {DB_JTAG_TDO[0]}]
set db1_jtag_outputs [get_ports {DB_JTAG_TDI[1] DB_JTAG_TMS[1]}]
set db1_jtag_inputs  [get_ports {DB_JTAG_TDO[1]}]

##### DB 0 #####
# generated jtag clock is at least divided by 4
# max JTAG clock rate = 20 MHz
# source clock rate = 50 MHz
# only even dividers -> minimum value = 4
set db0_jtag_clk_register [get_registers {ctrlport_to_jtag:db0_jtag|bitq_fsm:jtag_master|bitq_state.HIGH}]
create_generated_clock -source $pll_clk_out_pin \
  -name db0_jtag_clk $db0_jtag_clk_register \
  -divide_by 4

# see White Rabbit DAC for futher explanation
set_false_path -from $db0_jtag_clk_register -to $db0_jtag_clk_register

create_generated_clock \
  -source $db0_jtag_clk_register \
  -name db0_jtag_out_clk [get_ports {DB_JTAG_TCK[0]}]

set_output_delay -clock db0_jtag_out_clk \
  -max [expr {$db_jtag_setup + $db_jtag_board_delay + $buffer_prop_max}] \
  $db0_jtag_outputs
set_output_delay -clock db0_jtag_out_clk \
  -min [expr {-$db_jtag_hold - $db_jtag_board_delay - $buffer_prop_min}] \
  $db0_jtag_outputs
# data is driven on CPLD on falling edge, which is 2 clock cycles ahead
# of the latch edge
set_multicycle_path -setup -start -to $db0_jtag_outputs 2
set_multicycle_path -hold  -start -to $db0_jtag_outputs 3

# maximum delay accounts for slow clock and data propagation as
# well as clock to out time
set_input_delay -clock_fall -clock db0_jtag_out_clk \
  -max [expr {$db_jtag_clk_to_out + 2*$db_jtag_board_delay + 2*$buffer_prop_max}] \
  $db0_jtag_inputs
# worst-case everything changes immediatelly
set_input_delay -clock_fall -clock db0_jtag_out_clk \
  -min [expr {2*$buffer_prop_min}] \
  $db0_jtag_inputs
set_multicycle_path -setup -end -from $db0_jtag_inputs 2
set_multicycle_path -hold  -end -from $db0_jtag_inputs 3

##### DB 1 #####
# generated jtag clock is at least divided by 4
set db1_jtag_clk_register [get_registers {ctrlport_to_jtag:db1_jtag|bitq_fsm:jtag_master|bitq_state.HIGH}]
create_generated_clock -source $pll_clk_out_pin \
  -name db1_jtag_clk $db1_jtag_clk_register \
  -divide_by 4
# see White Rabbit DAC for futher explanation
set_false_path -from $db1_jtag_clk_register -to $db1_jtag_clk_register
create_generated_clock \
  -source $db1_jtag_clk_register \
  -name db1_jtag_out_clk [get_ports {DB_JTAG_TCK[1]}]

set_output_delay -clock db1_jtag_out_clk \
  -max [expr {$db_jtag_setup + $db_jtag_board_delay + $buffer_prop_max}] \
  $db1_jtag_outputs
set_output_delay -clock db1_jtag_out_clk \
  -min [expr {-$db_jtag_hold - $db_jtag_board_delay - $buffer_prop_min}] \
  $db1_jtag_outputs
set_multicycle_path -setup -start -to $db1_jtag_outputs 2
set_multicycle_path -hold  -start -to $db1_jtag_outputs 3

# maximum delay accounts for slow clock and data propagation as
# well as clock to out time
set_input_delay -clock_fall -clock db1_jtag_out_clk \
  -max [expr {$db_jtag_clk_to_out + 2*$db_jtag_board_delay + 2*$buffer_prop_max}] \
  $db1_jtag_inputs
# ideally everything changes immediatelly
set_input_delay -clock_fall -clock db1_jtag_out_clk \
  -min [expr {2*$buffer_prop_min}] \
  $db1_jtag_inputs
set_multicycle_path -setup -end -from $db1_jtag_inputs 2
set_multicycle_path -hold  -end -from $db1_jtag_inputs 3



#####################################################################
# DB clock and reset
#####################################################################
# Output clocks for the daughterboards (SPI control)
create_generated_clock -source $pll_clk_out_pin \
  -name db0_ref_clk [get_ports {DB_REF_CLK[0]}]
create_generated_clock -source $pll_clk_out_pin \
  -name db1_ref_clk [get_ports {DB_REF_CLK[1]}]

# output reset within one clock period
set_max_delay -to [get_ports {DB_ARST[0] DB_ARST[1]}] $CLK_100_period
set_min_delay -to [get_ports {DB_ARST[0] DB_ARST[1]}] 0

#####################################################################
# DB SPI interfaces
#####################################################################
# ---------          -----------------          -----------------
#  FPGA   | CS/SCLK/ |    MB CPLD    |          |       DB      |
#         |-- MOSI ->|--------> R1 ->|--------->|               |
#  SPI    |          |               |          |  SPI          |
#  master |<- MISO --|<- R2 <--------|<---------|  slave        |
# ---------          -----------------          -----------------
#
# The output clocks are derived from the PLL reference clock (PRC). The SCLK
# edges are aligned with the rising edge of PLL reference clock. There are two
# registers R1 and R2 in the SPI path between FPGA and DB.
# For the transmission of data from master to slave those registers are
# transparent. The overall reception is just delayed by 1 PLL reference clock
# cycle. In the other direction the MISO timing is different. The falling edge
# of SCLK is used for changing the data signals. The propagation of this signal
# to the DB is delayed by 1 PLL reference clock period because of register R1.
# The MISO signal is captured on the rising edge of SCLK on the FPGA. Register
# R2 in the MB CPLD changes the timing in a way that MISO has to be stable on
# the rising edge of PLL reference clock before the SCLK rising edge.
# Additionally a minimum of two PLL reference clock cycles are required for
# processing in the SPI slave. The number of processing cycles is denoted by n.

# Here is an example for n=2 and SPI bus with CPHA=0 and CPOL=0.
# Data is driven on the falling edge and captured on the rising edge of the
# clock signal. The falling edge of the SCLK@DB is delayed by a clock cycle
# because of R1. The FPGA as SPI master is capturing the data on the rising edge
# of SCLK. The register R2 on the MB CPLD is capturing the data one clock cycle
# earlier. Therefore MISO has to be stable one clock cycle earlier then the
# original SCLK at the MB CPLD input. The effective SCLK signal to use for the
# timing constraints of the DB therefore has a low period which is reduced by 2
# clock cycles (R1 + R2) of PLL reference clock. It still has the same period as
# SCLK. In this example the low period would be 2 PRC cycles and the high period
# would be 6 PRC cycles.
# The following waveform illustrates the timing for n=2. Based on the defined
# delays <XXXX> denotes the time when the signal is not stable.
#
#                          <--- R1 --->|<-------- n=2 -------->|<--- R2 --->
# PRC                   ___/-----\_____/-----\_____/-----\_____/-----\_____/----
# SCLK                  ---\_______________________________________________/----
# SCLK @ DB (ideal)     ---------------\________________________________________
# SCLK @ DB (effective) ---------------\_______________________/----------------
# MOSI output @ MB CPLD --------------<XXXX>------------------------------------
# MISO input @ MB CPLD  -------------------------<XXXX>-------------------------
# DB propagation and processing             <--------->
#       MOSI change @ FPGA ^
#                MOSI change @ MB CPLD ^
#                                       MISO capture @ MB CPLD ^
#                                                      MISO capture @ FPGA ^
#
# Although the delays are defined based on PLL reference clock the SPI bus clock
# must be divided by at least n+2, where n>1 to be functional. Increase n in
# case the DB propagation and processing time does not fit into n PLL reference
# clock cycles taking the delays from below into account (see waveform above).
# Make sure you defined the SPI bus clock frequency for the slave to n*PLL clock
# period (effective SPI clock). Set the required SPI DB clock divider on the
# FPGA before starting data transfer.
#
# The constants for this interface are defined in db_spi_shared_constants.sdc

#### DB 0 ####
create_generated_clock -source [get_ports {PLL_REF_CLK}] \
  -name db0_ctrl_clk_int [get_registers {DB_CTRL_SCLK[0]~reg0}]
create_generated_clock -source [get_registers {DB_CTRL_SCLK[0]~reg0}] \
  -name db0_ctrl_clk [get_ports {DB_CTRL_SCLK[0]}]

set db0_ctrl_outputs [get_ports {DB_CTRL_MOSI[0] DB_CTRL_CS_N[0]}]
set_output_delay -clock db0_ctrl_clk -max $db_cpld_spi_max_out $db0_ctrl_outputs
set_output_delay -clock db0_ctrl_clk -min $db_cpld_spi_min_out $db0_ctrl_outputs

set db0_ctrl_inputs [get_ports {DB_CTRL_MISO[0]}]
set_input_delay -clock db0_ctrl_clk -max $db_cpld_spi_max_in $db0_ctrl_inputs
set_input_delay -clock db0_ctrl_clk -min $db_cpld_spi_min_in $db0_ctrl_inputs

#### DB 1 ####
create_generated_clock -source [get_ports {PLL_REF_CLK}] \
  -name db1_ctrl_clk_int [get_registers {DB_CTRL_SCLK[1]~reg0}]
create_generated_clock -source [get_registers {DB_CTRL_SCLK[1]~reg0}] \
  -name db1_ctrl_clk [get_ports DB_CTRL_SCLK[1]]

set db1_ctrl_outputs [get_ports {DB_CTRL_MOSI[1] DB_CTRL_CS_N[1]}]
set_output_delay -clock db1_ctrl_clk -max $db_cpld_spi_max_out $db1_ctrl_outputs
set_output_delay -clock db1_ctrl_clk -min $db_cpld_spi_min_out $db1_ctrl_outputs

set db1_ctrl_inputs [get_ports {DB_CTRL_MISO[1]}]
set_input_delay -clock db1_ctrl_clk -max $db_cpld_spi_max_in $db1_ctrl_inputs
set_input_delay -clock db1_ctrl_clk -min $db_cpld_spi_min_in $db1_ctrl_inputs

#####################################################################
# DB specific LED constraints
#####################################################################

# LED signals
set led_outputs [get_ports {QSFP0_LED_ACTIVE[*] QSFP0_LED_LINK[*] \
  QSFP1_LED_ACTIVE[*] QSFP1_LED_LINK[*]}]
set_min_delay -to $led_outputs 0
set_max_delay -to $led_outputs $prc_clock_period

#####################################################################
# MB CPLD PS SPI passthrough x410 specific
#####################################################################
#
# Get port to apply the multi-cycle constraint for x410 specific port.
# see common.sdc for more extensive doc and explanation
set binary_cs_ports_ti [get_ports {DB_CALEEPROM_CS_N[*]}]
set_multicycle_path -setup -start -to $binary_cs_ports_ti $ps_spi_setup_multicycle
set_multicycle_path -hold  -start -to $binary_cs_ports_ti $ps_spi_hold_multicycle

###### DB Calibration EEPROM ######
# Use worst case board propagation delays to estimate input and output
# timing. The longest path assuming 170 ps/in is:
# db0_caleeprom_spi_cs_n | 4.387 in | 0.746 ns
set eeprom_board_prop_delay 0.746
# Within the path to the EEPROM on the DB there is a level-transistor.
# The maximum propagation delays are 0.1..3.3 ns to the DB and 3.7 ns from the DB.
set eeprom_lvl_trans_to_db_delay_min 0.1
set eeprom_lvl_trans_to_db_delay_max 3.3
set eeprom_lvl_trans_from_db_delay_max 3.7
# Data in setup and hold times of the EEPROM are 5ns (based on the
# CS_N setup and hold times).
set db_eeprom_setup 5
set db_eeprom_hold 5
# Ouput valid from SCK is min 0 ns and max 8 ns.
set db_eeprom_output_valid 8

# max out path assuming clock delay is 0 and data delay is maximum value
set eeprom_max_out [expr {$eeprom_board_prop_delay + $eeprom_lvl_trans_to_db_delay_max + $db_eeprom_setup}]
# min out path assuming clock delay is maximal and data delay is 0
set eeprom_min_out [expr {-($eeprom_board_prop_delay + $eeprom_lvl_trans_to_db_delay_min + $db_eeprom_hold)}]
# board propagation to eeprom and back + lvl_translator back and forth + clock to data on eeprom
set eeprom_max_in  [expr {$eeprom_board_prop_delay*2 + $eeprom_lvl_trans_to_db_delay_max + $eeprom_lvl_trans_from_db_delay_max + $db_eeprom_output_valid}]
# assuming no delay for everything
set eeprom_min_in  0

### DB 0
create_generated_clock -source [get_ports PS_CPLD_SCLK] \
  -name db0_eeprom_clk [get_ports {DB_CALEEPROM_SCLK[0]}]

set db0_eeprom_outputs [get_ports {DB_CALEEPROM_MOSI[0] DB_CALEEPROM_CS_N[0]}]
set_output_delay -clock db0_eeprom_clk -max $eeprom_max_out $db0_eeprom_outputs
set_output_delay -clock db0_eeprom_clk -min $eeprom_min_out $db0_eeprom_outputs

set db0_eeprom_inputs [get_ports {DB_CALEEPROM_MISO[0]}]
# data is changed on the falling edge
set_input_delay -clock db0_eeprom_clk -clock_fall -max $eeprom_max_in $db0_eeprom_inputs
set_input_delay -clock db0_eeprom_clk -clock_fall -min $eeprom_min_in $db0_eeprom_inputs

### DB 1
create_generated_clock -source [get_ports PS_CPLD_SCLK] \
  -name db1_eeprom_clk [get_ports {DB_CALEEPROM_SCLK[1]}]

set db1_eeprom_outputs [get_ports {DB_CALEEPROM_MOSI[1] DB_CALEEPROM_CS_N[1]}]
set_output_delay -clock db1_eeprom_clk -max $eeprom_max_out $db1_eeprom_outputs
set_output_delay -clock db1_eeprom_clk -min $eeprom_min_out $db1_eeprom_outputs

set db1_eeprom_inputs [get_ports {DB_CALEEPROM_MISO[1]}]
# data is changed on the falling edge
set_input_delay -clock db1_eeprom_clk -clock_fall -max $eeprom_max_in $db1_eeprom_inputs
set_input_delay -clock db1_eeprom_clk -clock_fall -min $eeprom_min_in $db1_eeprom_inputs


