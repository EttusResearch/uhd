set scriptDir [file dirname [info script]]

source "$scriptDir/chdr_dma_rx.tcl"
source "$scriptDir/chdr_dma_tx.tcl"

# Hierarchical cell: dma
proc create_hier_cell_dma { parentCell nameHier numPorts } {

  if { $parentCell eq "" || $nameHier eq "" || $numPorts eq "" } {
     puts "ERROR: create_hier_cell_dma() - Empty argument(s)!"
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     puts "ERROR: Unable to find parent cell <$parentCell>!"
     return
  }

  if { $numPorts < 2 } {
     puts "ERROR: numPorts invalid: $numPorts"
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     puts "ERROR: Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  #########################
  # Pin list
  #########################
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 m_axis_dma
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_RX_DMA
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_TX_DMA
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 s_axis_dma
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_rx_dmac
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_tx_dmac
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_regfile

  create_bd_pin -dir I bus_clk
  create_bd_pin -dir I bus_rstn
  create_bd_pin -dir I clk40
  create_bd_pin -dir I clk40_rstn
  create_bd_pin -dir O rx_irq
  create_bd_pin -dir O tx_irq

  #########################
  # Instantiate IPs
  #########################
  # Create instance: rx
  create_hier_cell_rx_dma $hier_obj rx $numPorts

  # Create instance: tx
  create_hier_cell_tx_dma $hier_obj tx $numPorts

  # Used to set frame size of RX DMA engines
  set axi_regfile_0 [ create_bd_cell -type ip -vlnv ettus.com:ip:axi_regfile:1.0 axi_regfile_0 ]
  set_property -dict [ list \
CONFIG.NUM_REGS $numPorts \
 ] $axi_regfile_0

  set util_reduced_logic_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_reduced_logic:2.0 util_reduced_logic_0 ]
  set_property -dict [ list \
CONFIG.C_OPERATION {or} \
CONFIG.C_SIZE $numPorts \
 ] $util_reduced_logic_0

  set util_reduced_logic_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_reduced_logic:2.0 util_reduced_logic_1 ]
  set_property -dict [ list \
CONFIG.C_OPERATION {or} \
CONFIG.C_SIZE $numPorts \
 ] $util_reduced_logic_1

  #########################
  # Wiring
  #########################
  # Clocks and resets
  connect_bd_net -net bus_clk_1 \
     [get_bd_pins bus_clk] \
     [get_bd_pins rx/bus_clk] \
     [get_bd_pins tx/bus_clk]
  connect_bd_net -net bus_rstn_1 \
     [get_bd_pins bus_rstn] \
     [get_bd_pins rx/bus_rstn] \
     [get_bd_pins tx/bus_rstn]
  connect_bd_net -net clk40_1 \
     [get_bd_pins clk40] \
     [get_bd_pins rx/clk40] \
     [get_bd_pins tx/clk40] \
     [get_bd_pins axi_regfile_0/S_AXI_ACLK]
  connect_bd_net -net clk40_rstn_1 \
     [get_bd_pins clk40_rstn] \
     [get_bd_pins axi_regfile_0/S_AXI_ARESETN] \
     [get_bd_pins rx/clk40_rstn] \
     [get_bd_pins tx/clk40_rstn]

  # AXI buses
  connect_bd_intf_net -intf_net s_axi_rx_dmac_1 \
     [get_bd_intf_pins s_axi_rx_dmac] \
     [get_bd_intf_pins rx/s_axi_rx_dmac]
  connect_bd_intf_net -intf_net rx_dma_M_AXI_RX_DMA \
     [get_bd_intf_pins M_AXI_RX_DMA] \
     [get_bd_intf_pins rx/M_AXI_RX_DMA]
  connect_bd_intf_net -intf_net s_axi_tx_dmac_1 \
     [get_bd_intf_pins s_axi_tx_dmac] \
     [get_bd_intf_pins tx/s_axi_tx_dmac]
  connect_bd_intf_net -intf_net tx_M_AXI_TX_DMA \
     [get_bd_intf_pins M_AXI_TX_DMA] \
     [get_bd_intf_pins tx/M_AXI_TX_DMA]
  connect_bd_intf_net -intf_net s_axi_regfile_1 \
     [get_bd_intf_pins s_axi_regfile] \
     [get_bd_intf_pins axi_regfile_0/S_AXI]

  # RX CHDR
  connect_bd_intf_net -intf_net s_axis_dma_1 \
     [get_bd_intf_pins s_axis_dma] \
     [get_bd_intf_pins rx/S_AXIS_DMA]

  # TX CHDR
  connect_bd_intf_net -intf_net m_axis_dma_1 \
     [get_bd_intf_pins tx/M_AXIS_DMA] \
     [get_bd_intf_pins m_axis_dma]

  # IRQs and Frame Sizes
  connect_bd_net -net frame_sizes \
     [get_bd_pins axi_regfile_0/regs] \
     [get_bd_pins rx/mtu_regs]
  connect_bd_net -net rx_irq1 \
     [get_bd_pins rx/irq] \
     [get_bd_pins util_reduced_logic_0/Op1]
  connect_bd_net -net tx_irq1 \
     [get_bd_pins tx/irq] \
     [get_bd_pins util_reduced_logic_1/Op1]
  connect_bd_net -net util_reduced_logic_0_Res \
     [get_bd_pins rx_irq] \
     [get_bd_pins util_reduced_logic_0/Res]
  connect_bd_net -net util_reduced_logic_1_Res \
     [get_bd_pins tx_irq] \
     [get_bd_pins util_reduced_logic_1/Res]

  # Restore current instance
  current_bd_instance $oldCurInst
}


