

#***********************************************************
# The following constraints target the Transceiver Physical*
# Interface which is instantiated in the Example Design.   *
#***********************************************************
#-----------------------------------------------------------
# Transceiver I/O placement:                               -
#-----------------------------------------------------------


# Place the transceiver components, chosen for this example design
# *** These values should be modified according to your specific design ***

#set_property LOC GTXE2_CHANNEL_X0Y1 [get_cells */*/*/transceiver_inst/gtwizard_inst/*/gtwizard_i/gt0_GTWIZARD_i/gtxe2_i]


#-----------------------------------------------------------
# Clock source used for the IDELAY Controller (if present) -
# and for the transceiver reset circuitry                  -
#-----------------------------------------------------------


create_clock -name independent_clock -period 5.000 [get_ports independent_clock]

#-----------------------------------------------------------
# PCS/PMA Clock period Constraints: please do not relax    -
#-----------------------------------------------------------

create_clock -add -name gtrefclk -period 8.000 [get_ports gtrefclk_p]


#-----------------------------------------------------------
# Transceiver I/O placement:                               -
#-----------------------------------------------------------

# Place the transceiver components, chosen for this example design
# *** These values should be modified according to your specific design ***

#set_property LOC H6 [get_ports gtrefclk_p]
#set_property LOC H5 [get_ports gtrefclk_n]


#***********************************************************
# The following constraints target the GMII implemented in *
# the Example Design.                                      *
#***********************************************************
# If the GMII is intended to be an internal interface,     *
# the GMII signals can be connected directly to user       *
# logic and all of the following constraints in this file  *
# should be removed.                                       *
#                                                          *
# If the GMII is intended to be an external interface,     *
# all of the following constraints in this file should be  *
# maintained.                                              *
#***********************************************************

#-----------------------------------------------------------
# GMII IOSTANDARD Constraints: please select an I/O        -
# Standard (LVTTL is suggested).                           -
#-----------------------------------------------------------

# Please update the IOSTANDARD according to that available in the device

#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_txd[0]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_txd[1]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_txd[2]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_txd[3]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_txd[4]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_txd[5]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_txd[6]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_txd[7]}]
#set_property IOSTANDARD LVCMOS33 [get_ports gmii_tx_en]
#set_property IOSTANDARD LVCMOS33 [get_ports gmii_tx_er]

#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_rxd[0]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_rxd[1]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_rxd[2]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_rxd[3]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_rxd[4]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_rxd[5]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_rxd[6]}]
#set_property IOSTANDARD LVCMOS33 [get_ports {gmii_rxd[7]}]
#set_property IOSTANDARD LVCMOS33 [get_ports gmii_rx_dv]
#set_property IOSTANDARD LVCMOS33 [get_ports gmii_rx_er]

#set_property IOSTANDARD LVCMOS33 [get_ports gmii_tx_clk]
#set_property IOSTANDARD LVCMOS33 [get_ports gmii_rx_clk]

#-----------------------------------------------------------
# Lock down the GMII Tx signals to the same bank for low   -
# skew.  This is an example placement only.                -
#-----------------------------------------------------------


#-----------------------------------------------------------
# To Adjust GMII Tx Input Setup/Hold Timing                -
#-----------------------------------------------------------
# These constraints will be set at a later date when device speed files have matured

#set_property IDELAY_VALUE 0 [get_cells delay_gmii_tx_en]
#set_property IDELAY_VALUE 0 [get_cells delay_gmii_tx_er]

#set_property IDELAY_VALUE 0 [get_cells {gmii_data_bus[7].delay_gmii_txd}]
#set_property IDELAY_VALUE 0 [get_cells {gmii_data_bus[6].delay_gmii_txd}]
#set_property IDELAY_VALUE 0 [get_cells {gmii_data_bus[5].delay_gmii_txd}]
#set_property IDELAY_VALUE 0 [get_cells {gmii_data_bus[4].delay_gmii_txd}]
#set_property IDELAY_VALUE 0 [get_cells {gmii_data_bus[3].delay_gmii_txd}]
#set_property IDELAY_VALUE 0 [get_cells {gmii_data_bus[2].delay_gmii_txd}]
#set_property IDELAY_VALUE 0 [get_cells {gmii_data_bus[1].delay_gmii_txd}]
#set_property IDELAY_VALUE 0 [get_cells {gmii_data_bus[0].delay_gmii_txd}]



#-----------------------------------------------------------
# To check (analyze) GMII Tx Input Setup/Hold Timing       -
#-----------------------------------------------------------

create_clock -name gmii_tx_clk -period 8.000 [get_ports gmii_tx_clk]



#-----------------------------------------------------------
# Fast Skew maximises output setup and hold timing         -
#-----------------------------------------------------------
set_property SLEW FAST [get_ports {gmii_rxd[*]}]
set_property SLEW FAST [get_ports gmii_rx_dv]
set_property SLEW FAST [get_ports gmii_rx_er]
set_property SLEW FAST [get_ports gmii_rx_clk]


#-----------------------------------------------------------
# GMII Transmitter Constraints:  place flip-flops in IOB   -
#-----------------------------------------------------------
#set_property IOB TRUE [get_cells gmii_txd_IBUF*]
#set_property IOB TRUE [get_cells gmii_tx_en_IBUF*]
#set_property IOB TRUE [get_cells gmii_tx_er_IBUF*]

#-----------------------------------------------------------
# GMII Receiver Constraints:  place flip-flops in IOB      -
#-----------------------------------------------------------
#set_property IOB TRUE [get_cells gmii_rxd_obuf_reg*]
#set_property IOB TRUE [get_cells gmii_rx_dv_obuf_reg]
#set_property IOB TRUE [get_cells gmii_rx_er_obuf_reg]



#-----------------------------------------------------------
# GMII Tx Elastic Buffer Constraints                       -
#-----------------------------------------------------------

# Control Gray Code delay and skew across clock boundary
set_false_path -to [get_pins -hier -filter {name =~ *tx_elastic_buffer_inst/reclock_rd_addrgray*/data_sync*/D}]
set_false_path -to [get_pins -hier -filter {name =~ *tx_elastic_buffer_inst/reclock_wr_addrgray*/data_sync*/D}]

# Constrain between Distributed Memory (output data) and the 1st set of flip-flops
set_false_path -from [get_clocks gmii_tx_clk] -to [get_pins -hier -filter {name =~ tx_elastic_buffer_inst/tx_en_fifo_reg1*/D}]
set_false_path -from [get_clocks gmii_tx_clk] -to [get_pins -hier -filter {name =~ tx_elastic_buffer_inst/tx_er_fifo_reg1*/D}]
set_false_path -from [get_clocks gmii_tx_clk] -to [get_pins -hier -filter {name =~ tx_elastic_buffer_inst/txd_fifo_reg1*/D}]

set_false_path -to [get_pins -hier -filter {name =~ *reset_sync*/PRE }]

set_false_path -to [get_pins -hier -filter {name =~ */core_resets_i/pma_reset_pipe_reg*/PRE}]
set_false_path -to [get_pins -hier -filter {name =~ */core_resets_i/pma_reset_pipe*[0]/D}]
