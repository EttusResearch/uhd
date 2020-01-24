# Hierarchical cell: tx
proc create_hier_cell_tx_dma { parentCell nameHier numPorts } {

  if { $parentCell eq "" || $nameHier eq "" || $numPorts eq "" } {
     puts "ERROR: create_hier_cell_tx() - Empty argument(s)!"
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
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 M_AXIS_DMA
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_TX_DMA
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_tx_dmac

  create_bd_pin -dir I bus_clk
  create_bd_pin -dir I bus_rstn
  create_bd_pin -dir I clk40
  create_bd_pin -dir I clk40_rstn
  create_bd_pin -dir O -from [expr $numPorts - 1] -to 0 irq

  #########################
  # Instantiate IPs
  #########################
  set axi_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_0 ]
  set_property -dict [ list \
     CONFIG.NUM_MI $numPorts \
 ] $axi_interconnect_0

  set axi_crossbar_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_crossbar:2.1 axi_crossbar_0 ]
  set_property -dict [ list \
     CONFIG.NUM_MI {1} \
     CONFIG.NUM_SI $numPorts
 ] $axi_crossbar_0

  set axis_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_interconnect:2.1 axis_interconnect_0 ]
  set_property -dict [ list \
     CONFIG.ARB_ON_TLAST {1} \
     CONFIG.ARB_ON_MAX_XFERS {0} \
     CONFIG.ENABLE_ADVANCED_OPTIONS {1} \
     CONFIG.M00_HAS_REGSLICE {1} \
     CONFIG.NUM_MI {1} \
     CONFIG.NUM_SI $numPorts \
  ] $axis_interconnect_0

  set xlconcat_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0 ]
  set_property -dict [ list \
     CONFIG.NUM_PORTS $numPorts \
  ] $xlconcat_0

  set xlconstant_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_0 ]
  set_property -dict [ list \
CONFIG.CONST_VAL {0} \
 ] $xlconstant_0

  #########################
  # Wiring
  #########################
  connect_bd_net -net bus_clk \
     [get_bd_pins bus_clk] \
     [get_bd_pins axis_interconnect_0/ACLK] \
     [get_bd_pins axis_interconnect_0/M00_AXIS_ACLK]
  connect_bd_net -net bus_rstn \
     [get_bd_pins bus_rstn] \
     [get_bd_pins axis_interconnect_0/ARESETN] \
     [get_bd_pins axis_interconnect_0/M00_AXIS_ARESETN]
  connect_bd_net -net clk40 \
     [get_bd_pins clk40] \
     [get_bd_pins axi_crossbar_0/aclk] \
     [get_bd_pins axi_interconnect_0/ACLK] \
     [get_bd_pins axi_interconnect_0/S00_ACLK]
  connect_bd_net -net clk40_rstn \
     [get_bd_pins clk40_rstn] \
     [get_bd_pins axi_crossbar_0/aresetn] \
     [get_bd_pins axi_interconnect_0/ARESETN] \
     [get_bd_pins axi_interconnect_0/S00_ARESETN]

  connect_bd_net -net xlconstant_0_dout \
     [get_bd_pins xlconstant_0/dout]
  connect_bd_net -net xlconcat_0_dout \
     [get_bd_pins irq] \
     [get_bd_pins xlconcat_0/dout]

  connect_bd_intf_net -intf_net M_AXI_TX_DMAC_1 \
     [get_bd_intf_pins s_axi_tx_dmac] \
     [get_bd_intf_pins axi_interconnect_0/S00_AXI]
  connect_bd_intf_net -intf_net axi_crossbar_0_M00_AXI \
     [get_bd_intf_pins M_AXI_TX_DMA] \
     [get_bd_intf_pins axi_crossbar_0/M00_AXI]
  connect_bd_intf_net -intf_net axis_interconnect_0_M00_AXIS \
     [get_bd_intf_pins M_AXIS_DMA] \
     [get_bd_intf_pins axis_interconnect_0/M00_AXIS]

  #########################
  # Per-port Section
  #########################
  for {set i 0} {$i < $numPorts} {incr i} {
     # Configure each port on axi_crossbar and axis_interconnect
     puts "Creating TX dma port ${i}"
     set_property [format "CONFIG.S%02d_SINGLE_THREAD" ${i}] {1} $axi_crossbar_0
     set_property -dict [ list \
        [format "CONFIG.S%02d_HAS_REGSLICE" ${i}] {1} \
     ] $axis_interconnect_0

     set axi_tx_dmac [ create_bd_cell -type ip -vlnv analog.com:user:axi_dmac:1.0 axi_tx_dmac_$i ]
     set_property -dict [ list \
        CONFIG.DMA_TYPE_DEST {1} \
        CONFIG.DMA_TYPE_SRC {0} \
     ] $axi_tx_dmac

     # Add a tuser signal indicating which DMA channel originated the packet
     # Hard-coded to handle up to 16 DMA channels
     # Convert i (in decimal) to 4-bit binary:
     binary scan [binary format c ${i}] B* i_binary
     set i_binary [string range ${i_binary} end-3 end]

     set tuser_appender [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_subset_converter:1.1 axis_subset_converter_${i} ]
     set_property -dict [ list \
        CONFIG.M_TUSER_WIDTH.VALUE_SRC USER \
     ] $tuser_appender
     set_property -dict [ list \
        CONFIG.M_TUSER_WIDTH {4} \
        CONFIG.TUSER_REMAP 4'b${i_binary} \
     ] $tuser_appender

     connect_bd_intf_net -intf_net [format "axis_subset_converter_%d_S_AXIS" ${i}] \
        [get_bd_intf_pins $axi_tx_dmac/m_axis] \
        [get_bd_intf_pins ${tuser_appender}/S_AXIS]
     connect_bd_intf_net -intf_net [format "S%02d_AXIS_1" ${i}] \
        [get_bd_intf_pins ${tuser_appender}/M_AXIS] \
        [get_bd_intf_pins [format "axis_interconnect_0/S%02d_AXIS" ${i}]]
     connect_bd_intf_net -intf_net axi_dmac_${i}_m_src_axi \
        [get_bd_intf_pins [format "axi_crossbar_0/S%02d_AXI" ${i}]] \
        [get_bd_intf_pins $axi_tx_dmac/m_src_axi]
     connect_bd_intf_net -intf_net [format "axi_interconnect_0_M%02d_AXI" ${i}] \
        [get_bd_intf_pins [format "axi_interconnect_0/M%02d_AXI" ${i}]] \
        [get_bd_intf_pins $axi_tx_dmac/s_axi]

     connect_bd_net [get_bd_pins $axi_tx_dmac/irq] [get_bd_pins xlconcat_0/In${i}]

     connect_bd_net -net clk40 \
        [get_bd_pins [format "axi_interconnect_0/M%02d_ACLK" ${i}]]\
        [get_bd_pins $axi_tx_dmac/m_axis_aclk] \
        [get_bd_pins $axi_tx_dmac/m_src_axi_aclk] \
        [get_bd_pins $axi_tx_dmac/s_axi_aclk] \
	[get_bd_pins $tuser_appender/aclk] \
        [get_bd_pins [format "axis_interconnect_0/S%02d_AXIS_ACLK" ${i}]]

     connect_bd_net -net clk40_rstn \
        [get_bd_pins [format "axi_interconnect_0/M%02d_ARESETN" ${i}]] \
        [get_bd_pins $axi_tx_dmac/m_src_axi_aresetn] \
        [get_bd_pins $axi_tx_dmac/s_axi_aresetn] \
	[get_bd_pins $tuser_appender/aresetn] \
        [get_bd_pins [format "axis_interconnect_0/S%02d_AXIS_ARESETN" ${i}]]

     connect_bd_net -net xlconstant_0_dout \
        [get_bd_pins [format "axis_interconnect_0/S%02d_ARB_REQ_SUPPRESS" ${i}]]
  }

  # Restore current instance
  current_bd_instance $oldCurInst
}


