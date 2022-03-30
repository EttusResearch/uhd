
################################################################
# This is a generated script based on design: axi_eth_dma_bd
#
# Though there are limitations about the generated script,
# the main purpose of this utility is to make learning
# IP Integrator Tcl commands easier.
################################################################

namespace eval _tcl {
proc get_script_folder {} {
   set script_path [file normalize [info script]]
   set script_folder [file dirname $script_path]
   return $script_folder
}
}
variable script_folder
set script_folder [_tcl::get_script_folder]

################################################################
# Check if script is running in correct Vivado version.
################################################################
set scripts_vivado_version 2021.1
set current_vivado_version [version -short]

if { [string first $scripts_vivado_version $current_vivado_version] == -1 } {
   puts ""
   catch {common::send_gid_msg -ssname BD::TCL -id 2041 -severity "ERROR" "This script was generated using Vivado <$scripts_vivado_version> and is being run in <$current_vivado_version> of Vivado. Please run the script in Vivado <$scripts_vivado_version> then open the design in Vivado <$current_vivado_version>. Upgrade the design by running \"Tools => Report => Report IP Status...\", then run write_bd_tcl to create an updated script."}

   return 1
}

################################################################
# START
################################################################

# To test this script, run the following commands from Vivado Tcl console:
# source axi_eth_dma_bd_script.tcl

# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./myproj/project_1.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
   create_project project_1 myproj -part xczu28dr-ffvg1517-1-e
}


# CHANGE DESIGN NAME HERE
variable design_name
set design_name axi_eth_dma_bd

# If you do not already have an existing IP Integrator design open,
# you can create a design using the following command:
#    create_bd_design $design_name

# Creating design if needed
set errMsg ""
set nRet 0

set cur_design [current_bd_design -quiet]
set list_cells [get_bd_cells -quiet]

if { ${design_name} eq "" } {
   # USE CASES:
   #    1) Design_name not set

   set errMsg "Please set the variable <design_name> to a non-empty value."
   set nRet 1

} elseif { ${cur_design} ne "" && ${list_cells} eq "" } {
   # USE CASES:
   #    2): Current design opened AND is empty AND names same.
   #    3): Current design opened AND is empty AND names diff; design_name NOT in project.
   #    4): Current design opened AND is empty AND names diff; design_name exists in project.

   if { $cur_design ne $design_name } {
      common::send_gid_msg -ssname BD::TCL -id 2001 -severity "INFO" "Changing value of <design_name> from <$design_name> to <$cur_design> since current design is empty."
      set design_name [get_property NAME $cur_design]
   }
   common::send_gid_msg -ssname BD::TCL -id 2002 -severity "INFO" "Constructing design in IPI design <$cur_design>..."

} elseif { ${cur_design} ne "" && $list_cells ne "" && $cur_design eq $design_name } {
   # USE CASES:
   #    5) Current design opened AND has components AND same names.

   set errMsg "Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 1
} elseif { [get_files -quiet ${design_name}.bd] ne "" } {
   # USE CASES: 
   #    6) Current opened design, has components, but diff names, design_name exists in project.
   #    7) No opened design, design_name exists in project.

   set errMsg "Design <$design_name> already exists in your project, please set the variable <design_name> to another value."
   set nRet 2

} else {
   # USE CASES:
   #    8) No opened design, design_name not in project.
   #    9) Current opened design, has components, but diff names, design_name not in project.

   common::send_gid_msg -ssname BD::TCL -id 2003 -severity "INFO" "Currently there is no design <$design_name> in project, so creating one..."

   create_bd_design $design_name

   common::send_gid_msg -ssname BD::TCL -id 2004 -severity "INFO" "Making design <$design_name> as current_bd_design."
   current_bd_design $design_name

}

common::send_gid_msg -ssname BD::TCL -id 2005 -severity "INFO" "Currently the variable <design_name> is equal to \"$design_name\"."

if { $nRet != 0 } {
   catch {common::send_gid_msg -ssname BD::TCL -id 2006 -severity "ERROR" $errMsg}
   return $nRet
}

set bCheckIPsPassed 1
##################################################################
# CHECK IPs
##################################################################
set bCheckIPs 1
if { $bCheckIPs == 1 } {
   set list_check_ips "\ 
xilinx.com:ip:axi_dma:7.1\
xilinx.com:ip:smartconnect:1.0\
"

   set list_ips_missing ""
   common::send_gid_msg -ssname BD::TCL -id 2011 -severity "INFO" "Checking if the following IPs exist in the project's IP catalog: $list_check_ips ."

   foreach ip_vlnv $list_check_ips {
      set ip_obj [get_ipdefs -all $ip_vlnv]
      if { $ip_obj eq "" } {
         lappend list_ips_missing $ip_vlnv
      }
   }

   if { $list_ips_missing ne "" } {
      catch {common::send_gid_msg -ssname BD::TCL -id 2012 -severity "ERROR" "The following IPs are not found in the IP Catalog:\n  $list_ips_missing\n\nResolution: Please add the repository containing the IP(s) to the project." }
      set bCheckIPsPassed 0
   }

}

