
################################################################
# This is a generated script based on design: adc_full_bd
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
# source adc_full_bd_script.tcl


# The design that will be created by this Tcl script contains the following 
# module references:
# adc_iq_repacker

# Please add the sources of those modules before sourcing this Tcl script.

# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./myproj/project_1.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
   create_project project_1 myproj -part xczu28dr-ffvg1517-2-e
}


# CHANGE DESIGN NAME HERE
variable design_name
set design_name adc_full_bd

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

  # Add USER_COMMENTS on $design_name
  set_property USER_COMMENTS.comment_0 "Scale_2x is a simple shift to left by 2 logic and does not need any pipeline stage" [get_bd_designs $design_name]

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
xilinx.com:ip:axis_register_slice:1.1\
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

##################################################################
# CHECK Modules
##################################################################
set bCheckModules 1
if { $bCheckModules == 1 } {
   set list_check_mods "\ 
adc_iq_repacker\
"

   set list_mods_missing ""
   common::send_gid_msg -ssname BD::TCL -id 2020 -severity "INFO" "Checking if the following modules exist in the project's sources: $list_check_mods ."

   foreach mod_vlnv $list_check_mods {
      if { [can_resolve_reference $mod_vlnv] == 0 } {
         lappend list_mods_missing $mod_vlnv
      }
   }

   if { $list_mods_missing ne "" } {
      catch {common::send_gid_msg -ssname BD::TCL -id 2021 -severity "ERROR" "The following module(s) are not found in the project: $list_mods_missing" }
      common::send_gid_msg -ssname BD::TCL -id 2022 -severity "INFO" "Please add source files for the missing module(s) above."
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
  set adc_data_out [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_data_out ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {512000000} \
   ] $adc_data_out

  set adc_i_data_in [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 adc_i_data_in ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {512000000} \
   CONFIG.HAS_TKEEP {0} \
   CONFIG.HAS_TLAST {0} \
   CONFIG.HAS_TREADY {1} \
   CONFIG.HAS_TSTRB {0} \
   CONFIG.LAYERED_METADATA {undef} \
   CONFIG.TDATA_NUM_BYTES {16} \
   CONFIG.TDEST_WIDTH {0} \
   CONFIG.TID_WIDTH {0} \
   CONFIG.TUSER_WIDTH {0} \
   ] $adc_i_data_in

  set adc_q_data_in [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 adc_q_data_in ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {512000000} \
   CONFIG.HAS_TKEEP {0} \
   CONFIG.HAS_TLAST {0} \
   CONFIG.HAS_TREADY {1} \
   CONFIG.HAS_TSTRB {0} \
   CONFIG.LAYERED_METADATA {undef} \
   CONFIG.TDATA_NUM_BYTES {16} \
   CONFIG.TDEST_WIDTH {0} \
   CONFIG.TID_WIDTH {0} \
   CONFIG.TUSER_WIDTH {0} \
   ] $adc_q_data_in


  # Create ports
  set enable_data_to_repacker_rclk [ create_bd_port -dir I enable_data_to_repacker_rclk ]
  set rfdc_adc_axi_resetn_rclk [ create_bd_port -dir I -type rst rfdc_adc_axi_resetn_rclk ]
  set rfdc_clk [ create_bd_port -dir I -type clk -freq_hz 512000000 rfdc_clk ]
  set swap_iq_rclk [ create_bd_port -dir I swap_iq_rclk ]

  # Create instance: adc_data_to_axi, and set properties
  set adc_data_to_axi [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_register_slice:1.1 adc_data_to_axi ]
  set_property -dict [ list \
   CONFIG.REG_CONFIG {1} \
   CONFIG.TDATA_NUM_BYTES {32} \
 ] $adc_data_to_axi

  # Create instance: adc_i_data_from_axi, and set properties
  set adc_i_data_from_axi [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_register_slice:1.1 adc_i_data_from_axi ]
  set_property -dict [ list \
   CONFIG.REG_CONFIG {0} \
   CONFIG.TDATA_NUM_BYTES {16} \
 ] $adc_i_data_from_axi

  # Create instance: adc_iq_repacker, and set properties
  set block_name adc_iq_repacker
  set block_cell_name adc_iq_repacker
  if { [catch {set adc_iq_repacker [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $adc_iq_repacker eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
    set_property -dict [ list \
   CONFIG.SPC {8} \
 ] $adc_iq_repacker

  # Create instance: adc_q_data_from_axi, and set properties
  set adc_q_data_from_axi [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_register_slice:1.1 adc_q_data_from_axi ]
  set_property -dict [ list \
   CONFIG.REG_CONFIG {0} \
   CONFIG.TDATA_NUM_BYTES {16} \
 ] $adc_q_data_from_axi

  # Create instance: const_1, and set properties
  set const_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 const_1 ]

  # Create interface connections
  connect_bd_intf_net -intf_net adc_i_data_in_1 [get_bd_intf_ports adc_i_data_in] [get_bd_intf_pins adc_i_data_from_axi/S_AXIS]
  connect_bd_intf_net -intf_net adc_iq_repacker_0_data_out [get_bd_intf_pins adc_data_to_axi/S_AXIS] [get_bd_intf_pins adc_iq_repacker/data_out]
  connect_bd_intf_net -intf_net adc_q_data_from_axi1_M_AXIS [get_bd_intf_ports adc_data_out] [get_bd_intf_pins adc_data_to_axi/M_AXIS]
  connect_bd_intf_net -intf_net adc_q_data_in_1 [get_bd_intf_ports adc_q_data_in] [get_bd_intf_pins adc_q_data_from_axi/S_AXIS]

  # Create port connections
  connect_bd_net -net adc_i_data_from_axi_m_axis_tdata [get_bd_pins adc_i_data_from_axi/m_axis_tdata] [get_bd_pins adc_iq_repacker/adc_i_in]
  connect_bd_net -net adc_i_data_from_axi_m_axis_tvalid [get_bd_pins adc_i_data_from_axi/m_axis_tvalid] [get_bd_pins adc_iq_repacker/valid_in]
  connect_bd_net -net adc_q_data_from_axi_m_axis_tdata [get_bd_pins adc_iq_repacker/adc_q_in] [get_bd_pins adc_q_data_from_axi/m_axis_tdata]
  connect_bd_net -net const_1_dout [get_bd_pins adc_i_data_from_axi/m_axis_tready] [get_bd_pins adc_q_data_from_axi/m_axis_tready] [get_bd_pins const_1/dout]
  connect_bd_net -net enable_data_to_repacker_rclk_1 [get_bd_ports enable_data_to_repacker_rclk] [get_bd_pins adc_iq_repacker/enable]
  connect_bd_net -net rfdc_adc_axi_resetn_rclk_1 [get_bd_ports rfdc_adc_axi_resetn_rclk] [get_bd_pins adc_data_to_axi/aresetn] [get_bd_pins adc_i_data_from_axi/aresetn] [get_bd_pins adc_q_data_from_axi/aresetn]
  connect_bd_net -net rfdc_clk_1 [get_bd_ports rfdc_clk] [get_bd_pins adc_data_to_axi/aclk] [get_bd_pins adc_i_data_from_axi/aclk] [get_bd_pins adc_iq_repacker/clk] [get_bd_pins adc_q_data_from_axi/aclk]
  connect_bd_net -net swap_iq_rclk_1 [get_bd_ports swap_iq_rclk] [get_bd_pins adc_iq_repacker/swap_iq]

  # Create address segments


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


