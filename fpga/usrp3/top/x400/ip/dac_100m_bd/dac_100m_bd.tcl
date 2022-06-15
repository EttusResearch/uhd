
################################################################
# This is a generated script based on design: dac_100m_bd
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
# source dac_100m_bd_script.tcl


# The design that will be created by this Tcl script contains the following 
# module references:
# dac_1_3_clk_converter, dac_2_1_clk_converter, duc_saturate

# Please add the sources of those modules before sourcing this Tcl script.

# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./myproj/project_1.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
   create_project project_1 myproj -part xczu28dr-ffvg1517-1-e
}


# CHANGE DESIGN NAME HERE
variable design_name
set design_name dac_100m_bd

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
xilinx.com:ip:xlconstant:1.1\
xilinx.com:ip:fir_compiler:7.2\
xilinx.com:ip:xlconcat:2.1\
xilinx.com:ip:axis_register_slice:1.1\
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
dac_1_3_clk_converter\
dac_2_1_clk_converter\
duc_saturate\
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
  set dac_data_in [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_data_in ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {122880000} \
   CONFIG.HAS_TKEEP {0} \
   CONFIG.HAS_TLAST {0} \
   CONFIG.HAS_TREADY {1} \
   CONFIG.HAS_TSTRB {0} \
   CONFIG.LAYERED_METADATA {undef} \
   CONFIG.TDATA_NUM_BYTES {4} \
   CONFIG.TDEST_WIDTH {0} \
   CONFIG.TID_WIDTH {0} \
   CONFIG.TUSER_WIDTH {0} \
   ] $dac_data_in

  set dac_data_out [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 dac_data_out ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   ] $dac_data_out


  # Create ports
  set dac_data_in_resetn_dclk [ create_bd_port -dir I -type rst dac_data_in_resetn_dclk ]
  set dac_data_in_resetn_rclk [ create_bd_port -dir I -type rst dac_data_in_resetn_rclk ]
  set dac_data_in_resetn_rclk2x [ create_bd_port -dir I -type rst dac_data_in_resetn_rclk2x ]
  set data_clk [ create_bd_port -dir I -type clk -freq_hz 122880000 data_clk ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {dac_data_in} \
   CONFIG.ASSOCIATED_RESET {dac_data_in_resetn_dclk} \
 ] $data_clk
  set rfdc_clk [ create_bd_port -dir I -type clk -freq_hz 184320000 rfdc_clk ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {dac_data_out} \
   CONFIG.ASSOCIATED_RESET {dac_data_in_resetn_rclk} \
 ] $rfdc_clk
  set rfdc_clk_2x [ create_bd_port -dir I -type clk -freq_hz 368640000 rfdc_clk_2x ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_RESET {dac_data_in_resetn_rclk2x} \
 ] $rfdc_clk_2x

  # Create instance: constant_high, and set properties
  set constant_high [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 constant_high ]

  # Create instance: dac_1_3_clk_converter_0, and set properties
  set block_name dac_1_3_clk_converter
  set block_cell_name dac_1_3_clk_converter_0
  if { [catch {set dac_1_3_clk_converter_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $dac_1_3_clk_converter_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: dac_2_1_clk_converter_0, and set properties
  set block_name dac_2_1_clk_converter
  set block_cell_name dac_2_1_clk_converter_0
  if { [catch {set dac_2_1_clk_converter_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $dac_2_1_clk_converter_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: dac_interpolator, and set properties
  set dac_interpolator [ create_bd_cell -type ip -vlnv xilinx.com:ip:fir_compiler:7.2 dac_interpolator ]
  set_property -dict [ list \
   CONFIG.Clock_Frequency {368.64} \
   CONFIG.CoefficientVector {-7,0,24,37,0,-78,-107,0,189,244,0,-389,-484,0,723,873,0,-1245,-1473,0,2029,2364,0,-3177,-3668,0,4862,5592,0,-7418,-8579,0,11675,13820,0,-20461,-26115,0,53699,108144,131069,108144,53699,0,-26115,-20461,0,13820,11675,0,-8579,-7418,0,5592,4862,0,-3668,-3177,0,2364,2029,0,-1473,-1245,0,873,723,0,-484,-389,0,244,189,0,-107,-78,0,37,24,0,-7}\
   CONFIG.Coefficient_Fractional_Bits {0} \
   CONFIG.Coefficient_Sets {1} \
   CONFIG.Coefficient_Sign {Signed} \
   CONFIG.Coefficient_Structure {Inferred} \
   CONFIG.Coefficient_Width {18} \
   CONFIG.ColumnConfig {14} \
   CONFIG.Data_Fractional_Bits {0} \
   CONFIG.Data_Width {16} \
   CONFIG.Decimation_Rate {1} \
   CONFIG.Filter_Architecture {Systolic_Multiply_Accumulate} \
   CONFIG.Filter_Type {Interpolation} \
   CONFIG.Has_ARESETn {true} \
   CONFIG.Interpolation_Rate {3} \
   CONFIG.M_DATA_Has_TREADY {false} \
   CONFIG.Number_Channels {1} \
   CONFIG.Number_Paths {2} \
   CONFIG.Output_Rounding_Mode {Convergent_Rounding_to_Even} \
   CONFIG.Output_Width {18} \
   CONFIG.Quantization {Integer_Coefficients} \
   CONFIG.RateSpecification {Frequency_Specification} \
   CONFIG.Reset_Data_Vector {false} \
   CONFIG.S_DATA_Has_FIFO {false} \
   CONFIG.Sample_Frequency {122.88} \
   CONFIG.Zero_Pack_Factor {1} \
 ] $dac_interpolator

  # Create instance: data_combiner, and set properties
  set data_combiner [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 data_combiner ]
  set_property -dict [ list \
   CONFIG.IN0_WIDTH {32} \
   CONFIG.IN1_WIDTH {32} \
 ] $data_combiner

  # Create instance: duc_saturate, and set properties
  set block_name duc_saturate
  set block_cell_name duc_saturate
  if { [catch {set duc_saturate [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $duc_saturate eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: registered_dac_data, and set properties
  set registered_dac_data [ create_bd_cell -type ip -vlnv xilinx.com:ip:axis_register_slice:1.1 registered_dac_data ]
  set_property -dict [ list \
   CONFIG.REG_CONFIG {1} \
   CONFIG.TDATA_NUM_BYTES {4} \
 ] $registered_dac_data

  # Create interface connections
  connect_bd_intf_net -intf_net dac_1_3_clk_converter_0_m_axis [get_bd_intf_pins dac_1_3_clk_converter_0/m_axis] [get_bd_intf_pins dac_interpolator/S_AXIS_DATA]
  connect_bd_intf_net -intf_net dac_2_1_clk_converter_0_m_axis [get_bd_intf_ports dac_data_out] [get_bd_intf_pins dac_2_1_clk_converter_0/m_axis]
  connect_bd_intf_net -intf_net dac_data_in_1 [get_bd_intf_ports dac_data_in] [get_bd_intf_pins dac_1_3_clk_converter_0/s_axis]

  # Create port connections
  connect_bd_net -net aclk_0_1 [get_bd_ports data_clk] [get_bd_pins dac_1_3_clk_converter_0/s_axis_aclk]
  connect_bd_net -net aresetn_0_1 [get_bd_ports dac_data_in_resetn_dclk] [get_bd_pins dac_1_3_clk_converter_0/s_axis_aresetn]
  connect_bd_net -net axis_register_slice_0_m_axis_tdata [get_bd_pins data_combiner/In0] [get_bd_pins registered_dac_data/m_axis_tdata]
  connect_bd_net -net dac_data_in_resetn_rclk2x [get_bd_ports dac_data_in_resetn_rclk2x] [get_bd_pins dac_1_3_clk_converter_0/m_axis_aresetn] [get_bd_pins dac_2_1_clk_converter_0/s_axis_aresetn] [get_bd_pins dac_interpolator/aresetn] [get_bd_pins registered_dac_data/aresetn]
  connect_bd_net -net dac_data_in_resetn_rclk_1 [get_bd_ports dac_data_in_resetn_rclk] [get_bd_pins dac_2_1_clk_converter_0/m_axis_aresetn]
  connect_bd_net -net dac_interpolator_m_axis_data_tdata [get_bd_pins dac_interpolator/m_axis_data_tdata] [get_bd_pins duc_saturate/cDataIn]
  connect_bd_net -net dac_interpolator_m_axis_data_tvalid [get_bd_pins dac_interpolator/m_axis_data_tvalid] [get_bd_pins duc_saturate/cDataValidIn]
  connect_bd_net -net data_combiner_dout [get_bd_pins dac_2_1_clk_converter_0/s_axis_tdata] [get_bd_pins data_combiner/dout]
  connect_bd_net -net ddc_saturate_0_cDataOut [get_bd_pins data_combiner/In1] [get_bd_pins duc_saturate/cDataOut] [get_bd_pins registered_dac_data/s_axis_tdata]
  connect_bd_net -net duc_saturate_0_cDataValidOut [get_bd_pins duc_saturate/cDataValidOut] [get_bd_pins registered_dac_data/s_axis_tvalid]
  connect_bd_net -net m_axis_aclk_0_1 [get_bd_ports rfdc_clk_2x] [get_bd_pins dac_1_3_clk_converter_0/m_axis_aclk] [get_bd_pins dac_2_1_clk_converter_0/s_axis_aclk] [get_bd_pins dac_interpolator/aclk] [get_bd_pins duc_saturate/Clk] [get_bd_pins registered_dac_data/aclk]
  connect_bd_net -net registered_dac_data_m_axis_tvalid [get_bd_pins dac_2_1_clk_converter_0/s_axis_tvalid] [get_bd_pins registered_dac_data/m_axis_tvalid]
  connect_bd_net -net rfdc_clk_1 [get_bd_ports rfdc_clk] [get_bd_pins dac_2_1_clk_converter_0/m_axis_aclk]
  connect_bd_net -net xlconstant_0_dout [get_bd_pins constant_high/dout] [get_bd_pins registered_dac_data/m_axis_tready]

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


