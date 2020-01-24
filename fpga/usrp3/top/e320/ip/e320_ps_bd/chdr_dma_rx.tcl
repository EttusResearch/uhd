set scriptDir [file dirname [info script]]

source "$scriptDir/chdr_dma_frame_size.tcl"

proc create_hier_cell_rx_dma_channel { parentCell nameHier } {

  if { $parentCell eq "" || $nameHier eq "" } {
     puts "ERROR: create_hier_cell_dma() - Empty argument(s)!"
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     puts "ERROR: Unable to find parent cell <$parentCell>!"
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
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 S_AXIS
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_dest_axi
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi

  create_bd_pin -dir I -from 15 -to 0 frame_size
  create_bd_pin -dir O -type intr irq
  create_bd_pin -dir I -type rst m_dest_axi_aresetn
  create_bd_pin -dir I -type clk s_axi_aclk
  create_bd_pin -dir I -type rst s_axi_aresetn
  create_bd_pin -dir I -type clk s_axis_aclk

  #########################
  # Instantiate IPs
  #########################
  set reset_inv [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_vector_logic:2.0 reset_inv ]
  set_property -dict [ list \
     CONFIG.C_SIZE {1} \
     CONFIG.C_OPERATION {not} \
  ] $reset_inv

  set chdr_padder [ create_bd_cell -type module -reference chdr_pad_packet chdr_padder ]
  set_property -dict [ list \
     CONFIG.CHDR_W {64} \
  ] $chdr_padder
  set_property CONFIG.POLARITY ACTIVE_HIGH [get_bd_pins chdr_padder/rst]

  set axi_rx_dmac [ create_bd_cell -type ip -vlnv analog.com:user:axi_dmac:1.0 axi_rx_dmac ]
  set_property -dict [ list \
     CONFIG.ASYNC_CLK_DEST_REQ {true} \
     CONFIG.ASYNC_CLK_REQ_SRC {true} \
     CONFIG.ASYNC_CLK_SRC_DEST {false} \
     CONFIG.DMA_AXI_PROTOCOL_DEST {1} \
     CONFIG.DMA_TYPE_SRC {1} \
     CONFIG.SYNC_TRANSFER_START {false} \
  ] $axi_rx_dmac

  #########################
  # Wiring
  #########################

  # Top-level connections
  connect_bd_net -net aclk_1 \
     [get_bd_pins s_axis_aclk] \
     [get_bd_pins chdr_padder/clk] \
     [get_bd_pins axi_rx_dmac/m_dest_axi_aclk] \
     [get_bd_pins axi_rx_dmac/s_axis_aclk]
  connect_bd_net -net aresetn_1 \
     [get_bd_pins m_dest_axi_aresetn] \
     [get_bd_pins reset_inv/Op1] \
     [get_bd_pins axi_rx_dmac/m_dest_axi_aresetn]
  connect_bd_net -net areset_1 \
     [get_bd_pins reset_inv/Res] \
     [get_bd_pins chdr_padder/rst]
  connect_bd_net -net s_axi_aclk_1 \
     [get_bd_pins s_axi_aclk] \
     [get_bd_pins axi_rx_dmac/s_axi_aclk]
  connect_bd_net -net s_axi_aresetn_1 \
     [get_bd_pins s_axi_aresetn] \
     [get_bd_pins axi_rx_dmac/s_axi_aresetn]
  connect_bd_net -net axi_rx_dmac_irq \
     [get_bd_pins irq] \
     [get_bd_pins axi_rx_dmac/irq]
  connect_bd_net -net mtu \
     [get_bd_pins frame_size] \
     [get_bd_pins chdr_padder/len]

  # Control and DMA ports
  connect_bd_intf_net -intf_net axi_rx_dmac_s_axi \
     [get_bd_intf_pins s_axi] \
     [get_bd_intf_pins axi_rx_dmac/s_axi]
  connect_bd_intf_net -intf_net axi_rx_dmac_m_dest_axi \
     [get_bd_intf_pins m_dest_axi] \
     [get_bd_intf_pins axi_rx_dmac/m_dest_axi]

  # AXI-Stream ports
  connect_bd_intf_net -intf_net s_axis_dma \
     [get_bd_intf_pins S_AXIS] \
     [get_bd_intf_pins chdr_padder/s_axis]
  connect_bd_intf_net -intf_net s_axis_dma_padded \
     [get_bd_intf_pins chdr_padder/m_axis] \
     [get_bd_intf_pins axi_rx_dmac/s_axis]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: rx
proc create_hier_cell_rx_dma { parentCell nameHier numPorts } {

  if { $parentCell eq "" || $nameHier eq "" || $numPorts eq "" } {
     puts "ERROR: create_hier_cell_rx() - Empty argument(s)!"
     return
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     puts "ERROR: Unable to find parent cell <$parentCell>!"
     return
  }

  if { $numPorts < 1 } {
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
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_RX_DMA
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 S_AXIS_DMA
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_rx_dmac

  create_bd_pin -dir I bus_clk
  create_bd_pin -dir I bus_rstn
  create_bd_pin -dir I clk40
  create_bd_pin -dir I clk40_rstn
  create_bd_pin -dir O -from [expr $numPorts - 1] -to 0 irq
  create_bd_pin -dir I -from [expr $numPorts * 32 - 1] -to 0 mtu_regs
  #########################
  # Instantiate IPs
  #########################
  # For sharing one S_AXI_HP port across all RX DMA engines
  set axi_crossbar_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_crossbar:2.1 axi_crossbar_0 ]
  set_property -dict [ list \
     CONFIG.CONNECTIVITY_MODE {SASD} \
     CONFIG.NUM_MI {1} \
     CONFIG.NUM_SI $numPorts \
     CONFIG.R_REGISTER {1} \
  ] $axi_crossbar_0

  # For fanning out AXI-Lite bus to all RX DMA engines
  set axi_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_0 ]
  set_property -dict [ list \
     CONFIG.NUM_MI $numPorts \
  ] $axi_interconnect_0

  # Routes AXI-Stream to appropriate RX DMA engine
  set axis_switch_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_switch:1.1 axis_switch_0 ]
  set_property -dict [ list \
     CONFIG.DECODER_REG {1} \
     CONFIG.NUM_MI $numPorts \
     CONFIG.NUM_SI {1} \
  ] $axis_switch_0

  # Cross domains from incoming AXI-Stream to RX DMA engines domain
  # Note that the fifo_generator_0 is hard-coded to have 4 TDEST bits, so we
  # are limited to 16 RX DMA channels
  set fifo_generator_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:fifo_generator:13.2 fifo_generator_0 ]
  set_property -dict [ list \
     CONFIG.Clock_Type_AXI {Independent_Clock} \
     CONFIG.Empty_Threshold_Assert_Value_axis {1021} \
     CONFIG.Empty_Threshold_Assert_Value_rach {13} \
     CONFIG.Empty_Threshold_Assert_Value_rdch {1021} \
     CONFIG.Empty_Threshold_Assert_Value_wach {13} \
     CONFIG.Empty_Threshold_Assert_Value_wdch {1021} \
     CONFIG.Empty_Threshold_Assert_Value_wrch {13} \
     CONFIG.Enable_TLAST {true} \
     CONFIG.FIFO_Implementation_axis {Independent_Clocks_Block_RAM} \
     CONFIG.FIFO_Implementation_rach {Independent_Clocks_Distributed_RAM} \
     CONFIG.FIFO_Implementation_rdch {Independent_Clocks_Block_RAM} \
     CONFIG.FIFO_Implementation_wach {Independent_Clocks_Distributed_RAM} \
     CONFIG.FIFO_Implementation_wdch {Independent_Clocks_Block_RAM} \
     CONFIG.FIFO_Implementation_wrch {Independent_Clocks_Distributed_RAM} \
     CONFIG.Full_Flags_Reset_Value {1} \
     CONFIG.Full_Threshold_Assert_Value_axis {1023} \
     CONFIG.Full_Threshold_Assert_Value_rach {15} \
     CONFIG.Full_Threshold_Assert_Value_wach {15} \
     CONFIG.Full_Threshold_Assert_Value_wrch {15} \
     CONFIG.HAS_TKEEP {false} \
     CONFIG.INTERFACE_TYPE {AXI_STREAM} \
     CONFIG.Input_Depth_axis {1024} \
     CONFIG.Reset_Type {Asynchronous_Reset} \
     CONFIG.TDATA_NUM_BYTES {8} \
     CONFIG.TDEST_WIDTH {4} \
     CONFIG.TKEEP_WIDTH {0} \
     CONFIG.TSTRB_WIDTH {8} \
     CONFIG.TUSER_WIDTH {0} \
  ] $fifo_generator_0

  set rx_dmac_irq_concat [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 rx_dmac_irq_concat ]
  set_property -dict [ list \
     CONFIG.NUM_PORTS $numPorts \
  ] $rx_dmac_irq_concat

  create_hier_cell_mtu $hier_obj mtu $numPorts

  #########################
  # Wiring
  #########################
  connect_bd_intf_net -intf_net S00_AXIS_1 \
     [get_bd_intf_pins S_AXIS_DMA] \
     [get_bd_intf_pins fifo_generator_0/S_AXIS]

  connect_bd_intf_net -intf_net axi_crossbar_0_M00_AXI \
     [get_bd_intf_pins M_AXI_RX_DMA] \
     [get_bd_intf_pins axi_crossbar_0/M00_AXI]

  connect_bd_intf_net -intf_net fifo_generator_0_M_AXIS \
     [get_bd_intf_pins axis_switch_0/S00_AXIS] \
     [get_bd_intf_pins fifo_generator_0/M_AXIS]

  connect_bd_intf_net -intf_net s_axi_rx_dmac_1 \
     [get_bd_intf_pins s_axi_rx_dmac] \
     [get_bd_intf_pins axi_interconnect_0/S00_AXI]

  connect_bd_net -net aresetn_1 \
     [get_bd_pins bus_rstn] \
     [get_bd_pins fifo_generator_0/s_aresetn]
  connect_bd_net -net bus_clk \
     [get_bd_pins bus_clk] \
     [get_bd_pins fifo_generator_0/s_aclk]

  connect_bd_net -net clk40 \
     [get_bd_pins clk40] \
     [get_bd_pins axi_crossbar_0/aclk] \
     [get_bd_pins axi_interconnect_0/ACLK] \
     [get_bd_pins axi_interconnect_0/S00_ACLK] \
     [get_bd_pins axis_switch_0/aclk] \
     [get_bd_pins fifo_generator_0/m_aclk]

  connect_bd_net -net clk40_rstn \
     [get_bd_pins clk40_rstn] \
     [get_bd_pins axi_crossbar_0/aresetn] \
     [get_bd_pins axi_interconnect_0/ARESETN] \
     [get_bd_pins axi_interconnect_0/S00_ARESETN] \
     [get_bd_pins axis_switch_0/aresetn]

  connect_bd_net -net mtu_regs_1 \
     [get_bd_pins mtu_regs] \
     [get_bd_pins mtu/mtu_regs]

  connect_bd_net -net rx_dmac_irq_concat_dout \
     [get_bd_pins irq] \
     [get_bd_pins rx_dmac_irq_concat/dout]

  #########################
  # Per-port Section
  #########################
  for {set i 0} {$i < $numPorts} {incr i} {
     puts "Instantiating rx_dma port ${i}"
     create_hier_cell_rx_dma_channel $hier_obj dma$i

     set_property -dict [ list \
        [format "CONFIG.S%02d_SINGLE_THREAD" ${i}] {1} \
     ] $axi_crossbar_0

     connect_bd_intf_net -intf_net [format "axis_switch_0_M%02d_AXIS" ${i}] \
        [get_bd_intf_pins [format "axis_switch_0/M%02d_AXIS" ${i}]] \
        [get_bd_intf_pins dma${i}/S_AXIS]

     connect_bd_intf_net -intf_net [format "axi_interconnect_0_M%02d_AXI" ${i}] \
        [get_bd_intf_pins [format "axi_interconnect_0/M%02d_AXI" ${i}]] \
        [get_bd_intf_pins dma${i}/s_axi]

     connect_bd_intf_net -intf_net dma${i}_m_dest_axi \
        [get_bd_intf_pins [format "axi_crossbar_0/S%02d_AXI" ${i}]] \
        [get_bd_intf_pins dma${i}/m_dest_axi]

     connect_bd_net -net clk40 \
        [get_bd_pins [format "axi_interconnect_0/M%02d_ACLK" ${i}]] \
        [get_bd_pins dma${i}/s_axi_aclk] \
        [get_bd_pins dma${i}/s_axis_aclk]

     connect_bd_net -net clk40_rstn \
        [get_bd_pins [format "axi_interconnect_0/M%02d_ARESETN" ${i}]] \
        [get_bd_pins dma${i}/m_dest_axi_aresetn] \
        [get_bd_pins dma${i}/s_axi_aresetn]

     connect_bd_net -net dma${i}_irq \
        [get_bd_pins dma${i}/irq] \
        [get_bd_pins rx_dmac_irq_concat/In${i}]

     connect_bd_net -net frame_size_${i} \
        [get_bd_pins dma${i}/frame_size] \
        [get_bd_pins mtu/mtu${i}]
  }

  # Restore current instance
  current_bd_instance $oldCurInst
}



