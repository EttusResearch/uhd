# CHANGE DESIGN NAME HERE
set design_name n310_ps_bd

# Creating design if needed
set errMsg ""
set nRet 0

set cur_design [current_bd_design -quiet]
set list_cells [get_bd_cells -quiet]

create_bd_design $design_name
current_bd_design $design_name

if { $nRet != 0 } {
   puts $errMsg
   return $nRet
}

set scriptDir [file dirname [info script]]

##################################################################
# DESIGN PROCs
##################################################################
source "$scriptDir/chdr_dma_top.tcl"

# Procedure to create entire design; Provide argument to make
# procedure reusable. If parentCell is "", will use root.
proc create_root_design { parentCell } {

  if { $parentCell eq "" } {
     set parentCell [get_bd_cells /]
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


  # Create interface ports
  set DDR [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:ddrx_rtl:1.0 DDR ]
  set GPIO_0 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 GPIO_0 ]
  set m_axis_dma [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 m_axis_dma ]
  set s_axis_dma [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 s_axis_dma ]
  set_property -dict [ list \
   CONFIG.HAS_TLAST 1 \
   CONFIG.TDATA_NUM_BYTES 8 \
   CONFIG.TDEST_WIDTH 4 \
  ] $s_axis_dma
  set WR_UART [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:uart_rtl:1.0 WR_UART ]
  set M_AXI_ETH_DMA0 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_ETH_DMA0 ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.DATA_WIDTH {32} \
CONFIG.FREQ_HZ {40000000} \
CONFIG.HAS_BURST {0} \
CONFIG.HAS_CACHE {0} \
CONFIG.HAS_LOCK {0} \
CONFIG.HAS_PROT {0} \
CONFIG.HAS_QOS {0} \
CONFIG.HAS_WSTRB {1} \
CONFIG.NUM_READ_OUTSTANDING {2} \
CONFIG.NUM_WRITE_OUTSTANDING {2} \
CONFIG.PROTOCOL {AXI4LITE} \
 ] $M_AXI_ETH_DMA0
  set M_AXI_WR [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_WR ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.DATA_WIDTH {32} \
CONFIG.FREQ_HZ {62500000} \
CONFIG.HAS_BURST {0} \
CONFIG.HAS_CACHE {0} \
CONFIG.HAS_LOCK {0} \
CONFIG.HAS_PROT {0} \
CONFIG.HAS_QOS {0} \
CONFIG.HAS_WSTRB {1} \
CONFIG.NUM_READ_OUTSTANDING {2} \
CONFIG.NUM_WRITE_OUTSTANDING {2} \
CONFIG.PROTOCOL {AXI4LITE} \
 ] $M_AXI_WR
  set M_AXI_ETH_DMA1 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_ETH_DMA1 ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.DATA_WIDTH {32} \
CONFIG.FREQ_HZ {40000000} \
CONFIG.HAS_BURST {0} \
CONFIG.HAS_CACHE {0} \
CONFIG.HAS_LOCK {0} \
CONFIG.HAS_PROT {0} \
CONFIG.HAS_QOS {0} \
CONFIG.HAS_WSTRB {1} \
CONFIG.NUM_READ_OUTSTANDING {2} \
CONFIG.NUM_WRITE_OUTSTANDING {2} \
CONFIG.PROTOCOL {AXI4LITE} \
 ] $M_AXI_ETH_DMA1
  set M_AXI_JESD0 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_JESD0 ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.DATA_WIDTH {32} \
CONFIG.FREQ_HZ {40000000} \
CONFIG.HAS_BURST {0} \
CONFIG.HAS_CACHE {0} \
CONFIG.HAS_LOCK {0} \
CONFIG.HAS_PROT {0} \
CONFIG.HAS_QOS {0} \
CONFIG.HAS_WSTRB {0} \
CONFIG.NUM_READ_OUTSTANDING {2} \
CONFIG.NUM_WRITE_OUTSTANDING {2} \
CONFIG.PROTOCOL {AXI4LITE} \
 ] $M_AXI_JESD0
  set M_AXI_JESD1 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_JESD1 ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.DATA_WIDTH {32} \
CONFIG.FREQ_HZ {40000000} \
CONFIG.HAS_BURST {0} \
CONFIG.HAS_CACHE {0} \
CONFIG.HAS_LOCK {0} \
CONFIG.HAS_PROT {0} \
CONFIG.HAS_QOS {0} \
CONFIG.HAS_WSTRB {0} \
CONFIG.NUM_READ_OUTSTANDING {2} \
CONFIG.NUM_WRITE_OUTSTANDING {2} \
CONFIG.PROTOCOL {AXI4LITE} \
 ] $M_AXI_JESD1
  set M_AXI_NET0 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_NET0 ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.DATA_WIDTH {32} \
CONFIG.FREQ_HZ {40000000} \
CONFIG.HAS_BURST {0} \
CONFIG.HAS_CACHE {0} \
CONFIG.HAS_LOCK {0} \
CONFIG.HAS_PROT {0} \
CONFIG.HAS_QOS {0} \
CONFIG.HAS_WSTRB {0} \
CONFIG.NUM_READ_OUTSTANDING {2} \
CONFIG.NUM_WRITE_OUTSTANDING {2} \
CONFIG.PROTOCOL {AXI4LITE} \
 ] $M_AXI_NET0
  set M_AXI_NET1 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_NET1 ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.DATA_WIDTH {32} \
CONFIG.FREQ_HZ {40000000} \
CONFIG.HAS_BURST {0} \
CONFIG.HAS_CACHE {0} \
CONFIG.HAS_LOCK {0} \
CONFIG.HAS_PROT {0} \
CONFIG.HAS_QOS {0} \
CONFIG.HAS_WSTRB {0} \
CONFIG.NUM_READ_OUTSTANDING {2} \
CONFIG.NUM_WRITE_OUTSTANDING {2} \
CONFIG.PROTOCOL {AXI4LITE} \
 ] $M_AXI_NET1
  set M_AXI_NET2 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_NET2 ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.DATA_WIDTH {32} \
CONFIG.FREQ_HZ {40000000} \
CONFIG.HAS_BURST {0} \
CONFIG.HAS_CACHE {0} \
CONFIG.HAS_LOCK {0} \
CONFIG.HAS_PROT {0} \
CONFIG.HAS_QOS {0} \
CONFIG.HAS_WSTRB {0} \
CONFIG.NUM_READ_OUTSTANDING {2} \
CONFIG.NUM_WRITE_OUTSTANDING {2} \
CONFIG.PROTOCOL {AXI4LITE} \
 ] $M_AXI_NET2
  set M_AXI_XBAR [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M_AXI_XBAR ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.DATA_WIDTH {32} \
CONFIG.FREQ_HZ {40000000} \
CONFIG.HAS_BURST {0} \
CONFIG.HAS_CACHE {0} \
CONFIG.HAS_LOCK {0} \
CONFIG.HAS_PROT {0} \
CONFIG.HAS_QOS {0} \
CONFIG.HAS_WSTRB {0} \
CONFIG.NUM_READ_OUTSTANDING {2} \
CONFIG.NUM_WRITE_OUTSTANDING {2} \
CONFIG.PROTOCOL {AXI4LITE} \
 ] $M_AXI_XBAR
  set S_AXI_GP0 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI_GP0 ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.ARUSER_WIDTH {0} \
