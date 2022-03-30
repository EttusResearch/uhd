
################################################################
# This is a generated script based on design: eth_100g_bd
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
# source eth_100g_bd_script.tcl

# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./myproj/project_1.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
   create_project project_1 myproj -part xczu28dr-ffvg1517-1-e
}


# CHANGE DESIGN NAME HERE
variable design_name
set design_name eth_100g_bd

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
xilinx.com:ip:cmac_usplus:3.1\
xilinx.com:ip:xlconstant:1.1\
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
  set core_drp [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:drp_rtl:1.0 core_drp ]

  set eth100g_rx [ create_bd_intf_port -mode Master -vlnv xilinx.com:display_cmac_usplus:lbus_ports:2.0 eth100g_rx ]

  set eth100g_tx [ create_bd_intf_port -mode Slave -vlnv xilinx.com:display_cmac_usplus:lbus_ports:2.0 eth100g_tx ]

  set gt_serial_port_0 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gt_rtl:1.0 gt_serial_port_0 ]

  set refclk [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 refclk ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {156250000} \
   ] $refclk

  set s_axi [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.ARUSER_WIDTH {0} \
   CONFIG.AWUSER_WIDTH {0} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {32} \
   CONFIG.HAS_BRESP {1} \
   CONFIG.HAS_BURST {0} \
   CONFIG.HAS_CACHE {0} \
   CONFIG.HAS_LOCK {0} \
   CONFIG.HAS_PROT {0} \
   CONFIG.HAS_QOS {0} \
   CONFIG.HAS_REGION {0} \
   CONFIG.HAS_RRESP {1} \
   CONFIG.HAS_WSTRB {1} \
   CONFIG.ID_WIDTH {0} \
   CONFIG.MAX_BURST_LENGTH {1} \
   CONFIG.NUM_READ_OUTSTANDING {1} \
   CONFIG.NUM_READ_THREADS {1} \
   CONFIG.NUM_WRITE_OUTSTANDING {1} \
   CONFIG.NUM_WRITE_THREADS {1} \
   CONFIG.PROTOCOL {AXI4LITE} \
   CONFIG.READ_WRITE_MODE {READ_WRITE} \
   CONFIG.RUSER_BITS_PER_BYTE {0} \
   CONFIG.RUSER_WIDTH {0} \
   CONFIG.SUPPORTS_NARROW_BURST {0} \
   CONFIG.WUSER_BITS_PER_BYTE {0} \
   CONFIG.WUSER_WIDTH {0} \
   ] $s_axi


  # Create ports
  set ctl_tx_pause_req [ create_bd_port -dir I -from 8 -to 0 ctl_tx_pause_req ]
  set ctl_tx_resend_pause [ create_bd_port -dir I ctl_tx_resend_pause ]
  set drp_clk [ create_bd_port -dir I -type clk drp_clk ]
  set gt_txusrclk2 [ create_bd_port -dir O -type clk gt_txusrclk2 ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {eth100g_rx:eth100g_tx} \
   CONFIG.FREQ_HZ {322265625} \
 ] $gt_txusrclk2
  set init_clk [ create_bd_port -dir I -type clk init_clk ]
  set pm_tick [ create_bd_port -dir I pm_tick ]
  set rx_clk [ create_bd_port -dir I -type clk -freq_hz 322265625 rx_clk ]
  set s_axi_aclk [ create_bd_port -dir I -type clk s_axi_aclk ]
  set s_axi_sreset [ create_bd_port -dir I -type rst s_axi_sreset ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_HIGH} \
 ] $s_axi_sreset
  set stat_rx_aligned [ create_bd_port -dir O stat_rx_aligned ]
  set stat_rx_pause_req [ create_bd_port -dir O -from 8 -to 0 stat_rx_pause_req ]
  set sys_reset [ create_bd_port -dir I -type rst sys_reset ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_HIGH} \
 ] $sys_reset
  set tx_ovfout [ create_bd_port -dir O tx_ovfout ]
  set tx_unfout [ create_bd_port -dir O tx_unfout ]
  set usr_rx_reset [ create_bd_port -dir O -type rst usr_rx_reset ]
  set usr_tx_reset [ create_bd_port -dir O -type rst usr_tx_reset ]

  # Create instance: cmac_usplus_0, and set properties
  set cmac_usplus_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:cmac_usplus:3.1 cmac_usplus_0 ]
  set_property -dict [ list \
   CONFIG.CMAC_CAUI4_MODE {1} \
   CONFIG.CMAC_CORE_SELECT {CMACE4_X0Y0} \
   CONFIG.ENABLE_AXI_INTERFACE {1} \
   CONFIG.GT_DRP_CLK {100} \
   CONFIG.GT_GROUP_SELECT {X0Y4~X0Y7} \
   CONFIG.GT_REF_CLK_FREQ {156.25} \
   CONFIG.INCLUDE_AUTO_NEG_LT_LOGIC {0} \
   CONFIG.INCLUDE_RS_FEC {1} \
   CONFIG.INCLUDE_SHARED_LOGIC {2} \
   CONFIG.INCLUDE_STATISTICS_COUNTERS {1} \
   CONFIG.LANE10_GT_LOC {NA} \
   CONFIG.LANE1_GT_LOC {X0Y4} \
   CONFIG.LANE2_GT_LOC {X0Y5} \
   CONFIG.LANE3_GT_LOC {X0Y6} \
   CONFIG.LANE4_GT_LOC {X0Y7} \
   CONFIG.LANE5_GT_LOC {NA} \
   CONFIG.LANE6_GT_LOC {NA} \
   CONFIG.LANE7_GT_LOC {NA} \
   CONFIG.LANE8_GT_LOC {NA} \
   CONFIG.LANE9_GT_LOC {NA} \
   CONFIG.NUM_LANES {4x25} \
   CONFIG.RX_CHECK_ACK {0} \
   CONFIG.RX_EQ_MODE {AUTO} \
   CONFIG.RX_FLOW_CONTROL {1} \
   CONFIG.TX_FLOW_CONTROL {1} \
   CONFIG.USER_INTERFACE {LBUS} \
 ] $cmac_usplus_0

  # Create instance: tie_loopback, and set properties
  set tie_loopback [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 tie_loopback ]
  set_property -dict [ list \
   CONFIG.CONST_VAL {0} \
   CONFIG.CONST_WIDTH {12} \
 ] $tie_loopback

  # Create instance: tie_zero, and set properties
  set tie_zero [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 tie_zero ]
  set_property -dict [ list \
   CONFIG.CONST_VAL {0} \
 ] $tie_zero

  # Create interface connections
  connect_bd_intf_net -intf_net RefClk_1 [get_bd_intf_ports refclk] [get_bd_intf_pins cmac_usplus_0/gt_ref_clk]
  connect_bd_intf_net -intf_net cmac_usplus_0_gt_serial_port [get_bd_intf_ports gt_serial_port_0] [get_bd_intf_pins cmac_usplus_0/gt_serial_port]
  connect_bd_intf_net -intf_net cmac_usplus_0_lbus_rx [get_bd_intf_ports eth100g_rx] [get_bd_intf_pins cmac_usplus_0/lbus_rx]
  connect_bd_intf_net -intf_net eth_100g_tx_1 [get_bd_intf_ports eth100g_tx] [get_bd_intf_pins cmac_usplus_0/lbus_tx]
  connect_bd_intf_net -intf_net sDrp_1 [get_bd_intf_ports core_drp] [get_bd_intf_pins cmac_usplus_0/core_drp]
  connect_bd_intf_net -intf_net s_axi_1 [get_bd_intf_ports s_axi] [get_bd_intf_pins cmac_usplus_0/s_axi]

  # Create port connections
  connect_bd_net -net SysClk_1 [get_bd_ports init_clk] [get_bd_pins cmac_usplus_0/init_clk]
  connect_bd_net -net aResetIn_1 [get_bd_ports sys_reset] [get_bd_pins cmac_usplus_0/sys_reset]
  connect_bd_net -net cmac_usplus_0_gt_txusrclk2 [get_bd_ports gt_txusrclk2] [get_bd_pins cmac_usplus_0/gt_txusrclk2]
  connect_bd_net -net cmac_usplus_0_stat_rx_aligned [get_bd_ports stat_rx_aligned] [get_bd_pins cmac_usplus_0/stat_rx_aligned]
  connect_bd_net -net cmac_usplus_0_stat_rx_pause_req [get_bd_ports stat_rx_pause_req] [get_bd_pins cmac_usplus_0/stat_rx_pause_req]
  connect_bd_net -net cmac_usplus_0_tx_ovfout [get_bd_ports tx_ovfout] [get_bd_pins cmac_usplus_0/tx_ovfout]
  connect_bd_net -net cmac_usplus_0_tx_unfout [get_bd_ports tx_unfout] [get_bd_pins cmac_usplus_0/tx_unfout]
  connect_bd_net -net cmac_usplus_0_usr_rx_reset [get_bd_ports usr_rx_reset] [get_bd_pins cmac_usplus_0/usr_rx_reset]
  connect_bd_net -net cmac_usplus_0_usr_tx_reset [get_bd_ports usr_tx_reset] [get_bd_pins cmac_usplus_0/usr_tx_reset]
  connect_bd_net -net ctl_tx_pause_req_1 [get_bd_ports ctl_tx_pause_req] [get_bd_pins cmac_usplus_0/ctl_tx_pause_req]
  connect_bd_net -net ctl_tx_resend_pause_1 [get_bd_ports ctl_tx_resend_pause] [get_bd_pins cmac_usplus_0/ctl_tx_resend_pause]
  connect_bd_net -net drp_clk_1 [get_bd_ports drp_clk] [get_bd_pins cmac_usplus_0/drp_clk]
  connect_bd_net -net pm_tick_1 [get_bd_ports pm_tick] [get_bd_pins cmac_usplus_0/pm_tick]
  connect_bd_net -net rx_clk_1 [get_bd_ports rx_clk] [get_bd_pins cmac_usplus_0/rx_clk]
  connect_bd_net -net s_axi_aclk_1 [get_bd_ports s_axi_aclk] [get_bd_pins cmac_usplus_0/s_axi_aclk]
  connect_bd_net -net s_axi_sreset_1 [get_bd_ports s_axi_sreset] [get_bd_pins cmac_usplus_0/s_axi_sreset]
  connect_bd_net -net tie_loopback_dout [get_bd_pins cmac_usplus_0/gt_loopback_in] [get_bd_pins tie_loopback/dout]
  connect_bd_net -net tie_zero_dout [get_bd_pins cmac_usplus_0/gtwiz_reset_rx_datapath] [get_bd_pins cmac_usplus_0/gtwiz_reset_tx_datapath] [get_bd_pins tie_zero/dout]

  # Create address segments
  assign_bd_address -offset 0x00000000 -range 0x00002000 -target_address_space [get_bd_addr_spaces s_axi] [get_bd_addr_segs cmac_usplus_0/s_axi/Reg] -force


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