if { $bCheckIPsPassed != 1 } {
  common::send_gid_msg -ssname BD::TCL -id 2023 -severity "WARNING" "Will not continue with creation of design due to the error(s) above."
  return 3
}

##################################################################
# DESIGN PROCs
##################################################################



# Procedure to create entire design; Provide argument to make
# procedure reusable. If parentCell is "", will use root.
proc create_root_design { parentCell } {

  variable script_folder
  variable design_name

  if { $parentCell eq "" } {
     set parentCell [get_bd_cells /]
  }

  # Get object for parentCell
  set parentObj [get_bd_cells $parentCell]
  if { $parentObj == "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2090 -severity "ERROR" "Unable to find parent cell <$parentCell>!"}
     return
  }

  # Make sure parentObj is hier blk
  set parentType [get_property TYPE $parentObj]
  if { $parentType ne "hier" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2091 -severity "ERROR" "Parent <$parentObj> has TYPE = <$parentType>. Expected to be <hier>."}
     return
  }

  # Save current instance; Restore later
  set oldCurInst [current_bd_instance .]

  # Set parent object as current
  current_bd_instance $parentObj


  # Create interface ports
  set axi_eth_dma [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 axi_eth_dma ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {40} \
   CONFIG.ARUSER_WIDTH {0} \
   CONFIG.AWUSER_WIDTH {0} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {32} \
   CONFIG.FREQ_HZ {40000000} \
   CONFIG.HAS_BRESP {1} \
   CONFIG.HAS_BURST {0} \
   CONFIG.HAS_CACHE {0} \
   CONFIG.HAS_LOCK {0} \
   CONFIG.HAS_PROT {0} \
   CONFIG.HAS_QOS {0} \
   CONFIG.HAS_REGION {0} \
   CONFIG.HAS_RRESP {1} \
   CONFIG.HAS_WSTRB {0} \
   CONFIG.ID_WIDTH {0} \
   CONFIG.MAX_BURST_LENGTH {1} \
   CONFIG.NUM_READ_OUTSTANDING {8} \
   CONFIG.NUM_READ_THREADS {1} \
   CONFIG.NUM_WRITE_OUTSTANDING {8} \
   CONFIG.NUM_WRITE_THREADS {1} \
   CONFIG.PROTOCOL {AXI4LITE} \
   CONFIG.READ_WRITE_MODE {READ_WRITE} \
   CONFIG.RUSER_BITS_PER_BYTE {0} \
   CONFIG.RUSER_WIDTH {0} \
   CONFIG.SUPPORTS_NARROW_BURST {0} \
   CONFIG.WUSER_BITS_PER_BYTE {0} \
   CONFIG.WUSER_WIDTH {0} \
   ] $axi_eth_dma

  set axi_hp [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 axi_hp ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {49} \
   CONFIG.DATA_WIDTH {128} \
   CONFIG.FREQ_HZ {40000000} \
   CONFIG.HAS_REGION {0} \
   CONFIG.NUM_READ_OUTSTANDING {16} \
   CONFIG.NUM_WRITE_OUTSTANDING {16} \
   CONFIG.PROTOCOL {AXI4} \
   ] $axi_hp

  set c2e [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 c2e ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {40000000} \
   ] $c2e

  set e2c [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 e2c ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {40000000} \
   CONFIG.HAS_TKEEP {1} \
   CONFIG.HAS_TLAST {1} \
   CONFIG.HAS_TREADY {1} \
   CONFIG.HAS_TSTRB {0} \
   CONFIG.LAYERED_METADATA {undef} \
   CONFIG.TDATA_NUM_BYTES {8} \
   CONFIG.TDEST_WIDTH {0} \
   CONFIG.TID_WIDTH {0} \
   CONFIG.TUSER_WIDTH {0} \
   ] $e2c


  # Create ports
  set clk40 [ create_bd_port -dir I -type clk -freq_hz 40000000 clk40 ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {e2c:axi_eth_dma:axi_hp:c2e} \
   CONFIG.ASSOCIATED_RESET {clk40_rstn} \
 ] $clk40
  set clk40_rstn [ create_bd_port -dir I -type rst clk40_rstn ]
  set eth_rx_irq [ create_bd_port -dir O -type intr eth_rx_irq ]
  set eth_tx_irq [ create_bd_port -dir O -type intr eth_tx_irq ]

  # Create instance: axi_eth_dma, and set properties
  set axi_eth_dma [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dma:7.1 axi_eth_dma ]
  set_property -dict [ list \
   CONFIG.c_addr_width {36} \
   CONFIG.c_enable_multi_channel {0} \
   CONFIG.c_include_mm2s_dre {1} \
   CONFIG.c_include_s2mm {1} \
   CONFIG.c_include_s2mm_dre {1} \
   CONFIG.c_m_axi_mm2s_data_width {64} \
   CONFIG.c_m_axi_s2mm_data_width {64} \
   CONFIG.c_m_axis_mm2s_tdata_width {64} \
   CONFIG.c_micro_dma {0} \
   CONFIG.c_mm2s_burst_size {8} \
   CONFIG.c_s2mm_burst_size {16} \
   CONFIG.c_sg_include_stscntrl_strm {0} \
 ] $axi_eth_dma

  # Create instance: smartconnect_eth_dma, and set properties
  set smartconnect_eth_dma [ create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 smartconnect_eth_dma ]
  set_property -dict [ list \
   CONFIG.NUM_SI {3} \
 ] $smartconnect_eth_dma

  # Create interface connections
  connect_bd_intf_net -intf_net Conn3 [get_bd_intf_ports c2e] [get_bd_intf_pins axi_eth_dma/M_AXIS_MM2S]
  connect_bd_intf_net -intf_net Conn4 [get_bd_intf_ports e2c] [get_bd_intf_pins axi_eth_dma/S_AXIS_S2MM]
  connect_bd_intf_net -intf_net axi_eth_dma_internal_M_AXI_MM2S [get_bd_intf_pins axi_eth_dma/M_AXI_MM2S] [get_bd_intf_pins smartconnect_eth_dma/S01_AXI]
  connect_bd_intf_net -intf_net axi_eth_dma_internal_M_AXI_S2MM [get_bd_intf_pins axi_eth_dma/M_AXI_S2MM] [get_bd_intf_pins smartconnect_eth_dma/S02_AXI]
  connect_bd_intf_net -intf_net axi_eth_dma_internal_M_AXI_SG [get_bd_intf_pins axi_eth_dma/M_AXI_SG] [get_bd_intf_pins smartconnect_eth_dma/S00_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_common_m_axi_eth_dma_ctrl [get_bd_intf_ports axi_eth_dma] [get_bd_intf_pins axi_eth_dma/S_AXI_LITE]
  connect_bd_intf_net -intf_net smartconnect_0_M00_AXI [get_bd_intf_ports axi_hp] [get_bd_intf_pins smartconnect_eth_dma/M00_AXI]

  # Create port connections
  connect_bd_net -net axi_eth_dma_mm2s_introut [get_bd_ports eth_tx_irq] [get_bd_pins axi_eth_dma/mm2s_introut]
  connect_bd_net -net axi_eth_dma_s2mm_introut [get_bd_ports eth_rx_irq] [get_bd_pins axi_eth_dma/s2mm_introut]
  connect_bd_net -net clk40 [get_bd_ports clk40] [get_bd_pins axi_eth_dma/m_axi_mm2s_aclk] [get_bd_pins axi_eth_dma/m_axi_s2mm_aclk] [get_bd_pins axi_eth_dma/m_axi_sg_aclk] [get_bd_pins axi_eth_dma/s_axi_lite_aclk] [get_bd_pins smartconnect_eth_dma/aclk]
  connect_bd_net -net clk40_rstn [get_bd_ports clk40_rstn] [get_bd_pins axi_eth_dma/axi_resetn] [get_bd_pins smartconnect_eth_dma/aresetn]

  # Create address segments
  assign_bd_address -offset 0x00000000 -range 0x001000000000 -target_address_space [get_bd_addr_spaces axi_eth_dma/Data_SG] [get_bd_addr_segs axi_hp/Reg] -force
  assign_bd_address -offset 0x00000000 -range 0x001000000000 -target_address_space [get_bd_addr_spaces axi_eth_dma/Data_MM2S] [get_bd_addr_segs axi_hp/Reg] -force
  assign_bd_address -offset 0x00000000 -range 0x001000000000 -target_address_space [get_bd_addr_spaces axi_eth_dma/Data_S2MM] [get_bd_addr_segs axi_hp/Reg] -force
  assign_bd_address -offset 0x00000000 -range 0x010000000000 -target_address_space [get_bd_addr_spaces axi_eth_dma] [get_bd_addr_segs axi_eth_dma/S_AXI_LITE/Reg] -force


  # Restore current instance
  current_bd_instance $oldCurInst

  validate_bd_design
  save_bd_design
}
# End of create_root_design()


##################################################################
# MAIN FLOW
##################################################################

create_root_design ""