CONFIG.AWUSER_WIDTH {0} \
CONFIG.BUSER_WIDTH {0} \
CONFIG.DATA_WIDTH {32} \
CONFIG.HAS_BRESP {1} \
CONFIG.HAS_BURST {1} \
CONFIG.HAS_CACHE {1} \
CONFIG.HAS_LOCK {1} \
CONFIG.HAS_PROT {1} \
CONFIG.HAS_QOS {1} \
CONFIG.HAS_REGION {0} \
CONFIG.HAS_RRESP {1} \
CONFIG.HAS_WSTRB {1} \
CONFIG.ID_WIDTH {5} \
CONFIG.MAX_BURST_LENGTH {16} \
CONFIG.NUM_READ_OUTSTANDING {8} \
CONFIG.NUM_WRITE_OUTSTANDING {8} \
CONFIG.PROTOCOL {AXI4} \
CONFIG.READ_WRITE_MODE {READ_WRITE} \
CONFIG.RUSER_WIDTH {0} \
CONFIG.SUPPORTS_NARROW_BURST {1} \
CONFIG.WUSER_WIDTH {0} \
 ] $S_AXI_GP0
  set S_AXI_GP1 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI_GP1 ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.ARUSER_WIDTH {0} \
CONFIG.AWUSER_WIDTH {0} \
CONFIG.BUSER_WIDTH {0} \
CONFIG.DATA_WIDTH {32} \
CONFIG.HAS_BRESP {1} \
CONFIG.HAS_BURST {1} \
CONFIG.HAS_CACHE {1} \
CONFIG.HAS_LOCK {1} \
CONFIG.HAS_PROT {1} \
CONFIG.HAS_QOS {1} \
CONFIG.HAS_REGION {0} \
CONFIG.HAS_RRESP {1} \
CONFIG.HAS_WSTRB {1} \
CONFIG.ID_WIDTH {5} \
CONFIG.MAX_BURST_LENGTH {16} \
CONFIG.NUM_READ_OUTSTANDING {8} \
CONFIG.NUM_WRITE_OUTSTANDING {8} \
CONFIG.PROTOCOL {AXI4} \
CONFIG.READ_WRITE_MODE {READ_WRITE} \
CONFIG.RUSER_WIDTH {0} \
CONFIG.SUPPORTS_NARROW_BURST {1} \
CONFIG.WUSER_WIDTH {0} \
 ] $S_AXI_GP1
  set S_AXI_HP0 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI_HP0 ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.ARUSER_WIDTH {0} \
CONFIG.AWUSER_WIDTH {0} \
CONFIG.BUSER_WIDTH {0} \
CONFIG.DATA_WIDTH {64} \
CONFIG.FREQ_HZ {40000000} \
CONFIG.HAS_BRESP {1} \
CONFIG.HAS_BURST {1} \
CONFIG.HAS_CACHE {1} \
CONFIG.HAS_LOCK {1} \
CONFIG.HAS_PROT {1} \
CONFIG.HAS_QOS {1} \
CONFIG.HAS_REGION {0} \
CONFIG.HAS_RRESP {1} \
CONFIG.HAS_WSTRB {1} \
CONFIG.ID_WIDTH {5} \
CONFIG.MAX_BURST_LENGTH {16} \
CONFIG.NUM_READ_OUTSTANDING {8} \
CONFIG.NUM_WRITE_OUTSTANDING {8} \
CONFIG.PROTOCOL {AXI4} \
CONFIG.READ_WRITE_MODE {READ_WRITE} \
CONFIG.RUSER_WIDTH {0} \
CONFIG.SUPPORTS_NARROW_BURST {1} \
CONFIG.WUSER_WIDTH {0} \
 ] $S_AXI_HP0
  set S_AXI_HP1 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI_HP1 ]
  set_property -dict [ list \
CONFIG.ADDR_WIDTH {32} \
CONFIG.ARUSER_WIDTH {0} \
CONFIG.AWUSER_WIDTH {0} \
CONFIG.BUSER_WIDTH {0} \
CONFIG.DATA_WIDTH {64} \
CONFIG.FREQ_HZ {40000000} \
CONFIG.HAS_BRESP {1} \
CONFIG.HAS_BURST {1} \
CONFIG.HAS_CACHE {1} \
CONFIG.HAS_LOCK {1} \
CONFIG.HAS_PROT {1} \
CONFIG.HAS_QOS {1} \
CONFIG.HAS_REGION {0} \
CONFIG.HAS_RRESP {1} \
CONFIG.HAS_WSTRB {1} \
CONFIG.ID_WIDTH {5} \
CONFIG.MAX_BURST_LENGTH {16} \
CONFIG.NUM_READ_OUTSTANDING {8} \
CONFIG.NUM_WRITE_OUTSTANDING {8} \
CONFIG.PROTOCOL {AXI4} \
CONFIG.READ_WRITE_MODE {READ_WRITE} \
CONFIG.RUSER_WIDTH {0} \
CONFIG.SUPPORTS_NARROW_BURST {1} \
CONFIG.WUSER_WIDTH {0} \
 ] $S_AXI_HP1
  set USBIND_0 [ create_bd_intf_port -mode Master -vlnv xilinx.com:display_processing_system7:usbctrl_rtl:1.0 USBIND_0 ]

  # Create ports
  set DDR_VRN [ create_bd_port -dir IO DDR_VRN ]
  set DDR_VRP [ create_bd_port -dir IO DDR_VRP ]
  set FCLK_CLK0 [ create_bd_port -dir O -type clk FCLK_CLK0 ]
  set FCLK_CLK1 [ create_bd_port -dir O -type clk FCLK_CLK1 ]
  set FCLK_CLK2 [ create_bd_port -dir O -type clk FCLK_CLK2 ]
  set FCLK_CLK3 [ create_bd_port -dir O -type clk FCLK_CLK3 ]
  set FCLK_RESET0_N [ create_bd_port -dir O -type rst FCLK_RESET0_N ]
  set FCLK_RESET1_N [ create_bd_port -dir O -type rst FCLK_RESET1_N ]
  set FCLK_RESET2_N [ create_bd_port -dir O -type rst FCLK_RESET2_N ]
  set FCLK_RESET3_N [ create_bd_port -dir O -type rst FCLK_RESET3_N ]
  set IRQ_F2P [ create_bd_port -dir I -from 15 -to 0 -type intr IRQ_F2P ]
  set_property -dict [ list \
CONFIG.PortWidth {16} \
CONFIG.SENSITIVITY {EDGE_RISING} \
 ] $IRQ_F2P
  set MIO [ create_bd_port -dir IO -from 53 -to 0 MIO ]
  set PS_CLK [ create_bd_port -dir IO PS_CLK ]
  set PS_PORB [ create_bd_port -dir IO PS_PORB ]
  set PS_SRSTB [ create_bd_port -dir IO PS_SRSTB ]
  set SPI0_MISO_I [ create_bd_port -dir I SPI0_MISO_I ]
  set SPI0_MISO_O [ create_bd_port -dir O SPI0_MISO_O ]
  set SPI0_MISO_T [ create_bd_port -dir O SPI0_MISO_T ]
  set SPI0_MOSI_I [ create_bd_port -dir I SPI0_MOSI_I ]
  set SPI0_MOSI_O [ create_bd_port -dir O SPI0_MOSI_O ]
  set SPI0_MOSI_T [ create_bd_port -dir O SPI0_MOSI_T ]
  set SPI0_SCLK_I [ create_bd_port -dir I SPI0_SCLK_I ]
  set SPI0_SCLK_O [ create_bd_port -dir O SPI0_SCLK_O ]
  set SPI0_SCLK_T [ create_bd_port -dir O SPI0_SCLK_T ]
  set SPI0_SS1_O [ create_bd_port -dir O SPI0_SS1_O ]
  set SPI0_SS2_O [ create_bd_port -dir O SPI0_SS2_O ]
  set SPI0_SS_I [ create_bd_port -dir I SPI0_SS_I ]
  set SPI0_SS_O [ create_bd_port -dir O SPI0_SS_O ]
  set SPI0_SS_T [ create_bd_port -dir O SPI0_SS_T ]
  set SPI1_MISO_I [ create_bd_port -dir I SPI1_MISO_I ]
  set SPI1_MISO_O [ create_bd_port -dir O SPI1_MISO_O ]
  set SPI1_MISO_T [ create_bd_port -dir O SPI1_MISO_T ]
  set SPI1_MOSI_I [ create_bd_port -dir I SPI1_MOSI_I ]
  set SPI1_MOSI_O [ create_bd_port -dir O SPI1_MOSI_O ]
  set SPI1_MOSI_T [ create_bd_port -dir O SPI1_MOSI_T ]
  set SPI1_SCLK_I [ create_bd_port -dir I SPI1_SCLK_I ]
  set SPI1_SCLK_O [ create_bd_port -dir O SPI1_SCLK_O ]
  set SPI1_SCLK_T [ create_bd_port -dir O SPI1_SCLK_T ]
  set SPI1_SS1_O [ create_bd_port -dir O SPI1_SS1_O ]
  set SPI1_SS2_O [ create_bd_port -dir O SPI1_SS2_O ]
  set SPI1_SS_I [ create_bd_port -dir I SPI1_SS_I ]
  set SPI1_SS_O [ create_bd_port -dir O SPI1_SS_O ]
  set SPI1_SS_T [ create_bd_port -dir O SPI1_SS_T ]
  set JTAG0_TCK [ create_bd_port -dir IO JTAG0_TCK ]
  set JTAG0_TMS [ create_bd_port -dir IO JTAG0_TMS ]
  set JTAG0_TDI [ create_bd_port -dir IO JTAG0_TDI ]
  set JTAG0_TDO [ create_bd_port -dir I  JTAG0_TDO ]
  set JTAG1_TCK [ create_bd_port -dir IO JTAG1_TCK ]
  set JTAG1_TMS [ create_bd_port -dir IO JTAG1_TMS ]
  set JTAG1_TDI [ create_bd_port -dir IO JTAG1_TDI ]
  set JTAG1_TDO [ create_bd_port -dir I  JTAG1_TDO ]
  set S_AXI_GP0_ACLK [ create_bd_port -dir I -type clk S_AXI_GP0_ACLK ]
  set_property -dict [ list \
CONFIG.ASSOCIATED_RESET {S_AXI_GP0_ARESETN} \
CONFIG.FREQ_HZ {40000000} \
 ] $S_AXI_GP0_ACLK
  set S_AXI_GP0_ARESETN [ create_bd_port -dir I -type rst S_AXI_GP0_ARESETN ]
  set S_AXI_GP1_ACLK [ create_bd_port -dir I -type clk S_AXI_GP1_ACLK ]
  set_property -dict [ list \
CONFIG.ASSOCIATED_RESET {S_AXI_GP1_ARESETN} \
CONFIG.FREQ_HZ {40000000} \
 ] $S_AXI_GP1_ACLK
  set S_AXI_GP1_ARESETN [ create_bd_port -dir I -type rst S_AXI_GP1_ARESETN ]
  set S_AXI_HP0_ACLK [ create_bd_port -dir I -type clk S_AXI_HP0_ACLK ]
  set_property -dict [ list \
CONFIG.FREQ_HZ {40000000} \
 ] $S_AXI_HP0_ACLK
  set S_AXI_HP0_ARESETN [ create_bd_port -dir I -type rst S_AXI_HP0_ARESETN ]
  set S_AXI_HP1_ACLK [ create_bd_port -dir I -type clk S_AXI_HP1_ACLK ]
  set_property -dict [ list \
CONFIG.ASSOCIATED_RESET {S_AXI_HP1_ARESETN} \
CONFIG.FREQ_HZ {40000000} \
 ] $S_AXI_HP1_ACLK
  set S_AXI_HP1_ARESETN [ create_bd_port -dir I -type rst S_AXI_HP1_ARESETN ]
  set bus_clk [ create_bd_port -dir I -type clk bus_clk ]
  set_property -dict [ list \
CONFIG.ASSOCIATED_BUSIF {m_axis_dma:s_axis_dma} \
CONFIG.ASSOCIATED_RESET {bus_rstn} \
CONFIG.FREQ_HZ {200000000} \
 ] $bus_clk
  set bus_rstn [ create_bd_port -dir I -type rst bus_rstn ]

  set M_AXI_WR_CLK [ create_bd_port -dir I -type clk M_AXI_WR_CLK ]
  set_property -dict [ list \
CONFIG.ASSOCIATED_BUSIF {M_AXI_WR} \
CONFIG.ASSOCIATED_RESET {M_AXI_WR_RSTn} \
CONFIG.FREQ_HZ {62500000} \
 ] $M_AXI_WR_CLK
  set M_AXI_WR_RSTn [ create_bd_port -dir I -type rst M_AXI_WR_RSTn ]

  set clk40 [ create_bd_port -dir I -type clk clk40 ]
  set_property -dict [ list \
CONFIG.ASSOCIATED_BUSIF {M_AXI_NET0:M_AXI_NET1:M_AXI_XBAR:M_AXI_JESD0:M_AXI_JESD1:M_AXI_ETH_DMA0:M_AXI_ETH_DMA1:M_AXI_NET2} \
CONFIG.ASSOCIATED_RESET {clk40_rstn} \
CONFIG.FREQ_HZ {40000000} \
 ] $clk40
  set clk40_rstn [ create_bd_port -dir I -type rst clk40_rstn ]

  create_bd_port -dir I qsfp_sda_i
  create_bd_port -dir O qsfp_sda_o
  create_bd_port -dir O qsfp_sda_t
  create_bd_port -dir I qsfp_scl_i
  create_bd_port -dir O qsfp_scl_o
  create_bd_port -dir O qsfp_scl_t

  # Create instance: axi_interconnect_hp0, and set properties
  set axi_interconnect_hp0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_hp0 ]
  set_property -dict [ list \
CONFIG.ENABLE_ADVANCED_OPTIONS {0} \
CONFIG.NUM_MI {1} \
CONFIG.NUM_SI {2} \
 ] $axi_interconnect_hp0

  set axi_interconnect_hp1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_hp1 ]
  set_property -dict [ list \
CONFIG.ENABLE_ADVANCED_OPTIONS {0} \
CONFIG.NUM_MI {1} \
CONFIG.NUM_SI {2} \
 ] $axi_interconnect_hp1

  # Create instance: axi_interconnect_0, and set properties
  set axi_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_0 ]
  set_property -dict [ list \
CONFIG.ENABLE_ADVANCED_OPTIONS {0} \
CONFIG.NUM_MI {16} \
 ] $axi_interconnect_0

  # Create instance: axi_uartlite_0, and set properties
  set axi_uartlite_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_uartlite:2.0 axi_uartlite_0 ]
  set_property -dict [ list \
CONFIG.C_BAUDRATE {115200} \
 ] $axi_uartlite_0

  # Create instance: dma
  create_hier_cell_dma [current_bd_instance .] dma 10

  # Create instance: processing_system7_0, and set properties
  set processing_system7_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 processing_system7_0 ]
  set_property -dict [ list \
CONFIG.PCW_APU_PERIPHERAL_FREQMHZ {800} \
CONFIG.PCW_ENET0_ENET0_IO {MIO 16 .. 27} \
CONFIG.PCW_ENET0_GRP_MDIO_ENABLE {1} \
CONFIG.PCW_ENET0_PERIPHERAL_ENABLE {1} \
CONFIG.PCW_ENET0_RESET_ENABLE {1} \
CONFIG.PCW_ENET0_RESET_IO {MIO 6} \
CONFIG.PCW_EN_CLK1_PORT {1} \
CONFIG.PCW_EN_CLK2_PORT {1} \
CONFIG.PCW_EN_CLK3_PORT {1} \
CONFIG.PCW_EN_RST1_PORT {1} \
CONFIG.PCW_EN_RST2_PORT {1} \
CONFIG.PCW_EN_RST3_PORT {1} \
CONFIG.PCW_FPGA0_PERIPHERAL_FREQMHZ {100} \
CONFIG.PCW_FPGA1_PERIPHERAL_FREQMHZ {40} \
CONFIG.PCW_FPGA2_PERIPHERAL_FREQMHZ {166.6667} \
CONFIG.PCW_FPGA3_PERIPHERAL_FREQMHZ {200} \
CONFIG.PCW_GPIO_EMIO_GPIO_ENABLE {1} \
CONFIG.PCW_GPIO_MIO_GPIO_ENABLE {1} \
CONFIG.PCW_GPIO_MIO_GPIO_IO {MIO} \
CONFIG.PCW_I2C0_I2C0_IO {MIO 50 .. 51} \
CONFIG.PCW_I2C0_PERIPHERAL_ENABLE {1} \
CONFIG.PCW_I2C0_RESET_ENABLE {1} \
CONFIG.PCW_I2C0_RESET_IO {MIO 3} \
CONFIG.PCW_IRQ_F2P_INTR {1} \
CONFIG.PCW_PJTAG_PERIPHERAL_ENABLE {1} \
CONFIG.PCW_PJTAG_PJTAG_IO {MIO 10 .. 13} \
CONFIG.PCW_PRESET_BANK0_VOLTAGE {LVCMOS 1.8V} \
CONFIG.PCW_PRESET_BANK1_VOLTAGE {LVCMOS 1.8V} \
CONFIG.PCW_SD0_PERIPHERAL_ENABLE {1} \
CONFIG.PCW_SPI0_PERIPHERAL_ENABLE {1} \
CONFIG.PCW_SPI1_PERIPHERAL_ENABLE {1} \
CONFIG.PCW_UART0_PERIPHERAL_ENABLE {1} \
CONFIG.PCW_UART0_UART0_IO {MIO 14 .. 15} \
CONFIG.PCW_UART1_PERIPHERAL_ENABLE {1} \
CONFIG.PCW_UART1_UART1_IO {MIO 8 .. 9} \
CONFIG.PCW_UIPARAM_DDR_BOARD_DELAY0 {0.096} \
CONFIG.PCW_UIPARAM_DDR_BOARD_DELAY1 {0.102} \
CONFIG.PCW_UIPARAM_DDR_BOARD_DELAY2 {0.100} \
CONFIG.PCW_UIPARAM_DDR_BOARD_DELAY3 {0.090} \
CONFIG.PCW_UIPARAM_DDR_DQS_TO_CLK_DELAY_0 {0.054} \
CONFIG.PCW_UIPARAM_DDR_DQS_TO_CLK_DELAY_1 {0.040} \
CONFIG.PCW_UIPARAM_DDR_DQS_TO_CLK_DELAY_2 {0.041} \
CONFIG.PCW_UIPARAM_DDR_DQS_TO_CLK_DELAY_3 {0.010} \
CONFIG.PCW_UIPARAM_DDR_PARTNO {MT41K256M16 RE-125} \
CONFIG.PCW_USB0_PERIPHERAL_ENABLE {1} \
CONFIG.PCW_USB0_RESET_ENABLE {1} \
CONFIG.PCW_USB0_RESET_IO {MIO 7} \
CONFIG.PCW_USE_FABRIC_INTERRUPT {1} \
CONFIG.PCW_USE_S_AXI_GP0 {0} \
CONFIG.PCW_USE_S_AXI_GP1 {0} \
CONFIG.PCW_USE_S_AXI_HP0 {1} \
CONFIG.PCW_USE_S_AXI_HP1 {1} \
CONFIG.PCW_USE_S_AXI_HP2 {1} \
CONFIG.PCW_USE_S_AXI_HP3 {1} \
 ] $processing_system7_0

  # Create instance: xlconcat_0, and set properties
  set xlconcat_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0 ]
  set_property -dict [ list \
CONFIG.IN0_WIDTH {8} \
CONFIG.NUM_PORTS {9} \
 ] $xlconcat_0

  # Create instance: xlslice_2, and set properties
  set xlslice_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlslice:1.0 xlslice_2 ]
  set_property -dict [ list \
CONFIG.DIN_FROM {7} \
CONFIG.DIN_TO {0} \
CONFIG.DIN_WIDTH {16} \
CONFIG.DOUT_WIDTH {8} \
 ] $xlslice_2

  # Create instance: jtag_0, jtag_1
  set jtag_0 [ create_bd_cell -type ip -vlnv ettus.com:ip:axi_bitq:1.0 jtag_0 ]
  set jtag_1 [ create_bd_cell -type ip -vlnv ettus.com:ip:axi_bitq:1.0 jtag_1 ]

  # Create instance: axi_iic_0 for QSFP i2c
  set axi_iic_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_iic:2.0 axi_iic_0 ]


  # Create interface connections
  connect_bd_intf_net -intf_net S00_AXI_1 [get_bd_intf_pins axi_interconnect_0/S00_AXI] [get_bd_intf_pins processing_system7_0/M_AXI_GP0]
  connect_bd_intf_net -intf_net S_AXI_GP0_1 [get_bd_intf_ports S_AXI_GP0] [get_bd_intf_pins axi_interconnect_hp0/S01_AXI]
  connect_bd_intf_net -intf_net S_AXI_GP1_1 [get_bd_intf_ports S_AXI_GP1] [get_bd_intf_pins axi_interconnect_hp1/S01_AXI]
  connect_bd_intf_net -intf_net S_AXI_HP0_1 [get_bd_intf_ports S_AXI_HP0] [get_bd_intf_pins axi_interconnect_hp0/S00_AXI]
  connect_bd_intf_net -intf_net S_AXI_HP1_1 [get_bd_intf_ports S_AXI_HP1] [get_bd_intf_pins axi_interconnect_hp1/S00_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M00_AXI [get_bd_intf_ports M_AXI_ETH_DMA0] [get_bd_intf_pins axi_interconnect_0/M00_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M01_AXI [get_bd_intf_ports M_AXI_NET0] [get_bd_intf_pins axi_interconnect_0/M01_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M02_AXI [get_bd_intf_ports M_AXI_ETH_DMA1] [get_bd_intf_pins axi_interconnect_0/M02_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M03_AXI [get_bd_intf_ports M_AXI_NET1] [get_bd_intf_pins axi_interconnect_0/M03_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M04_AXI [get_bd_intf_ports M_AXI_XBAR] [get_bd_intf_pins axi_interconnect_0/M04_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M05_AXI [get_bd_intf_ports M_AXI_JESD0] [get_bd_intf_pins axi_interconnect_0/M05_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M06_AXI [get_bd_intf_ports M_AXI_JESD1] [get_bd_intf_pins axi_interconnect_0/M06_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M07_AXI [get_bd_intf_ports M_AXI_NET2] [get_bd_intf_pins axi_interconnect_0/M07_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M08_AXI [get_bd_intf_pins axi_interconnect_0/M08_AXI] [get_bd_intf_pins dma/s_axi_rx_dmac]
  connect_bd_intf_net -intf_net axi_interconnect_0_M09_AXI [get_bd_intf_pins axi_interconnect_0/M09_AXI] [get_bd_intf_pins dma/s_axi_tx_dmac]
  connect_bd_intf_net -intf_net axi_interconnect_0_M10_AXI [get_bd_intf_pins axi_interconnect_0/M10_AXI] [get_bd_intf_pins dma/s_axi_regfile]
  connect_bd_intf_net -intf_net axi_interconnect_0_M11_AXI [get_bd_intf_pins jtag_0/S_AXI] [get_bd_intf_pins axi_interconnect_0/M11_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M12_AXI [get_bd_intf_pins jtag_1/S_AXI] [get_bd_intf_pins axi_interconnect_0/M12_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M13_AXI [get_bd_intf_pins axi_interconnect_0/M13_AXI] [get_bd_intf_pins axi_uartlite_0/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M14_AXI [get_bd_intf_ports M_AXI_WR] [get_bd_intf_pins axi_interconnect_0/M14_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M15_AXI [get_bd_intf_pins axi_iic_0/S_AXI] [get_bd_intf_pins axi_interconnect_0/M15_AXI]

  connect_bd_intf_net -intf_net axi_protocol_converter_hp0_M_AXI [get_bd_intf_pins axi_interconnect_hp0/M00_AXI] [get_bd_intf_pins processing_system7_0/S_AXI_HP0]
  connect_bd_intf_net -intf_net axi_protocol_converter_hp1_M_AXI [get_bd_intf_pins axi_interconnect_hp1/M00_AXI] [get_bd_intf_pins processing_system7_0/S_AXI_HP1]
  connect_bd_intf_net -intf_net dma_M_AXI_RX_DMA [get_bd_intf_pins dma/M_AXI_RX_DMA] [get_bd_intf_pins processing_system7_0/S_AXI_HP2]
  connect_bd_intf_net -intf_net dma_M_AXI_TX_DMA [get_bd_intf_pins dma/M_AXI_TX_DMA] [get_bd_intf_pins processing_system7_0/S_AXI_HP3]
  connect_bd_intf_net -intf_net s_axis_dma_1 [get_bd_intf_ports s_axis_dma] [get_bd_intf_pins dma/s_axis_dma]
  connect_bd_intf_net -intf_net m_axis_dma_1 [get_bd_intf_ports m_axis_dma] [get_bd_intf_pins dma/m_axis_dma]
  connect_bd_intf_net -intf_net processing_system7_0_DDR [get_bd_intf_ports DDR] [get_bd_intf_pins processing_system7_0/DDR]
  connect_bd_intf_net -intf_net processing_system7_0_GPIO_0 [get_bd_intf_ports GPIO_0] [get_bd_intf_pins processing_system7_0/GPIO_0]
  connect_bd_intf_net -intf_net processing_system7_0_USBIND_0 [get_bd_intf_ports USBIND_0] [get_bd_intf_pins processing_system7_0/USBIND_0]
  connect_bd_intf_net -intf_net WR_UART [get_bd_intf_ports WR_UART] [get_bd_intf_pins axi_uartlite_0/UART]

  # Create port connections
  connect_bd_net -net IRQ_F2P_1 [get_bd_ports IRQ_F2P] [get_bd_pins xlslice_2/Din]
  connect_bd_net -net SPI0_MISO_I_1 [get_bd_ports SPI0_MISO_I] [get_bd_pins processing_system7_0/SPI0_MISO_I]
  connect_bd_net -net SPI0_MOSI_I_1 [get_bd_ports SPI0_MOSI_I] [get_bd_pins processing_system7_0/SPI0_MOSI_I]
  connect_bd_net -net SPI0_SCLK_I_1 [get_bd_ports SPI0_SCLK_I] [get_bd_pins processing_system7_0/SPI0_SCLK_I]
  connect_bd_net -net SPI0_SS_I_1 [get_bd_ports SPI0_SS_I] [get_bd_pins processing_system7_0/SPI0_SS_I]
  connect_bd_net -net SPI1_MISO_I_1 [get_bd_ports SPI1_MISO_I] [get_bd_pins processing_system7_0/SPI1_MISO_I]
  connect_bd_net -net SPI1_MOSI_I_1 [get_bd_ports SPI1_MOSI_I] [get_bd_pins processing_system7_0/SPI1_MOSI_I]
  connect_bd_net -net SPI1_SCLK_I_1 [get_bd_ports SPI1_SCLK_I] [get_bd_pins processing_system7_0/SPI1_SCLK_I]
  connect_bd_net -net SPI1_SS_I_1 [get_bd_ports SPI1_SS_I] [get_bd_pins processing_system7_0/SPI1_SS_I]
  connect_bd_net -net S_AXI_HP0_ACLK_1 [get_bd_pins axi_interconnect_hp0/S01_ACLK]
  connect_bd_net -net S_AXI_HP0_ARESETN_1 [get_bd_pins axi_interconnect_hp0/S01_ARESETN]
  connect_bd_net -net S_AXI_HP1_ACLK_1 [get_bd_pins axi_interconnect_hp1/S01_ACLK]
  connect_bd_net -net S_AXI_HP1_ARESETN_1 [get_bd_pins axi_interconnect_hp1/S01_ARESETN]
  connect_bd_net -net S_AXI_HP0_ACLK_1 [get_bd_ports S_AXI_HP0_ACLK] [get_bd_pins axi_interconnect_hp0/M00_ACLK] [get_bd_pins axi_interconnect_hp0/S00_ACLK] [get_bd_pins processing_system7_0/S_AXI_HP0_ACLK] [get_bd_pins axi_interconnect_hp0/ACLK]
  connect_bd_net -net S_AXI_HP0_ARESETN_1 [get_bd_ports S_AXI_HP0_ARESETN] [get_bd_pins axi_interconnect_hp0/M00_ARESETN] [get_bd_pins axi_interconnect_hp0/S00_ARESETN] [get_bd_pins axi_interconnect_hp0/ARESETN]
  connect_bd_net -net S_AXI_HP1_ACLK_1 [get_bd_ports S_AXI_HP1_ACLK] [get_bd_pins axi_interconnect_hp1/M00_ACLK] [get_bd_pins axi_interconnect_hp1/S00_ACLK] [get_bd_pins processing_system7_0/S_AXI_HP1_ACLK] [get_bd_pins axi_interconnect_hp1/ACLK]
  connect_bd_net -net S_AXI_HP1_ARESETN_1 [get_bd_ports S_AXI_HP1_ARESETN] [get_bd_pins axi_interconnect_hp1/M00_ARESETN] [get_bd_pins axi_interconnect_hp1/S00_ARESETN] [get_bd_pins axi_interconnect_hp1/ARESETN]
  connect_bd_net -net bus_clk [get_bd_ports bus_clk] [get_bd_pins dma/bus_clk]
  connect_bd_net -net bus_rstn [get_bd_ports bus_rstn] [get_bd_pins dma/bus_rstn]
  connect_bd_net -net clk40 [get_bd_ports clk40] [get_bd_pins axi_interconnect_0/M00_ACLK] [get_bd_pins axi_interconnect_0/M01_ACLK] [get_bd_pins axi_interconnect_0/M02_ACLK] [get_bd_pins axi_interconnect_0/M03_ACLK] [get_bd_pins axi_interconnect_0/M04_ACLK] [get_bd_pins axi_interconnect_0/M05_ACLK] [get_bd_pins axi_interconnect_0/M06_ACLK] [get_bd_pins axi_interconnect_0/M07_ACLK] [get_bd_pins axi_interconnect_0/M08_ACLK] [get_bd_pins axi_interconnect_0/M09_ACLK] [get_bd_pins axi_interconnect_0/M10_ACLK] [get_bd_pins dma/clk40] [get_bd_pins processing_system7_0/S_AXI_HP2_ACLK] [get_bd_pins processing_system7_0/S_AXI_HP3_ACLK] [get_bd_pins axi_interconnect_0/M11_ACLK] [get_bd_pins axi_interconnect_0/M12_ACLK] [get_bd_pins axi_interconnect_0/M15_ACLK]
  connect_bd_net -net clk40_rstn [get_bd_ports clk40_rstn] [get_bd_pins axi_interconnect_0/M00_ARESETN] [get_bd_pins axi_interconnect_0/M01_ARESETN] [get_bd_pins axi_interconnect_0/M02_ARESETN] [get_bd_pins axi_interconnect_0/M03_ARESETN] [get_bd_pins axi_interconnect_0/M04_ARESETN] [get_bd_pins axi_interconnect_0/M05_ARESETN] [get_bd_pins axi_interconnect_0/M06_ARESETN] [get_bd_pins axi_interconnect_0/M07_ARESETN] [get_bd_pins axi_interconnect_0/M08_ARESETN] [get_bd_pins axi_interconnect_0/M09_ARESETN] [get_bd_pins axi_interconnect_0/M10_ARESETN] [get_bd_pins axi_interconnect_0/M11_ARESETN] [get_bd_pins axi_interconnect_0/M12_ARESETN] [get_bd_pins axi_interconnect_0/M15_ARESETN] [get_bd_pins dma/clk40_rstn]
  connect_bd_net -net M_AXI_WR_CLK [get_bd_ports M_AXI_WR_CLK] [get_bd_pins axi_interconnect_0/M14_ACLK]
  connect_bd_net -net M_AXI_WR_RSTn [get_bd_ports M_AXI_WR_RSTn] [get_bd_pins axi_interconnect_0/M14_ARESETN]
  connect_bd_net -net ddr_vrn [get_bd_ports DDR_VRN] [get_bd_pins processing_system7_0/DDR_VRN]
  connect_bd_net -net ddr_vrp [get_bd_ports DDR_VRP] [get_bd_pins processing_system7_0/DDR_VRP]
  connect_bd_net -net dma_tx_irq [get_bd_pins dma/tx_irq] [get_bd_pins xlconcat_0/In2]
  connect_bd_net -net clk40 [get_bd_ports clk40] [get_bd_pins axi_interconnect_0/ACLK] [get_bd_pins axi_interconnect_0/S00_ACLK] [get_bd_pins processing_system7_0/M_AXI_GP0_ACLK] [get_bd_pins jtag_0/S_AXI_ACLK] [get_bd_pins jtag_1/S_AXI_ACLK] [get_bd_pins axi_iic_0/s_axi_aclk]
  connect_bd_net -net clk40_rstn [get_bd_ports clk40_rstn] [get_bd_pins axi_interconnect_0/ARESETN] [get_bd_pins axi_interconnect_0/S00_ARESETN] [get_bd_pins jtag_0/S_AXI_ARESETN] [get_bd_pins jtag_1/S_AXI_ARESETN] [get_bd_pins axi_iic_0/s_axi_aresetn]
  connect_bd_net -net mio [get_bd_ports MIO] [get_bd_pins processing_system7_0/MIO]
  connect_bd_net -net processing_system7_0_FCLK_CLK0 [get_bd_ports FCLK_CLK0] [get_bd_pins processing_system7_0/FCLK_CLK0]
  connect_bd_net -net processing_system7_0_FCLK_CLK1 [get_bd_ports FCLK_CLK1] [get_bd_pins processing_system7_0/FCLK_CLK1]
  connect_bd_net -net processing_system7_0_FCLK_CLK2 [get_bd_ports FCLK_CLK2] [get_bd_pins processing_system7_0/FCLK_CLK2]
  connect_bd_net -net processing_system7_0_FCLK_CLK3 [get_bd_ports FCLK_CLK3] [get_bd_pins processing_system7_0/FCLK_CLK3]
  connect_bd_net -net processing_system7_0_FCLK_RESET0_N [get_bd_ports FCLK_RESET0_N] [get_bd_pins processing_system7_0/FCLK_RESET0_N]
  connect_bd_net -net processing_system7_0_FCLK_RESET1_N [get_bd_ports FCLK_RESET1_N] [get_bd_pins processing_system7_0/FCLK_RESET1_N]
  connect_bd_net -net processing_system7_0_FCLK_RESET2_N [get_bd_ports FCLK_RESET2_N] [get_bd_pins processing_system7_0/FCLK_RESET2_N]
  connect_bd_net -net processing_system7_0_FCLK_RESET3_N [get_bd_ports FCLK_RESET3_N] [get_bd_pins processing_system7_0/FCLK_RESET3_N]
  connect_bd_net -net processing_system7_0_SPI0_MISO_O [get_bd_ports SPI0_MISO_O] [get_bd_pins processing_system7_0/SPI0_MISO_O]
  connect_bd_net -net processing_system7_0_SPI0_MISO_T [get_bd_ports SPI0_MISO_T] [get_bd_pins processing_system7_0/SPI0_MISO_T]
  connect_bd_net -net processing_system7_0_SPI0_MOSI_O [get_bd_ports SPI0_MOSI_O] [get_bd_pins processing_system7_0/SPI0_MOSI_O]
  connect_bd_net -net processing_system7_0_SPI0_MOSI_T [get_bd_ports SPI0_MOSI_T] [get_bd_pins processing_system7_0/SPI0_MOSI_T]
  connect_bd_net -net processing_system7_0_SPI0_SCLK_O [get_bd_ports SPI0_SCLK_O] [get_bd_pins processing_system7_0/SPI0_SCLK_O]
  connect_bd_net -net processing_system7_0_SPI0_SCLK_T [get_bd_ports SPI0_SCLK_T] [get_bd_pins processing_system7_0/SPI0_SCLK_T]
  connect_bd_net -net processing_system7_0_SPI0_SS1_O [get_bd_ports SPI0_SS1_O] [get_bd_pins processing_system7_0/SPI0_SS1_O]
  connect_bd_net -net processing_system7_0_SPI0_SS2_O [get_bd_ports SPI0_SS2_O] [get_bd_pins processing_system7_0/SPI0_SS2_O]
  connect_bd_net -net processing_system7_0_SPI0_SS_O [get_bd_ports SPI0_SS_O] [get_bd_pins processing_system7_0/SPI0_SS_O]
  connect_bd_net -net processing_system7_0_SPI0_SS_T [get_bd_ports SPI0_SS_T] [get_bd_pins processing_system7_0/SPI0_SS_T]
  connect_bd_net -net processing_system7_0_SPI1_MISO_O [get_bd_ports SPI1_MISO_O] [get_bd_pins processing_system7_0/SPI1_MISO_O]
  connect_bd_net -net processing_system7_0_SPI1_MISO_T [get_bd_ports SPI1_MISO_T] [get_bd_pins processing_system7_0/SPI1_MISO_T]
  connect_bd_net -net processing_system7_0_SPI1_MOSI_O [get_bd_ports SPI1_MOSI_O] [get_bd_pins processing_system7_0/SPI1_MOSI_O]
  connect_bd_net -net processing_system7_0_SPI1_MOSI_T [get_bd_ports SPI1_MOSI_T] [get_bd_pins processing_system7_0/SPI1_MOSI_T]
  connect_bd_net -net processing_system7_0_SPI1_SCLK_O [get_bd_ports SPI1_SCLK_O] [get_bd_pins processing_system7_0/SPI1_SCLK_O]
  connect_bd_net -net processing_system7_0_SPI1_SCLK_T [get_bd_ports SPI1_SCLK_T] [get_bd_pins processing_system7_0/SPI1_SCLK_T]
  connect_bd_net -net processing_system7_0_SPI1_SS1_O [get_bd_ports SPI1_SS1_O] [get_bd_pins processing_system7_0/SPI1_SS1_O]
  connect_bd_net -net processing_system7_0_SPI1_SS2_O [get_bd_ports SPI1_SS2_O] [get_bd_pins processing_system7_0/SPI1_SS2_O]
  connect_bd_net -net processing_system7_0_SPI1_SS_O [get_bd_ports SPI1_SS_O] [get_bd_pins processing_system7_0/SPI1_SS_O]
  connect_bd_net -net processing_system7_0_SPI1_SS_T [get_bd_ports SPI1_SS_T] [get_bd_pins processing_system7_0/SPI1_SS_T]
  connect_bd_net -net ps_clk [get_bd_ports PS_CLK] [get_bd_pins processing_system7_0/PS_CLK]
  connect_bd_net -net ps_porb [get_bd_ports PS_PORB] [get_bd_pins processing_system7_0/PS_PORB]
  connect_bd_net -net ps_srstb [get_bd_ports PS_SRSTB] [get_bd_pins processing_system7_0/PS_SRSTB]
  connect_bd_net -net rx_dma_irq [get_bd_pins dma/rx_irq] [get_bd_pins xlconcat_0/In1]
  connect_bd_net -net xlconcat_0_dout [get_bd_pins processing_system7_0/IRQ_F2P] [get_bd_pins xlconcat_0/dout]
  connect_bd_net -net xlslice_2_Dout [get_bd_pins xlconcat_0/In0] [get_bd_pins xlslice_2/Dout]
  connect_bd_net -net JTAG0_TCK [get_bd_ports JTAG0_TCK] [get_bd_pins jtag_0/bit_clk]
  connect_bd_net -net JTAG0_TMS [get_bd_ports JTAG0_TMS] [get_bd_pins jtag_0/bit_stb]
  connect_bd_net -net JTAG0_TDI [get_bd_ports JTAG0_TDI] [get_bd_pins jtag_0/bit_out]
  connect_bd_net -net JTAG0_TDO [get_bd_ports JTAG0_TDO] [get_bd_pins jtag_0/bit_in]
  connect_bd_net -net JTAG1_TCK [get_bd_ports JTAG1_TCK] [get_bd_pins jtag_1/bit_clk]
  connect_bd_net -net JTAG1_TMS [get_bd_ports JTAG1_TMS] [get_bd_pins jtag_1/bit_stb]
  connect_bd_net -net JTAG1_TDI [get_bd_ports JTAG1_TDI] [get_bd_pins jtag_1/bit_out]
  connect_bd_net -net JTAG1_TDO [get_bd_ports JTAG1_TDO] [get_bd_pins jtag_1/bit_in]
  connect_bd_net [get_bd_pins axi_uartlite_0/interrupt] [get_bd_pins xlconcat_0/In3]
  connect_bd_net [get_bd_pins axi_iic_0/iic2intc_irpt] [get_bd_pins xlconcat_0/In4]
  connect_bd_net -net qsfp_sda_i [get_bd_pins axi_iic_0/sda_i] [get_bd_ports qsfp_sda_i]
  connect_bd_net -net qsfp_sda_o [get_bd_pins axi_iic_0/sda_o] [get_bd_ports qsfp_sda_o]
  connect_bd_net -net qsfp_sda_t [get_bd_pins axi_iic_0/sda_t] [get_bd_ports qsfp_sda_t]
  connect_bd_net -net qsfp_scl_i [get_bd_pins axi_iic_0/scl_i] [get_bd_ports qsfp_scl_i]
  connect_bd_net -net qsfp_scl_o [get_bd_pins axi_iic_0/scl_o] [get_bd_ports qsfp_scl_o]
  connect_bd_net -net qsfp_scl_t [get_bd_pins axi_iic_0/scl_t] [get_bd_ports qsfp_scl_t]
  connect_bd_net [get_bd_ports clk40] [get_bd_pins axi_uartlite_0/s_axi_aclk]
  connect_bd_net [get_bd_ports clk40_rstn] [get_bd_pins axi_uartlite_0/s_axi_aresetn]
  connect_bd_net [get_bd_ports clk40] [get_bd_pins axi_interconnect_0/M13_ACLK]
  connect_bd_net [get_bd_ports clk40_rstn] [get_bd_pins axi_interconnect_0/M13_ARESETN]

  # Create address segments
  create_bd_addr_seg -range 0x4000 -offset 0x40000000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs M_AXI_ETH_DMA0/Reg] SEG_M_AXI_ETH_DMA0_Reg
  create_bd_addr_seg -range 0x4000 -offset 0x40008000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs M_AXI_ETH_DMA1/Reg] SEG_M_AXI_ETH_DMA1_Reg
  create_bd_addr_seg -range 0x4000 -offset 0x40014000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs M_AXI_JESD0/Reg] SEG_M_AXI_JESD0_Reg
  create_bd_addr_seg -range 0x4000 -offset 0x40004000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs M_AXI_NET0/Reg] SEG_M_AXI_NET0_Reg
  create_bd_addr_seg -range 0x4000 -offset 0x4000C000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs M_AXI_NET1/Reg] SEG_M_AXI_NET1_Reg
  create_bd_addr_seg -range 0x20000 -offset 0x40020000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs M_AXI_NET2/Reg] SEG_M_AXI_NET2_Reg
  create_bd_addr_seg -range 0x4000 -offset 0x40018000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs M_AXI_JESD1/Reg] SEG_M_AXI_JESD1_Reg
  create_bd_addr_seg -range 0x4000 -offset 0x40010000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs M_AXI_XBAR/Reg] SEG_M_AXI_XBAR_Reg
  create_bd_addr_seg -range 0x10000 -offset 0x43CA0000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/tx/axi_tx_dmac_0/s_axi/axi_lite] SEG_axi_dmac_0_axi_lite
  create_bd_addr_seg -range 0x10000 -offset 0x43CB0000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/tx/axi_tx_dmac_1/s_axi/axi_lite] SEG_axi_dmac_1_axi_lite
  create_bd_addr_seg -range 0x10000 -offset 0x43CC0000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/tx/axi_tx_dmac_2/s_axi/axi_lite] SEG_axi_dmac_2_axi_lite
  create_bd_addr_seg -range 0x10000 -offset 0x43CD0000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/tx/axi_tx_dmac_3/s_axi/axi_lite] SEG_axi_dmac_3_axi_lite
  create_bd_addr_seg -range 0x10000 -offset 0x43CE0000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/tx/axi_tx_dmac_4/s_axi/axi_lite] SEG_axi_dmac_4_axi_lite
  create_bd_addr_seg -range 0x10000 -offset 0x43CF0000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/tx/axi_tx_dmac_5/s_axi/axi_lite] SEG_axi_dmac_5_axi_lite
  create_bd_addr_seg -range 0x10000 -offset 0x43D00000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/tx/axi_tx_dmac_6/s_axi/axi_lite] SEG_axi_dmac_6_axi_lite
  create_bd_addr_seg -range 0x10000 -offset 0x43D10000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/tx/axi_tx_dmac_7/s_axi/axi_lite] SEG_axi_dmac_7_axi_lite
  create_bd_addr_seg -range 0x10000 -offset 0x43D20000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/tx/axi_tx_dmac_8/s_axi/axi_lite] SEG_axi_dmac_8_axi_lite
  create_bd_addr_seg -range 0x10000 -offset 0x43D30000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/tx/axi_tx_dmac_9/s_axi/axi_lite] SEG_axi_dmac_9_axi_lite
  create_bd_addr_seg -range 0x40000 -offset 0x43D40000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs M_AXI_WR/Reg] SEG_M_AXI_WR_Reg
  create_bd_addr_seg -range 0x10000 -offset 0x43D80000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs axi_iic_0/S_AXI/Reg] SEG_axi_iic_0_Reg
  create_bd_addr_seg -range 0x1000 -offset 0x42080000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/axi_regfile_0/S_AXI/regs] SEG_axi_regfile_0_regs
  create_bd_addr_seg -range 0x10000 -offset 0x43C00000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/rx/dma0/axi_rx_dmac/s_axi/axi_lite] SEG_axi_rx_dmac_axi_lite
  create_bd_addr_seg -range 0x10000 -offset 0x43C10000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/rx/dma1/axi_rx_dmac/s_axi/axi_lite] SEG_axi_rx_dmac_axi_lite11
  create_bd_addr_seg -range 0x10000 -offset 0x43C20000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/rx/dma2/axi_rx_dmac/s_axi/axi_lite] SEG_axi_rx_dmac_axi_lite13
  create_bd_addr_seg -range 0x10000 -offset 0x43C30000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/rx/dma3/axi_rx_dmac/s_axi/axi_lite] SEG_axi_rx_dmac_axi_lite15
  create_bd_addr_seg -range 0x10000 -offset 0x43C40000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/rx/dma4/axi_rx_dmac/s_axi/axi_lite] SEG_axi_rx_dmac_axi_lite17
  create_bd_addr_seg -range 0x10000 -offset 0x43C50000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/rx/dma5/axi_rx_dmac/s_axi/axi_lite] SEG_axi_rx_dmac_axi_lite19
  create_bd_addr_seg -range 0x10000 -offset 0x43C60000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/rx/dma6/axi_rx_dmac/s_axi/axi_lite] SEG_axi_rx_dmac_axi_lite21
  create_bd_addr_seg -range 0x10000 -offset 0x43C70000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/rx/dma7/axi_rx_dmac/s_axi/axi_lite] SEG_axi_rx_dmac_axi_lite23
  create_bd_addr_seg -range 0x10000 -offset 0x43C80000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/rx/dma8/axi_rx_dmac/s_axi/axi_lite] SEG_axi_rx_dmac_axi_lite25
  create_bd_addr_seg -range 0x10000 -offset 0x43C90000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs dma/rx/dma9/axi_rx_dmac/s_axi/axi_lite] SEG_axi_rx_dmac_axi_lite27
  create_bd_addr_seg -range 0x1000 -offset 0x42100000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs jtag_0/S_AXI/reg0] SEG_jtag_0_reg0
  create_bd_addr_seg -range 0x1000 -offset 0x42200000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs jtag_1/S_AXI/reg0] SEG_jtag_1_reg0
  create_bd_addr_seg -range 0x1000 -offset 0x42C00000 [get_bd_addr_spaces processing_system7_0/Data] [get_bd_addr_segs {axi_uartlite_0/S_AXI/Reg }] SEG_WR_UART_Reg
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/tx/axi_tx_dmac_0/m_src_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP3/HP3_DDR_LOWOCM] SEG_processing_system7_0_HP3_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/tx/axi_tx_dmac_1/m_src_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP3/HP3_DDR_LOWOCM] SEG_processing_system7_0_HP3_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/tx/axi_tx_dmac_2/m_src_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP3/HP3_DDR_LOWOCM] SEG_processing_system7_0_HP3_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/tx/axi_tx_dmac_3/m_src_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP3/HP3_DDR_LOWOCM] SEG_processing_system7_0_HP3_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/tx/axi_tx_dmac_4/m_src_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP3/HP3_DDR_LOWOCM] SEG_processing_system7_0_HP3_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/tx/axi_tx_dmac_5/m_src_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP3/HP3_DDR_LOWOCM] SEG_processing_system7_0_HP3_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/tx/axi_tx_dmac_6/m_src_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP3/HP3_DDR_LOWOCM] SEG_processing_system7_0_HP3_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/tx/axi_tx_dmac_7/m_src_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP3/HP3_DDR_LOWOCM] SEG_processing_system7_0_HP3_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/tx/axi_tx_dmac_8/m_src_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP3/HP3_DDR_LOWOCM] SEG_processing_system7_0_HP3_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/tx/axi_tx_dmac_9/m_src_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP3/HP3_DDR_LOWOCM] SEG_processing_system7_0_HP3_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/rx/dma0/axi_rx_dmac/m_dest_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP2/HP2_DDR_LOWOCM] SEG_processing_system7_0_HP2_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/rx/dma1/axi_rx_dmac/m_dest_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP2/HP2_DDR_LOWOCM] SEG_processing_system7_0_HP2_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/rx/dma2/axi_rx_dmac/m_dest_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP2/HP2_DDR_LOWOCM] SEG_processing_system7_0_HP2_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/rx/dma3/axi_rx_dmac/m_dest_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP2/HP2_DDR_LOWOCM] SEG_processing_system7_0_HP2_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/rx/dma4/axi_rx_dmac/m_dest_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP2/HP2_DDR_LOWOCM] SEG_processing_system7_0_HP2_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/rx/dma5/axi_rx_dmac/m_dest_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP2/HP2_DDR_LOWOCM] SEG_processing_system7_0_HP2_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/rx/dma6/axi_rx_dmac/m_dest_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP2/HP2_DDR_LOWOCM] SEG_processing_system7_0_HP2_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/rx/dma7/axi_rx_dmac/m_dest_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP2/HP2_DDR_LOWOCM] SEG_processing_system7_0_HP2_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/rx/dma8/axi_rx_dmac/m_dest_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP2/HP2_DDR_LOWOCM] SEG_processing_system7_0_HP2_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces dma/rx/dma9/axi_rx_dmac/m_dest_axi] [get_bd_addr_segs processing_system7_0/S_AXI_HP2/HP2_DDR_LOWOCM] SEG_processing_system7_0_HP2_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces S_AXI_GP0] [get_bd_addr_segs processing_system7_0/S_AXI_HP0/HP0_DDR_LOWOCM] SEG_processing_system7_0_GP0_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces S_AXI_GP1] [get_bd_addr_segs processing_system7_0/S_AXI_HP1/HP1_DDR_LOWOCM] SEG_processing_system7_0_GP1_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces S_AXI_HP0] [get_bd_addr_segs processing_system7_0/S_AXI_HP0/HP0_DDR_LOWOCM] SEG_processing_system7_0_HP0_DDR_LOWOCM
  create_bd_addr_seg -range 0x40000000 -offset 0x0 [get_bd_addr_spaces S_AXI_HP1] [get_bd_addr_segs processing_system7_0/S_AXI_HP1/HP1_DDR_LOWOCM] SEG_processing_system7_0_HP1_DDR_LOWOCM

  # Restore current instance
  current_bd_instance $oldCurInst

  save_bd_design
}
# End of create_root_design()


##################################################################
# MAIN FLOW
##################################################################

create_root_design ""


