
################################################################
# This is a generated script based on design: x410_ps_rfdc_bd
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
# source x410_ps_rfdc_bd_script.tcl


# The design that will be created by this Tcl script contains the following 
# module references:
# capture_sysref, x410_clock_gates, rf_nco_reset, x410_rf_reset_controller, x410_rf_reset_controller, gpio_to_axis_mux

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
set design_name x410_ps_rfdc_bd

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
  set_property USER_COMMENTS.comment_2 "reg_reset_mmcm:
[0] = mmcm_reset_n (default b0)" [get_bd_designs $design_name]
  set_property USER_COMMENTS.comment_3 "reg_invert_iq:
[15:8] = invert DAC channels
[7:0] = invert ADC channels" [get_bd_designs $design_name]
  set_property USER_COMMENTS.comment_4 "The ADC/DAC mappings need to match the RFDC settings,
but there's no clear way to pull them out of the RFDC configuration
above. This means that the values in these constants need to be
manually matched to the RFDC configuration object.

The format of the ADC/DAC maps is documented in gen_x4xx_rfdc_regmap.py." [get_bd_designs $design_name]

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
ettus.com:ip:axi_bitq:1.0\
xilinx.com:ip:zynq_ultra_ps_e:3.3\
xilinx.com:ip:xlconcat:2.1\
xilinx.com:ip:xlconstant:1.1\
xilinx.com:ip:clk_wiz:6.0\
xilinx.com:ip:axi_gpio:2.0\
xilinx.com:ip:usp_rf_data_converter:2.5\
xilinx.com:ip:xlslice:1.0\
xilinx.com:ip:axi_protocol_converter:2.1\
xilinx.com:ip:axi_dma:7.1\
xilinx.com:ip:smartconnect:1.0\
xilinx.com:ip:util_ds_buf:2.2\
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
capture_sysref\
x410_clock_gates\
rf_nco_reset\
x410_rf_reset_controller\
x410_rf_reset_controller\
gpio_to_axis_mux\
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


# Hierarchical cell: rf_clock_buffers
proc create_hier_cell_rf_clock_buffers { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_rf_clock_buffers() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins

  # Create pins
  create_bd_pin -dir O -from 0 -to 0 rfdc_clk
  create_bd_pin -dir O -from 0 -to 0 rfdc_clk_2x
  create_bd_pin -dir I -from 0 -to 0 rfdc_clk_2x_ce
  create_bd_pin -dir I -from 0 -to 0 -type clk rfdc_clk_2x_pll
  create_bd_pin -dir I -from 0 -to 0 rfdc_clk_ce
  create_bd_pin -dir I -from 0 -to 0 -type clk rfdc_clk_pll

  # Create instance: rfdc_clk_1x_buf, and set properties
  set rfdc_clk_1x_buf [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_ds_buf:2.2 rfdc_clk_1x_buf ]
  set_property -dict [ list \
   CONFIG.C_BUF_TYPE {BUFGCE} \
 ] $rfdc_clk_1x_buf

  # Create instance: rfdc_clk_2x_buf, and set properties
  set rfdc_clk_2x_buf [ create_bd_cell -type ip -vlnv xilinx.com:ip:util_ds_buf:2.2 rfdc_clk_2x_buf ]
  set_property -dict [ list \
   CONFIG.C_BUF_TYPE {BUFGCE} \
 ] $rfdc_clk_2x_buf

  # Create port connections
  connect_bd_net -net BUFGCE_I2_1 [get_bd_pins rfdc_clk_2x_pll] [get_bd_pins rfdc_clk_2x_buf/BUFGCE_I]
  connect_bd_net -net rfdc_clk_1 [get_bd_pins rfdc_clk_pll] [get_bd_pins rfdc_clk_1x_buf/BUFGCE_I]
  connect_bd_net -net rfdc_clk_1x_buf_BUFGCE_O [get_bd_pins rfdc_clk] [get_bd_pins rfdc_clk_1x_buf/BUFGCE_O]
  connect_bd_net -net rfdc_clk_2x_buf_BUFGCE_O [get_bd_pins rfdc_clk_2x] [get_bd_pins rfdc_clk_2x_buf/BUFGCE_O]
  connect_bd_net -net rfdc_clk_2x_ce_1 [get_bd_pins rfdc_clk_2x_ce] [get_bd_pins rfdc_clk_2x_buf/BUFGCE_CE]
  connect_bd_net -net rfdc_clk_ce_1 [get_bd_pins rfdc_clk_ce] [get_bd_pins rfdc_clk_1x_buf/BUFGCE_CE]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: calibration_muxes
proc create_hier_cell_calibration_muxes { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_calibration_muxes() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI_1

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_tile228_ch0_din

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_tile228_ch1_din

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_tile229_ch0_din

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_tile229_ch1_din

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 m_axis_0_0

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 m_axis_1_0

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 m_axis_2_0

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 m_axis_3_0


  # Create pins
  create_bd_pin -dir I -type clk s_axi_aclk_0
  create_bd_pin -dir I -type rst s_axi_config_aresetn
  create_bd_pin -dir I -type clk s_axi_config_clk

  # Create instance: axi_gpio_data, and set properties
  set axi_gpio_data [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 axi_gpio_data ]
  set_property -dict [ list \
   CONFIG.C_ALL_OUTPUTS {1} \
   CONFIG.C_ALL_OUTPUTS_2 {1} \
   CONFIG.C_GPIO2_WIDTH {8} \
   CONFIG.C_IS_DUAL {1} \
 ] $axi_gpio_data

  # Create instance: constant0_1b, and set properties
  set constant0_1b [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 constant0_1b ]
  set_property -dict [ list \
   CONFIG.CONST_VAL {0} \
 ] $constant0_1b

  # Create instance: gpio_to_axis_mux_0, and set properties
  set block_name gpio_to_axis_mux
  set block_cell_name gpio_to_axis_mux_0
  if { [catch {set gpio_to_axis_mux_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $gpio_to_axis_mux_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
    set_property -dict [ list \
   CONFIG.kGpioWidth {32} \
 ] $gpio_to_axis_mux_0

  # Create instance: xlconstant_1, and set properties
  set xlconstant_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_1 ]
  set_property -dict [ list \
   CONFIG.CONST_VAL {0} \
   CONFIG.CONST_WIDTH {256} \
 ] $xlconstant_1

  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins m_axis_0_0] [get_bd_intf_pins gpio_to_axis_mux_0/m_axis_0]
  connect_bd_intf_net -intf_net Conn2 [get_bd_intf_pins m_axis_1_0] [get_bd_intf_pins gpio_to_axis_mux_0/m_axis_1]
  connect_bd_intf_net -intf_net Conn5 [get_bd_intf_pins dac_tile228_ch0_din] [get_bd_intf_pins gpio_to_axis_mux_0/s_axis_0]
  connect_bd_intf_net -intf_net Conn6 [get_bd_intf_pins dac_tile228_ch1_din] [get_bd_intf_pins gpio_to_axis_mux_0/s_axis_1]
  connect_bd_intf_net -intf_net Conn10 [get_bd_intf_pins S_AXI_1] [get_bd_intf_pins axi_gpio_data/S_AXI]
  connect_bd_intf_net -intf_net dac_tile229_ch0_din_1 [get_bd_intf_pins dac_tile229_ch0_din] [get_bd_intf_pins gpio_to_axis_mux_0/s_axis_4]
  connect_bd_intf_net -intf_net dac_tile229_ch1_din_1 [get_bd_intf_pins dac_tile229_ch1_din] [get_bd_intf_pins gpio_to_axis_mux_0/s_axis_5]
  connect_bd_intf_net -intf_net gpio_to_axis_mux_0_m_axis_4 [get_bd_intf_pins m_axis_2_0] [get_bd_intf_pins gpio_to_axis_mux_0/m_axis_4]
  connect_bd_intf_net -intf_net gpio_to_axis_mux_0_m_axis_5 [get_bd_intf_pins m_axis_3_0] [get_bd_intf_pins gpio_to_axis_mux_0/m_axis_5]

  # Create port connections
  connect_bd_net -net Net [get_bd_pins s_axi_config_clk] [get_bd_pins axi_gpio_data/s_axi_aclk]
  connect_bd_net -net Net1 [get_bd_pins s_axi_config_aresetn] [get_bd_pins axi_gpio_data/s_axi_aresetn]
  connect_bd_net -net axi_gpio_0_gpio_io_o [get_bd_pins axi_gpio_data/gpio_io_o] [get_bd_pins gpio_to_axis_mux_0/gpio]
  connect_bd_net -net axi_gpio_data_gpio2_io_o [get_bd_pins axi_gpio_data/gpio2_io_o] [get_bd_pins gpio_to_axis_mux_0/mux_select]
  connect_bd_net -net constant0_1b_dout [get_bd_pins constant0_1b/dout] [get_bd_pins gpio_to_axis_mux_0/s_axis_tvalid_2] [get_bd_pins gpio_to_axis_mux_0/s_axis_tvalid_3] [get_bd_pins gpio_to_axis_mux_0/s_axis_tvalid_6] [get_bd_pins gpio_to_axis_mux_0/s_axis_tvalid_7]
  connect_bd_net -net s_axi_aclk_0_1 [get_bd_pins s_axi_aclk_0] [get_bd_pins gpio_to_axis_mux_0/m_axis_0_aclk] [get_bd_pins gpio_to_axis_mux_0/m_axis_1_aclk] [get_bd_pins gpio_to_axis_mux_0/m_axis_2_aclk] [get_bd_pins gpio_to_axis_mux_0/m_axis_3_aclk] [get_bd_pins gpio_to_axis_mux_0/m_axis_4_aclk] [get_bd_pins gpio_to_axis_mux_0/m_axis_5_aclk] [get_bd_pins gpio_to_axis_mux_0/m_axis_6_aclk] [get_bd_pins gpio_to_axis_mux_0/m_axis_7_aclk] [get_bd_pins gpio_to_axis_mux_0/s_axis_0_aclk] [get_bd_pins gpio_to_axis_mux_0/s_axis_1_aclk] [get_bd_pins gpio_to_axis_mux_0/s_axis_2_aclk] [get_bd_pins gpio_to_axis_mux_0/s_axis_3_aclk] [get_bd_pins gpio_to_axis_mux_0/s_axis_4_aclk] [get_bd_pins gpio_to_axis_mux_0/s_axis_5_aclk] [get_bd_pins gpio_to_axis_mux_0/s_axis_6_aclk] [get_bd_pins gpio_to_axis_mux_0/s_axis_7_aclk]
  connect_bd_net -net xlconstant_1_dout [get_bd_pins gpio_to_axis_mux_0/s_axis_tdata_2] [get_bd_pins gpio_to_axis_mux_0/s_axis_tdata_3] [get_bd_pins gpio_to_axis_mux_0/s_axis_tdata_6] [get_bd_pins gpio_to_axis_mux_0/s_axis_tdata_7] [get_bd_pins xlconstant_1/dout]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: ThresholdRegister
proc create_hier_cell_ThresholdRegister { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_ThresholdRegister() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S_AXI


  # Create pins
  create_bd_pin -dir I -from 0 -to 0 In0
  create_bd_pin -dir I -from 0 -to 0 In1
  create_bd_pin -dir I -from 0 -to 0 In2
  create_bd_pin -dir I -from 0 -to 0 In3
  create_bd_pin -dir I -from 0 -to 0 In5
  create_bd_pin -dir I -from 0 -to 0 In6
  create_bd_pin -dir I -from 0 -to 0 In7
  create_bd_pin -dir I -from 0 -to 0 In8
  create_bd_pin -dir I -type rst s_axi_config_aresetn
  create_bd_pin -dir I -type clk s_axi_config_clk

  # Create instance: axi_gpio_0, and set properties
  set axi_gpio_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 axi_gpio_0 ]
  set_property -dict [ list \
   CONFIG.C_ALL_INPUTS {1} \
   CONFIG.C_GPIO_WIDTH {12} \
 ] $axi_gpio_0

  # Create instance: xlconcat_0, and set properties
  set xlconcat_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0 ]
  set_property -dict [ list \
   CONFIG.NUM_PORTS {9} \
 ] $xlconcat_0

  # Create instance: xlconstant_0, and set properties
  set xlconstant_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_0 ]
  set_property -dict [ list \
   CONFIG.CONST_WIDTH {4} \
 ] $xlconstant_0

  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins S_AXI] [get_bd_intf_pins axi_gpio_0/S_AXI]

  # Create port connections
  connect_bd_net -net In0_1 [get_bd_pins In0] [get_bd_pins xlconcat_0/In0]
  connect_bd_net -net In1_1 [get_bd_pins In1] [get_bd_pins xlconcat_0/In1]
  connect_bd_net -net In2_1 [get_bd_pins In2] [get_bd_pins xlconcat_0/In2]
  connect_bd_net -net In3_1 [get_bd_pins In3] [get_bd_pins xlconcat_0/In3]
  connect_bd_net -net In5_1 [get_bd_pins In5] [get_bd_pins xlconcat_0/In5]
  connect_bd_net -net In6_1 [get_bd_pins In6] [get_bd_pins xlconcat_0/In6]
  connect_bd_net -net In7_1 [get_bd_pins In7] [get_bd_pins xlconcat_0/In7]
  connect_bd_net -net In8_1 [get_bd_pins In8] [get_bd_pins xlconcat_0/In8]
  connect_bd_net -net s_axi_config_aresetn_1 [get_bd_pins s_axi_config_aresetn] [get_bd_pins axi_gpio_0/s_axi_aresetn]
  connect_bd_net -net s_axi_config_clk_1 [get_bd_pins s_axi_config_clk] [get_bd_pins axi_gpio_0/s_axi_aclk]
  connect_bd_net -net xlconcat_0_dout [get_bd_pins axi_gpio_0/gpio_io_i] [get_bd_pins xlconcat_0/dout]
  connect_bd_net -net xlconstant_0_dout [get_bd_pins xlconcat_0/In4] [get_bd_pins xlconstant_0/dout]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: eth_dma_internal
proc create_hier_cell_eth_dma_internal { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_eth_dma_internal() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_to_ps

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 m_axis_eth_dma

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_eth_dma_ctrl

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 s_axis_eth_dma


  # Create pins
  create_bd_pin -dir I -type clk bus_clk
  create_bd_pin -dir I -type rst bus_rstn
  create_bd_pin -dir I -type clk clk40
  create_bd_pin -dir I -type rst clk40_rstn
  create_bd_pin -dir O -from 1 -to 0 irq

  # Create instance: axi_eth_dma_internal, and set properties
  set axi_eth_dma_internal [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dma:7.1 axi_eth_dma_internal ]
  set_property -dict [ list \
   CONFIG.c_addr_width {36} \
   CONFIG.c_enable_multi_channel {0} \
   CONFIG.c_include_mm2s_dre {1} \
   CONFIG.c_include_s2mm {1} \
   CONFIG.c_include_s2mm_dre {1} \
   CONFIG.c_m_axi_mm2s_data_width {128} \
   CONFIG.c_m_axi_s2mm_data_width {128} \
   CONFIG.c_m_axis_mm2s_tdata_width {64} \
   CONFIG.c_micro_dma {0} \
   CONFIG.c_mm2s_burst_size {16} \
   CONFIG.c_s2mm_burst_size {16} \
   CONFIG.c_sg_include_stscntrl_strm {0} \
 ] $axi_eth_dma_internal

  # Create instance: pl_ps_irq1_concat, and set properties
  set pl_ps_irq1_concat [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 pl_ps_irq1_concat ]
  set_property -dict [ list \
   CONFIG.NUM_PORTS {2} \
 ] $pl_ps_irq1_concat

  # Create instance: smartconnect_dma, and set properties
  set smartconnect_dma [ create_bd_cell -type ip -vlnv xilinx.com:ip:smartconnect:1.0 smartconnect_dma ]
  set_property -dict [ list \
   CONFIG.NUM_SI {3} \
 ] $smartconnect_dma

  # Create interface connections
  connect_bd_intf_net -intf_net Conn3 [get_bd_intf_pins m_axis_eth_dma] [get_bd_intf_pins axi_eth_dma_internal/M_AXIS_MM2S]
  connect_bd_intf_net -intf_net Conn4 [get_bd_intf_pins s_axis_eth_dma] [get_bd_intf_pins axi_eth_dma_internal/S_AXIS_S2MM]
  connect_bd_intf_net -intf_net axi_eth_dma_internal_M_AXI_MM2S [get_bd_intf_pins axi_eth_dma_internal/M_AXI_MM2S] [get_bd_intf_pins smartconnect_dma/S01_AXI]
  connect_bd_intf_net -intf_net axi_eth_dma_internal_M_AXI_S2MM [get_bd_intf_pins axi_eth_dma_internal/M_AXI_S2MM] [get_bd_intf_pins smartconnect_dma/S02_AXI]
  connect_bd_intf_net -intf_net axi_eth_dma_internal_M_AXI_SG [get_bd_intf_pins axi_eth_dma_internal/M_AXI_SG] [get_bd_intf_pins smartconnect_dma/S00_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_common_m_axi_eth_dma_ctrl [get_bd_intf_pins s_axi_eth_dma_ctrl] [get_bd_intf_pins axi_eth_dma_internal/S_AXI_LITE]
  connect_bd_intf_net -intf_net smartconnect_0_M00_AXI [get_bd_intf_pins m_axi_to_ps] [get_bd_intf_pins smartconnect_dma/M00_AXI]

  # Create port connections
  connect_bd_net -net axi_eth_dma_internal_mm2s_introut [get_bd_pins axi_eth_dma_internal/mm2s_introut] [get_bd_pins pl_ps_irq1_concat/In0]
  connect_bd_net -net axi_eth_dma_internal_s2mm_introut [get_bd_pins axi_eth_dma_internal/s2mm_introut] [get_bd_pins pl_ps_irq1_concat/In1]
  connect_bd_net -net bus_rstn_1 [get_bd_pins bus_rstn] [get_bd_pins smartconnect_dma/aresetn]
  connect_bd_net -net clk40_1 [get_bd_pins clk40] [get_bd_pins axi_eth_dma_internal/s_axi_lite_aclk]
  connect_bd_net -net clk40_rstn_1 [get_bd_pins clk40_rstn] [get_bd_pins axi_eth_dma_internal/axi_resetn]
  connect_bd_net -net m_axi_sg_aclk_0_1 [get_bd_pins bus_clk] [get_bd_pins axi_eth_dma_internal/m_axi_mm2s_aclk] [get_bd_pins axi_eth_dma_internal/m_axi_s2mm_aclk] [get_bd_pins axi_eth_dma_internal/m_axi_sg_aclk] [get_bd_pins smartconnect_dma/aclk]
  connect_bd_net -net xlconcat_0_dout [get_bd_pins irq] [get_bd_pins pl_ps_irq1_concat/dout]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: axi_interconnect_common
proc create_hier_cell_axi_interconnect_common { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_axi_interconnect_common() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_app

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_core

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_eth_dma_ctrl

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_eth_internal

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_jtag

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_mpm_ep

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_rf

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_rpu

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_common

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_lpd


  # Create pins
  create_bd_pin -dir I -type clk clk40
  create_bd_pin -dir I -type rst clk40_rstn

  # Create instance: axi_interconnect_0, and set properties
  set axi_interconnect_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_0 ]
  set_property -dict [ list \
   CONFIG.NUM_MI {7} \
 ] $axi_interconnect_0

  # Create instance: axi_protocol_convert_0, and set properties
  set axi_protocol_convert_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_protocol_converter:2.1 axi_protocol_convert_0 ]

  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins m_axi_jtag] [get_bd_intf_pins axi_interconnect_0/M01_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M00_AXI [get_bd_intf_pins m_axi_app] [get_bd_intf_pins axi_interconnect_0/M00_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M02_AXI [get_bd_intf_pins m_axi_rf] [get_bd_intf_pins axi_interconnect_0/M02_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M03_AXI [get_bd_intf_pins m_axi_mpm_ep] [get_bd_intf_pins axi_interconnect_0/M03_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M04_AXI [get_bd_intf_pins m_axi_core] [get_bd_intf_pins axi_interconnect_0/M04_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M05_AXI [get_bd_intf_pins m_axi_eth_dma_ctrl] [get_bd_intf_pins axi_interconnect_0/M05_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M06_AXI [get_bd_intf_pins m_axi_eth_internal] [get_bd_intf_pins axi_interconnect_0/M06_AXI]
  connect_bd_intf_net -intf_net axi_protocol_convert_0_M_AXI [get_bd_intf_pins m_axi_rpu] [get_bd_intf_pins axi_protocol_convert_0/M_AXI]
  connect_bd_intf_net -intf_net inst_zynq_ps_M_AXI_HPM0_FPD [get_bd_intf_pins s_axi_common] [get_bd_intf_pins axi_interconnect_0/S00_AXI]
  connect_bd_intf_net -intf_net s_axi_lpd_1 [get_bd_intf_pins s_axi_lpd] [get_bd_intf_pins axi_protocol_convert_0/S_AXI]

  # Create port connections
  connect_bd_net -net M01_ARESETN_1 [get_bd_pins clk40_rstn] [get_bd_pins axi_interconnect_0/ARESETN] [get_bd_pins axi_interconnect_0/M00_ARESETN] [get_bd_pins axi_interconnect_0/M01_ARESETN] [get_bd_pins axi_interconnect_0/M02_ARESETN] [get_bd_pins axi_interconnect_0/M03_ARESETN] [get_bd_pins axi_interconnect_0/M04_ARESETN] [get_bd_pins axi_interconnect_0/M05_ARESETN] [get_bd_pins axi_interconnect_0/M06_ARESETN] [get_bd_pins axi_interconnect_0/S00_ARESETN] [get_bd_pins axi_protocol_convert_0/aresetn]
  connect_bd_net -net clk40_1 [get_bd_pins clk40] [get_bd_pins axi_interconnect_0/ACLK] [get_bd_pins axi_interconnect_0/M00_ACLK] [get_bd_pins axi_interconnect_0/M01_ACLK] [get_bd_pins axi_interconnect_0/M02_ACLK] [get_bd_pins axi_interconnect_0/M03_ACLK] [get_bd_pins axi_interconnect_0/M04_ACLK] [get_bd_pins axi_interconnect_0/M05_ACLK] [get_bd_pins axi_interconnect_0/M06_ACLK] [get_bd_pins axi_interconnect_0/S00_ACLK] [get_bd_pins axi_protocol_convert_0/aclk]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: rfdc
proc create_hier_cell_rfdc { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_rfdc() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 adc0_clk

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 adc2_clk

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile224_ch0_dout_i

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile224_ch0_dout_q

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 adc_tile224_ch0_vin

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile224_ch1_dout_i

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile224_ch1_dout_q

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 adc_tile224_ch1_vin

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile226_ch0_dout_i

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile226_ch0_dout_q

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 adc_tile226_ch0_vin

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile226_ch1_dout_i

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile226_ch1_dout_q

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 adc_tile226_ch1_vin

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 dac0_clk

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 dac1_clk

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_tile228_ch0_din

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 dac_tile228_ch0_vout

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_tile228_ch1_din

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 dac_tile228_ch1_vout

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_tile229_ch0_din

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 dac_tile229_ch0_vout

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_tile229_ch1_din

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 dac_tile229_ch1_vout

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_config

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:display_usp_rf_data_converter:diff_pins_rtl:1.0 sysref_rf_in


  # Create pins
  create_bd_pin -dir O adc_data_out_resetn_dclk
  create_bd_pin -dir O adc_enable_data_rclk
  create_bd_pin -dir I adc_reset_pulse_dclk
  create_bd_pin -dir O adc_rfdc_axi_resetn_rclk
  create_bd_pin -dir O dac_data_in_resetn_dclk
  create_bd_pin -dir O dac_data_in_resetn_dclk2x
  create_bd_pin -dir O dac_data_in_resetn_rclk
  create_bd_pin -dir O dac_data_in_resetn_rclk2x
  create_bd_pin -dir I dac_reset_pulse_dclk
  create_bd_pin -dir O -type clk data_clk
  create_bd_pin -dir O -type clk data_clk_2x
  create_bd_pin -dir O data_clock_locked
  create_bd_pin -dir I enable_gated_clocks_clk40
  create_bd_pin -dir I enable_sysref_rclk
  create_bd_pin -dir O fir_resetn_rclk2x
  create_bd_pin -dir O gated_base_clks_valid_clk40
  create_bd_pin -dir O nco_reset_done_dclk
  create_bd_pin -dir I -type clk pll_ref_clk_in
  create_bd_pin -dir O -type clk pll_ref_clk_out
  create_bd_pin -dir O -from 3 -to 0 radio0_invert_adc_iq_r0clk
  create_bd_pin -dir O -from 3 -to 0 radio0_invert_dac_iq_r0clk
  create_bd_pin -dir O -from 3 -to 0 radio1_invert_adc_iq_r1clk
  create_bd_pin -dir O -from 3 -to 0 radio1_invert_dac_iq_r1clk
  create_bd_pin -dir I -from 31 -to 0 rf_axi_status_sclk
  create_bd_pin -dir I -from 31 -to 0 rf_dsp_info_sclk
  create_bd_pin -dir I -from 31 -to 0 rf_rfdc_info_sclk
  create_bd_pin -dir O -from 0 -to 0 rfdc_clk
  create_bd_pin -dir O -from 0 -to 0 rfdc_clk_2x
  create_bd_pin -dir O -type intr rfdc_irq
  create_bd_pin -dir I -type rst s_axi_config_aresetn
  create_bd_pin -dir I -type clk s_axi_config_clk
  create_bd_pin -dir I start_nco_reset_dclk
  create_bd_pin -dir O sysref_out_pclk
  create_bd_pin -dir O sysref_out_rclk
  create_bd_pin -dir I sysref_pl_in

  # Create instance: ThresholdRegister
  create_hier_cell_ThresholdRegister $hier_obj ThresholdRegister

  # Create instance: axi_interconnect_rf, and set properties
  set axi_interconnect_rf [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 axi_interconnect_rf ]
  set_property -dict [ list \
   CONFIG.ENABLE_ADVANCED_OPTIONS {0} \
   CONFIG.NUM_MI {13} \
   CONFIG.STRATEGY {1} \
 ] $axi_interconnect_rf

  # Create instance: calibration_muxes
  create_hier_cell_calibration_muxes $hier_obj calibration_muxes

  # Create instance: capture_sysref, and set properties
  set block_name capture_sysref
  set block_cell_name capture_sysref
  if { [catch {set capture_sysref [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $capture_sysref eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  set_property -dict [ list \
   CONFIG.FREQ_HZ {61440000} \
   CONFIG.PHASE {0} \
   CONFIG.CLK_DOMAIN {x410_ps_rfdc_bd_pll_ref_clk_in} \
 ] [get_bd_pins /rfdc/capture_sysref/pll_ref_clk]

  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   CONFIG.PHASE {0} \
   CONFIG.CLK_DOMAIN {x410_ps_rfdc_bd_pll_ref_clk_in} \
 ] [get_bd_pins /rfdc/capture_sysref/rfdc_clk]

  # Create instance: clock_gates_0, and set properties
  set block_name x410_clock_gates
  set block_cell_name clock_gates_0
  if { [catch {set clock_gates_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $clock_gates_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: const_1, and set properties
  set const_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 const_1 ]

  # Create instance: data_clock_mmcm, and set properties
  set data_clock_mmcm [ create_bd_cell -type ip -vlnv xilinx.com:ip:clk_wiz:6.0 data_clock_mmcm ]
  set_property -dict [ list \
   CONFIG.AXI_DRP {true} \
   CONFIG.CLKIN1_JITTER_PS {162.76} \
   CONFIG.CLKOUT1_DRIVES {Buffer} \
   CONFIG.CLKOUT1_JITTER {116.960} \
   CONFIG.CLKOUT1_PHASE_ERROR {124.626} \
   CONFIG.CLKOUT1_REQUESTED_OUT_FREQ {61.44} \
   CONFIG.CLKOUT2_DRIVES {Buffer} \
   CONFIG.CLKOUT2_JITTER {104.559} \
   CONFIG.CLKOUT2_PHASE_ERROR {124.626} \
   CONFIG.CLKOUT2_REQUESTED_OUT_FREQ {122.88} \
   CONFIG.CLKOUT2_USED {true} \
   CONFIG.CLKOUT3_DRIVES {Buffer} \
   CONFIG.CLKOUT3_JITTER {98.017} \
   CONFIG.CLKOUT3_PHASE_ERROR {124.626} \
   CONFIG.CLKOUT3_REQUESTED_OUT_FREQ {184.32} \
   CONFIG.CLKOUT3_USED {true} \
   CONFIG.CLKOUT4_DRIVES {Buffer} \
   CONFIG.CLKOUT4_JITTER {93.671} \
   CONFIG.CLKOUT4_PHASE_ERROR {124.626} \
   CONFIG.CLKOUT4_REQUESTED_OUT_FREQ {245.76} \
   CONFIG.CLKOUT4_USED {true} \
   CONFIG.CLKOUT5_DRIVES {Buffer} \
   CONFIG.CLKOUT5_JITTER {87.938} \
   CONFIG.CLKOUT5_PHASE_ERROR {124.626} \
   CONFIG.CLKOUT5_REQUESTED_OUT_FREQ {368.64} \
   CONFIG.CLKOUT5_USED {true} \
   CONFIG.CLKOUT6_DRIVES {Buffer} \
   CONFIG.CLKOUT7_DRIVES {Buffer} \
   CONFIG.CLK_OUT1_PORT {pll_ref_clk_out} \
   CONFIG.CLK_OUT2_PORT {data_clk} \
   CONFIG.CLK_OUT3_PORT {rfdc_clk} \
   CONFIG.CLK_OUT4_PORT {data_clk_2x} \
   CONFIG.CLK_OUT5_PORT {rfdc_clk_2x} \
   CONFIG.ENABLE_CLOCK_MONITOR {false} \
   CONFIG.FEEDBACK_SOURCE {FDBK_AUTO} \
   CONFIG.MMCM_CLKFBOUT_MULT_F {24.000} \
   CONFIG.MMCM_CLKIN1_PERIOD {16.276} \
   CONFIG.MMCM_CLKIN2_PERIOD {10.0} \
   CONFIG.MMCM_CLKOUT0_DIVIDE_F {24.000} \
   CONFIG.MMCM_CLKOUT1_DIVIDE {12} \
   CONFIG.MMCM_CLKOUT2_DIVIDE {8} \
   CONFIG.MMCM_CLKOUT3_DIVIDE {6} \
   CONFIG.MMCM_CLKOUT4_DIVIDE {4} \
   CONFIG.MMCM_DIVCLK_DIVIDE {1} \
   CONFIG.NUM_OUT_CLKS {5} \
   CONFIG.PHASE_DUTY_CONFIG {false} \
   CONFIG.PRIMITIVE {MMCM} \
   CONFIG.PRIM_IN_FREQ {61.44} \
   CONFIG.PRIM_SOURCE {No_buffer} \
   CONFIG.SECONDARY_SOURCE {Single_ended_clock_capable_pin} \
   CONFIG.USE_CLKFB_STOPPED {false} \
   CONFIG.USE_DYN_RECONFIG {true} \
   CONFIG.USE_INCLK_STOPPED {false} \
   CONFIG.USE_LOCKED {true} \
   CONFIG.USE_PHASE_ALIGNMENT {true} \
   CONFIG.USE_POWER_DOWN {false} \
   CONFIG.USE_RESET {true} \
   CONFIG.USE_SAFE_CLOCK_STARTUP {false} \
 ] $data_clock_mmcm

  # Create instance: reg_clock_gate_control, and set properties
  set reg_clock_gate_control [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 reg_clock_gate_control ]
  set_property -dict [ list \
   CONFIG.C_ALL_INPUTS_2 {1} \
   CONFIG.C_ALL_OUTPUTS {1} \
   CONFIG.C_GPIO_WIDTH {32} \
   CONFIG.C_IS_DUAL {1} \
 ] $reg_clock_gate_control

  # Create instance: reg_invert_iq_radio0, and set properties
  set reg_invert_iq_radio0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 reg_invert_iq_radio0 ]
  set_property -dict [ list \
   CONFIG.C_ALL_OUTPUTS {1} \
 ] $reg_invert_iq_radio0

  # Create instance: reg_invert_iq_radio1, and set properties
  set reg_invert_iq_radio1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 reg_invert_iq_radio1 ]
  set_property -dict [ list \
   CONFIG.C_ALL_OUTPUTS {1} \
 ] $reg_invert_iq_radio1

  # Create instance: reg_reset_mmcm, and set properties
  set reg_reset_mmcm [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 reg_reset_mmcm ]
  set_property -dict [ list \
   CONFIG.C_ALL_OUTPUTS {1} \
   CONFIG.C_GPIO_WIDTH {1} \
 ] $reg_reset_mmcm

  # Create instance: reg_rf_axi_status, and set properties
  set reg_rf_axi_status [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 reg_rf_axi_status ]
  set_property -dict [ list \
   CONFIG.C_ALL_INPUTS {1} \
   CONFIG.C_ALL_INPUTS_2 {1} \
   CONFIG.C_ALL_OUTPUTS {0} \
   CONFIG.C_GPIO_WIDTH {32} \
   CONFIG.C_IS_DUAL {1} \
 ] $reg_rf_axi_status

  # Create instance: reg_rf_reset_control_radio0, and set properties
  set reg_rf_reset_control_radio0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 reg_rf_reset_control_radio0 ]
  set_property -dict [ list \
   CONFIG.C_ALL_INPUTS_2 {1} \
   CONFIG.C_ALL_OUTPUTS {1} \
   CONFIG.C_GPIO_WIDTH {32} \
   CONFIG.C_IS_DUAL {1} \
 ] $reg_rf_reset_control_radio0

  # Create instance: reg_rf_reset_control_radio1, and set properties
  set reg_rf_reset_control_radio1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 reg_rf_reset_control_radio1 ]
  set_property -dict [ list \
   CONFIG.C_ALL_INPUTS_2 {1} \
   CONFIG.C_ALL_OUTPUTS {1} \
   CONFIG.C_GPIO_WIDTH {32} \
   CONFIG.C_IS_DUAL {1} \
 ] $reg_rf_reset_control_radio1

  # Create instance: reg_rfdc_info, and set properties
  set reg_rfdc_info [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 reg_rfdc_info ]
  set_property -dict [ list \
   CONFIG.C_ALL_INPUTS {1} \
 ] $reg_rfdc_info

  # Create instance: reg_rfdc_tile_mapping, and set properties
  set reg_rfdc_tile_mapping [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 reg_rfdc_tile_mapping ]
  set_property -dict [ list \
   CONFIG.C_ALL_INPUTS {1} \
   CONFIG.C_ALL_INPUTS_2 {1} \
   CONFIG.C_IS_DUAL {1} \
 ] $reg_rfdc_tile_mapping

  # Create instance: rf_clock_buffers
  create_hier_cell_rf_clock_buffers $hier_obj rf_clock_buffers

  # Create instance: rf_data_converter, and set properties
  set rf_data_converter [ create_bd_cell -type ip -vlnv xilinx.com:ip:usp_rf_data_converter:2.5 rf_data_converter ]
  set_property -dict [ list \
   CONFIG.ADC0_Enable {1} \
   CONFIG.ADC0_Fabric_Freq {184.320} \
   CONFIG.ADC0_Multi_Tile_Sync {true} \
   CONFIG.ADC0_Outclk_Freq {184.320} \
   CONFIG.ADC0_Refclk_Freq {2949.120} \
   CONFIG.ADC0_Sampling_Rate {2.94912} \
   CONFIG.ADC1_Enable {0} \
   CONFIG.ADC1_Fabric_Freq {0.0} \
   CONFIG.ADC1_Multi_Tile_Sync {false} \
   CONFIG.ADC1_Outclk_Freq {15.625} \
   CONFIG.ADC1_Refclk_Freq {2000.000} \
   CONFIG.ADC1_Sampling_Rate {2.0} \
   CONFIG.ADC224_En {true} \
   CONFIG.ADC225_En {false} \
   CONFIG.ADC2_Enable {1} \
   CONFIG.ADC2_Fabric_Freq {184.320} \
   CONFIG.ADC2_Multi_Tile_Sync {true} \
   CONFIG.ADC2_Outclk_Freq {184.320} \
   CONFIG.ADC2_Refclk_Freq {2949.120} \
   CONFIG.ADC2_Sampling_Rate {2.94912} \
   CONFIG.ADC_Data_Type00 {1} \
   CONFIG.ADC_Data_Type01 {1} \
   CONFIG.ADC_Data_Type02 {1} \
   CONFIG.ADC_Data_Type03 {1} \
   CONFIG.ADC_Data_Type10 {0} \
   CONFIG.ADC_Data_Type11 {0} \
   CONFIG.ADC_Data_Type12 {0} \
   CONFIG.ADC_Data_Type13 {0} \
   CONFIG.ADC_Data_Type20 {1} \
   CONFIG.ADC_Data_Type21 {1} \
   CONFIG.ADC_Data_Type22 {1} \
   CONFIG.ADC_Data_Type23 {1} \
   CONFIG.ADC_Data_Width00 {8} \
   CONFIG.ADC_Data_Width01 {8} \
   CONFIG.ADC_Data_Width02 {8} \
   CONFIG.ADC_Data_Width03 {8} \
   CONFIG.ADC_Data_Width10 {8} \
   CONFIG.ADC_Data_Width11 {8} \
   CONFIG.ADC_Data_Width12 {8} \
   CONFIG.ADC_Data_Width13 {8} \
   CONFIG.ADC_Data_Width20 {8} \
   CONFIG.ADC_Data_Width21 {8} \
   CONFIG.ADC_Data_Width22 {8} \
   CONFIG.ADC_Data_Width23 {8} \
   CONFIG.ADC_Debug {false} \
   CONFIG.ADC_Decimation_Mode00 {2} \
   CONFIG.ADC_Decimation_Mode01 {2} \
   CONFIG.ADC_Decimation_Mode02 {2} \
   CONFIG.ADC_Decimation_Mode03 {2} \
   CONFIG.ADC_Decimation_Mode10 {0} \
   CONFIG.ADC_Decimation_Mode11 {0} \
   CONFIG.ADC_Decimation_Mode12 {0} \
   CONFIG.ADC_Decimation_Mode13 {0} \
   CONFIG.ADC_Decimation_Mode20 {2} \
   CONFIG.ADC_Decimation_Mode21 {2} \
   CONFIG.ADC_Decimation_Mode22 {2} \
   CONFIG.ADC_Decimation_Mode23 {2} \
   CONFIG.ADC_Dither00 {false} \
   CONFIG.ADC_Dither01 {false} \
   CONFIG.ADC_Dither02 {false} \
   CONFIG.ADC_Dither03 {false} \
   CONFIG.ADC_Dither10 {true} \
   CONFIG.ADC_Dither11 {true} \
   CONFIG.ADC_Dither12 {true} \
   CONFIG.ADC_Dither13 {true} \
   CONFIG.ADC_Dither20 {false} \
   CONFIG.ADC_Dither21 {false} \
   CONFIG.ADC_Dither22 {false} \
   CONFIG.ADC_Dither23 {false} \
   CONFIG.ADC_Mixer_Mode00 {0} \
   CONFIG.ADC_Mixer_Mode01 {0} \
   CONFIG.ADC_Mixer_Mode02 {0} \
   CONFIG.ADC_Mixer_Mode03 {0} \
   CONFIG.ADC_Mixer_Mode10 {2} \
   CONFIG.ADC_Mixer_Mode11 {2} \
   CONFIG.ADC_Mixer_Mode12 {2} \
   CONFIG.ADC_Mixer_Mode13 {2} \
   CONFIG.ADC_Mixer_Mode20 {0} \
   CONFIG.ADC_Mixer_Mode21 {0} \
   CONFIG.ADC_Mixer_Mode22 {0} \
   CONFIG.ADC_Mixer_Mode23 {0} \
   CONFIG.ADC_Mixer_Type00 {2} \
   CONFIG.ADC_Mixer_Type01 {2} \
   CONFIG.ADC_Mixer_Type02 {2} \
   CONFIG.ADC_Mixer_Type03 {2} \
   CONFIG.ADC_Mixer_Type10 {3} \
   CONFIG.ADC_Mixer_Type11 {3} \
   CONFIG.ADC_Mixer_Type12 {3} \
   CONFIG.ADC_Mixer_Type13 {3} \
   CONFIG.ADC_Mixer_Type20 {2} \
   CONFIG.ADC_Mixer_Type21 {2} \
   CONFIG.ADC_Mixer_Type22 {2} \
   CONFIG.ADC_Mixer_Type23 {2} \
   CONFIG.ADC_NCO_Freq00 {0.200} \
   CONFIG.ADC_NCO_Freq01 {0.200} \
   CONFIG.ADC_NCO_Freq02 {0.200} \
   CONFIG.ADC_NCO_Freq03 {0.200} \
   CONFIG.ADC_NCO_Freq10 {0.0} \
   CONFIG.ADC_NCO_Freq11 {0.0} \
   CONFIG.ADC_NCO_Freq12 {0.0} \
   CONFIG.ADC_NCO_Freq13 {0.0} \
   CONFIG.ADC_NCO_Freq20 {0.200} \
   CONFIG.ADC_NCO_Freq21 {0.200} \
   CONFIG.ADC_NCO_Freq22 {0.200} \
   CONFIG.ADC_NCO_Freq23 {0.200} \
   CONFIG.ADC_NCO_Freq30 {0.0} \
   CONFIG.ADC_NCO_Freq31 {0.0} \
   CONFIG.ADC_NCO_RTS {true} \
   CONFIG.ADC_RTS {true} \
   CONFIG.ADC_Slice00_Enable {true} \
   CONFIG.ADC_Slice01_Enable {true} \
   CONFIG.ADC_Slice02_Enable {true} \
   CONFIG.ADC_Slice03_Enable {true} \
   CONFIG.ADC_Slice10_Enable {false} \
   CONFIG.ADC_Slice11_Enable {false} \
   CONFIG.ADC_Slice12_Enable {false} \
   CONFIG.ADC_Slice13_Enable {false} \
   CONFIG.ADC_Slice20_Enable {true} \
   CONFIG.ADC_Slice21_Enable {true} \
   CONFIG.ADC_Slice22_Enable {true} \
   CONFIG.ADC_Slice23_Enable {true} \
   CONFIG.Axiclk_Freq {40} \
   CONFIG.Calibration_Freeze {true} \
   CONFIG.Converter_Setup {1} \
   CONFIG.DAC0_Band {0} \
   CONFIG.DAC0_Enable {1} \
   CONFIG.DAC0_Fabric_Freq {184.320} \
   CONFIG.DAC0_Multi_Tile_Sync {true} \
   CONFIG.DAC0_Outclk_Freq {184.320} \
   CONFIG.DAC0_Refclk_Freq {2949.120} \
   CONFIG.DAC0_Sampling_Rate {2.94912} \
   CONFIG.DAC1_Enable {1} \
   CONFIG.DAC1_Fabric_Freq {184.320} \
   CONFIG.DAC1_Multi_Tile_Sync {true} \
   CONFIG.DAC1_Outclk_Freq {184.320} \
   CONFIG.DAC1_Refclk_Freq {2949.120} \
   CONFIG.DAC1_Sampling_Rate {2.94912} \
   CONFIG.DAC228_En {true} \
   CONFIG.DAC_Data_Type00 {0} \
   CONFIG.DAC_Data_Type01 {0} \
   CONFIG.DAC_Data_Width00 {16} \
   CONFIG.DAC_Data_Width01 {16} \
   CONFIG.DAC_Data_Width02 {16} \
   CONFIG.DAC_Data_Width03 {16} \
   CONFIG.DAC_Data_Width10 {16} \
   CONFIG.DAC_Data_Width11 {16} \
   CONFIG.DAC_Debug {false} \
   CONFIG.DAC_Interpolation_Mode00 {2} \
   CONFIG.DAC_Interpolation_Mode01 {2} \
   CONFIG.DAC_Interpolation_Mode02 {0} \
   CONFIG.DAC_Interpolation_Mode03 {0} \
   CONFIG.DAC_Interpolation_Mode10 {2} \
   CONFIG.DAC_Interpolation_Mode11 {2} \
   CONFIG.DAC_Invsinc_Ctrl00 {false} \
   CONFIG.DAC_Mixer_Mode00 {0} \
   CONFIG.DAC_Mixer_Mode01 {0} \
   CONFIG.DAC_Mixer_Mode02 {2} \
   CONFIG.DAC_Mixer_Mode03 {2} \
   CONFIG.DAC_Mixer_Mode10 {0} \
   CONFIG.DAC_Mixer_Mode11 {0} \
   CONFIG.DAC_Mixer_Mode13 {2} \
   CONFIG.DAC_Mixer_Mode20 {2} \
   CONFIG.DAC_Mixer_Mode21 {2} \
   CONFIG.DAC_Mixer_Mode30 {2} \
   CONFIG.DAC_Mixer_Mode31 {2} \
   CONFIG.DAC_Mixer_Type00 {2} \
   CONFIG.DAC_Mixer_Type01 {2} \
   CONFIG.DAC_Mixer_Type02 {3} \
   CONFIG.DAC_Mixer_Type03 {3} \
   CONFIG.DAC_Mixer_Type10 {2} \
   CONFIG.DAC_Mixer_Type11 {2} \
   CONFIG.DAC_NCO_Freq00 {0.200} \
   CONFIG.DAC_NCO_Freq01 {0.200} \
   CONFIG.DAC_NCO_Freq10 {0.200} \
   CONFIG.DAC_NCO_Freq11 {0.200} \
   CONFIG.DAC_NCO_RTS {true} \
   CONFIG.DAC_Output_Current {1} \
   CONFIG.DAC_RTS {false} \
   CONFIG.DAC_Slice00_Enable {true} \
   CONFIG.DAC_Slice01_Enable {true} \
   CONFIG.DAC_Slice02_Enable {false} \
   CONFIG.DAC_Slice03_Enable {false} \
   CONFIG.DAC_Slice10_Enable {true} \
   CONFIG.DAC_Slice11_Enable {true} \
   CONFIG.mADC_Data_Type00 {0} \
   CONFIG.mADC_Data_Type01 {0} \
   CONFIG.mADC_Data_Type02 {0} \
   CONFIG.mADC_Data_Type03 {0} \
   CONFIG.mADC_Data_Width00 {8} \
   CONFIG.mADC_Data_Width01 {8} \
   CONFIG.mADC_Data_Width02 {8} \
   CONFIG.mADC_Data_Width03 {8} \
   CONFIG.mADC_Decimation_Mode00 {0} \
   CONFIG.mADC_Decimation_Mode01 {0} \
   CONFIG.mADC_Decimation_Mode02 {0} \
   CONFIG.mADC_Decimation_Mode03 {0} \
   CONFIG.mADC_Dither00 {true} \
   CONFIG.mADC_Dither01 {true} \
   CONFIG.mADC_Dither02 {true} \
   CONFIG.mADC_Dither03 {true} \
   CONFIG.mADC_Enable {0} \
   CONFIG.mADC_Fabric_Freq {0.0} \
   CONFIG.mADC_Mixer_Mode00 {2} \
   CONFIG.mADC_Mixer_Mode01 {2} \
   CONFIG.mADC_Mixer_Mode02 {2} \
   CONFIG.mADC_Mixer_Mode03 {2} \
   CONFIG.mADC_Mixer_Type00 {3} \
   CONFIG.mADC_Mixer_Type01 {3} \
   CONFIG.mADC_Mixer_Type02 {3} \
   CONFIG.mADC_Mixer_Type03 {3} \
   CONFIG.mADC_Multi_Tile_Sync {false} \
   CONFIG.mADC_NCO_Freq02 {0.0} \
   CONFIG.mADC_NCO_Freq03 {0.0} \
   CONFIG.mADC_Outclk_Freq {15.625} \
   CONFIG.mADC_Refclk_Freq {2000.000} \
   CONFIG.mADC_Sampling_Rate {2.0} \
   CONFIG.mADC_Slice00_Enable {false} \
   CONFIG.mADC_Slice01_Enable {false} \
   CONFIG.mADC_Slice02_Enable {false} \
   CONFIG.mADC_Slice03_Enable {false} \
   CONFIG.mDAC_Band {0} \
   CONFIG.mDAC_Data_Type00 {0} \
   CONFIG.mDAC_Data_Type01 {0} \
   CONFIG.mDAC_Data_Width00 {2} \
   CONFIG.mDAC_Data_Width01 {2} \
   CONFIG.mDAC_Data_Width02 {16} \
   CONFIG.mDAC_Data_Width03 {16} \
   CONFIG.mDAC_Enable {1} \
   CONFIG.mDAC_Fabric_Freq {368.640} \
   CONFIG.mDAC_Interpolation_Mode00 {8} \
   CONFIG.mDAC_Interpolation_Mode01 {8} \
   CONFIG.mDAC_Interpolation_Mode02 {0} \
   CONFIG.mDAC_Interpolation_Mode03 {0} \
   CONFIG.mDAC_Invsinc_Ctrl00 {false} \
   CONFIG.mDAC_Mixer_Mode00 {0} \
   CONFIG.mDAC_Mixer_Mode01 {0} \
   CONFIG.mDAC_Mixer_Mode02 {2} \
   CONFIG.mDAC_Mixer_Mode03 {2} \
   CONFIG.mDAC_Mixer_Type00 {2} \
   CONFIG.mDAC_Mixer_Type01 {2} \
   CONFIG.mDAC_Mixer_Type02 {3} \
   CONFIG.mDAC_Mixer_Type03 {3} \
   CONFIG.mDAC_Multi_Tile_Sync {false} \
   CONFIG.mDAC_NCO_Freq01 {0} \
   CONFIG.mDAC_Outclk_Freq {184.320} \
   CONFIG.mDAC_Refclk_Freq {2949.120} \
   CONFIG.mDAC_Sampling_Rate {2.94912} \
   CONFIG.mDAC_Slice00_Enable {true} \
   CONFIG.mDAC_Slice01_Enable {true} \
   CONFIG.mDAC_Slice02_Enable {false} \
   CONFIG.mDAC_Slice03_Enable {false} \
 ] $rf_data_converter

  # Create instance: rf_nco_reset_0, and set properties
  set block_name rf_nco_reset
  set block_cell_name rf_nco_reset_0
  if { [catch {set rf_nco_reset_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $rf_nco_reset_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: rf_reset_controller_0, and set properties
  set block_name x410_rf_reset_controller
  set block_cell_name rf_reset_controller_0
  if { [catch {set rf_reset_controller_0 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $rf_reset_controller_0 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: rf_reset_controller_1, and set properties
  set block_name x410_rf_reset_controller
  set block_cell_name rf_reset_controller_1
  if { [catch {set rf_reset_controller_1 [create_bd_cell -type module -reference $block_name $block_cell_name] } errmsg] } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2095 -severity "ERROR" "Unable to add referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   } elseif { $rf_reset_controller_1 eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2096 -severity "ERROR" "Unable to referenced block <$block_name>. Please add the files for ${block_name}'s definition into the project."}
     return 1
   }
  
  # Create instance: rfdc_adc_map, and set properties
  set rfdc_adc_map [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 rfdc_adc_map ]
  set_property -dict [ list \
   CONFIG.CONST_VAL {0b0000001001100000000000000100} \
   CONFIG.CONST_WIDTH {32} \
 ] $rfdc_adc_map

  # Create instance: rfdc_dac_map, and set properties
  set rfdc_dac_map [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 rfdc_dac_map ]
  set_property -dict [ list \
   CONFIG.CONST_VAL {0b0000010100010000000001000000} \
   CONFIG.CONST_WIDTH {32} \
 ] $rfdc_dac_map

  # Create instance: slice_iqswap_11_8_radio0, and set properties
  set slice_iqswap_11_8_radio0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlslice:1.0 slice_iqswap_11_8_radio0 ]
  set_property -dict [ list \
   CONFIG.DIN_FROM {11} \
   CONFIG.DIN_TO {8} \
   CONFIG.DOUT_WIDTH {4} \
 ] $slice_iqswap_11_8_radio0

  # Create instance: slice_iqswap_11_8_radio1, and set properties
  set slice_iqswap_11_8_radio1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlslice:1.0 slice_iqswap_11_8_radio1 ]
  set_property -dict [ list \
   CONFIG.DIN_FROM {11} \
   CONFIG.DIN_TO {8} \
   CONFIG.DOUT_WIDTH {4} \
 ] $slice_iqswap_11_8_radio1

  # Create instance: slice_iqswap_3_0_radio0, and set properties
  set slice_iqswap_3_0_radio0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlslice:1.0 slice_iqswap_3_0_radio0 ]
  set_property -dict [ list \
   CONFIG.DIN_FROM {3} \
   CONFIG.DOUT_WIDTH {4} \
 ] $slice_iqswap_3_0_radio0

  # Create instance: slice_iqswap_3_0_radio1, and set properties
  set slice_iqswap_3_0_radio1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlslice:1.0 slice_iqswap_3_0_radio1 ]
  set_property -dict [ list \
   CONFIG.DIN_FROM {3} \
   CONFIG.DOUT_WIDTH {4} \
 ] $slice_iqswap_3_0_radio1

  # Create instance: xlconstant_0, and set properties
  set xlconstant_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_0 ]

  # Create interface connections
  connect_bd_intf_net -intf_net S_AXI_1_1 [get_bd_intf_pins axi_interconnect_rf/M06_AXI] [get_bd_intf_pins calibration_muxes/S_AXI_1]
  connect_bd_intf_net -intf_net adc0_clk_0_1 [get_bd_intf_pins adc0_clk] [get_bd_intf_pins rf_data_converter/adc0_clk]
  connect_bd_intf_net -intf_net adc2_clk_0_1 [get_bd_intf_pins adc2_clk] [get_bd_intf_pins rf_data_converter/adc2_clk]
  connect_bd_intf_net -intf_net axi_interconnect_0_M00_AXI [get_bd_intf_pins axi_interconnect_rf/M00_AXI] [get_bd_intf_pins rf_data_converter/s_axi]
  connect_bd_intf_net -intf_net axi_interconnect_rf_M01_AXI [get_bd_intf_pins axi_interconnect_rf/M01_AXI] [get_bd_intf_pins data_clock_mmcm/s_axi_lite]
  connect_bd_intf_net -intf_net axi_interconnect_rf_M02_AXI [get_bd_intf_pins axi_interconnect_rf/M02_AXI] [get_bd_intf_pins reg_invert_iq_radio0/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_rf_M03_AXI [get_bd_intf_pins axi_interconnect_rf/M03_AXI] [get_bd_intf_pins reg_reset_mmcm/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_rf_M04_AXI [get_bd_intf_pins axi_interconnect_rf/M04_AXI] [get_bd_intf_pins reg_rf_reset_control_radio0/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_rf_M05_AXI [get_bd_intf_pins axi_interconnect_rf/M05_AXI] [get_bd_intf_pins reg_rf_axi_status/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_rf_M07_AXI [get_bd_intf_pins axi_interconnect_rf/M07_AXI] [get_bd_intf_pins reg_clock_gate_control/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_rf_M08_AXI [get_bd_intf_pins ThresholdRegister/S_AXI] [get_bd_intf_pins axi_interconnect_rf/M08_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_rf_M09_AXI [get_bd_intf_pins axi_interconnect_rf/M09_AXI] [get_bd_intf_pins reg_rfdc_tile_mapping/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_rf_M10_AXI [get_bd_intf_pins axi_interconnect_rf/M10_AXI] [get_bd_intf_pins reg_rfdc_info/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_rf_M11_AXI [get_bd_intf_pins axi_interconnect_rf/M11_AXI] [get_bd_intf_pins reg_invert_iq_radio1/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_rf_M12_AXI [get_bd_intf_pins axi_interconnect_rf/M12_AXI] [get_bd_intf_pins reg_rf_reset_control_radio1/S_AXI]
  connect_bd_intf_net -intf_net calibration_muxes_m_axis_0_0 [get_bd_intf_pins calibration_muxes/m_axis_0_0] [get_bd_intf_pins rf_data_converter/s00_axis]
  connect_bd_intf_net -intf_net calibration_muxes_m_axis_1_0 [get_bd_intf_pins calibration_muxes/m_axis_1_0] [get_bd_intf_pins rf_data_converter/s01_axis]
  connect_bd_intf_net -intf_net calibration_muxes_m_axis_2_0 [get_bd_intf_pins calibration_muxes/m_axis_2_0] [get_bd_intf_pins rf_data_converter/s10_axis]
  connect_bd_intf_net -intf_net calibration_muxes_m_axis_3_0 [get_bd_intf_pins calibration_muxes/m_axis_3_0] [get_bd_intf_pins rf_data_converter/s11_axis]
  connect_bd_intf_net -intf_net dac0_clk_0_1 [get_bd_intf_pins dac0_clk] [get_bd_intf_pins rf_data_converter/dac0_clk]
  connect_bd_intf_net -intf_net dac1_clk_0_1 [get_bd_intf_pins dac1_clk] [get_bd_intf_pins rf_data_converter/dac1_clk]
  connect_bd_intf_net -intf_net dac_tile228_ch0_din_1 [get_bd_intf_pins dac_tile228_ch0_din] [get_bd_intf_pins calibration_muxes/dac_tile228_ch0_din]
  connect_bd_intf_net -intf_net dac_tile228_ch1_din_1 [get_bd_intf_pins dac_tile228_ch1_din] [get_bd_intf_pins calibration_muxes/dac_tile228_ch1_din]
  connect_bd_intf_net -intf_net dac_tile229_ch0_din_1 [get_bd_intf_pins dac_tile229_ch0_din] [get_bd_intf_pins calibration_muxes/dac_tile229_ch0_din]
  connect_bd_intf_net -intf_net dac_tile229_ch1_din_1 [get_bd_intf_pins dac_tile229_ch1_din] [get_bd_intf_pins calibration_muxes/dac_tile229_ch1_din]
  connect_bd_intf_net -intf_net rf_data_converter_m00_axis [get_bd_intf_pins adc_tile224_ch0_dout_i] [get_bd_intf_pins rf_data_converter/m00_axis]
  connect_bd_intf_net -intf_net rf_data_converter_m01_axis [get_bd_intf_pins adc_tile224_ch0_dout_q] [get_bd_intf_pins rf_data_converter/m01_axis]
  connect_bd_intf_net -intf_net rf_data_converter_m02_axis [get_bd_intf_pins adc_tile224_ch1_dout_i] [get_bd_intf_pins rf_data_converter/m02_axis]
  connect_bd_intf_net -intf_net rf_data_converter_m03_axis [get_bd_intf_pins adc_tile224_ch1_dout_q] [get_bd_intf_pins rf_data_converter/m03_axis]
  connect_bd_intf_net -intf_net rf_data_converter_m20_axis [get_bd_intf_pins adc_tile226_ch0_dout_i] [get_bd_intf_pins rf_data_converter/m20_axis]
  connect_bd_intf_net -intf_net rf_data_converter_m21_axis [get_bd_intf_pins adc_tile226_ch0_dout_q] [get_bd_intf_pins rf_data_converter/m21_axis]
  connect_bd_intf_net -intf_net rf_data_converter_m22_axis [get_bd_intf_pins adc_tile226_ch1_dout_i] [get_bd_intf_pins rf_data_converter/m22_axis]
  connect_bd_intf_net -intf_net rf_data_converter_m23_axis [get_bd_intf_pins adc_tile226_ch1_dout_q] [get_bd_intf_pins rf_data_converter/m23_axis]
  connect_bd_intf_net -intf_net rf_data_converter_vout00 [get_bd_intf_pins dac_tile228_ch0_vout] [get_bd_intf_pins rf_data_converter/vout00]
  connect_bd_intf_net -intf_net rf_data_converter_vout01 [get_bd_intf_pins dac_tile228_ch1_vout] [get_bd_intf_pins rf_data_converter/vout01]
  connect_bd_intf_net -intf_net rf_data_converter_vout10 [get_bd_intf_pins dac_tile229_ch0_vout] [get_bd_intf_pins rf_data_converter/vout10]
  connect_bd_intf_net -intf_net rf_data_converter_vout11 [get_bd_intf_pins dac_tile229_ch1_vout] [get_bd_intf_pins rf_data_converter/vout11]
  connect_bd_intf_net -intf_net s_axi_config_1 [get_bd_intf_pins s_axi_config] [get_bd_intf_pins axi_interconnect_rf/S00_AXI]
  connect_bd_intf_net -intf_net sysref_in_0_1 [get_bd_intf_pins sysref_rf_in] [get_bd_intf_pins rf_data_converter/sysref_in]
  connect_bd_intf_net -intf_net vin0_01_0_1 [get_bd_intf_pins adc_tile224_ch0_vin] [get_bd_intf_pins rf_data_converter/vin0_01]
  connect_bd_intf_net -intf_net vin0_23_0_1 [get_bd_intf_pins adc_tile224_ch1_vin] [get_bd_intf_pins rf_data_converter/vin0_23]
  connect_bd_intf_net -intf_net vin2_01_0_1 [get_bd_intf_pins adc_tile226_ch0_vin] [get_bd_intf_pins rf_data_converter/vin2_01]
  connect_bd_intf_net -intf_net vin2_23_0_1 [get_bd_intf_pins adc_tile226_ch1_vin] [get_bd_intf_pins rf_data_converter/vin2_23]

  # Create port connections
  connect_bd_net -net M02_ARESETN_1 [get_bd_pins axi_interconnect_rf/M02_ARESETN] [get_bd_pins axi_interconnect_rf/M11_ARESETN] [get_bd_pins const_1/dout] [get_bd_pins reg_invert_iq_radio0/s_axi_aresetn] [get_bd_pins reg_invert_iq_radio1/s_axi_aresetn]
  connect_bd_net -net adc_reset_pulse_dclk_1 [get_bd_pins adc_reset_pulse_dclk] [get_bd_pins rf_reset_controller_0/dAdcResetPulse] [get_bd_pins rf_reset_controller_1/dAdcResetPulse]
  connect_bd_net -net capture_sysref_0_sysref_out_rclk [get_bd_pins sysref_out_rclk] [get_bd_pins capture_sysref/sysref_out_rclk] [get_bd_pins rf_data_converter/user_sysref_adc] [get_bd_pins rf_data_converter/user_sysref_dac]
  connect_bd_net -net capture_sysref_sysref_out_pclk [get_bd_pins sysref_out_pclk] [get_bd_pins capture_sysref/sysref_out_pclk] [get_bd_pins rf_nco_reset_0/dSysref]
  connect_bd_net -net clk_in1_0_1 [get_bd_pins pll_ref_clk_in] [get_bd_pins data_clock_mmcm/clk_in1]
  connect_bd_net -net clock_gates_0_cSoftwareStatus [get_bd_pins clock_gates_0/rSoftwareStatus] [get_bd_pins reg_clock_gate_control/gpio2_io_i]
  connect_bd_net -net clock_gates_0_rGatedBaseClksValid [get_bd_pins gated_base_clks_valid_clk40] [get_bd_pins clock_gates_0/rGatedBaseClksValid]
  connect_bd_net -net clock_gates_0_rPllLocked [get_bd_pins data_clock_locked] [get_bd_pins clock_gates_0/rPllLocked]
  connect_bd_net -net clock_gates_0_rf2EnableBufg [get_bd_pins clock_gates_0/aEnableRfBufg2x] [get_bd_pins rf_clock_buffers/rfdc_clk_2x_ce]
  connect_bd_net -net clock_gates_0_rfEnableBufg [get_bd_pins clock_gates_0/aEnableRfBufg1x] [get_bd_pins rf_clock_buffers/rfdc_clk_ce]
  connect_bd_net -net dac_reset_pulse_dclk_1 [get_bd_pins dac_reset_pulse_dclk] [get_bd_pins rf_reset_controller_0/dDacResetPulse] [get_bd_pins rf_reset_controller_1/dDacResetPulse]
  connect_bd_net -net data_clk_2x_pll [get_bd_pins clock_gates_0/DataClk2xPll] [get_bd_pins data_clock_mmcm/data_clk_2x]
  connect_bd_net -net data_clock_mmcm_data_clk [get_bd_pins data_clk] [get_bd_pins clock_gates_0/DataClk1x] [get_bd_pins rf_nco_reset_0/DataClk] [get_bd_pins rf_reset_controller_0/DataClk] [get_bd_pins rf_reset_controller_1/DataClk]
  connect_bd_net -net data_clock_mmcm_data_clk1 [get_bd_pins clock_gates_0/DataClk1xPll] [get_bd_pins data_clock_mmcm/data_clk]
  connect_bd_net -net data_clock_mmcm_data_clk_2x [get_bd_pins data_clk_2x] [get_bd_pins clock_gates_0/DataClk2x] [get_bd_pins rf_reset_controller_0/DataClk2x] [get_bd_pins rf_reset_controller_1/DataClk2x]
  connect_bd_net -net data_clock_mmcm_locked [get_bd_pins clock_gates_0/aPllLocked] [get_bd_pins data_clock_mmcm/locked]
  connect_bd_net -net data_clock_mmcm_rfdc_clk [get_bd_pins rfdc_clk] [get_bd_pins calibration_muxes/s_axi_aclk_0] [get_bd_pins capture_sysref/rfdc_clk] [get_bd_pins rf_clock_buffers/rfdc_clk] [get_bd_pins rf_data_converter/m0_axis_aclk] [get_bd_pins rf_data_converter/m2_axis_aclk] [get_bd_pins rf_data_converter/s0_axis_aclk] [get_bd_pins rf_data_converter/s1_axis_aclk] [get_bd_pins rf_reset_controller_0/RfClk] [get_bd_pins rf_reset_controller_1/RfClk]
  connect_bd_net -net data_clock_mmcm_rfdc_clk1 [get_bd_pins data_clock_mmcm/rfdc_clk] [get_bd_pins rf_clock_buffers/rfdc_clk_pll]
  connect_bd_net -net data_clock_mmcm_rfdc_clk_2x [get_bd_pins rfdc_clk_2x] [get_bd_pins axi_interconnect_rf/M02_ACLK] [get_bd_pins axi_interconnect_rf/M11_ACLK] [get_bd_pins reg_invert_iq_radio0/s_axi_aclk] [get_bd_pins reg_invert_iq_radio1/s_axi_aclk] [get_bd_pins rf_clock_buffers/rfdc_clk_2x] [get_bd_pins rf_reset_controller_0/RfClk2x] [get_bd_pins rf_reset_controller_1/RfClk2x]
  connect_bd_net -net data_clock_mmcm_rfdc_clk_2x1 [get_bd_pins data_clock_mmcm/rfdc_clk_2x] [get_bd_pins rf_clock_buffers/rfdc_clk_2x_pll]
  connect_bd_net -net data_clock_mmcm_tdc_ref_clk [get_bd_pins pll_ref_clk_out] [get_bd_pins capture_sysref/pll_ref_clk] [get_bd_pins data_clock_mmcm/pll_ref_clk_out] [get_bd_pins rf_reset_controller_0/PllRefClk] [get_bd_pins rf_reset_controller_1/PllRefClk]
  connect_bd_net -net enable_gated_clocks_clk40_1 [get_bd_pins enable_gated_clocks_clk40] [get_bd_pins clock_gates_0/rSafeToEnableGatedClks]
  connect_bd_net -net enable_rclk_0_1 [get_bd_pins enable_sysref_rclk] [get_bd_pins capture_sysref/enable_rclk]
  connect_bd_net -net gpio2_io_i_0_2 [get_bd_pins rf_dsp_info_sclk] [get_bd_pins reg_rf_axi_status/gpio2_io_i]
  connect_bd_net -net gpio_io_i_0_1 [get_bd_pins rf_axi_status_sclk] [get_bd_pins reg_rf_axi_status/gpio_io_i]
  connect_bd_net -net reg_invert_iq_radio1_gpio_io_o [get_bd_pins reg_invert_iq_radio1/gpio_io_o] [get_bd_pins slice_iqswap_11_8_radio1/Din] [get_bd_pins slice_iqswap_3_0_radio1/Din]
  connect_bd_net -net reg_reset_mmcm_gpio_io_o [get_bd_pins axi_interconnect_rf/M01_ARESETN] [get_bd_pins clock_gates_0/rPllReset_n] [get_bd_pins data_clock_mmcm/s_axi_aresetn] [get_bd_pins reg_reset_mmcm/gpio_io_o]
  connect_bd_net -net reg_rf_reset_control1_gpio_io_o [get_bd_pins clock_gates_0/rSoftwareControl] [get_bd_pins reg_clock_gate_control/gpio_io_o]
  connect_bd_net -net reg_rf_reset_control_radio1_gpio_io_o [get_bd_pins reg_rf_reset_control_radio1/gpio_io_o] [get_bd_pins rf_reset_controller_1/cSoftwareControl]
  connect_bd_net -net reg_rf_resets_gpio_io_o [get_bd_pins reg_rf_reset_control_radio0/gpio_io_o] [get_bd_pins rf_reset_controller_0/cSoftwareControl]
  connect_bd_net -net rf_data_converter_adc0_01_over_threshold1 [get_bd_pins ThresholdRegister/In0] [get_bd_pins rf_data_converter/adc0_01_over_threshold1]
  connect_bd_net -net rf_data_converter_adc0_01_over_threshold2 [get_bd_pins ThresholdRegister/In1] [get_bd_pins rf_data_converter/adc0_01_over_threshold2]
  connect_bd_net -net rf_data_converter_adc0_23_over_threshold1 [get_bd_pins ThresholdRegister/In2] [get_bd_pins rf_data_converter/adc0_23_over_threshold1]
  connect_bd_net -net rf_data_converter_adc0_23_over_threshold2 [get_bd_pins ThresholdRegister/In3] [get_bd_pins rf_data_converter/adc0_23_over_threshold2]
  connect_bd_net -net rf_data_converter_adc0_nco_update_busy [get_bd_pins rf_data_converter/adc0_nco_update_busy] [get_bd_pins rf_nco_reset_0/cAdc0xNcoUpdateBusy]
  connect_bd_net -net rf_data_converter_adc2_01_over_threshold1 [get_bd_pins ThresholdRegister/In5] [get_bd_pins rf_data_converter/adc2_01_over_threshold1]
  connect_bd_net -net rf_data_converter_adc2_01_over_threshold2 [get_bd_pins ThresholdRegister/In6] [get_bd_pins rf_data_converter/adc2_01_over_threshold2]
  connect_bd_net -net rf_data_converter_adc2_23_over_threshold1 [get_bd_pins ThresholdRegister/In7] [get_bd_pins rf_data_converter/adc2_23_over_threshold1]
  connect_bd_net -net rf_data_converter_adc2_23_over_threshold2 [get_bd_pins ThresholdRegister/In8] [get_bd_pins rf_data_converter/adc2_23_over_threshold2]
  connect_bd_net -net rf_data_converter_adc2_nco_update_busy [get_bd_pins rf_data_converter/adc2_nco_update_busy] [get_bd_pins rf_nco_reset_0/cAdc2xNcoUpdateBusy]
  connect_bd_net -net rf_data_converter_dac0_nco_update_busy [get_bd_pins rf_data_converter/dac0_nco_update_busy] [get_bd_pins rf_nco_reset_0/cDac0xNcoUpdateBusy]
  connect_bd_net -net rf_data_converter_dac1_nco_update_busy [get_bd_pins rf_data_converter/dac1_nco_update_busy] [get_bd_pins rf_nco_reset_0/cDac1xNcoUpdateBusy]
  connect_bd_net -net rf_data_converter_irq [get_bd_pins rfdc_irq] [get_bd_pins rf_data_converter/irq]
  connect_bd_net -net rf_nco_reset_0_cAdc0xNcoUpdateReq [get_bd_pins rf_data_converter/adc0_nco_update_req] [get_bd_pins rf_nco_reset_0/cAdc0xNcoUpdateReq]
  connect_bd_net -net rf_nco_reset_0_cAdc2xNcoUpdateReq [get_bd_pins rf_data_converter/adc2_nco_update_req] [get_bd_pins rf_nco_reset_0/cAdc2xNcoUpdateReq]
  connect_bd_net -net rf_nco_reset_0_cDac0xNcoUpdateReq [get_bd_pins rf_data_converter/dac0_nco_update_req] [get_bd_pins rf_nco_reset_0/cDac0xNcoUpdateReq]
  connect_bd_net -net rf_nco_reset_0_cDac0xSysrefIntGating [get_bd_pins rf_data_converter/dac0_sysref_int_gating] [get_bd_pins rf_nco_reset_0/cDac0xSysrefIntGating]
  connect_bd_net -net rf_nco_reset_0_cDac0xSysrefIntReenable [get_bd_pins rf_data_converter/dac0_sysref_int_reenable] [get_bd_pins rf_nco_reset_0/cDac0xSysrefIntReenable]
  connect_bd_net -net rf_nco_reset_0_cDac1xNcoUpdateReq [get_bd_pins rf_data_converter/dac1_nco_update_req] [get_bd_pins rf_nco_reset_0/cDac1xNcoUpdateReq]
  connect_bd_net -net rf_nco_reset_0_cNcoPhaseRst [get_bd_pins rf_data_converter/adc0_01_nco_phase_rst] [get_bd_pins rf_data_converter/adc0_23_nco_phase_rst] [get_bd_pins rf_data_converter/adc2_01_nco_phase_rst] [get_bd_pins rf_data_converter/adc2_23_nco_phase_rst] [get_bd_pins rf_data_converter/dac00_nco_phase_rst] [get_bd_pins rf_data_converter/dac01_nco_phase_rst] [get_bd_pins rf_data_converter/dac10_nco_phase_rst] [get_bd_pins rf_data_converter/dac11_nco_phase_rst] [get_bd_pins rf_nco_reset_0/cNcoPhaseRst]
  connect_bd_net -net rf_nco_reset_0_cNcoUpdateEn [get_bd_pins rf_data_converter/adc0_01_nco_update_en] [get_bd_pins rf_data_converter/adc0_23_nco_update_en] [get_bd_pins rf_data_converter/adc2_01_nco_update_en] [get_bd_pins rf_data_converter/adc2_23_nco_update_en] [get_bd_pins rf_data_converter/dac00_nco_update_en] [get_bd_pins rf_data_converter/dac01_nco_update_en] [get_bd_pins rf_data_converter/dac10_nco_update_en] [get_bd_pins rf_data_converter/dac11_nco_update_en] [get_bd_pins rf_nco_reset_0/cNcoUpdateEn]
  connect_bd_net -net rf_nco_reset_0_dNcoResetDone [get_bd_pins nco_reset_done_dclk] [get_bd_pins rf_nco_reset_0/dNcoResetDone]
  connect_bd_net -net rf_reset_controller_0_cSoftwareStatus [get_bd_pins reg_rf_reset_control_radio0/gpio2_io_i] [get_bd_pins rf_reset_controller_0/cSoftwareStatus]
  connect_bd_net -net rf_reset_controller_0_d2DacFirReset_n [get_bd_pins dac_data_in_resetn_dclk2x] [get_bd_pins rf_reset_controller_0/d2DacFirReset_n]
  connect_bd_net -net rf_reset_controller_0_dAdcDataOutReset_n [get_bd_pins adc_data_out_resetn_dclk] [get_bd_pins rf_reset_controller_0/dAdcDataOutReset_n]
  connect_bd_net -net rf_reset_controller_0_dDacDataInReset_n [get_bd_pins dac_data_in_resetn_dclk] [get_bd_pins rf_reset_controller_0/dDacDataInReset_n]
  connect_bd_net -net rf_reset_controller_0_r2AdcFirReset_n [get_bd_pins fir_resetn_rclk2x] [get_bd_pins rf_reset_controller_0/r2AdcFirReset_n]
  connect_bd_net -net rf_reset_controller_0_r2DacFirReset_n [get_bd_pins dac_data_in_resetn_rclk2x] [get_bd_pins rf_reset_controller_0/r2DacFirReset_n]
  connect_bd_net -net rf_reset_controller_0_rAdcEnableData [get_bd_pins adc_enable_data_rclk] [get_bd_pins rf_reset_controller_0/rAdcEnableData]
  connect_bd_net -net rf_reset_controller_0_rAdcGearboxReset_n [get_bd_pins adc_rfdc_axi_resetn_rclk] [get_bd_pins rf_reset_controller_0/rAdcGearboxReset_n]
  connect_bd_net -net rf_reset_controller_0_rAdcRfdcAxiReset_n [get_bd_pins rf_data_converter/m0_axis_aresetn] [get_bd_pins rf_reset_controller_0/rAdcRfdcAxiReset_n]
  connect_bd_net -net rf_reset_controller_0_rDacGearboxReset_n [get_bd_pins dac_data_in_resetn_rclk] [get_bd_pins rf_reset_controller_0/rDacGearboxReset_n]
  connect_bd_net -net rf_reset_controller_0_rDacRfdcAxiReset_n [get_bd_pins rf_data_converter/s0_axis_aresetn] [get_bd_pins rf_reset_controller_0/rDacRfdcAxiReset_n]
  connect_bd_net -net rf_reset_controller_1_cSoftwareStatus [get_bd_pins reg_rf_reset_control_radio1/gpio2_io_i] [get_bd_pins rf_reset_controller_1/cSoftwareStatus]
  connect_bd_net -net rf_reset_controller_1_rAdcRfdcAxiReset_n [get_bd_pins rf_data_converter/m2_axis_aresetn] [get_bd_pins rf_reset_controller_1/rAdcRfdcAxiReset_n]
  connect_bd_net -net rf_reset_controller_1_rDacRfdcAxiReset_n [get_bd_pins rf_data_converter/s1_axis_aresetn] [get_bd_pins rf_reset_controller_1/rDacRfdcAxiReset_n]
  connect_bd_net -net rf_rfdc_info_sclk_1 [get_bd_pins rf_rfdc_info_sclk] [get_bd_pins reg_rfdc_info/gpio_io_i]
  connect_bd_net -net rfdc_adc_map_dout [get_bd_pins reg_rfdc_tile_mapping/gpio_io_i] [get_bd_pins rfdc_adc_map/dout]
  connect_bd_net -net rfdc_regs_gpio_io_o [get_bd_pins reg_invert_iq_radio0/gpio_io_o] [get_bd_pins slice_iqswap_11_8_radio0/Din] [get_bd_pins slice_iqswap_3_0_radio0/Din]
  connect_bd_net -net s_axi_aresetn_0_1 [get_bd_pins s_axi_config_aresetn] [get_bd_pins ThresholdRegister/s_axi_config_aresetn] [get_bd_pins axi_interconnect_rf/ARESETN] [get_bd_pins axi_interconnect_rf/M00_ARESETN] [get_bd_pins axi_interconnect_rf/M03_ARESETN] [get_bd_pins axi_interconnect_rf/M04_ARESETN] [get_bd_pins axi_interconnect_rf/M05_ARESETN] [get_bd_pins axi_interconnect_rf/M06_ARESETN] [get_bd_pins axi_interconnect_rf/M07_ARESETN] [get_bd_pins axi_interconnect_rf/M08_ARESETN] [get_bd_pins axi_interconnect_rf/M09_ARESETN] [get_bd_pins axi_interconnect_rf/M10_ARESETN] [get_bd_pins axi_interconnect_rf/M12_ARESETN] [get_bd_pins axi_interconnect_rf/S00_ARESETN] [get_bd_pins calibration_muxes/s_axi_config_aresetn] [get_bd_pins reg_clock_gate_control/s_axi_aresetn] [get_bd_pins reg_reset_mmcm/s_axi_aresetn] [get_bd_pins reg_rf_axi_status/s_axi_aresetn] [get_bd_pins reg_rf_reset_control_radio0/s_axi_aresetn] [get_bd_pins reg_rf_reset_control_radio1/s_axi_aresetn] [get_bd_pins reg_rfdc_info/s_axi_aresetn] [get_bd_pins reg_rfdc_tile_mapping/s_axi_aresetn] [get_bd_pins rf_data_converter/s_axi_aresetn]
  connect_bd_net -net s_axi_config_clk_1 [get_bd_pins s_axi_config_clk] [get_bd_pins ThresholdRegister/s_axi_config_clk] [get_bd_pins axi_interconnect_rf/ACLK] [get_bd_pins axi_interconnect_rf/M00_ACLK] [get_bd_pins axi_interconnect_rf/M01_ACLK] [get_bd_pins axi_interconnect_rf/M03_ACLK] [get_bd_pins axi_interconnect_rf/M04_ACLK] [get_bd_pins axi_interconnect_rf/M05_ACLK] [get_bd_pins axi_interconnect_rf/M06_ACLK] [get_bd_pins axi_interconnect_rf/M07_ACLK] [get_bd_pins axi_interconnect_rf/M08_ACLK] [get_bd_pins axi_interconnect_rf/M09_ACLK] [get_bd_pins axi_interconnect_rf/M10_ACLK] [get_bd_pins axi_interconnect_rf/M12_ACLK] [get_bd_pins axi_interconnect_rf/S00_ACLK] [get_bd_pins calibration_muxes/s_axi_config_clk] [get_bd_pins clock_gates_0/ReliableClk] [get_bd_pins data_clock_mmcm/s_axi_aclk] [get_bd_pins reg_clock_gate_control/s_axi_aclk] [get_bd_pins reg_reset_mmcm/s_axi_aclk] [get_bd_pins reg_rf_axi_status/s_axi_aclk] [get_bd_pins reg_rf_reset_control_radio0/s_axi_aclk] [get_bd_pins reg_rf_reset_control_radio1/s_axi_aclk] [get_bd_pins reg_rfdc_info/s_axi_aclk] [get_bd_pins reg_rfdc_tile_mapping/s_axi_aclk] [get_bd_pins rf_data_converter/s_axi_aclk] [get_bd_pins rf_nco_reset_0/ConfigClk] [get_bd_pins rf_reset_controller_0/ConfigClk] [get_bd_pins rf_reset_controller_1/ConfigClk]
  connect_bd_net -net slice_iqswap_11_8_radio0_Dout [get_bd_pins radio0_invert_dac_iq_r0clk] [get_bd_pins slice_iqswap_11_8_radio0/Dout]
  connect_bd_net -net slice_iqswap_11_8_radio1_Dout [get_bd_pins radio1_invert_dac_iq_r1clk] [get_bd_pins slice_iqswap_11_8_radio1/Dout]
  connect_bd_net -net slice_iqswap_3_0_radio0_Dout [get_bd_pins radio0_invert_adc_iq_r0clk] [get_bd_pins slice_iqswap_3_0_radio0/Dout]
  connect_bd_net -net slice_iqswap_3_0_radio1_Dout [get_bd_pins radio1_invert_adc_iq_r1clk] [get_bd_pins slice_iqswap_3_0_radio1/Dout]
  connect_bd_net -net start_nco_reset_rclk_1 [get_bd_pins start_nco_reset_dclk] [get_bd_pins rf_nco_reset_0/dStartNcoReset]
  connect_bd_net -net sysref_in_0_2 [get_bd_pins sysref_pl_in] [get_bd_pins capture_sysref/sysref_in]
  connect_bd_net -net xlconstant_0_dout [get_bd_pins rf_nco_reset_0/cAdc1xNcoUpdateBusy] [get_bd_pins rf_nco_reset_0/cAdc3xNcoUpdateBusy] [get_bd_pins xlconstant_0/dout]
  connect_bd_net -net xlconstant_1_dout [get_bd_pins reg_rfdc_tile_mapping/gpio2_io_i] [get_bd_pins rfdc_dac_map/dout]

  # Restore current instance
  current_bd_instance $oldCurInst
}

# Hierarchical cell: ps
proc create_hier_cell_ps { parentCell nameHier } {

  variable script_folder

  if { $parentCell eq "" || $nameHier eq "" } {
     catch {common::send_gid_msg -ssname BD::TCL -id 2092 -severity "ERROR" "create_hier_cell_ps() - Empty argument(s)!"}
     return
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

  # Create cell and set as current instance
  set hier_obj [create_bd_cell -type hier $nameHier]
  current_bd_instance $hier_obj

  # Create interface pins
  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 gpio_0

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_app

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_core

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_eth_internal

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_mpm_ep

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_rf

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_rpu

  create_bd_intf_pin -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 m_axis_eth_dma

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_hp0

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_hp1

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_hpc0

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_hpc1

  create_bd_intf_pin -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 s_axis_eth_dma


  # Create pins
  create_bd_pin -dir I -type clk bus_clk
  create_bd_pin -dir I -type rst bus_rstn
  create_bd_pin -dir I -type clk clk40
  create_bd_pin -dir I -type rst clk40_rstn
  create_bd_pin -dir I irq0_lpd_rpu_n
  create_bd_pin -dir I irq1_lpd_rpu_n
  create_bd_pin -dir IO jtag0_tck
  create_bd_pin -dir IO jtag0_tdi
  create_bd_pin -dir I jtag0_tdo
  create_bd_pin -dir IO jtag0_tms
  create_bd_pin -dir O -type clk pl_clk40
  create_bd_pin -dir O -type clk pl_clk100
  create_bd_pin -dir O -type clk pl_clk166
  create_bd_pin -dir O -type clk pl_clk200
  create_bd_pin -dir I -from 7 -to 0 -type intr pl_ps_irq0
  create_bd_pin -dir I -from 5 -to 0 pl_ps_irq1_1
  create_bd_pin -dir O -type rst pl_resetn0
  create_bd_pin -dir O -type rst pl_resetn1
  create_bd_pin -dir O -type rst pl_resetn2
  create_bd_pin -dir O -type rst pl_resetn3
  create_bd_pin -dir I -type clk s_axi_hp0_aclk
  create_bd_pin -dir I -type clk s_axi_hp1_aclk
  create_bd_pin -dir I -type clk s_axi_hpc0_aclk

  # Create instance: axi_interconnect_common
  create_hier_cell_axi_interconnect_common $hier_obj axi_interconnect_common

  # Create instance: cpld_jtag_engine, and set properties
  set cpld_jtag_engine [ create_bd_cell -type ip -vlnv ettus.com:ip:axi_bitq:1.0 cpld_jtag_engine ]

  set_property -dict [ list \
   CONFIG.FREQ_HZ {40000000} \
   CONFIG.CLK_DOMAIN {x410_ps_rfdc_bd_clk40} \
 ] [get_bd_intf_pins /ps/cpld_jtag_engine/S_AXI]

  set_property -dict [ list \
   CONFIG.FREQ_HZ {40000000} \
   CONFIG.CLK_DOMAIN {x410_ps_rfdc_bd_clk40} \
 ] [get_bd_pins /ps/cpld_jtag_engine/S_AXI_ACLK]

  # Create instance: eth_dma_internal
  create_hier_cell_eth_dma_internal $hier_obj eth_dma_internal

  # Create instance: hpc1_axi_interconnect, and set properties
  set hpc1_axi_interconnect [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_interconnect:2.1 hpc1_axi_interconnect ]
  set_property -dict [ list \
   CONFIG.NUM_MI {1} \
   CONFIG.NUM_SI {2} \
 ] $hpc1_axi_interconnect

  # Create instance: inst_zynq_ps, and set properties
  set inst_zynq_ps [ create_bd_cell -type ip -vlnv xilinx.com:ip:zynq_ultra_ps_e:3.3 inst_zynq_ps ]
  set_property -dict [ list \
   CONFIG.CAN0_BOARD_INTERFACE {custom} \
   CONFIG.CAN1_BOARD_INTERFACE {custom} \
   CONFIG.CSU_BOARD_INTERFACE {custom} \
   CONFIG.DP_BOARD_INTERFACE {custom} \
   CONFIG.GEM0_BOARD_INTERFACE {custom} \
   CONFIG.GEM1_BOARD_INTERFACE {custom} \
   CONFIG.GEM2_BOARD_INTERFACE {custom} \
   CONFIG.GEM3_BOARD_INTERFACE {custom} \
   CONFIG.GPIO_BOARD_INTERFACE {custom} \
   CONFIG.IIC0_BOARD_INTERFACE {custom} \
   CONFIG.IIC1_BOARD_INTERFACE {custom} \
   CONFIG.NAND_BOARD_INTERFACE {custom} \
   CONFIG.PCIE_BOARD_INTERFACE {custom} \
   CONFIG.PJTAG_BOARD_INTERFACE {custom} \
   CONFIG.PMU_BOARD_INTERFACE {custom} \
   CONFIG.PSU_BANK_0_IO_STANDARD {LVCMOS18} \
   CONFIG.PSU_BANK_1_IO_STANDARD {LVCMOS18} \
   CONFIG.PSU_BANK_2_IO_STANDARD {LVCMOS18} \
   CONFIG.PSU_BANK_3_IO_STANDARD {LVCMOS33} \
   CONFIG.PSU_DDR_RAM_HIGHADDR {0xFFFFFFFF} \
   CONFIG.PSU_DDR_RAM_HIGHADDR_OFFSET {0x800000000} \
   CONFIG.PSU_DDR_RAM_LOWADDR_OFFSET {0x80000000} \
   CONFIG.PSU_DYNAMIC_DDR_CONFIG_EN {0} \
   CONFIG.PSU_IMPORT_BOARD_PRESET {} \
   CONFIG.PSU_MIO_0_DIRECTION {inout} \
   CONFIG.PSU_MIO_0_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_0_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_0_POLARITY {Default} \
   CONFIG.PSU_MIO_0_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_0_SLEW {slow} \
   CONFIG.PSU_MIO_10_DIRECTION {inout} \
   CONFIG.PSU_MIO_10_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_10_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_10_POLARITY {Default} \
   CONFIG.PSU_MIO_10_PULLUPDOWN {disable} \
   CONFIG.PSU_MIO_10_SLEW {slow} \
   CONFIG.PSU_MIO_11_DIRECTION {inout} \
   CONFIG.PSU_MIO_11_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_11_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_11_POLARITY {Default} \
   CONFIG.PSU_MIO_11_PULLUPDOWN {disable} \
   CONFIG.PSU_MIO_11_SLEW {slow} \
   CONFIG.PSU_MIO_12_DIRECTION {inout} \
   CONFIG.PSU_MIO_12_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_12_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_12_POLARITY {Default} \
   CONFIG.PSU_MIO_12_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_12_SLEW {slow} \
   CONFIG.PSU_MIO_13_DIRECTION {inout} \
   CONFIG.PSU_MIO_13_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_13_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_13_POLARITY {Default} \
   CONFIG.PSU_MIO_13_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_13_SLEW {slow} \
   CONFIG.PSU_MIO_14_DIRECTION {inout} \
   CONFIG.PSU_MIO_14_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_14_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_14_POLARITY {Default} \
   CONFIG.PSU_MIO_14_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_14_SLEW {slow} \
   CONFIG.PSU_MIO_15_DIRECTION {inout} \
   CONFIG.PSU_MIO_15_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_15_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_15_POLARITY {Default} \
   CONFIG.PSU_MIO_15_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_15_SLEW {slow} \
   CONFIG.PSU_MIO_16_DIRECTION {inout} \
   CONFIG.PSU_MIO_16_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_16_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_16_POLARITY {Default} \
   CONFIG.PSU_MIO_16_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_16_SLEW {slow} \
   CONFIG.PSU_MIO_17_DIRECTION {inout} \
   CONFIG.PSU_MIO_17_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_17_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_17_POLARITY {Default} \
   CONFIG.PSU_MIO_17_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_17_SLEW {slow} \
   CONFIG.PSU_MIO_18_DIRECTION {inout} \
   CONFIG.PSU_MIO_18_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_18_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_18_POLARITY {Default} \
   CONFIG.PSU_MIO_18_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_18_SLEW {slow} \
   CONFIG.PSU_MIO_19_DIRECTION {inout} \
   CONFIG.PSU_MIO_19_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_19_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_19_POLARITY {Default} \
   CONFIG.PSU_MIO_19_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_19_SLEW {slow} \
   CONFIG.PSU_MIO_1_DIRECTION {inout} \
   CONFIG.PSU_MIO_1_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_1_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_1_POLARITY {Default} \
   CONFIG.PSU_MIO_1_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_1_SLEW {slow} \
   CONFIG.PSU_MIO_20_DIRECTION {inout} \
   CONFIG.PSU_MIO_20_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_20_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_20_POLARITY {Default} \
   CONFIG.PSU_MIO_20_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_20_SLEW {slow} \
   CONFIG.PSU_MIO_21_DIRECTION {inout} \
   CONFIG.PSU_MIO_21_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_21_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_21_POLARITY {Default} \
   CONFIG.PSU_MIO_21_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_21_SLEW {slow} \
   CONFIG.PSU_MIO_22_DIRECTION {out} \
   CONFIG.PSU_MIO_22_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_22_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_22_POLARITY {Default} \
   CONFIG.PSU_MIO_22_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_22_SLEW {slow} \
   CONFIG.PSU_MIO_23_DIRECTION {out} \
   CONFIG.PSU_MIO_23_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_23_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_23_POLARITY {Default} \
   CONFIG.PSU_MIO_23_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_23_SLEW {slow} \
   CONFIG.PSU_MIO_24_DIRECTION {inout} \
   CONFIG.PSU_MIO_24_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_24_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_24_POLARITY {Default} \
   CONFIG.PSU_MIO_24_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_24_SLEW {slow} \
   CONFIG.PSU_MIO_25_DIRECTION {inout} \
   CONFIG.PSU_MIO_25_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_25_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_25_POLARITY {Default} \
   CONFIG.PSU_MIO_25_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_25_SLEW {slow} \
   CONFIG.PSU_MIO_26_DIRECTION {inout} \
   CONFIG.PSU_MIO_26_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_26_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_26_POLARITY {Default} \
   CONFIG.PSU_MIO_26_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_26_SLEW {slow} \
   CONFIG.PSU_MIO_27_DIRECTION {inout} \
   CONFIG.PSU_MIO_27_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_27_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_27_POLARITY {Default} \
   CONFIG.PSU_MIO_27_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_27_SLEW {slow} \
   CONFIG.PSU_MIO_28_DIRECTION {inout} \
   CONFIG.PSU_MIO_28_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_28_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_28_POLARITY {Default} \
   CONFIG.PSU_MIO_28_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_28_SLEW {slow} \
   CONFIG.PSU_MIO_29_DIRECTION {inout} \
   CONFIG.PSU_MIO_29_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_29_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_29_POLARITY {Default} \
   CONFIG.PSU_MIO_29_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_29_SLEW {slow} \
   CONFIG.PSU_MIO_2_DIRECTION {inout} \
   CONFIG.PSU_MIO_2_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_2_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_2_POLARITY {Default} \
   CONFIG.PSU_MIO_2_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_2_SLEW {slow} \
   CONFIG.PSU_MIO_30_DIRECTION {in} \
   CONFIG.PSU_MIO_30_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_30_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_30_POLARITY {Default} \
   CONFIG.PSU_MIO_30_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_30_SLEW {fast} \
   CONFIG.PSU_MIO_31_DIRECTION {out} \
   CONFIG.PSU_MIO_31_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_31_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_31_POLARITY {Default} \
   CONFIG.PSU_MIO_31_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_31_SLEW {slow} \
   CONFIG.PSU_MIO_32_DIRECTION {out} \
   CONFIG.PSU_MIO_32_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_32_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_32_POLARITY {Default} \
   CONFIG.PSU_MIO_32_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_32_SLEW {slow} \
   CONFIG.PSU_MIO_33_DIRECTION {in} \
   CONFIG.PSU_MIO_33_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_33_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_33_POLARITY {Default} \
   CONFIG.PSU_MIO_33_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_33_SLEW {fast} \
   CONFIG.PSU_MIO_34_DIRECTION {out} \
   CONFIG.PSU_MIO_34_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_34_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_34_POLARITY {Default} \
   CONFIG.PSU_MIO_34_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_34_SLEW {fast} \
   CONFIG.PSU_MIO_35_DIRECTION {out} \
   CONFIG.PSU_MIO_35_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_35_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_35_POLARITY {Default} \
   CONFIG.PSU_MIO_35_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_35_SLEW {slow} \
   CONFIG.PSU_MIO_36_DIRECTION {inout} \
   CONFIG.PSU_MIO_36_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_36_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_36_POLARITY {Default} \
   CONFIG.PSU_MIO_36_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_36_SLEW {slow} \
   CONFIG.PSU_MIO_37_DIRECTION {inout} \
   CONFIG.PSU_MIO_37_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_37_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_37_POLARITY {Default} \
   CONFIG.PSU_MIO_37_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_37_SLEW {slow} \
   CONFIG.PSU_MIO_38_DIRECTION {inout} \
   CONFIG.PSU_MIO_38_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_38_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_38_POLARITY {Default} \
   CONFIG.PSU_MIO_38_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_38_SLEW {slow} \
   CONFIG.PSU_MIO_39_DIRECTION {inout} \
   CONFIG.PSU_MIO_39_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_39_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_39_POLARITY {Default} \
   CONFIG.PSU_MIO_39_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_39_SLEW {slow} \
   CONFIG.PSU_MIO_3_DIRECTION {inout} \
   CONFIG.PSU_MIO_3_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_3_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_3_POLARITY {Default} \
   CONFIG.PSU_MIO_3_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_3_SLEW {slow} \
   CONFIG.PSU_MIO_40_DIRECTION {inout} \
   CONFIG.PSU_MIO_40_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_40_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_40_POLARITY {Default} \
   CONFIG.PSU_MIO_40_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_40_SLEW {slow} \
   CONFIG.PSU_MIO_41_DIRECTION {inout} \
   CONFIG.PSU_MIO_41_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_41_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_41_POLARITY {Default} \
   CONFIG.PSU_MIO_41_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_41_SLEW {slow} \
   CONFIG.PSU_MIO_42_DIRECTION {inout} \
   CONFIG.PSU_MIO_42_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_42_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_42_POLARITY {Default} \
   CONFIG.PSU_MIO_42_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_42_SLEW {slow} \
   CONFIG.PSU_MIO_43_DIRECTION {inout} \
   CONFIG.PSU_MIO_43_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_43_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_43_POLARITY {Default} \
   CONFIG.PSU_MIO_43_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_43_SLEW {slow} \
   CONFIG.PSU_MIO_44_DIRECTION {inout} \
   CONFIG.PSU_MIO_44_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_44_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_44_POLARITY {Default} \
   CONFIG.PSU_MIO_44_PULLUPDOWN {pulldown} \
   CONFIG.PSU_MIO_44_SLEW {slow} \
   CONFIG.PSU_MIO_45_DIRECTION {in} \
   CONFIG.PSU_MIO_45_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_45_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_45_POLARITY {Default} \
   CONFIG.PSU_MIO_45_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_45_SLEW {fast} \
   CONFIG.PSU_MIO_46_DIRECTION {inout} \
   CONFIG.PSU_MIO_46_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_46_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_46_POLARITY {Default} \
   CONFIG.PSU_MIO_46_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_46_SLEW {slow} \
   CONFIG.PSU_MIO_47_DIRECTION {inout} \
   CONFIG.PSU_MIO_47_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_47_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_47_POLARITY {Default} \
   CONFIG.PSU_MIO_47_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_47_SLEW {slow} \
   CONFIG.PSU_MIO_48_DIRECTION {inout} \
   CONFIG.PSU_MIO_48_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_48_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_48_POLARITY {Default} \
   CONFIG.PSU_MIO_48_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_48_SLEW {slow} \
   CONFIG.PSU_MIO_49_DIRECTION {inout} \
   CONFIG.PSU_MIO_49_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_49_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_49_POLARITY {Default} \
   CONFIG.PSU_MIO_49_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_49_SLEW {slow} \
   CONFIG.PSU_MIO_4_DIRECTION {inout} \
   CONFIG.PSU_MIO_4_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_4_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_4_POLARITY {Default} \
   CONFIG.PSU_MIO_4_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_4_SLEW {slow} \
   CONFIG.PSU_MIO_50_DIRECTION {inout} \
   CONFIG.PSU_MIO_50_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_50_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_50_POLARITY {Default} \
   CONFIG.PSU_MIO_50_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_50_SLEW {slow} \
   CONFIG.PSU_MIO_51_DIRECTION {out} \
   CONFIG.PSU_MIO_51_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_51_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_51_POLARITY {Default} \
   CONFIG.PSU_MIO_51_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_51_SLEW {slow} \
   CONFIG.PSU_MIO_52_DIRECTION {in} \
   CONFIG.PSU_MIO_52_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_52_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_52_POLARITY {Default} \
   CONFIG.PSU_MIO_52_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_52_SLEW {fast} \
   CONFIG.PSU_MIO_53_DIRECTION {in} \
   CONFIG.PSU_MIO_53_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_53_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_53_POLARITY {Default} \
   CONFIG.PSU_MIO_53_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_53_SLEW {fast} \
   CONFIG.PSU_MIO_54_DIRECTION {inout} \
   CONFIG.PSU_MIO_54_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_54_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_54_POLARITY {Default} \
   CONFIG.PSU_MIO_54_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_54_SLEW {slow} \
   CONFIG.PSU_MIO_55_DIRECTION {in} \
   CONFIG.PSU_MIO_55_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_55_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_55_POLARITY {Default} \
   CONFIG.PSU_MIO_55_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_55_SLEW {fast} \
   CONFIG.PSU_MIO_56_DIRECTION {inout} \
   CONFIG.PSU_MIO_56_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_56_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_56_POLARITY {Default} \
   CONFIG.PSU_MIO_56_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_56_SLEW {slow} \
   CONFIG.PSU_MIO_57_DIRECTION {inout} \
   CONFIG.PSU_MIO_57_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_57_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_57_POLARITY {Default} \
   CONFIG.PSU_MIO_57_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_57_SLEW {slow} \
   CONFIG.PSU_MIO_58_DIRECTION {out} \
   CONFIG.PSU_MIO_58_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_58_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_58_POLARITY {Default} \
   CONFIG.PSU_MIO_58_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_58_SLEW {slow} \
   CONFIG.PSU_MIO_59_DIRECTION {inout} \
   CONFIG.PSU_MIO_59_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_59_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_59_POLARITY {Default} \
   CONFIG.PSU_MIO_59_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_59_SLEW {slow} \
   CONFIG.PSU_MIO_5_DIRECTION {inout} \
   CONFIG.PSU_MIO_5_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_5_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_5_POLARITY {Default} \
   CONFIG.PSU_MIO_5_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_5_SLEW {slow} \
   CONFIG.PSU_MIO_60_DIRECTION {inout} \
   CONFIG.PSU_MIO_60_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_60_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_60_POLARITY {Default} \
   CONFIG.PSU_MIO_60_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_60_SLEW {slow} \
   CONFIG.PSU_MIO_61_DIRECTION {inout} \
   CONFIG.PSU_MIO_61_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_61_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_61_POLARITY {Default} \
   CONFIG.PSU_MIO_61_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_61_SLEW {slow} \
   CONFIG.PSU_MIO_62_DIRECTION {inout} \
   CONFIG.PSU_MIO_62_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_62_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_62_POLARITY {Default} \
   CONFIG.PSU_MIO_62_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_62_SLEW {slow} \
   CONFIG.PSU_MIO_63_DIRECTION {inout} \
   CONFIG.PSU_MIO_63_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_63_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_63_POLARITY {Default} \
   CONFIG.PSU_MIO_63_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_63_SLEW {slow} \
   CONFIG.PSU_MIO_64_DIRECTION {out} \
   CONFIG.PSU_MIO_64_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_64_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_64_POLARITY {Default} \
   CONFIG.PSU_MIO_64_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_64_SLEW {slow} \
   CONFIG.PSU_MIO_65_DIRECTION {out} \
   CONFIG.PSU_MIO_65_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_65_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_65_POLARITY {Default} \
   CONFIG.PSU_MIO_65_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_65_SLEW {slow} \
   CONFIG.PSU_MIO_66_DIRECTION {out} \
   CONFIG.PSU_MIO_66_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_66_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_66_POLARITY {Default} \
   CONFIG.PSU_MIO_66_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_66_SLEW {slow} \
   CONFIG.PSU_MIO_67_DIRECTION {out} \
   CONFIG.PSU_MIO_67_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_67_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_67_POLARITY {Default} \
   CONFIG.PSU_MIO_67_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_67_SLEW {slow} \
   CONFIG.PSU_MIO_68_DIRECTION {out} \
   CONFIG.PSU_MIO_68_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_68_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_68_POLARITY {Default} \
   CONFIG.PSU_MIO_68_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_68_SLEW {slow} \
   CONFIG.PSU_MIO_69_DIRECTION {out} \
   CONFIG.PSU_MIO_69_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_69_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_69_POLARITY {Default} \
   CONFIG.PSU_MIO_69_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_69_SLEW {slow} \
   CONFIG.PSU_MIO_6_DIRECTION {inout} \
   CONFIG.PSU_MIO_6_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_6_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_6_POLARITY {Default} \
   CONFIG.PSU_MIO_6_PULLUPDOWN {pulldown} \
   CONFIG.PSU_MIO_6_SLEW {slow} \
   CONFIG.PSU_MIO_70_DIRECTION {in} \
   CONFIG.PSU_MIO_70_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_70_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_70_POLARITY {Default} \
   CONFIG.PSU_MIO_70_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_70_SLEW {fast} \
   CONFIG.PSU_MIO_71_DIRECTION {in} \
   CONFIG.PSU_MIO_71_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_71_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_71_POLARITY {Default} \
   CONFIG.PSU_MIO_71_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_71_SLEW {fast} \
   CONFIG.PSU_MIO_72_DIRECTION {in} \
   CONFIG.PSU_MIO_72_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_72_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_72_POLARITY {Default} \
   CONFIG.PSU_MIO_72_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_72_SLEW {fast} \
   CONFIG.PSU_MIO_73_DIRECTION {in} \
   CONFIG.PSU_MIO_73_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_73_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_73_POLARITY {Default} \
   CONFIG.PSU_MIO_73_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_73_SLEW {fast} \
   CONFIG.PSU_MIO_74_DIRECTION {in} \
   CONFIG.PSU_MIO_74_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_74_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_74_POLARITY {Default} \
   CONFIG.PSU_MIO_74_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_74_SLEW {fast} \
   CONFIG.PSU_MIO_75_DIRECTION {in} \
   CONFIG.PSU_MIO_75_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_75_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_75_POLARITY {Default} \
   CONFIG.PSU_MIO_75_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_75_SLEW {fast} \
   CONFIG.PSU_MIO_76_DIRECTION {out} \
   CONFIG.PSU_MIO_76_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_76_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_76_POLARITY {Default} \
   CONFIG.PSU_MIO_76_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_76_SLEW {slow} \
   CONFIG.PSU_MIO_77_DIRECTION {inout} \
   CONFIG.PSU_MIO_77_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_77_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_77_POLARITY {Default} \
   CONFIG.PSU_MIO_77_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_77_SLEW {slow} \
   CONFIG.PSU_MIO_7_DIRECTION {out} \
   CONFIG.PSU_MIO_7_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_7_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_7_POLARITY {Default} \
   CONFIG.PSU_MIO_7_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_7_SLEW {slow} \
   CONFIG.PSU_MIO_8_DIRECTION {out} \
   CONFIG.PSU_MIO_8_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_8_INPUT_TYPE {cmos} \
   CONFIG.PSU_MIO_8_POLARITY {Default} \
   CONFIG.PSU_MIO_8_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_8_SLEW {slow} \
   CONFIG.PSU_MIO_9_DIRECTION {inout} \
   CONFIG.PSU_MIO_9_DRIVE_STRENGTH {12} \
   CONFIG.PSU_MIO_9_INPUT_TYPE {schmitt} \
   CONFIG.PSU_MIO_9_POLARITY {Default} \
   CONFIG.PSU_MIO_9_PULLUPDOWN {pullup} \
   CONFIG.PSU_MIO_9_SLEW {slow} \
   CONFIG.PSU_MIO_TREE_PERIPHERALS {GPIO0 MIO#GPIO0 MIO#I2C 0#I2C 0#GPIO0 MIO#GPIO0 MIO#SPI 1#SPI 1#SPI 1#SPI 1#SPI\
1#SPI 1#GPIO0 MIO#SD 0#SD 0#SD 0#SD 0#SD 0#SD 0#SD 0#SD 0#SD 0#SD 0#SD 0#GPIO0\
MIO#GPIO0 MIO#GPIO1 MIO#GPIO1 MIO#GPIO1 MIO#GPIO1 MIO#UART 0#UART 0#UART 1#UART\
1#PMU GPO 2#PMU GPO 3#I2C 1#I2C 1#GPIO1 MIO#SD 1#SD 1#SD 1#SD 1#GPIO1 MIO#GPIO1\
MIO#SD 1#SD 1#SD 1#SD 1#SD 1#SD 1#SD 1#USB 0#USB 0#USB 0#USB 0#USB 0#USB 0#USB\
0#USB 0#USB 0#USB 0#USB 0#USB 0#Gem 3#Gem 3#Gem 3#Gem 3#Gem 3#Gem 3#Gem 3#Gem\
3#Gem 3#Gem 3#Gem 3#Gem 3#MDIO 3#MDIO 3}\
   CONFIG.PSU_MIO_TREE_SIGNALS {gpio0[0]#gpio0[1]#scl_out#sda_out#gpio0[4]#gpio0[5]#sclk_out#n_ss_out[2]#n_ss_out[1]#n_ss_out[0]#miso#mosi#gpio0[12]#sdio0_data_out[0]#sdio0_data_out[1]#sdio0_data_out[2]#sdio0_data_out[3]#sdio0_data_out[4]#sdio0_data_out[5]#sdio0_data_out[6]#sdio0_data_out[7]#sdio0_cmd_out#sdio0_clk_out#sdio0_bus_pow#gpio0[24]#gpio0[25]#gpio1[26]#gpio1[27]#gpio1[28]#gpio1[29]#rxd#txd#txd#rxd#gpo[2]#gpo[3]#scl_out#sda_out#gpio1[38]#sdio1_data_out[4]#sdio1_data_out[5]#sdio1_data_out[6]#sdio1_data_out[7]#gpio1[43]#gpio1[44]#sdio1_cd_n#sdio1_data_out[0]#sdio1_data_out[1]#sdio1_data_out[2]#sdio1_data_out[3]#sdio1_cmd_out#sdio1_clk_out#ulpi_clk_in#ulpi_dir#ulpi_tx_data[2]#ulpi_nxt#ulpi_tx_data[0]#ulpi_tx_data[1]#ulpi_stp#ulpi_tx_data[3]#ulpi_tx_data[4]#ulpi_tx_data[5]#ulpi_tx_data[6]#ulpi_tx_data[7]#rgmii_tx_clk#rgmii_txd[0]#rgmii_txd[1]#rgmii_txd[2]#rgmii_txd[3]#rgmii_tx_ctl#rgmii_rx_clk#rgmii_rxd[0]#rgmii_rxd[1]#rgmii_rxd[2]#rgmii_rxd[3]#rgmii_rx_ctl#gem3_mdc#gem3_mdio_out}\
   CONFIG.PSU_PERIPHERAL_BOARD_PRESET {} \
   CONFIG.PSU_SD0_INTERNAL_BUS_WIDTH {8} \
   CONFIG.PSU_SD1_INTERNAL_BUS_WIDTH {8} \
   CONFIG.PSU_SMC_CYCLE_T0 {NA} \
   CONFIG.PSU_SMC_CYCLE_T1 {NA} \
   CONFIG.PSU_SMC_CYCLE_T2 {NA} \
   CONFIG.PSU_SMC_CYCLE_T3 {NA} \
   CONFIG.PSU_SMC_CYCLE_T4 {NA} \
   CONFIG.PSU_SMC_CYCLE_T5 {NA} \
   CONFIG.PSU_SMC_CYCLE_T6 {NA} \
   CONFIG.PSU_USB3__DUAL_CLOCK_ENABLE {1} \
   CONFIG.PSU_VALUE_SILVERSION {3} \
   CONFIG.PSU__ACPU0__POWER__ON {1} \
   CONFIG.PSU__ACPU1__POWER__ON {1} \
   CONFIG.PSU__ACPU2__POWER__ON {1} \
   CONFIG.PSU__ACPU3__POWER__ON {1} \
   CONFIG.PSU__ACTUAL__IP {1} \
   CONFIG.PSU__ACT_DDR_FREQ_MHZ {1199.988037} \
   CONFIG.PSU__AFI0_COHERENCY {0} \
   CONFIG.PSU__AFI1_COHERENCY {0} \
   CONFIG.PSU__AUX_REF_CLK__FREQMHZ {33.333} \
   CONFIG.PSU__CAN0_LOOP_CAN1__ENABLE {0} \
   CONFIG.PSU__CAN0__GRP_CLK__ENABLE {0} \
   CONFIG.PSU__CAN0__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__CAN1__GRP_CLK__ENABLE {0} \
   CONFIG.PSU__CAN1__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__CRF_APB__ACPU_CTRL__ACT_FREQMHZ {1199.988037} \
   CONFIG.PSU__CRF_APB__ACPU_CTRL__DIVISOR0 {1} \
   CONFIG.PSU__CRF_APB__ACPU_CTRL__FREQMHZ {1200} \
   CONFIG.PSU__CRF_APB__ACPU_CTRL__SRCSEL {APLL} \
   CONFIG.PSU__CRF_APB__ACPU__FRAC_ENABLED {0} \
   CONFIG.PSU__CRF_APB__AFI0_REF_CTRL__ACT_FREQMHZ {667} \
   CONFIG.PSU__CRF_APB__AFI0_REF_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRF_APB__AFI0_REF_CTRL__FREQMHZ {667} \
   CONFIG.PSU__CRF_APB__AFI0_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRF_APB__AFI0_REF__ENABLE {0} \
   CONFIG.PSU__CRF_APB__AFI1_REF_CTRL__ACT_FREQMHZ {667} \
   CONFIG.PSU__CRF_APB__AFI1_REF_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRF_APB__AFI1_REF_CTRL__FREQMHZ {667} \
   CONFIG.PSU__CRF_APB__AFI1_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRF_APB__AFI1_REF__ENABLE {0} \
   CONFIG.PSU__CRF_APB__AFI2_REF_CTRL__ACT_FREQMHZ {667} \
   CONFIG.PSU__CRF_APB__AFI2_REF_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRF_APB__AFI2_REF_CTRL__FREQMHZ {667} \
   CONFIG.PSU__CRF_APB__AFI2_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRF_APB__AFI2_REF__ENABLE {0} \
   CONFIG.PSU__CRF_APB__AFI3_REF_CTRL__ACT_FREQMHZ {667} \
   CONFIG.PSU__CRF_APB__AFI3_REF_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRF_APB__AFI3_REF_CTRL__FREQMHZ {667} \
   CONFIG.PSU__CRF_APB__AFI3_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRF_APB__AFI3_REF__ENABLE {0} \
   CONFIG.PSU__CRF_APB__AFI4_REF_CTRL__ACT_FREQMHZ {667} \
   CONFIG.PSU__CRF_APB__AFI4_REF_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRF_APB__AFI4_REF_CTRL__FREQMHZ {667} \
   CONFIG.PSU__CRF_APB__AFI4_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRF_APB__AFI4_REF__ENABLE {0} \
   CONFIG.PSU__CRF_APB__AFI5_REF_CTRL__ACT_FREQMHZ {667} \
   CONFIG.PSU__CRF_APB__AFI5_REF_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRF_APB__AFI5_REF_CTRL__FREQMHZ {667} \
   CONFIG.PSU__CRF_APB__AFI5_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRF_APB__AFI5_REF__ENABLE {0} \
   CONFIG.PSU__CRF_APB__APLL_CTRL__DIV2 {1} \
   CONFIG.PSU__CRF_APB__APLL_CTRL__FBDIV {72} \
   CONFIG.PSU__CRF_APB__APLL_CTRL__FRACDATA {0.000000} \
   CONFIG.PSU__CRF_APB__APLL_CTRL__FRACFREQ {27.138} \
   CONFIG.PSU__CRF_APB__APLL_CTRL__SRCSEL {PSS_REF_CLK} \
   CONFIG.PSU__CRF_APB__APLL_FRAC_CFG__ENABLED {0} \
   CONFIG.PSU__CRF_APB__APLL_TO_LPD_CTRL__DIVISOR0 {3} \
   CONFIG.PSU__CRF_APB__APM_CTRL__ACT_FREQMHZ {1} \
   CONFIG.PSU__CRF_APB__APM_CTRL__DIVISOR0 {1} \
   CONFIG.PSU__CRF_APB__APM_CTRL__FREQMHZ {1} \
   CONFIG.PSU__CRF_APB__DBG_FPD_CTRL__ACT_FREQMHZ {249.997498} \
   CONFIG.PSU__CRF_APB__DBG_FPD_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRF_APB__DBG_FPD_CTRL__FREQMHZ {250} \
   CONFIG.PSU__CRF_APB__DBG_FPD_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRF_APB__DBG_TRACE_CTRL__ACT_FREQMHZ {250} \
   CONFIG.PSU__CRF_APB__DBG_TRACE_CTRL__DIVISOR0 {5} \
   CONFIG.PSU__CRF_APB__DBG_TRACE_CTRL__FREQMHZ {250} \
   CONFIG.PSU__CRF_APB__DBG_TRACE_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRF_APB__DBG_TSTMP_CTRL__ACT_FREQMHZ {249.997498} \
   CONFIG.PSU__CRF_APB__DBG_TSTMP_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRF_APB__DBG_TSTMP_CTRL__FREQMHZ {250} \
   CONFIG.PSU__CRF_APB__DBG_TSTMP_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRF_APB__DDR_CTRL__ACT_FREQMHZ {599.994019} \
   CONFIG.PSU__CRF_APB__DDR_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRF_APB__DDR_CTRL__FREQMHZ {1200} \
   CONFIG.PSU__CRF_APB__DDR_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRF_APB__DPDMA_REF_CTRL__ACT_FREQMHZ {599.994019} \
   CONFIG.PSU__CRF_APB__DPDMA_REF_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRF_APB__DPDMA_REF_CTRL__FREQMHZ {600} \
   CONFIG.PSU__CRF_APB__DPDMA_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRF_APB__DPLL_CTRL__DIV2 {1} \
   CONFIG.PSU__CRF_APB__DPLL_CTRL__FBDIV {72} \
   CONFIG.PSU__CRF_APB__DPLL_CTRL__FRACDATA {0.000000} \
   CONFIG.PSU__CRF_APB__DPLL_CTRL__FRACFREQ {27.138} \
   CONFIG.PSU__CRF_APB__DPLL_CTRL__SRCSEL {PSS_REF_CLK} \
   CONFIG.PSU__CRF_APB__DPLL_FRAC_CFG__ENABLED {0} \
   CONFIG.PSU__CRF_APB__DPLL_TO_LPD_CTRL__DIVISOR0 {3} \
   CONFIG.PSU__CRF_APB__DP_AUDIO_REF_CTRL__ACT_FREQMHZ {25} \
   CONFIG.PSU__CRF_APB__DP_AUDIO_REF_CTRL__DIVISOR0 {63} \
   CONFIG.PSU__CRF_APB__DP_AUDIO_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRF_APB__DP_AUDIO_REF_CTRL__FREQMHZ {25} \
   CONFIG.PSU__CRF_APB__DP_AUDIO_REF_CTRL__SRCSEL {VPLL} \
   CONFIG.PSU__CRF_APB__DP_AUDIO__FRAC_ENABLED {0} \
   CONFIG.PSU__CRF_APB__DP_STC_REF_CTRL__ACT_FREQMHZ {27} \
   CONFIG.PSU__CRF_APB__DP_STC_REF_CTRL__DIVISOR0 {6} \
   CONFIG.PSU__CRF_APB__DP_STC_REF_CTRL__DIVISOR1 {10} \
   CONFIG.PSU__CRF_APB__DP_STC_REF_CTRL__FREQMHZ {27} \
   CONFIG.PSU__CRF_APB__DP_STC_REF_CTRL__SRCSEL {VPLL} \
   CONFIG.PSU__CRF_APB__DP_VIDEO_REF_CTRL__ACT_FREQMHZ {320} \
   CONFIG.PSU__CRF_APB__DP_VIDEO_REF_CTRL__DIVISOR0 {5} \
   CONFIG.PSU__CRF_APB__DP_VIDEO_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRF_APB__DP_VIDEO_REF_CTRL__FREQMHZ {300} \
   CONFIG.PSU__CRF_APB__DP_VIDEO_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRF_APB__DP_VIDEO__FRAC_ENABLED {0} \
   CONFIG.PSU__CRF_APB__GDMA_REF_CTRL__ACT_FREQMHZ {599.994019} \
   CONFIG.PSU__CRF_APB__GDMA_REF_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRF_APB__GDMA_REF_CTRL__FREQMHZ {600} \
   CONFIG.PSU__CRF_APB__GDMA_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRF_APB__GPU_REF_CTRL__ACT_FREQMHZ {0} \
   CONFIG.PSU__CRF_APB__GPU_REF_CTRL__DIVISOR0 {3} \
   CONFIG.PSU__CRF_APB__GPU_REF_CTRL__FREQMHZ {600} \
   CONFIG.PSU__CRF_APB__GPU_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRF_APB__GTGREF0_REF_CTRL__ACT_FREQMHZ {-1} \
   CONFIG.PSU__CRF_APB__GTGREF0_REF_CTRL__DIVISOR0 {-1} \
   CONFIG.PSU__CRF_APB__GTGREF0_REF_CTRL__FREQMHZ {-1} \
   CONFIG.PSU__CRF_APB__GTGREF0_REF_CTRL__SRCSEL {NA} \
   CONFIG.PSU__CRF_APB__GTGREF0__ENABLE {NA} \
   CONFIG.PSU__CRF_APB__PCIE_REF_CTRL__ACT_FREQMHZ {250} \
   CONFIG.PSU__CRF_APB__PCIE_REF_CTRL__DIVISOR0 {6} \
   CONFIG.PSU__CRF_APB__PCIE_REF_CTRL__FREQMHZ {250} \
   CONFIG.PSU__CRF_APB__PCIE_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRF_APB__SATA_REF_CTRL__ACT_FREQMHZ {250} \
   CONFIG.PSU__CRF_APB__SATA_REF_CTRL__DIVISOR0 {5} \
   CONFIG.PSU__CRF_APB__SATA_REF_CTRL__FREQMHZ {250} \
   CONFIG.PSU__CRF_APB__SATA_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRF_APB__TOPSW_LSBUS_CTRL__ACT_FREQMHZ {99.999001} \
   CONFIG.PSU__CRF_APB__TOPSW_LSBUS_CTRL__DIVISOR0 {5} \
   CONFIG.PSU__CRF_APB__TOPSW_LSBUS_CTRL__FREQMHZ {100} \
   CONFIG.PSU__CRF_APB__TOPSW_LSBUS_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRF_APB__TOPSW_MAIN_CTRL__ACT_FREQMHZ {533.328003} \
   CONFIG.PSU__CRF_APB__TOPSW_MAIN_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRF_APB__TOPSW_MAIN_CTRL__FREQMHZ {533.333} \
   CONFIG.PSU__CRF_APB__TOPSW_MAIN_CTRL__SRCSEL {VPLL} \
   CONFIG.PSU__CRF_APB__VPLL_CTRL__DIV2 {1} \
   CONFIG.PSU__CRF_APB__VPLL_CTRL__FBDIV {64} \
   CONFIG.PSU__CRF_APB__VPLL_CTRL__FRACDATA {0.000000} \
   CONFIG.PSU__CRF_APB__VPLL_CTRL__FRACFREQ {27.138} \
   CONFIG.PSU__CRF_APB__VPLL_CTRL__SRCSEL {PSS_REF_CLK} \
   CONFIG.PSU__CRF_APB__VPLL_FRAC_CFG__ENABLED {0} \
   CONFIG.PSU__CRF_APB__VPLL_TO_LPD_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRL_APB__ADMA_REF_CTRL__ACT_FREQMHZ {499.994995} \
   CONFIG.PSU__CRL_APB__ADMA_REF_CTRL__DIVISOR0 {3} \
   CONFIG.PSU__CRL_APB__ADMA_REF_CTRL__FREQMHZ {500} \
   CONFIG.PSU__CRL_APB__ADMA_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__AFI6_REF_CTRL__ACT_FREQMHZ {500} \
   CONFIG.PSU__CRL_APB__AFI6_REF_CTRL__DIVISOR0 {3} \
   CONFIG.PSU__CRL_APB__AFI6_REF_CTRL__FREQMHZ {500} \
   CONFIG.PSU__CRL_APB__AFI6_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__AFI6__ENABLE {0} \
   CONFIG.PSU__CRL_APB__AMS_REF_CTRL__ACT_FREQMHZ {49.999500} \
   CONFIG.PSU__CRL_APB__AMS_REF_CTRL__DIVISOR0 {30} \
   CONFIG.PSU__CRL_APB__AMS_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__AMS_REF_CTRL__FREQMHZ {50} \
   CONFIG.PSU__CRL_APB__AMS_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__CAN0_REF_CTRL__ACT_FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__CAN0_REF_CTRL__DIVISOR0 {15} \
   CONFIG.PSU__CRL_APB__CAN0_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__CAN0_REF_CTRL__FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__CAN0_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__CAN1_REF_CTRL__ACT_FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__CAN1_REF_CTRL__DIVISOR0 {15} \
   CONFIG.PSU__CRL_APB__CAN1_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__CAN1_REF_CTRL__FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__CAN1_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__CPU_R5_CTRL__ACT_FREQMHZ {499.994995} \
   CONFIG.PSU__CRL_APB__CPU_R5_CTRL__DIVISOR0 {3} \
   CONFIG.PSU__CRL_APB__CPU_R5_CTRL__FREQMHZ {500} \
   CONFIG.PSU__CRL_APB__CPU_R5_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__CSU_PLL_CTRL__ACT_FREQMHZ {180} \
   CONFIG.PSU__CRL_APB__CSU_PLL_CTRL__DIVISOR0 {3} \
   CONFIG.PSU__CRL_APB__CSU_PLL_CTRL__FREQMHZ {180} \
   CONFIG.PSU__CRL_APB__CSU_PLL_CTRL__SRCSEL {SysOsc} \
   CONFIG.PSU__CRL_APB__DBG_LPD_CTRL__ACT_FREQMHZ {249.997498} \
   CONFIG.PSU__CRL_APB__DBG_LPD_CTRL__DIVISOR0 {6} \
   CONFIG.PSU__CRL_APB__DBG_LPD_CTRL__FREQMHZ {250} \
   CONFIG.PSU__CRL_APB__DBG_LPD_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__DEBUG_R5_ATCLK_CTRL__ACT_FREQMHZ {1000} \
   CONFIG.PSU__CRL_APB__DEBUG_R5_ATCLK_CTRL__DIVISOR0 {6} \
   CONFIG.PSU__CRL_APB__DEBUG_R5_ATCLK_CTRL__FREQMHZ {1000} \
   CONFIG.PSU__CRL_APB__DEBUG_R5_ATCLK_CTRL__SRCSEL {RPLL} \
   CONFIG.PSU__CRL_APB__DLL_REF_CTRL__ACT_FREQMHZ {1499.984985} \
   CONFIG.PSU__CRL_APB__DLL_REF_CTRL__FREQMHZ {1500} \
   CONFIG.PSU__CRL_APB__DLL_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__GEM0_REF_CTRL__ACT_FREQMHZ {125} \
   CONFIG.PSU__CRL_APB__GEM0_REF_CTRL__DIVISOR0 {12} \
   CONFIG.PSU__CRL_APB__GEM0_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__GEM0_REF_CTRL__FREQMHZ {125} \
   CONFIG.PSU__CRL_APB__GEM0_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__GEM1_REF_CTRL__ACT_FREQMHZ {125} \
   CONFIG.PSU__CRL_APB__GEM1_REF_CTRL__DIVISOR0 {12} \
   CONFIG.PSU__CRL_APB__GEM1_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__GEM1_REF_CTRL__FREQMHZ {125} \
   CONFIG.PSU__CRL_APB__GEM1_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__GEM2_REF_CTRL__ACT_FREQMHZ {125} \
   CONFIG.PSU__CRL_APB__GEM2_REF_CTRL__DIVISOR0 {12} \
   CONFIG.PSU__CRL_APB__GEM2_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__GEM2_REF_CTRL__FREQMHZ {125} \
   CONFIG.PSU__CRL_APB__GEM2_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__GEM3_REF_CTRL__ACT_FREQMHZ {124.998749} \
   CONFIG.PSU__CRL_APB__GEM3_REF_CTRL__DIVISOR0 {12} \
   CONFIG.PSU__CRL_APB__GEM3_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__GEM3_REF_CTRL__FREQMHZ {125} \
   CONFIG.PSU__CRL_APB__GEM3_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__GEM_TSU_REF_CTRL__ACT_FREQMHZ {249.997498} \
   CONFIG.PSU__CRL_APB__GEM_TSU_REF_CTRL__DIVISOR0 {6} \
   CONFIG.PSU__CRL_APB__GEM_TSU_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__GEM_TSU_REF_CTRL__FREQMHZ {250} \
   CONFIG.PSU__CRL_APB__GEM_TSU_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__I2C0_REF_CTRL__ACT_FREQMHZ {99.999001} \
   CONFIG.PSU__CRL_APB__I2C0_REF_CTRL__DIVISOR0 {15} \
   CONFIG.PSU__CRL_APB__I2C0_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__I2C0_REF_CTRL__FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__I2C0_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__I2C1_REF_CTRL__ACT_FREQMHZ {99.999001} \
   CONFIG.PSU__CRL_APB__I2C1_REF_CTRL__DIVISOR0 {15} \
   CONFIG.PSU__CRL_APB__I2C1_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__I2C1_REF_CTRL__FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__I2C1_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__IOPLL_CTRL__DIV2 {1} \
   CONFIG.PSU__CRL_APB__IOPLL_CTRL__FBDIV {90} \
   CONFIG.PSU__CRL_APB__IOPLL_CTRL__FRACDATA {0.000000} \
   CONFIG.PSU__CRL_APB__IOPLL_CTRL__FRACFREQ {27.138} \
   CONFIG.PSU__CRL_APB__IOPLL_CTRL__SRCSEL {PSS_REF_CLK} \
   CONFIG.PSU__CRL_APB__IOPLL_FRAC_CFG__ENABLED {0} \
   CONFIG.PSU__CRL_APB__IOPLL_TO_FPD_CTRL__DIVISOR0 {3} \
   CONFIG.PSU__CRL_APB__IOU_SWITCH_CTRL__ACT_FREQMHZ {266.664001} \
   CONFIG.PSU__CRL_APB__IOU_SWITCH_CTRL__DIVISOR0 {3} \
   CONFIG.PSU__CRL_APB__IOU_SWITCH_CTRL__FREQMHZ {267} \
   CONFIG.PSU__CRL_APB__IOU_SWITCH_CTRL__SRCSEL {RPLL} \
   CONFIG.PSU__CRL_APB__LPD_LSBUS_CTRL__ACT_FREQMHZ {99.999001} \
   CONFIG.PSU__CRL_APB__LPD_LSBUS_CTRL__DIVISOR0 {15} \
   CONFIG.PSU__CRL_APB__LPD_LSBUS_CTRL__FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__LPD_LSBUS_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__LPD_SWITCH_CTRL__ACT_FREQMHZ {499.994995} \
   CONFIG.PSU__CRL_APB__LPD_SWITCH_CTRL__DIVISOR0 {3} \
   CONFIG.PSU__CRL_APB__LPD_SWITCH_CTRL__FREQMHZ {500} \
   CONFIG.PSU__CRL_APB__LPD_SWITCH_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__NAND_REF_CTRL__ACT_FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__NAND_REF_CTRL__DIVISOR0 {15} \
   CONFIG.PSU__CRL_APB__NAND_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__NAND_REF_CTRL__FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__NAND_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__OCM_MAIN_CTRL__ACT_FREQMHZ {500} \
   CONFIG.PSU__CRL_APB__OCM_MAIN_CTRL__DIVISOR0 {3} \
   CONFIG.PSU__CRL_APB__OCM_MAIN_CTRL__FREQMHZ {500} \
   CONFIG.PSU__CRL_APB__OCM_MAIN_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__PCAP_CTRL__ACT_FREQMHZ {187.498123} \
   CONFIG.PSU__CRL_APB__PCAP_CTRL__DIVISOR0 {8} \
   CONFIG.PSU__CRL_APB__PCAP_CTRL__FREQMHZ {200} \
   CONFIG.PSU__CRL_APB__PCAP_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__PL0_REF_CTRL__ACT_FREQMHZ {99.999001} \
   CONFIG.PSU__CRL_APB__PL0_REF_CTRL__DIVISOR0 {4} \
   CONFIG.PSU__CRL_APB__PL0_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__PL0_REF_CTRL__FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__PL0_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRL_APB__PL1_REF_CTRL__ACT_FREQMHZ {39.999599} \
   CONFIG.PSU__CRL_APB__PL1_REF_CTRL__DIVISOR0 {10} \
   CONFIG.PSU__CRL_APB__PL1_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__PL1_REF_CTRL__FREQMHZ {40} \
   CONFIG.PSU__CRL_APB__PL1_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRL_APB__PL2_REF_CTRL__ACT_FREQMHZ {166.664993} \
   CONFIG.PSU__CRL_APB__PL2_REF_CTRL__DIVISOR0 {9} \
   CONFIG.PSU__CRL_APB__PL2_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__PL2_REF_CTRL__FREQMHZ {166.6667} \
   CONFIG.PSU__CRL_APB__PL2_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__PL3_REF_CTRL__ACT_FREQMHZ {199.998001} \
   CONFIG.PSU__CRL_APB__PL3_REF_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRL_APB__PL3_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__PL3_REF_CTRL__FREQMHZ {200} \
   CONFIG.PSU__CRL_APB__PL3_REF_CTRL__SRCSEL {DPLL} \
   CONFIG.PSU__CRL_APB__QSPI_REF_CTRL__ACT_FREQMHZ {299.997009} \
   CONFIG.PSU__CRL_APB__QSPI_REF_CTRL__DIVISOR0 {5} \
   CONFIG.PSU__CRL_APB__QSPI_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__QSPI_REF_CTRL__FREQMHZ {300} \
   CONFIG.PSU__CRL_APB__QSPI_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__RPLL_CTRL__DIV2 {1} \
   CONFIG.PSU__CRL_APB__RPLL_CTRL__FBDIV {48} \
   CONFIG.PSU__CRL_APB__RPLL_CTRL__FRACDATA {0.000000} \
   CONFIG.PSU__CRL_APB__RPLL_CTRL__FRACFREQ {27.138} \
   CONFIG.PSU__CRL_APB__RPLL_CTRL__SRCSEL {PSS_REF_CLK} \
   CONFIG.PSU__CRL_APB__RPLL_FRAC_CFG__ENABLED {0} \
   CONFIG.PSU__CRL_APB__RPLL_TO_FPD_CTRL__DIVISOR0 {2} \
   CONFIG.PSU__CRL_APB__SDIO0_REF_CTRL__ACT_FREQMHZ {199.998001} \
   CONFIG.PSU__CRL_APB__SDIO0_REF_CTRL__DIVISOR0 {4} \
   CONFIG.PSU__CRL_APB__SDIO0_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__SDIO0_REF_CTRL__FREQMHZ {200} \
   CONFIG.PSU__CRL_APB__SDIO0_REF_CTRL__SRCSEL {RPLL} \
   CONFIG.PSU__CRL_APB__SDIO1_REF_CTRL__ACT_FREQMHZ {199.998001} \
   CONFIG.PSU__CRL_APB__SDIO1_REF_CTRL__DIVISOR0 {4} \
   CONFIG.PSU__CRL_APB__SDIO1_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__SDIO1_REF_CTRL__FREQMHZ {200} \
   CONFIG.PSU__CRL_APB__SDIO1_REF_CTRL__SRCSEL {RPLL} \
   CONFIG.PSU__CRL_APB__SPI0_REF_CTRL__ACT_FREQMHZ {199.998001} \
   CONFIG.PSU__CRL_APB__SPI0_REF_CTRL__DIVISOR0 {7} \
   CONFIG.PSU__CRL_APB__SPI0_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__SPI0_REF_CTRL__FREQMHZ {200} \
   CONFIG.PSU__CRL_APB__SPI0_REF_CTRL__SRCSEL {RPLL} \
   CONFIG.PSU__CRL_APB__SPI1_REF_CTRL__ACT_FREQMHZ {199.998001} \
   CONFIG.PSU__CRL_APB__SPI1_REF_CTRL__DIVISOR0 {4} \
   CONFIG.PSU__CRL_APB__SPI1_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__SPI1_REF_CTRL__FREQMHZ {200} \
   CONFIG.PSU__CRL_APB__SPI1_REF_CTRL__SRCSEL {RPLL} \
   CONFIG.PSU__CRL_APB__TIMESTAMP_REF_CTRL__ACT_FREQMHZ {33.333000} \
   CONFIG.PSU__CRL_APB__TIMESTAMP_REF_CTRL__DIVISOR0 {1} \
   CONFIG.PSU__CRL_APB__TIMESTAMP_REF_CTRL__FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__TIMESTAMP_REF_CTRL__SRCSEL {PSS_REF_CLK} \
   CONFIG.PSU__CRL_APB__UART0_REF_CTRL__ACT_FREQMHZ {99.999001} \
   CONFIG.PSU__CRL_APB__UART0_REF_CTRL__DIVISOR0 {15} \
   CONFIG.PSU__CRL_APB__UART0_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__UART0_REF_CTRL__FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__UART0_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__UART1_REF_CTRL__ACT_FREQMHZ {99.999001} \
   CONFIG.PSU__CRL_APB__UART1_REF_CTRL__DIVISOR0 {15} \
   CONFIG.PSU__CRL_APB__UART1_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__UART1_REF_CTRL__FREQMHZ {100} \
   CONFIG.PSU__CRL_APB__UART1_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__USB0_BUS_REF_CTRL__ACT_FREQMHZ {249.997498} \
   CONFIG.PSU__CRL_APB__USB0_BUS_REF_CTRL__DIVISOR0 {6} \
   CONFIG.PSU__CRL_APB__USB0_BUS_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__USB0_BUS_REF_CTRL__FREQMHZ {250} \
   CONFIG.PSU__CRL_APB__USB0_BUS_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__USB1_BUS_REF_CTRL__ACT_FREQMHZ {250} \
   CONFIG.PSU__CRL_APB__USB1_BUS_REF_CTRL__DIVISOR0 {6} \
   CONFIG.PSU__CRL_APB__USB1_BUS_REF_CTRL__DIVISOR1 {1} \
   CONFIG.PSU__CRL_APB__USB1_BUS_REF_CTRL__FREQMHZ {250} \
   CONFIG.PSU__CRL_APB__USB1_BUS_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__USB3_DUAL_REF_CTRL__ACT_FREQMHZ {19.999800} \
   CONFIG.PSU__CRL_APB__USB3_DUAL_REF_CTRL__DIVISOR0 {25} \
   CONFIG.PSU__CRL_APB__USB3_DUAL_REF_CTRL__DIVISOR1 {3} \
   CONFIG.PSU__CRL_APB__USB3_DUAL_REF_CTRL__FREQMHZ {20} \
   CONFIG.PSU__CRL_APB__USB3_DUAL_REF_CTRL__SRCSEL {IOPLL} \
   CONFIG.PSU__CRL_APB__USB3__ENABLE {0} \
   CONFIG.PSU__CSUPMU__PERIPHERAL__VALID {1} \
   CONFIG.PSU__CSU_COHERENCY {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_0__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_0__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_10__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_10__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_11__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_11__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_12__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_12__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_1__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_1__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_2__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_2__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_3__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_3__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_4__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_4__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_5__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_5__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_6__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_6__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_7__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_7__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_8__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_8__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_9__ENABLE {0} \
   CONFIG.PSU__CSU__CSU_TAMPER_9__ERASE_BBRAM {0} \
   CONFIG.PSU__CSU__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__DDRC__ADDR_MIRROR {0} \
   CONFIG.PSU__DDRC__AL {0} \
   CONFIG.PSU__DDRC__BANK_ADDR_COUNT {2} \
   CONFIG.PSU__DDRC__BG_ADDR_COUNT {1} \
   CONFIG.PSU__DDRC__BRC_MAPPING {ROW_BANK_COL} \
   CONFIG.PSU__DDRC__BUS_WIDTH {64 Bit} \
   CONFIG.PSU__DDRC__CL {16} \
   CONFIG.PSU__DDRC__CLOCK_STOP_EN {0} \
   CONFIG.PSU__DDRC__COL_ADDR_COUNT {10} \
   CONFIG.PSU__DDRC__COMPONENTS {Components} \
   CONFIG.PSU__DDRC__CWL {12} \
   CONFIG.PSU__DDRC__DDR3L_T_REF_RANGE {NA} \
   CONFIG.PSU__DDRC__DDR3_T_REF_RANGE {NA} \
   CONFIG.PSU__DDRC__DDR4_ADDR_MAPPING {0} \
   CONFIG.PSU__DDRC__DDR4_CAL_MODE_ENABLE {0} \
   CONFIG.PSU__DDRC__DDR4_CRC_CONTROL {0} \
   CONFIG.PSU__DDRC__DDR4_MAXPWR_SAVING_EN {0} \
   CONFIG.PSU__DDRC__DDR4_T_REF_MODE {1} \
   CONFIG.PSU__DDRC__DDR4_T_REF_RANGE {High (95 Max)} \
   CONFIG.PSU__DDRC__DEEP_PWR_DOWN_EN {0} \
   CONFIG.PSU__DDRC__DEVICE_CAPACITY {8192 MBits} \
   CONFIG.PSU__DDRC__DIMM_ADDR_MIRROR {0} \
   CONFIG.PSU__DDRC__DM_DBI {DM_NO_DBI} \
   CONFIG.PSU__DDRC__DQMAP_0_3 {0} \
   CONFIG.PSU__DDRC__DQMAP_12_15 {0} \
   CONFIG.PSU__DDRC__DQMAP_16_19 {0} \
   CONFIG.PSU__DDRC__DQMAP_20_23 {0} \
   CONFIG.PSU__DDRC__DQMAP_24_27 {0} \
   CONFIG.PSU__DDRC__DQMAP_28_31 {0} \
   CONFIG.PSU__DDRC__DQMAP_32_35 {0} \
   CONFIG.PSU__DDRC__DQMAP_36_39 {0} \
   CONFIG.PSU__DDRC__DQMAP_40_43 {0} \
   CONFIG.PSU__DDRC__DQMAP_44_47 {0} \
   CONFIG.PSU__DDRC__DQMAP_48_51 {0} \
   CONFIG.PSU__DDRC__DQMAP_4_7 {0} \
   CONFIG.PSU__DDRC__DQMAP_52_55 {0} \
   CONFIG.PSU__DDRC__DQMAP_56_59 {0} \
   CONFIG.PSU__DDRC__DQMAP_60_63 {0} \
   CONFIG.PSU__DDRC__DQMAP_64_67 {0} \
   CONFIG.PSU__DDRC__DQMAP_68_71 {0} \
   CONFIG.PSU__DDRC__DQMAP_8_11 {0} \
   CONFIG.PSU__DDRC__DRAM_WIDTH {16 Bits} \
   CONFIG.PSU__DDRC__ECC {Disabled} \
   CONFIG.PSU__DDRC__ECC_SCRUB {0} \
   CONFIG.PSU__DDRC__ENABLE {1} \
   CONFIG.PSU__DDRC__ENABLE_2T_TIMING {0} \
   CONFIG.PSU__DDRC__ENABLE_DP_SWITCH {0} \
   CONFIG.PSU__DDRC__ENABLE_LP4_HAS_ECC_COMP {0} \
   CONFIG.PSU__DDRC__ENABLE_LP4_SLOWBOOT {0} \
   CONFIG.PSU__DDRC__EN_2ND_CLK {0} \
   CONFIG.PSU__DDRC__FGRM {1X} \
   CONFIG.PSU__DDRC__FREQ_MHZ {1} \
   CONFIG.PSU__DDRC__LPDDR3_DUALRANK_SDP {0} \
   CONFIG.PSU__DDRC__LPDDR3_T_REF_RANGE {NA} \
   CONFIG.PSU__DDRC__LPDDR4_T_REF_RANGE {NA} \
   CONFIG.PSU__DDRC__LP_ASR {manual normal} \
   CONFIG.PSU__DDRC__MEMORY_TYPE {DDR 4} \
   CONFIG.PSU__DDRC__PARITY_ENABLE {0} \
   CONFIG.PSU__DDRC__PER_BANK_REFRESH {0} \
   CONFIG.PSU__DDRC__PHY_DBI_MODE {0} \
   CONFIG.PSU__DDRC__PLL_BYPASS {0} \
   CONFIG.PSU__DDRC__PWR_DOWN_EN {0} \
   CONFIG.PSU__DDRC__RANK_ADDR_COUNT {0} \
   CONFIG.PSU__DDRC__RD_DQS_CENTER {0} \
   CONFIG.PSU__DDRC__ROW_ADDR_COUNT {16} \
   CONFIG.PSU__DDRC__SB_TARGET {15-15-15} \
   CONFIG.PSU__DDRC__SELF_REF_ABORT {0} \
   CONFIG.PSU__DDRC__SPEED_BIN {DDR4_2400P} \
   CONFIG.PSU__DDRC__STATIC_RD_MODE {0} \
   CONFIG.PSU__DDRC__TRAIN_DATA_EYE {1} \
   CONFIG.PSU__DDRC__TRAIN_READ_GATE {1} \
   CONFIG.PSU__DDRC__TRAIN_WRITE_LEVEL {1} \
   CONFIG.PSU__DDRC__T_FAW {30.0} \
   CONFIG.PSU__DDRC__T_RAS_MIN {32} \
   CONFIG.PSU__DDRC__T_RC {45.32} \
   CONFIG.PSU__DDRC__T_RCD {16} \
   CONFIG.PSU__DDRC__T_RP {16} \
   CONFIG.PSU__DDRC__VENDOR_PART {OTHERS} \
   CONFIG.PSU__DDRC__VIDEO_BUFFER_SIZE {0} \
   CONFIG.PSU__DDRC__VREF {1} \
   CONFIG.PSU__DDR_HIGH_ADDRESS_GUI_ENABLE {1} \
   CONFIG.PSU__DDR_QOS_ENABLE {0} \
   CONFIG.PSU__DDR_QOS_FIX_HP0_RDQOS {} \
   CONFIG.PSU__DDR_QOS_FIX_HP0_WRQOS {} \
   CONFIG.PSU__DDR_QOS_FIX_HP1_RDQOS {} \
   CONFIG.PSU__DDR_QOS_FIX_HP1_WRQOS {} \
   CONFIG.PSU__DDR_QOS_FIX_HP2_RDQOS {} \
   CONFIG.PSU__DDR_QOS_FIX_HP2_WRQOS {} \
   CONFIG.PSU__DDR_QOS_FIX_HP3_RDQOS {} \
   CONFIG.PSU__DDR_QOS_FIX_HP3_WRQOS {} \
   CONFIG.PSU__DDR_QOS_HP0_RDQOS {} \
   CONFIG.PSU__DDR_QOS_HP0_WRQOS {} \
   CONFIG.PSU__DDR_QOS_HP1_RDQOS {} \
   CONFIG.PSU__DDR_QOS_HP1_WRQOS {} \
   CONFIG.PSU__DDR_QOS_HP2_RDQOS {} \
   CONFIG.PSU__DDR_QOS_HP2_WRQOS {} \
   CONFIG.PSU__DDR_QOS_HP3_RDQOS {} \
   CONFIG.PSU__DDR_QOS_HP3_WRQOS {} \
   CONFIG.PSU__DDR_QOS_RD_HPR_THRSHLD {} \
   CONFIG.PSU__DDR_QOS_RD_LPR_THRSHLD {} \
   CONFIG.PSU__DDR_QOS_WR_THRSHLD {} \
   CONFIG.PSU__DDR_SW_REFRESH_ENABLED {1} \
   CONFIG.PSU__DDR__INTERFACE__FREQMHZ {600.000} \
   CONFIG.PSU__DEVICE_TYPE {RFSOC} \
   CONFIG.PSU__DISPLAYPORT__LANE0__ENABLE {0} \
   CONFIG.PSU__DISPLAYPORT__LANE1__ENABLE {0} \
   CONFIG.PSU__DISPLAYPORT__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__DLL__ISUSED {1} \
   CONFIG.PSU__DPAUX__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__ENABLE__DDR__REFRESH__SIGNALS {0} \
   CONFIG.PSU__ENET0__FIFO__ENABLE {0} \
   CONFIG.PSU__ENET0__GRP_MDIO__ENABLE {0} \
   CONFIG.PSU__ENET0__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__ENET0__PTP__ENABLE {0} \
   CONFIG.PSU__ENET0__TSU__ENABLE {0} \
   CONFIG.PSU__ENET1__FIFO__ENABLE {0} \
   CONFIG.PSU__ENET1__GRP_MDIO__ENABLE {0} \
   CONFIG.PSU__ENET1__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__ENET1__PTP__ENABLE {0} \
   CONFIG.PSU__ENET1__TSU__ENABLE {0} \
   CONFIG.PSU__ENET2__FIFO__ENABLE {0} \
   CONFIG.PSU__ENET2__GRP_MDIO__ENABLE {0} \
   CONFIG.PSU__ENET2__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__ENET2__PTP__ENABLE {0} \
   CONFIG.PSU__ENET2__TSU__ENABLE {0} \
   CONFIG.PSU__ENET3__FIFO__ENABLE {0} \
   CONFIG.PSU__ENET3__GRP_MDIO__ENABLE {1} \
   CONFIG.PSU__ENET3__GRP_MDIO__IO {MIO 76 .. 77} \
   CONFIG.PSU__ENET3__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__ENET3__PERIPHERAL__IO {MIO 64 .. 75} \
   CONFIG.PSU__ENET3__PTP__ENABLE {0} \
   CONFIG.PSU__ENET3__TSU__ENABLE {0} \
   CONFIG.PSU__EN_AXI_STATUS_PORTS {0} \
   CONFIG.PSU__EN_EMIO_TRACE {0} \
   CONFIG.PSU__EP__IP {0} \
   CONFIG.PSU__EXPAND__CORESIGHT {0} \
   CONFIG.PSU__EXPAND__FPD_SLAVES {0} \
   CONFIG.PSU__EXPAND__GIC {0} \
   CONFIG.PSU__EXPAND__LOWER_LPS_SLAVES {0} \
   CONFIG.PSU__EXPAND__UPPER_LPS_SLAVES {0} \
   CONFIG.PSU__FPDMASTERS_COHERENCY {0} \
   CONFIG.PSU__FPD_SLCR__WDT1__ACT_FREQMHZ {99.999001} \
   CONFIG.PSU__FPD_SLCR__WDT1__FREQMHZ {99.999001} \
   CONFIG.PSU__FPD_SLCR__WDT_CLK_SEL__SELECT {APB} \
   CONFIG.PSU__FPGA_PL0_ENABLE {1} \
   CONFIG.PSU__FPGA_PL1_ENABLE {1} \
   CONFIG.PSU__FPGA_PL2_ENABLE {1} \
   CONFIG.PSU__FPGA_PL3_ENABLE {1} \
   CONFIG.PSU__FP__POWER__ON {1} \
   CONFIG.PSU__FTM__CTI_IN_0 {0} \
   CONFIG.PSU__FTM__CTI_IN_1 {0} \
   CONFIG.PSU__FTM__CTI_IN_2 {0} \
   CONFIG.PSU__FTM__CTI_IN_3 {0} \
   CONFIG.PSU__FTM__CTI_OUT_0 {0} \
   CONFIG.PSU__FTM__CTI_OUT_1 {0} \
   CONFIG.PSU__FTM__CTI_OUT_2 {0} \
   CONFIG.PSU__FTM__CTI_OUT_3 {0} \
   CONFIG.PSU__FTM__GPI {0} \
   CONFIG.PSU__FTM__GPO {0} \
   CONFIG.PSU__GEM0_COHERENCY {0} \
   CONFIG.PSU__GEM0_ROUTE_THROUGH_FPD {0} \
   CONFIG.PSU__GEM1_COHERENCY {0} \
   CONFIG.PSU__GEM1_ROUTE_THROUGH_FPD {0} \
   CONFIG.PSU__GEM2_COHERENCY {0} \
   CONFIG.PSU__GEM2_ROUTE_THROUGH_FPD {0} \
   CONFIG.PSU__GEM3_COHERENCY {0} \
   CONFIG.PSU__GEM3_ROUTE_THROUGH_FPD {0} \
   CONFIG.PSU__GEM__TSU__ENABLE {0} \
   CONFIG.PSU__GEN_IPI_0__MASTER {APU} \
   CONFIG.PSU__GEN_IPI_10__MASTER {NONE} \
   CONFIG.PSU__GEN_IPI_1__MASTER {RPU0} \
   CONFIG.PSU__GEN_IPI_2__MASTER {RPU1} \
   CONFIG.PSU__GEN_IPI_3__MASTER {PMU} \
   CONFIG.PSU__GEN_IPI_4__MASTER {PMU} \
   CONFIG.PSU__GEN_IPI_5__MASTER {PMU} \
   CONFIG.PSU__GEN_IPI_6__MASTER {PMU} \
   CONFIG.PSU__GEN_IPI_7__MASTER {NONE} \
   CONFIG.PSU__GEN_IPI_8__MASTER {NONE} \
   CONFIG.PSU__GEN_IPI_9__MASTER {NONE} \
   CONFIG.PSU__GPIO0_MIO__IO {MIO 0 .. 25} \
   CONFIG.PSU__GPIO0_MIO__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__GPIO1_MIO__IO {MIO 26 .. 51} \
   CONFIG.PSU__GPIO1_MIO__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__GPIO2_MIO__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__GPIO_EMIO_WIDTH {64} \
   CONFIG.PSU__GPIO_EMIO__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__GPIO_EMIO__PERIPHERAL__IO {64} \
   CONFIG.PSU__GPIO_EMIO__WIDTH {[91:0]} \
   CONFIG.PSU__GPU_PP0__POWER__ON {0} \
   CONFIG.PSU__GPU_PP1__POWER__ON {0} \
   CONFIG.PSU__GT_REF_CLK__FREQMHZ {33.333} \
   CONFIG.PSU__GT__PRE_EMPH_LVL_4 {} \
   CONFIG.PSU__GT__VLT_SWNG_LVL_4 {} \
   CONFIG.PSU__HIGH_ADDRESS__ENABLE {1} \
   CONFIG.PSU__HPM0_FPD__NUM_READ_THREADS {4} \
   CONFIG.PSU__HPM0_FPD__NUM_WRITE_THREADS {4} \
   CONFIG.PSU__HPM0_LPD__NUM_READ_THREADS {4} \
   CONFIG.PSU__HPM0_LPD__NUM_WRITE_THREADS {4} \
   CONFIG.PSU__HPM1_FPD__NUM_READ_THREADS {4} \
   CONFIG.PSU__HPM1_FPD__NUM_WRITE_THREADS {4} \
   CONFIG.PSU__I2C0_LOOP_I2C1__ENABLE {0} \
   CONFIG.PSU__I2C0__GRP_INT__ENABLE {0} \
   CONFIG.PSU__I2C0__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__I2C0__PERIPHERAL__IO {MIO 2 .. 3} \
   CONFIG.PSU__I2C1__GRP_INT__ENABLE {0} \
   CONFIG.PSU__I2C1__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__I2C1__PERIPHERAL__IO {MIO 36 .. 37} \
   CONFIG.PSU__IOU_SLCR__IOU_TTC_APB_CLK__TTC0_SEL {APB} \
   CONFIG.PSU__IOU_SLCR__IOU_TTC_APB_CLK__TTC1_SEL {APB} \
   CONFIG.PSU__IOU_SLCR__IOU_TTC_APB_CLK__TTC2_SEL {APB} \
   CONFIG.PSU__IOU_SLCR__IOU_TTC_APB_CLK__TTC3_SEL {APB} \
   CONFIG.PSU__IOU_SLCR__TTC0__ACT_FREQMHZ {100} \
   CONFIG.PSU__IOU_SLCR__TTC0__FREQMHZ {100} \
   CONFIG.PSU__IOU_SLCR__TTC1__ACT_FREQMHZ {100} \
   CONFIG.PSU__IOU_SLCR__TTC1__FREQMHZ {100} \
   CONFIG.PSU__IOU_SLCR__TTC2__ACT_FREQMHZ {100} \
   CONFIG.PSU__IOU_SLCR__TTC2__FREQMHZ {100} \
   CONFIG.PSU__IOU_SLCR__TTC3__ACT_FREQMHZ {100} \
   CONFIG.PSU__IOU_SLCR__TTC3__FREQMHZ {100} \
   CONFIG.PSU__IOU_SLCR__WDT0__ACT_FREQMHZ {99.999001} \
   CONFIG.PSU__IOU_SLCR__WDT0__FREQMHZ {99.999001} \
   CONFIG.PSU__IOU_SLCR__WDT_CLK_SEL__SELECT {APB} \
   CONFIG.PSU__IRQ_P2F_ADMA_CHAN__INT {0} \
   CONFIG.PSU__IRQ_P2F_AIB_AXI__INT {0} \
   CONFIG.PSU__IRQ_P2F_AMS__INT {0} \
   CONFIG.PSU__IRQ_P2F_APM_FPD__INT {0} \
   CONFIG.PSU__IRQ_P2F_APU_COMM__INT {0} \
   CONFIG.PSU__IRQ_P2F_APU_CPUMNT__INT {0} \
   CONFIG.PSU__IRQ_P2F_APU_CTI__INT {0} \
   CONFIG.PSU__IRQ_P2F_APU_EXTERR__INT {0} \
   CONFIG.PSU__IRQ_P2F_APU_IPI__INT {0} \
   CONFIG.PSU__IRQ_P2F_APU_L2ERR__INT {0} \
   CONFIG.PSU__IRQ_P2F_APU_PMU__INT {0} \
   CONFIG.PSU__IRQ_P2F_APU_REGS__INT {0} \
   CONFIG.PSU__IRQ_P2F_ATB_LPD__INT {0} \
   CONFIG.PSU__IRQ_P2F_CAN0__INT {0} \
   CONFIG.PSU__IRQ_P2F_CAN1__INT {0} \
   CONFIG.PSU__IRQ_P2F_CLKMON__INT {0} \
   CONFIG.PSU__IRQ_P2F_CSUPMU_WDT__INT {0} \
   CONFIG.PSU__IRQ_P2F_CSU_DMA__INT {0} \
   CONFIG.PSU__IRQ_P2F_CSU__INT {0} \
   CONFIG.PSU__IRQ_P2F_DDR_SS__INT {0} \
   CONFIG.PSU__IRQ_P2F_DPDMA__INT {0} \
   CONFIG.PSU__IRQ_P2F_DPORT__INT {0} \
   CONFIG.PSU__IRQ_P2F_EFUSE__INT {0} \
   CONFIG.PSU__IRQ_P2F_ENT0_WAKEUP__INT {0} \
   CONFIG.PSU__IRQ_P2F_ENT0__INT {0} \
   CONFIG.PSU__IRQ_P2F_ENT1_WAKEUP__INT {0} \
   CONFIG.PSU__IRQ_P2F_ENT1__INT {0} \
   CONFIG.PSU__IRQ_P2F_ENT2_WAKEUP__INT {0} \
   CONFIG.PSU__IRQ_P2F_ENT2__INT {0} \
   CONFIG.PSU__IRQ_P2F_ENT3_WAKEUP__INT {0} \
   CONFIG.PSU__IRQ_P2F_ENT3__INT {0} \
   CONFIG.PSU__IRQ_P2F_FPD_APB__INT {0} \
   CONFIG.PSU__IRQ_P2F_FPD_ATB_ERR__INT {0} \
   CONFIG.PSU__IRQ_P2F_FP_WDT__INT {0} \
   CONFIG.PSU__IRQ_P2F_GDMA_CHAN__INT {0} \
   CONFIG.PSU__IRQ_P2F_GPIO__INT {0} \
   CONFIG.PSU__IRQ_P2F_GPU__INT {0} \
   CONFIG.PSU__IRQ_P2F_I2C0__INT {0} \
   CONFIG.PSU__IRQ_P2F_I2C1__INT {0} \
   CONFIG.PSU__IRQ_P2F_LPD_APB__INT {0} \
   CONFIG.PSU__IRQ_P2F_LPD_APM__INT {0} \
   CONFIG.PSU__IRQ_P2F_LP_WDT__INT {0} \
   CONFIG.PSU__IRQ_P2F_NAND__INT {0} \
   CONFIG.PSU__IRQ_P2F_OCM_ERR__INT {0} \
   CONFIG.PSU__IRQ_P2F_PCIE_DMA__INT {0} \
   CONFIG.PSU__IRQ_P2F_PCIE_LEGACY__INT {0} \
   CONFIG.PSU__IRQ_P2F_PCIE_MSC__INT {0} \
   CONFIG.PSU__IRQ_P2F_PCIE_MSI__INT {0} \
   CONFIG.PSU__IRQ_P2F_PL_IPI__INT {0} \
   CONFIG.PSU__IRQ_P2F_QSPI__INT {0} \
   CONFIG.PSU__IRQ_P2F_R5_CORE0_ECC_ERR__INT {0} \
   CONFIG.PSU__IRQ_P2F_R5_CORE1_ECC_ERR__INT {0} \
   CONFIG.PSU__IRQ_P2F_RPU_IPI__INT {0} \
   CONFIG.PSU__IRQ_P2F_RPU_PERMON__INT {0} \
   CONFIG.PSU__IRQ_P2F_RTC_ALARM__INT {0} \
   CONFIG.PSU__IRQ_P2F_RTC_SECONDS__INT {0} \
   CONFIG.PSU__IRQ_P2F_SATA__INT {0} \
   CONFIG.PSU__IRQ_P2F_SDIO0_WAKE__INT {0} \
   CONFIG.PSU__IRQ_P2F_SDIO0__INT {0} \
   CONFIG.PSU__IRQ_P2F_SDIO1_WAKE__INT {0} \
   CONFIG.PSU__IRQ_P2F_SDIO1__INT {0} \
   CONFIG.PSU__IRQ_P2F_SPI0__INT {0} \
   CONFIG.PSU__IRQ_P2F_SPI1__INT {0} \
   CONFIG.PSU__IRQ_P2F_TTC0__INT0 {0} \
   CONFIG.PSU__IRQ_P2F_TTC0__INT1 {0} \
   CONFIG.PSU__IRQ_P2F_TTC0__INT2 {0} \
   CONFIG.PSU__IRQ_P2F_TTC1__INT0 {0} \
   CONFIG.PSU__IRQ_P2F_TTC1__INT1 {0} \
   CONFIG.PSU__IRQ_P2F_TTC1__INT2 {0} \
   CONFIG.PSU__IRQ_P2F_TTC2__INT0 {0} \
   CONFIG.PSU__IRQ_P2F_TTC2__INT1 {0} \
   CONFIG.PSU__IRQ_P2F_TTC2__INT2 {0} \
   CONFIG.PSU__IRQ_P2F_TTC3__INT0 {0} \
   CONFIG.PSU__IRQ_P2F_TTC3__INT1 {0} \
   CONFIG.PSU__IRQ_P2F_TTC3__INT2 {0} \
   CONFIG.PSU__IRQ_P2F_UART0__INT {0} \
   CONFIG.PSU__IRQ_P2F_UART1__INT {0} \
   CONFIG.PSU__IRQ_P2F_USB3_ENDPOINT__INT0 {0} \
   CONFIG.PSU__IRQ_P2F_USB3_ENDPOINT__INT1 {0} \
   CONFIG.PSU__IRQ_P2F_USB3_OTG__INT0 {0} \
   CONFIG.PSU__IRQ_P2F_USB3_OTG__INT1 {0} \
   CONFIG.PSU__IRQ_P2F_USB3_PMU_WAKEUP__INT {0} \
   CONFIG.PSU__IRQ_P2F_XMPU_FPD__INT {0} \
   CONFIG.PSU__IRQ_P2F_XMPU_LPD__INT {0} \
   CONFIG.PSU__IRQ_P2F__INTF_FPD_SMMU__INT {0} \
   CONFIG.PSU__IRQ_P2F__INTF_PPD_CCI__INT {0} \
   CONFIG.PSU__L2_BANK0__POWER__ON {1} \
   CONFIG.PSU__LPDMA0_COHERENCY {0} \
   CONFIG.PSU__LPDMA1_COHERENCY {0} \
   CONFIG.PSU__LPDMA2_COHERENCY {0} \
   CONFIG.PSU__LPDMA3_COHERENCY {0} \
   CONFIG.PSU__LPDMA4_COHERENCY {0} \
   CONFIG.PSU__LPDMA5_COHERENCY {0} \
   CONFIG.PSU__LPDMA6_COHERENCY {0} \
   CONFIG.PSU__LPDMA7_COHERENCY {0} \
   CONFIG.PSU__LPD_SLCR__CSUPMU_WDT_CLK_SEL__SELECT {APB} \
   CONFIG.PSU__LPD_SLCR__CSUPMU__ACT_FREQMHZ {100.000000} \
   CONFIG.PSU__LPD_SLCR__CSUPMU__FREQMHZ {100.000000} \
   CONFIG.PSU__MAXIGP0__DATA_WIDTH {32} \
   CONFIG.PSU__MAXIGP1__DATA_WIDTH {128} \
   CONFIG.PSU__MAXIGP2__DATA_WIDTH {32} \
   CONFIG.PSU__M_AXI_GP0_SUPPORTS_NARROW_BURST {1} \
   CONFIG.PSU__M_AXI_GP1_SUPPORTS_NARROW_BURST {1} \
   CONFIG.PSU__M_AXI_GP2_SUPPORTS_NARROW_BURST {1} \
   CONFIG.PSU__NAND_COHERENCY {0} \
   CONFIG.PSU__NAND_ROUTE_THROUGH_FPD {0} \
   CONFIG.PSU__NAND__CHIP_ENABLE__ENABLE {0} \
   CONFIG.PSU__NAND__DATA_STROBE__ENABLE {0} \
   CONFIG.PSU__NAND__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__NAND__READY0_BUSY__ENABLE {0} \
   CONFIG.PSU__NAND__READY1_BUSY__ENABLE {0} \
   CONFIG.PSU__NAND__READY_BUSY__ENABLE {0} \
   CONFIG.PSU__NUM_FABRIC_RESETS {4} \
   CONFIG.PSU__OCM_BANK0__POWER__ON {1} \
   CONFIG.PSU__OCM_BANK1__POWER__ON {1} \
   CONFIG.PSU__OCM_BANK2__POWER__ON {1} \
   CONFIG.PSU__OCM_BANK3__POWER__ON {1} \
   CONFIG.PSU__OVERRIDE_HPX_QOS {0} \
   CONFIG.PSU__OVERRIDE__BASIC_CLOCK {0} \
   CONFIG.PSU__PCIE__ACS_VIOLAION {0} \
   CONFIG.PSU__PCIE__ACS_VIOLATION {0} \
   CONFIG.PSU__PCIE__AER_CAPABILITY {0} \
   CONFIG.PSU__PCIE__ATOMICOP_EGRESS_BLOCKED {0} \
   CONFIG.PSU__PCIE__BAR0_64BIT {0} \
   CONFIG.PSU__PCIE__BAR0_ENABLE {0} \
   CONFIG.PSU__PCIE__BAR0_PREFETCHABLE {0} \
   CONFIG.PSU__PCIE__BAR0_VAL {} \
   CONFIG.PSU__PCIE__BAR1_64BIT {0} \
   CONFIG.PSU__PCIE__BAR1_ENABLE {0} \
   CONFIG.PSU__PCIE__BAR1_PREFETCHABLE {0} \
   CONFIG.PSU__PCIE__BAR1_VAL {} \
   CONFIG.PSU__PCIE__BAR2_64BIT {0} \
   CONFIG.PSU__PCIE__BAR2_ENABLE {0} \
   CONFIG.PSU__PCIE__BAR2_PREFETCHABLE {0} \
   CONFIG.PSU__PCIE__BAR2_VAL {} \
   CONFIG.PSU__PCIE__BAR3_64BIT {0} \
   CONFIG.PSU__PCIE__BAR3_ENABLE {0} \
   CONFIG.PSU__PCIE__BAR3_PREFETCHABLE {0} \
   CONFIG.PSU__PCIE__BAR3_VAL {} \
   CONFIG.PSU__PCIE__BAR4_64BIT {0} \
   CONFIG.PSU__PCIE__BAR4_ENABLE {0} \
   CONFIG.PSU__PCIE__BAR4_PREFETCHABLE {0} \
   CONFIG.PSU__PCIE__BAR4_VAL {} \
   CONFIG.PSU__PCIE__BAR5_64BIT {0} \
   CONFIG.PSU__PCIE__BAR5_ENABLE {0} \
   CONFIG.PSU__PCIE__BAR5_PREFETCHABLE {0} \
   CONFIG.PSU__PCIE__BAR5_VAL {} \
   CONFIG.PSU__PCIE__CLASS_CODE_BASE {} \
   CONFIG.PSU__PCIE__CLASS_CODE_INTERFACE {} \
   CONFIG.PSU__PCIE__CLASS_CODE_SUB {} \
   CONFIG.PSU__PCIE__CLASS_CODE_VALUE {} \
   CONFIG.PSU__PCIE__COMPLETER_ABORT {0} \
   CONFIG.PSU__PCIE__COMPLTION_TIMEOUT {0} \
   CONFIG.PSU__PCIE__CORRECTABLE_INT_ERR {0} \
   CONFIG.PSU__PCIE__CRS_SW_VISIBILITY {0} \
   CONFIG.PSU__PCIE__DEVICE_ID {} \
   CONFIG.PSU__PCIE__ECRC_CHECK {0} \
   CONFIG.PSU__PCIE__ECRC_ERR {0} \
   CONFIG.PSU__PCIE__ECRC_GEN {0} \
   CONFIG.PSU__PCIE__EROM_ENABLE {0} \
   CONFIG.PSU__PCIE__EROM_VAL {} \
   CONFIG.PSU__PCIE__FLOW_CONTROL_ERR {0} \
   CONFIG.PSU__PCIE__FLOW_CONTROL_PROTOCOL_ERR {0} \
   CONFIG.PSU__PCIE__HEADER_LOG_OVERFLOW {0} \
   CONFIG.PSU__PCIE__INTX_GENERATION {0} \
   CONFIG.PSU__PCIE__LANE0__ENABLE {0} \
   CONFIG.PSU__PCIE__LANE1__ENABLE {0} \
   CONFIG.PSU__PCIE__LANE2__ENABLE {0} \
   CONFIG.PSU__PCIE__LANE3__ENABLE {0} \
   CONFIG.PSU__PCIE__MC_BLOCKED_TLP {0} \
   CONFIG.PSU__PCIE__MSIX_BAR_INDICATOR {} \
   CONFIG.PSU__PCIE__MSIX_CAPABILITY {0} \
   CONFIG.PSU__PCIE__MSIX_PBA_BAR_INDICATOR {} \
   CONFIG.PSU__PCIE__MSIX_PBA_OFFSET {0} \
   CONFIG.PSU__PCIE__MSIX_TABLE_OFFSET {0} \
   CONFIG.PSU__PCIE__MSIX_TABLE_SIZE {0} \
   CONFIG.PSU__PCIE__MSI_64BIT_ADDR_CAPABLE {0} \
   CONFIG.PSU__PCIE__MSI_CAPABILITY {0} \
   CONFIG.PSU__PCIE__MULTIHEADER {0} \
   CONFIG.PSU__PCIE__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__PCIE__PERIPHERAL__ENDPOINT_ENABLE {1} \
   CONFIG.PSU__PCIE__PERIPHERAL__ROOTPORT_ENABLE {0} \
   CONFIG.PSU__PCIE__PERM_ROOT_ERR_UPDATE {0} \
   CONFIG.PSU__PCIE__RECEIVER_ERR {0} \
   CONFIG.PSU__PCIE__RECEIVER_OVERFLOW {0} \
   CONFIG.PSU__PCIE__RESET__POLARITY {Active Low} \
   CONFIG.PSU__PCIE__REVISION_ID {} \
   CONFIG.PSU__PCIE__SUBSYSTEM_ID {} \
   CONFIG.PSU__PCIE__SUBSYSTEM_VENDOR_ID {} \
   CONFIG.PSU__PCIE__SURPRISE_DOWN {0} \
   CONFIG.PSU__PCIE__TLP_PREFIX_BLOCKED {0} \
   CONFIG.PSU__PCIE__UNCORRECTABL_INT_ERR {0} \
   CONFIG.PSU__PCIE__VENDOR_ID {} \
   CONFIG.PSU__PJTAG__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__PL_CLK0_BUF {TRUE} \
   CONFIG.PSU__PL_CLK1_BUF {TRUE} \
   CONFIG.PSU__PL_CLK2_BUF {TRUE} \
   CONFIG.PSU__PL_CLK3_BUF {TRUE} \
   CONFIG.PSU__PL__POWER__ON {1} \
   CONFIG.PSU__PMU_COHERENCY {0} \
   CONFIG.PSU__PMU__AIBACK__ENABLE {0} \
   CONFIG.PSU__PMU__EMIO_GPI__ENABLE {0} \
   CONFIG.PSU__PMU__EMIO_GPO__ENABLE {0} \
   CONFIG.PSU__PMU__GPI0__ENABLE {0} \
   CONFIG.PSU__PMU__GPI1__ENABLE {0} \
   CONFIG.PSU__PMU__GPI2__ENABLE {0} \
   CONFIG.PSU__PMU__GPI3__ENABLE {0} \
   CONFIG.PSU__PMU__GPI4__ENABLE {0} \
   CONFIG.PSU__PMU__GPI5__ENABLE {0} \
   CONFIG.PSU__PMU__GPO0__ENABLE {0} \
   CONFIG.PSU__PMU__GPO1__ENABLE {0} \
   CONFIG.PSU__PMU__GPO2__ENABLE {1} \
   CONFIG.PSU__PMU__GPO2__IO {MIO 34} \
   CONFIG.PSU__PMU__GPO2__POLARITY {high} \
   CONFIG.PSU__PMU__GPO3__ENABLE {1} \
   CONFIG.PSU__PMU__GPO3__IO {MIO 35} \
   CONFIG.PSU__PMU__GPO3__POLARITY {high} \
   CONFIG.PSU__PMU__GPO4__ENABLE {0} \
   CONFIG.PSU__PMU__GPO5__ENABLE {0} \
   CONFIG.PSU__PMU__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__PMU__PLERROR__ENABLE {0} \
   CONFIG.PSU__PRESET_APPLIED {0} \
   CONFIG.PSU__PROTECTION__DDR_SEGMENTS {NONE} \
   CONFIG.PSU__PROTECTION__DEBUG {0} \
   CONFIG.PSU__PROTECTION__ENABLE {0} \
   CONFIG.PSU__PROTECTION__FPD_SEGMENTS {SA:0xFD1A0000 ;SIZE:1280;UNIT:KB ;RegionTZ:Secure\
;WrAllowed:Read/Write;subsystemId:PMU Firmware   |   SA:0xFD000000\
;SIZE:64;UNIT:KB ;RegionTZ:Secure ;WrAllowed:Read/Write;subsystemId:PMU\
Firmware   |   SA:0xFD010000 ;SIZE:64;UNIT:KB ;RegionTZ:Secure\
;WrAllowed:Read/Write;subsystemId:PMU Firmware   |   SA:0xFD020000\
;SIZE:64;UNIT:KB ;RegionTZ:Secure ;WrAllowed:Read/Write;subsystemId:PMU\
Firmware   |   SA:0xFD030000 ;SIZE:64;UNIT:KB ;RegionTZ:Secure\
;WrAllowed:Read/Write;subsystemId:PMU Firmware   |   SA:0xFD040000\
;SIZE:64;UNIT:KB ;RegionTZ:Secure ;WrAllowed:Read/Write;subsystemId:PMU\
Firmware   |   SA:0xFD050000 ;SIZE:64;UNIT:KB ;RegionTZ:Secure\
;WrAllowed:Read/Write;subsystemId:PMU Firmware   |   SA:0xFD610000\
;SIZE:512;UNIT:KB ;RegionTZ:Secure ;WrAllowed:Read/Write;subsystemId:PMU\
Firmware   |   SA:0xFD5D0000 ;SIZE:64;UNIT:KB ;RegionTZ:Secure\
;WrAllowed:Read/Write;subsystemId:PMU Firmware}\
   CONFIG.PSU__PROTECTION__LOCK_UNUSED_SEGMENTS {0} \
   CONFIG.PSU__PROTECTION__LPD_SEGMENTS {SA:0xFF980000 ;SIZE:64;UNIT:KB ;RegionTZ:Secure\
;WrAllowed:Read/Write;subsystemId:PMU Firmware|SA:0xFF5E0000 ;SIZE:2560;UNIT:KB\
;RegionTZ:Secure ;WrAllowed:Read/Write;subsystemId:PMU Firmware|SA:0xFFCC0000\
;SIZE:64;UNIT:KB ;RegionTZ:Secure ;WrAllowed:Read/Write;subsystemId:PMU\
Firmware|SA:0xFF180000 ;SIZE:768;UNIT:KB ;RegionTZ:Secure\
;WrAllowed:Read/Write;subsystemId:PMU Firmware|SA:0xFF410000 ;SIZE:640;UNIT:KB\
;RegionTZ:Secure ;WrAllowed:Read/Write;subsystemId:PMU Firmware|SA:0xFFA70000\
;SIZE:64;UNIT:KB ;RegionTZ:Secure ;WrAllowed:Read/Write;subsystemId:PMU\
Firmware|SA:0xFF9A0000 ;SIZE:64;UNIT:KB ;RegionTZ:Secure\
;WrAllowed:Read/Write;subsystemId:PMU Firmware}\
   CONFIG.PSU__PROTECTION__MASTERS {USB1:NonSecure;0|USB0:NonSecure;1|S_AXI_LPD:NA;0|S_AXI_HPC1_FPD:NA;1|S_AXI_HPC0_FPD:NA;1|S_AXI_HP3_FPD:NA;0|S_AXI_HP2_FPD:NA;0|S_AXI_HP1_FPD:NA;1|S_AXI_HP0_FPD:NA;1|S_AXI_ACP:NA;0|S_AXI_ACE:NA;0|SD1:NonSecure;1|SD0:NonSecure;1|SATA1:NonSecure;0|SATA0:NonSecure;0|RPU1:Secure;1|RPU0:Secure;1|QSPI:NonSecure;0|PMU:NA;1|PCIe:NonSecure;0|NAND:NonSecure;0|LDMA:NonSecure;1|GPU:NonSecure;1|GEM3:NonSecure;1|GEM2:NonSecure;0|GEM1:NonSecure;0|GEM0:NonSecure;0|FDMA:NonSecure;1|DP:NonSecure;0|DAP:NA;1|Coresight:NA;1|CSU:NA;1|APU:NA;1}\
   CONFIG.PSU__PROTECTION__MASTERS_TZ {GEM0:NonSecure|SD1:NonSecure|GEM2:NonSecure|GEM1:NonSecure|GEM3:NonSecure|PCIe:NonSecure|DP:NonSecure|NAND:NonSecure|GPU:NonSecure|USB1:NonSecure|USB0:NonSecure|LDMA:NonSecure|FDMA:NonSecure|QSPI:NonSecure|SD0:NonSecure}\
   CONFIG.PSU__PROTECTION__OCM_SEGMENTS {NONE} \
   CONFIG.PSU__PROTECTION__PRESUBSYSTEMS {NONE} \
   CONFIG.PSU__PROTECTION__SLAVES {LPD;USB3_1_XHCI;FE300000;FE3FFFFF;0|LPD;USB3_1;FF9E0000;FF9EFFFF;0|LPD;USB3_0_XHCI;FE200000;FE2FFFFF;1|LPD;USB3_0;FF9D0000;FF9DFFFF;1|LPD;UART1;FF010000;FF01FFFF;1|LPD;UART0;FF000000;FF00FFFF;1|LPD;TTC3;FF140000;FF14FFFF;0|LPD;TTC2;FF130000;FF13FFFF;0|LPD;TTC1;FF120000;FF12FFFF;0|LPD;TTC0;FF110000;FF11FFFF;0|FPD;SWDT1;FD4D0000;FD4DFFFF;1|LPD;SWDT0;FF150000;FF15FFFF;0|LPD;SPI1;FF050000;FF05FFFF;1|LPD;SPI0;FF040000;FF04FFFF;0|FPD;SMMU_REG;FD5F0000;FD5FFFFF;1|FPD;SMMU;FD800000;FDFFFFFF;1|FPD;SIOU;FD3D0000;FD3DFFFF;1|FPD;SERDES;FD400000;FD47FFFF;1|LPD;SD1;FF170000;FF17FFFF;1|LPD;SD0;FF160000;FF16FFFF;1|FPD;SATA;FD0C0000;FD0CFFFF;0|LPD;RTC;FFA60000;FFA6FFFF;1|LPD;RSA_CORE;FFCE0000;FFCEFFFF;1|LPD;RPU;FF9A0000;FF9AFFFF;1|LPD;R5_TCM_RAM_GLOBAL;FFE00000;FFE3FFFF;1|LPD;R5_1_Instruction_Cache;FFEC0000;FFECFFFF;1|LPD;R5_1_Data_Cache;FFED0000;FFEDFFFF;1|LPD;R5_1_BTCM_GLOBAL;FFEB0000;FFEBFFFF;1|LPD;R5_1_ATCM_GLOBAL;FFE90000;FFE9FFFF;1|LPD;R5_0_Instruction_Cache;FFE40000;FFE4FFFF;1|LPD;R5_0_Data_Cache;FFE50000;FFE5FFFF;1|LPD;R5_0_BTCM_GLOBAL;FFE20000;FFE2FFFF;1|LPD;R5_0_ATCM_GLOBAL;FFE00000;FFE0FFFF;1|LPD;QSPI_Linear_Address;C0000000;DFFFFFFF;1|LPD;QSPI;FF0F0000;FF0FFFFF;0|LPD;PMU_RAM;FFDC0000;FFDDFFFF;1|LPD;PMU_GLOBAL;FFD80000;FFDBFFFF;1|FPD;PCIE_MAIN;FD0E0000;FD0EFFFF;0|FPD;PCIE_LOW;E0000000;EFFFFFFF;0|FPD;PCIE_HIGH2;8000000000;BFFFFFFFFF;0|FPD;PCIE_HIGH1;600000000;7FFFFFFFF;0|FPD;PCIE_DMA;FD0F0000;FD0FFFFF;0|FPD;PCIE_ATTRIB;FD480000;FD48FFFF;0|LPD;OCM_XMPU_CFG;FFA70000;FFA7FFFF;1|LPD;OCM_SLCR;FF960000;FF96FFFF;1|OCM;OCM;FFFC0000;FFFFFFFF;1|LPD;NAND;FF100000;FF10FFFF;0|LPD;MBISTJTAG;FFCF0000;FFCFFFFF;1|LPD;LPD_XPPU_SINK;FF9C0000;FF9CFFFF;1|LPD;LPD_XPPU;FF980000;FF98FFFF;1|LPD;LPD_SLCR_SECURE;FF4B0000;FF4DFFFF;1|LPD;LPD_SLCR;FF410000;FF4AFFFF;1|LPD;LPD_GPV;FE100000;FE1FFFFF;1|LPD;LPD_DMA_7;FFAF0000;FFAFFFFF;1|LPD;LPD_DMA_6;FFAE0000;FFAEFFFF;1|LPD;LPD_DMA_5;FFAD0000;FFADFFFF;1|LPD;LPD_DMA_4;FFAC0000;FFACFFFF;1|LPD;LPD_DMA_3;FFAB0000;FFABFFFF;1|LPD;LPD_DMA_2;FFAA0000;FFAAFFFF;1|LPD;LPD_DMA_1;FFA90000;FFA9FFFF;1|LPD;LPD_DMA_0;FFA80000;FFA8FFFF;1|LPD;IPI_CTRL;FF380000;FF3FFFFF;1|LPD;IOU_SLCR;FF180000;FF23FFFF;1|LPD;IOU_SECURE_SLCR;FF240000;FF24FFFF;1|LPD;IOU_SCNTRS;FF260000;FF26FFFF;1|LPD;IOU_SCNTR;FF250000;FF25FFFF;1|LPD;IOU_GPV;FE000000;FE0FFFFF;1|LPD;I2C1;FF030000;FF03FFFF;1|LPD;I2C0;FF020000;FF02FFFF;1|FPD;GPU;FD4B0000;FD4BFFFF;0|LPD;GPIO;FF0A0000;FF0AFFFF;1|LPD;GEM3;FF0E0000;FF0EFFFF;1|LPD;GEM2;FF0D0000;FF0DFFFF;0|LPD;GEM1;FF0C0000;FF0CFFFF;0|LPD;GEM0;FF0B0000;FF0BFFFF;0|FPD;FPD_XMPU_SINK;FD4F0000;FD4FFFFF;1|FPD;FPD_XMPU_CFG;FD5D0000;FD5DFFFF;1|FPD;FPD_SLCR_SECURE;FD690000;FD6CFFFF;1|FPD;FPD_SLCR;FD610000;FD68FFFF;1|FPD;FPD_DMA_CH7;FD570000;FD57FFFF;1|FPD;FPD_DMA_CH6;FD560000;FD56FFFF;1|FPD;FPD_DMA_CH5;FD550000;FD55FFFF;1|FPD;FPD_DMA_CH4;FD540000;FD54FFFF;1|FPD;FPD_DMA_CH3;FD530000;FD53FFFF;1|FPD;FPD_DMA_CH2;FD520000;FD52FFFF;1|FPD;FPD_DMA_CH1;FD510000;FD51FFFF;1|FPD;FPD_DMA_CH0;FD500000;FD50FFFF;1|LPD;EFUSE;FFCC0000;FFCCFFFF;1|FPD;Display\
Port;FD4A0000;FD4AFFFF;0|FPD;DPDMA;FD4C0000;FD4CFFFF;0|FPD;DDR_XMPU5_CFG;FD050000;FD05FFFF;1|FPD;DDR_XMPU4_CFG;FD040000;FD04FFFF;1|FPD;DDR_XMPU3_CFG;FD030000;FD03FFFF;1|FPD;DDR_XMPU2_CFG;FD020000;FD02FFFF;1|FPD;DDR_XMPU1_CFG;FD010000;FD01FFFF;1|FPD;DDR_XMPU0_CFG;FD000000;FD00FFFF;1|FPD;DDR_QOS_CTRL;FD090000;FD09FFFF;1|FPD;DDR_PHY;FD080000;FD08FFFF;1|DDR;DDR_LOW;0;7FFFFFFF;1|DDR;DDR_HIGH;800000000;87FFFFFFF;1|FPD;DDDR_CTRL;FD070000;FD070FFF;1|LPD;Coresight;FE800000;FEFFFFFF;1|LPD;CSU_DMA;FFC80000;FFC9FFFF;1|LPD;CSU;FFCA0000;FFCAFFFF;1|LPD;CRL_APB;FF5E0000;FF85FFFF;1|FPD;CRF_APB;FD1A0000;FD2DFFFF;1|FPD;CCI_REG;FD5E0000;FD5EFFFF;1|LPD;CAN1;FF070000;FF07FFFF;0|LPD;CAN0;FF060000;FF06FFFF;0|FPD;APU;FD5C0000;FD5CFFFF;1|LPD;APM_INTC_IOU;FFA20000;FFA2FFFF;1|LPD;APM_FPD_LPD;FFA30000;FFA3FFFF;1|FPD;APM_5;FD490000;FD49FFFF;1|FPD;APM_0;FD0B0000;FD0BFFFF;1|LPD;APM2;FFA10000;FFA1FFFF;1|LPD;APM1;FFA00000;FFA0FFFF;1|LPD;AMS;FFA50000;FFA5FFFF;1|FPD;AFI_5;FD3B0000;FD3BFFFF;1|FPD;AFI_4;FD3A0000;FD3AFFFF;1|FPD;AFI_3;FD390000;FD39FFFF;1|FPD;AFI_2;FD380000;FD38FFFF;1|FPD;AFI_1;FD370000;FD37FFFF;1|FPD;AFI_0;FD360000;FD36FFFF;1|LPD;AFIFM6;FF9B0000;FF9BFFFF;1|FPD;ACPU_GIC;F9010000;F907FFFF;1}\
   CONFIG.PSU__PROTECTION__SUBSYSTEMS {PMU Firmware:PMU} \
   CONFIG.PSU__PSS_ALT_REF_CLK__ENABLE {0} \
   CONFIG.PSU__PSS_ALT_REF_CLK__FREQMHZ {33.333} \
   CONFIG.PSU__PSS_REF_CLK__FREQMHZ {33.333} \
   CONFIG.PSU__QSPI_COHERENCY {0} \
   CONFIG.PSU__QSPI_ROUTE_THROUGH_FPD {0} \
   CONFIG.PSU__QSPI__GRP_FBCLK__ENABLE {0} \
   CONFIG.PSU__QSPI__PERIPHERAL__DATA_MODE {<Select>} \
   CONFIG.PSU__QSPI__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__QSPI__PERIPHERAL__IO {<Select>} \
   CONFIG.PSU__QSPI__PERIPHERAL__MODE {<Select>} \
   CONFIG.PSU__REPORT__DBGLOG {0} \
   CONFIG.PSU__RPU_COHERENCY {0} \
   CONFIG.PSU__RPU__POWER__ON {1} \
   CONFIG.PSU__SATA__LANE0__ENABLE {0} \
   CONFIG.PSU__SATA__LANE1__ENABLE {0} \
   CONFIG.PSU__SATA__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__SAXIGP0__DATA_WIDTH {128} \
   CONFIG.PSU__SAXIGP1__DATA_WIDTH {128} \
   CONFIG.PSU__SAXIGP2__DATA_WIDTH {128} \
   CONFIG.PSU__SAXIGP3__DATA_WIDTH {128} \
   CONFIG.PSU__SAXIGP4__DATA_WIDTH {128} \
   CONFIG.PSU__SAXIGP5__DATA_WIDTH {128} \
   CONFIG.PSU__SAXIGP6__DATA_WIDTH {128} \
   CONFIG.PSU__SD0_COHERENCY {0} \
   CONFIG.PSU__SD0_ROUTE_THROUGH_FPD {0} \
   CONFIG.PSU__SD0__DATA_TRANSFER_MODE {8Bit} \
   CONFIG.PSU__SD0__GRP_CD__ENABLE {0} \
   CONFIG.PSU__SD0__GRP_POW__ENABLE {1} \
   CONFIG.PSU__SD0__GRP_POW__IO {MIO 23} \
   CONFIG.PSU__SD0__GRP_WP__ENABLE {0} \
   CONFIG.PSU__SD0__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__SD0__PERIPHERAL__IO {MIO 13 .. 22} \
   CONFIG.PSU__SD0__RESET__ENABLE {1} \
   CONFIG.PSU__SD0__SLOT_TYPE {eMMC} \
   CONFIG.PSU__SD1_COHERENCY {0} \
   CONFIG.PSU__SD1_ROUTE_THROUGH_FPD {0} \
   CONFIG.PSU__SD1__DATA_TRANSFER_MODE {8Bit} \
   CONFIG.PSU__SD1__GRP_CD__ENABLE {1} \
   CONFIG.PSU__SD1__GRP_CD__IO {MIO 45} \
   CONFIG.PSU__SD1__GRP_POW__ENABLE {0} \
   CONFIG.PSU__SD1__GRP_WP__ENABLE {0} \
   CONFIG.PSU__SD1__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__SD1__PERIPHERAL__IO {MIO 39 .. 51} \
   CONFIG.PSU__SD1__RESET__ENABLE {0} \
   CONFIG.PSU__SD1__SLOT_TYPE {SD 3.0} \
   CONFIG.PSU__SPI0_LOOP_SPI1__ENABLE {0} \
   CONFIG.PSU__SPI0__GRP_SS0__ENABLE {0} \
   CONFIG.PSU__SPI0__GRP_SS0__IO {<Select>} \
   CONFIG.PSU__SPI0__GRP_SS1__ENABLE {0} \
   CONFIG.PSU__SPI0__GRP_SS2__ENABLE {0} \
   CONFIG.PSU__SPI0__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__SPI0__PERIPHERAL__IO {<Select>} \
   CONFIG.PSU__SPI1__GRP_SS0__ENABLE {1} \
   CONFIG.PSU__SPI1__GRP_SS0__IO {MIO 9} \
   CONFIG.PSU__SPI1__GRP_SS1__ENABLE {1} \
   CONFIG.PSU__SPI1__GRP_SS1__IO {MIO 8} \
   CONFIG.PSU__SPI1__GRP_SS2__ENABLE {1} \
   CONFIG.PSU__SPI1__GRP_SS2__IO {MIO 7} \
   CONFIG.PSU__SPI1__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__SPI1__PERIPHERAL__IO {MIO 6 .. 11} \
   CONFIG.PSU__SWDT0__CLOCK__ENABLE {0} \
   CONFIG.PSU__SWDT0__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__SWDT0__PERIPHERAL__IO {NA} \
   CONFIG.PSU__SWDT0__RESET__ENABLE {0} \
   CONFIG.PSU__SWDT1__CLOCK__ENABLE {0} \
   CONFIG.PSU__SWDT1__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__SWDT1__PERIPHERAL__IO {NA} \
   CONFIG.PSU__SWDT1__RESET__ENABLE {0} \
   CONFIG.PSU__TCM0A__POWER__ON {1} \
   CONFIG.PSU__TCM0B__POWER__ON {1} \
   CONFIG.PSU__TCM1A__POWER__ON {1} \
   CONFIG.PSU__TCM1B__POWER__ON {1} \
   CONFIG.PSU__TESTSCAN__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__TRACE_PIPELINE_WIDTH {8} \
   CONFIG.PSU__TRACE__INTERNAL_WIDTH {32} \
   CONFIG.PSU__TRACE__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__TRISTATE__INVERTED {1} \
   CONFIG.PSU__TSU__BUFG_PORT_PAIR {0} \
   CONFIG.PSU__TTC0__CLOCK__ENABLE {0} \
   CONFIG.PSU__TTC0__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__TTC0__PERIPHERAL__IO {NA} \
   CONFIG.PSU__TTC0__WAVEOUT__ENABLE {0} \
   CONFIG.PSU__TTC1__CLOCK__ENABLE {0} \
   CONFIG.PSU__TTC1__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__TTC1__PERIPHERAL__IO {NA} \
   CONFIG.PSU__TTC1__WAVEOUT__ENABLE {0} \
   CONFIG.PSU__TTC2__CLOCK__ENABLE {0} \
   CONFIG.PSU__TTC2__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__TTC2__PERIPHERAL__IO {NA} \
   CONFIG.PSU__TTC2__WAVEOUT__ENABLE {0} \
   CONFIG.PSU__TTC3__CLOCK__ENABLE {0} \
   CONFIG.PSU__TTC3__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__TTC3__PERIPHERAL__IO {NA} \
   CONFIG.PSU__TTC3__WAVEOUT__ENABLE {0} \
   CONFIG.PSU__UART0_LOOP_UART1__ENABLE {0} \
   CONFIG.PSU__UART0__BAUD_RATE {115200} \
   CONFIG.PSU__UART0__MODEM__ENABLE {0} \
   CONFIG.PSU__UART0__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__UART0__PERIPHERAL__IO {MIO 30 .. 31} \
   CONFIG.PSU__UART1__BAUD_RATE {115200} \
   CONFIG.PSU__UART1__MODEM__ENABLE {0} \
   CONFIG.PSU__UART1__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__UART1__PERIPHERAL__IO {MIO 32 .. 33} \
   CONFIG.PSU__USB0_COHERENCY {0} \
   CONFIG.PSU__USB0__PERIPHERAL__ENABLE {1} \
   CONFIG.PSU__USB0__PERIPHERAL__IO {MIO 52 .. 63} \
   CONFIG.PSU__USB0__RESET__ENABLE {0} \
   CONFIG.PSU__USB1_COHERENCY {0} \
   CONFIG.PSU__USB1__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__USB1__RESET__ENABLE {0} \
   CONFIG.PSU__USB2_0__EMIO__ENABLE {0} \
   CONFIG.PSU__USB2_1__EMIO__ENABLE {0} \
   CONFIG.PSU__USB3_0__EMIO__ENABLE {0} \
   CONFIG.PSU__USB3_0__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__USB3_1__EMIO__ENABLE {0} \
   CONFIG.PSU__USB3_1__PERIPHERAL__ENABLE {0} \
   CONFIG.PSU__USB__RESET__MODE {Boot Pin} \
   CONFIG.PSU__USB__RESET__POLARITY {Active Low} \
   CONFIG.PSU__USE_DIFF_RW_CLK_GP0 {0} \
   CONFIG.PSU__USE_DIFF_RW_CLK_GP1 {0} \
   CONFIG.PSU__USE_DIFF_RW_CLK_GP2 {0} \
   CONFIG.PSU__USE_DIFF_RW_CLK_GP3 {0} \
   CONFIG.PSU__USE_DIFF_RW_CLK_GP4 {0} \
   CONFIG.PSU__USE_DIFF_RW_CLK_GP5 {0} \
   CONFIG.PSU__USE_DIFF_RW_CLK_GP6 {0} \
   CONFIG.PSU__USE__ADMA {0} \
   CONFIG.PSU__USE__APU_LEGACY_INTERRUPT {0} \
   CONFIG.PSU__USE__AUDIO {0} \
   CONFIG.PSU__USE__CLK {0} \
   CONFIG.PSU__USE__CLK0 {0} \
   CONFIG.PSU__USE__CLK1 {0} \
   CONFIG.PSU__USE__CLK2 {0} \
   CONFIG.PSU__USE__CLK3 {0} \
   CONFIG.PSU__USE__CROSS_TRIGGER {0} \
   CONFIG.PSU__USE__DDR_INTF_REQUESTED {0} \
   CONFIG.PSU__USE__DEBUG__TEST {0} \
   CONFIG.PSU__USE__EVENT_RPU {0} \
   CONFIG.PSU__USE__FABRIC__RST {1} \
   CONFIG.PSU__USE__FTM {0} \
   CONFIG.PSU__USE__GDMA {0} \
   CONFIG.PSU__USE__IRQ {0} \
   CONFIG.PSU__USE__IRQ0 {1} \
   CONFIG.PSU__USE__IRQ1 {1} \
   CONFIG.PSU__USE__M_AXI_GP0 {1} \
   CONFIG.PSU__USE__M_AXI_GP1 {0} \
   CONFIG.PSU__USE__M_AXI_GP2 {1} \
   CONFIG.PSU__USE__PROC_EVENT_BUS {0} \
   CONFIG.PSU__USE__RPU_LEGACY_INTERRUPT {1} \
   CONFIG.PSU__USE__RST0 {0} \
   CONFIG.PSU__USE__RST1 {0} \
   CONFIG.PSU__USE__RST2 {0} \
   CONFIG.PSU__USE__RST3 {0} \
   CONFIG.PSU__USE__RTC {0} \
   CONFIG.PSU__USE__STM {0} \
   CONFIG.PSU__USE__S_AXI_ACE {0} \
   CONFIG.PSU__USE__S_AXI_ACP {0} \
   CONFIG.PSU__USE__S_AXI_GP0 {1} \
   CONFIG.PSU__USE__S_AXI_GP1 {1} \
   CONFIG.PSU__USE__S_AXI_GP2 {1} \
   CONFIG.PSU__USE__S_AXI_GP3 {1} \
   CONFIG.PSU__USE__S_AXI_GP4 {0} \
   CONFIG.PSU__USE__S_AXI_GP5 {0} \
   CONFIG.PSU__USE__S_AXI_GP6 {0} \
   CONFIG.PSU__USE__USB3_0_HUB {0} \
   CONFIG.PSU__USE__USB3_1_HUB {0} \
   CONFIG.PSU__USE__VIDEO {0} \
   CONFIG.PSU__VIDEO_REF_CLK__ENABLE {0} \
   CONFIG.PSU__VIDEO_REF_CLK__FREQMHZ {33.333} \
   CONFIG.QSPI_BOARD_INTERFACE {custom} \
   CONFIG.SATA_BOARD_INTERFACE {custom} \
   CONFIG.SD0_BOARD_INTERFACE {custom} \
   CONFIG.SD1_BOARD_INTERFACE {custom} \
   CONFIG.SPI0_BOARD_INTERFACE {custom} \
   CONFIG.SPI1_BOARD_INTERFACE {custom} \
   CONFIG.SUBPRESET1 {Custom} \
   CONFIG.SUBPRESET2 {Custom} \
   CONFIG.SWDT0_BOARD_INTERFACE {custom} \
   CONFIG.SWDT1_BOARD_INTERFACE {custom} \
   CONFIG.TRACE_BOARD_INTERFACE {custom} \
   CONFIG.TTC0_BOARD_INTERFACE {custom} \
   CONFIG.TTC1_BOARD_INTERFACE {custom} \
   CONFIG.TTC2_BOARD_INTERFACE {custom} \
   CONFIG.TTC3_BOARD_INTERFACE {custom} \
   CONFIG.UART0_BOARD_INTERFACE {custom} \
   CONFIG.UART1_BOARD_INTERFACE {custom} \
   CONFIG.USB0_BOARD_INTERFACE {custom} \
   CONFIG.USB1_BOARD_INTERFACE {custom} \
 ] $inst_zynq_ps

  # Create instance: xlconcat_0, and set properties
  set xlconcat_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconcat:2.1 xlconcat_0 ]

  # Create instance: xlconstant_1, and set properties
  set xlconstant_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlconstant:1.1 xlconstant_1 ]
  set_property -dict [ list \
   CONFIG.CONST_VAL {1} \
 ] $xlconstant_1

  # Create interface connections
  connect_bd_intf_net -intf_net Conn1 [get_bd_intf_pins m_axi_core] [get_bd_intf_pins axi_interconnect_common/m_axi_core]
  connect_bd_intf_net -intf_net Conn2 [get_bd_intf_pins s_axi_hpc0] [get_bd_intf_pins inst_zynq_ps/S_AXI_HPC0_FPD]
  connect_bd_intf_net -intf_net Conn3 [get_bd_intf_pins m_axis_eth_dma] [get_bd_intf_pins eth_dma_internal/m_axis_eth_dma]
  connect_bd_intf_net -intf_net Conn4 [get_bd_intf_pins s_axis_eth_dma] [get_bd_intf_pins eth_dma_internal/s_axis_eth_dma]
  connect_bd_intf_net -intf_net Conn5 [get_bd_intf_pins m_axi_eth_internal] [get_bd_intf_pins axi_interconnect_common/m_axi_eth_internal]
  connect_bd_intf_net -intf_net S00_AXI_1 [get_bd_intf_pins eth_dma_internal/m_axi_to_ps] [get_bd_intf_pins hpc1_axi_interconnect/S00_AXI]
  connect_bd_intf_net -intf_net S_AXI_HPC1_FPD_0_1 [get_bd_intf_pins s_axi_hpc1] [get_bd_intf_pins hpc1_axi_interconnect/S01_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_0_M00_AXI [get_bd_intf_pins hpc1_axi_interconnect/M00_AXI] [get_bd_intf_pins inst_zynq_ps/S_AXI_HPC1_FPD]
  connect_bd_intf_net -intf_net axi_interconnect_0_M01_AXI [get_bd_intf_pins m_axi_rf] [get_bd_intf_pins axi_interconnect_common/m_axi_rf]
  connect_bd_intf_net -intf_net axi_interconnect_common_M01_AXI [get_bd_intf_pins axi_interconnect_common/m_axi_jtag] [get_bd_intf_pins cpld_jtag_engine/S_AXI]
  connect_bd_intf_net -intf_net axi_interconnect_common_m_axi_eth_dma_ctrl [get_bd_intf_pins axi_interconnect_common/m_axi_eth_dma_ctrl] [get_bd_intf_pins eth_dma_internal/s_axi_eth_dma_ctrl]
  connect_bd_intf_net -intf_net axi_interconnect_common_m_axi_mpm_ep [get_bd_intf_pins m_axi_mpm_ep] [get_bd_intf_pins axi_interconnect_common/m_axi_mpm_ep]
  connect_bd_intf_net -intf_net axi_interconnect_common_m_axi_rpu [get_bd_intf_pins m_axi_rpu] [get_bd_intf_pins axi_interconnect_common/m_axi_rpu]
  connect_bd_intf_net -intf_net axi_interconnect_common_m_axi_uhd [get_bd_intf_pins m_axi_app] [get_bd_intf_pins axi_interconnect_common/m_axi_app]
  connect_bd_intf_net -intf_net inst_zynq_ps_GPIO_0 [get_bd_intf_pins gpio_0] [get_bd_intf_pins inst_zynq_ps/GPIO_0]
  connect_bd_intf_net -intf_net inst_zynq_ps_M_AXI_HPM0_FPD [get_bd_intf_pins axi_interconnect_common/s_axi_common] [get_bd_intf_pins inst_zynq_ps/M_AXI_HPM0_FPD]
  connect_bd_intf_net -intf_net inst_zynq_ps_M_AXI_HPM0_LPD [get_bd_intf_pins axi_interconnect_common/s_axi_lpd] [get_bd_intf_pins inst_zynq_ps/M_AXI_HPM0_LPD]
  connect_bd_intf_net -intf_net s_axi_hp0_1 [get_bd_intf_pins s_axi_hp0] [get_bd_intf_pins inst_zynq_ps/S_AXI_HP0_FPD]
  connect_bd_intf_net -intf_net s_axi_hp1_1 [get_bd_intf_pins s_axi_hp1] [get_bd_intf_pins inst_zynq_ps/S_AXI_HP1_FPD]

  # Create port connections
  connect_bd_net -net Net [get_bd_pins jtag0_tck] [get_bd_pins cpld_jtag_engine/bit_clk]
  connect_bd_net -net Net1 [get_bd_pins jtag0_tdi] [get_bd_pins cpld_jtag_engine/bit_out]
  connect_bd_net -net Net2 [get_bd_pins jtag0_tms] [get_bd_pins cpld_jtag_engine/bit_stb]
  connect_bd_net -net bus_rstn_1 [get_bd_pins bus_rstn] [get_bd_pins eth_dma_internal/bus_rstn] [get_bd_pins hpc1_axi_interconnect/ARESETN] [get_bd_pins hpc1_axi_interconnect/M00_ARESETN] [get_bd_pins hpc1_axi_interconnect/S00_ARESETN] [get_bd_pins hpc1_axi_interconnect/S01_ARESETN]
  connect_bd_net -net clk40_1 [get_bd_pins clk40] [get_bd_pins axi_interconnect_common/clk40] [get_bd_pins cpld_jtag_engine/S_AXI_ACLK] [get_bd_pins eth_dma_internal/clk40] [get_bd_pins inst_zynq_ps/maxihpm0_fpd_aclk] [get_bd_pins inst_zynq_ps/maxihpm0_lpd_aclk]
  connect_bd_net -net clk40_rstn_1 [get_bd_pins clk40_rstn] [get_bd_pins axi_interconnect_common/clk40_rstn] [get_bd_pins cpld_jtag_engine/S_AXI_ARESETN] [get_bd_pins eth_dma_internal/clk40_rstn]
  connect_bd_net -net eth_dma_internal_irq [get_bd_pins eth_dma_internal/irq] [get_bd_pins xlconcat_0/In0]
  connect_bd_net -net inst_zynq_ps_pl_clk0 [get_bd_pins pl_clk100] [get_bd_pins inst_zynq_ps/pl_clk0]
  connect_bd_net -net inst_zynq_ps_pl_clk1 [get_bd_pins pl_clk40] [get_bd_pins inst_zynq_ps/pl_clk1]
  connect_bd_net -net inst_zynq_ps_pl_clk2 [get_bd_pins pl_clk166] [get_bd_pins inst_zynq_ps/pl_clk2]
  connect_bd_net -net inst_zynq_ps_pl_clk3 [get_bd_pins pl_clk200] [get_bd_pins inst_zynq_ps/pl_clk3]
  connect_bd_net -net inst_zynq_ps_pl_resetn0 [get_bd_pins pl_resetn0] [get_bd_pins inst_zynq_ps/pl_resetn0]
  connect_bd_net -net inst_zynq_ps_pl_resetn1 [get_bd_pins pl_resetn1] [get_bd_pins inst_zynq_ps/pl_resetn1]
  connect_bd_net -net inst_zynq_ps_pl_resetn2 [get_bd_pins pl_resetn2] [get_bd_pins inst_zynq_ps/pl_resetn2]
  connect_bd_net -net inst_zynq_ps_pl_resetn3 [get_bd_pins pl_resetn3] [get_bd_pins inst_zynq_ps/pl_resetn3]
  connect_bd_net -net jtag0_tdo_1 [get_bd_pins jtag0_tdo] [get_bd_pins cpld_jtag_engine/bit_in]
  connect_bd_net -net m_axi_sg_aclk_0_1 [get_bd_pins bus_clk] [get_bd_pins eth_dma_internal/bus_clk] [get_bd_pins hpc1_axi_interconnect/ACLK] [get_bd_pins hpc1_axi_interconnect/M00_ACLK] [get_bd_pins hpc1_axi_interconnect/S00_ACLK] [get_bd_pins hpc1_axi_interconnect/S01_ACLK] [get_bd_pins inst_zynq_ps/saxihpc1_fpd_aclk]
  connect_bd_net -net nirq0_lpd_rpu_0_1 [get_bd_pins irq0_lpd_rpu_n] [get_bd_pins inst_zynq_ps/nirq0_lpd_rpu]
  connect_bd_net -net nirq1_lpd_rpu_0_1 [get_bd_pins irq1_lpd_rpu_n] [get_bd_pins inst_zynq_ps/nirq1_lpd_rpu]
  connect_bd_net -net pl_ps_irq0_1 [get_bd_pins pl_ps_irq0] [get_bd_pins inst_zynq_ps/pl_ps_irq0]
  connect_bd_net -net pl_ps_irq1_1_1 [get_bd_pins pl_ps_irq1_1] [get_bd_pins xlconcat_0/In1]
  connect_bd_net -net s_axi_hp0_aclk_1 [get_bd_pins s_axi_hp0_aclk] [get_bd_pins inst_zynq_ps/saxihp0_fpd_aclk]
  connect_bd_net -net s_axi_hp1_aclk_1 [get_bd_pins s_axi_hp1_aclk] [get_bd_pins inst_zynq_ps/saxihp1_fpd_aclk]
  connect_bd_net -net saxihpc0_fpd_aclk_0_1 [get_bd_pins s_axi_hpc0_aclk] [get_bd_pins inst_zynq_ps/saxihpc0_fpd_aclk]
  connect_bd_net -net xlconcat_0_dout [get_bd_pins inst_zynq_ps/pl_ps_irq1] [get_bd_pins xlconcat_0/dout]
  connect_bd_net -net xlconstant_0_dout [get_bd_pins inst_zynq_ps/nfiq0_lpd_rpu] [get_bd_pins inst_zynq_ps/nfiq1_lpd_rpu] [get_bd_pins xlconstant_1/dout]

  # Restore current instance
  current_bd_instance $oldCurInst
}


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
  set adc0_clk [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 adc0_clk ]

  set adc2_clk [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 adc2_clk ]

  set adc_tile224_ch0_dout_i [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile224_ch0_dout_i ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   ] $adc_tile224_ch0_dout_i

  set adc_tile224_ch0_dout_q [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile224_ch0_dout_q ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   ] $adc_tile224_ch0_dout_q

  set adc_tile224_ch0_vin [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 adc_tile224_ch0_vin ]

  set adc_tile224_ch1_dout_i [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile224_ch1_dout_i ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   ] $adc_tile224_ch1_dout_i

  set adc_tile224_ch1_dout_q [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile224_ch1_dout_q ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   ] $adc_tile224_ch1_dout_q

  set adc_tile224_ch1_vin [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 adc_tile224_ch1_vin ]

  set adc_tile226_ch0_dout_i [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile226_ch0_dout_i ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   ] $adc_tile226_ch0_dout_i

  set adc_tile226_ch0_dout_q [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile226_ch0_dout_q ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   ] $adc_tile226_ch0_dout_q

  set adc_tile226_ch0_vin [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 adc_tile226_ch0_vin ]

  set adc_tile226_ch1_dout_i [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile226_ch1_dout_i ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   ] $adc_tile226_ch1_dout_i

  set adc_tile226_ch1_dout_q [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 adc_tile226_ch1_dout_q ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   ] $adc_tile226_ch1_dout_q

  set adc_tile226_ch1_vin [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 adc_tile226_ch1_vin ]

  set dac0_clk [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 dac0_clk ]

  set dac1_clk [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:diff_clock_rtl:1.0 dac1_clk ]

  set dac_tile228_ch0_din [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_tile228_ch0_din ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   CONFIG.HAS_TKEEP {0} \
   CONFIG.HAS_TLAST {0} \
   CONFIG.HAS_TREADY {1} \
   CONFIG.HAS_TSTRB {0} \
   CONFIG.LAYERED_METADATA {undef} \
   CONFIG.TDATA_NUM_BYTES {32} \
   CONFIG.TDEST_WIDTH {0} \
   CONFIG.TID_WIDTH {0} \
   CONFIG.TUSER_WIDTH {0} \
   ] $dac_tile228_ch0_din

  set dac_tile228_ch0_vout [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 dac_tile228_ch0_vout ]

  set dac_tile228_ch1_din [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_tile228_ch1_din ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   CONFIG.HAS_TKEEP {0} \
   CONFIG.HAS_TLAST {0} \
   CONFIG.HAS_TREADY {1} \
   CONFIG.HAS_TSTRB {0} \
   CONFIG.LAYERED_METADATA {undef} \
   CONFIG.TDATA_NUM_BYTES {32} \
   CONFIG.TDEST_WIDTH {0} \
   CONFIG.TID_WIDTH {0} \
   CONFIG.TUSER_WIDTH {0} \
   ] $dac_tile228_ch1_din

  set dac_tile228_ch1_vout [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 dac_tile228_ch1_vout ]

  set dac_tile229_ch0_din [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_tile229_ch0_din ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   CONFIG.HAS_TKEEP {0} \
   CONFIG.HAS_TLAST {0} \
   CONFIG.HAS_TREADY {1} \
   CONFIG.HAS_TSTRB {0} \
   CONFIG.LAYERED_METADATA {undef} \
   CONFIG.TDATA_NUM_BYTES {32} \
   CONFIG.TDEST_WIDTH {0} \
   CONFIG.TID_WIDTH {0} \
   CONFIG.TUSER_WIDTH {0} \
   ] $dac_tile229_ch0_din

  set dac_tile229_ch0_vout [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 dac_tile229_ch0_vout ]

  set dac_tile229_ch1_din [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 dac_tile229_ch1_din ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {184320000} \
   CONFIG.HAS_TKEEP {0} \
   CONFIG.HAS_TLAST {0} \
   CONFIG.HAS_TREADY {1} \
   CONFIG.HAS_TSTRB {0} \
   CONFIG.LAYERED_METADATA {undef} \
   CONFIG.TDATA_NUM_BYTES {32} \
   CONFIG.TDEST_WIDTH {0} \
   CONFIG.TID_WIDTH {0} \
   CONFIG.TUSER_WIDTH {0} \
   ] $dac_tile229_ch1_din

  set dac_tile229_ch1_vout [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:diff_analog_io_rtl:1.0 dac_tile229_ch1_vout ]

  set gpio_0 [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:gpio_rtl:1.0 gpio_0 ]

  set m_axi_app [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_app ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {40} \
   CONFIG.DATA_WIDTH {32} \
   CONFIG.FREQ_HZ {40000000} \
   CONFIG.HAS_REGION {0} \
   CONFIG.NUM_READ_OUTSTANDING {2} \
   CONFIG.NUM_WRITE_OUTSTANDING {2} \
   CONFIG.PROTOCOL {AXI4LITE} \
   ] $m_axi_app

  set m_axi_core [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_core ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {40} \
   CONFIG.DATA_WIDTH {32} \
   CONFIG.FREQ_HZ {40000000} \
   CONFIG.HAS_BURST {0} \
   CONFIG.HAS_CACHE {0} \
   CONFIG.HAS_LOCK {0} \
   CONFIG.HAS_QOS {0} \
   CONFIG.HAS_REGION {0} \
   CONFIG.NUM_READ_OUTSTANDING {2} \
   CONFIG.NUM_WRITE_OUTSTANDING {2} \
   CONFIG.PROTOCOL {AXI4LITE} \
   ] $m_axi_core

  set m_axi_eth_internal [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_eth_internal ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {40} \
   CONFIG.DATA_WIDTH {32} \
   CONFIG.FREQ_HZ {40000000} \
   CONFIG.HAS_BURST {0} \
   CONFIG.HAS_CACHE {0} \
   CONFIG.HAS_LOCK {0} \
   CONFIG.HAS_QOS {0} \
   CONFIG.HAS_REGION {0} \
   CONFIG.PROTOCOL {AXI4LITE} \
   ] $m_axi_eth_internal

  set m_axi_mpm_ep [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_mpm_ep ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {40} \
   CONFIG.DATA_WIDTH {32} \
   CONFIG.FREQ_HZ {40000000} \
   CONFIG.HAS_BURST {0} \
   CONFIG.HAS_CACHE {0} \
   CONFIG.HAS_LOCK {0} \
   CONFIG.HAS_QOS {0} \
   CONFIG.HAS_REGION {0} \
   CONFIG.NUM_READ_OUTSTANDING {2} \
   CONFIG.NUM_WRITE_OUTSTANDING {2} \
   CONFIG.PROTOCOL {AXI4LITE} \
   ] $m_axi_mpm_ep

  set m_axi_rpu [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 m_axi_rpu ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {40} \
   CONFIG.DATA_WIDTH {32} \
   CONFIG.FREQ_HZ {40000000} \
   CONFIG.HAS_BURST {0} \
   CONFIG.HAS_CACHE {0} \
   CONFIG.HAS_LOCK {0} \
   CONFIG.HAS_QOS {0} \
   CONFIG.HAS_REGION {0} \
   CONFIG.PROTOCOL {AXI4LITE} \
   ] $m_axi_rpu

  set m_axis_eth_dma [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:axis_rtl:1.0 m_axis_eth_dma ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {200000000} \
   ] $m_axis_eth_dma

  set s_axi_hp0 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_hp0 ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {49} \
   CONFIG.ARUSER_WIDTH {1} \
   CONFIG.AWUSER_WIDTH {1} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {128} \
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
   CONFIG.ID_WIDTH {6} \
   CONFIG.MAX_BURST_LENGTH {16} \
   CONFIG.NUM_READ_OUTSTANDING {16} \
   CONFIG.NUM_READ_THREADS {1} \
   CONFIG.NUM_WRITE_OUTSTANDING {16} \
   CONFIG.NUM_WRITE_THREADS {1} \
   CONFIG.PROTOCOL {AXI4} \
   CONFIG.READ_WRITE_MODE {READ_WRITE} \
   CONFIG.RUSER_BITS_PER_BYTE {0} \
   CONFIG.RUSER_WIDTH {0} \
   CONFIG.SUPPORTS_NARROW_BURST {1} \
   CONFIG.WUSER_BITS_PER_BYTE {0} \
   CONFIG.WUSER_WIDTH {0} \
   ] $s_axi_hp0

  set s_axi_hp1 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_hp1 ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {49} \
   CONFIG.ARUSER_WIDTH {1} \
   CONFIG.AWUSER_WIDTH {1} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {128} \
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
   CONFIG.ID_WIDTH {6} \
   CONFIG.MAX_BURST_LENGTH {16} \
   CONFIG.NUM_READ_OUTSTANDING {16} \
   CONFIG.NUM_READ_THREADS {1} \
   CONFIG.NUM_WRITE_OUTSTANDING {16} \
   CONFIG.NUM_WRITE_THREADS {1} \
   CONFIG.PROTOCOL {AXI4} \
   CONFIG.READ_WRITE_MODE {READ_WRITE} \
   CONFIG.RUSER_BITS_PER_BYTE {0} \
   CONFIG.RUSER_WIDTH {0} \
   CONFIG.SUPPORTS_NARROW_BURST {1} \
   CONFIG.WUSER_BITS_PER_BYTE {0} \
   CONFIG.WUSER_WIDTH {0} \
   ] $s_axi_hp1

  set s_axi_hpc0 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_hpc0 ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {49} \
   CONFIG.ARUSER_WIDTH {1} \
   CONFIG.AWUSER_WIDTH {1} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {128} \
   CONFIG.HAS_BRESP {1} \
   CONFIG.HAS_BURST {1} \
   CONFIG.HAS_CACHE {1} \
   CONFIG.HAS_LOCK {1} \
   CONFIG.HAS_PROT {1} \
   CONFIG.HAS_QOS {1} \
   CONFIG.HAS_REGION {0} \
   CONFIG.HAS_RRESP {1} \
   CONFIG.HAS_WSTRB {1} \
   CONFIG.ID_WIDTH {6} \
   CONFIG.MAX_BURST_LENGTH {256} \
   CONFIG.NUM_READ_OUTSTANDING {16} \
   CONFIG.NUM_READ_THREADS {1} \
   CONFIG.NUM_WRITE_OUTSTANDING {16} \
   CONFIG.NUM_WRITE_THREADS {1} \
   CONFIG.PROTOCOL {AXI4} \
   CONFIG.READ_WRITE_MODE {READ_WRITE} \
   CONFIG.RUSER_BITS_PER_BYTE {0} \
   CONFIG.RUSER_WIDTH {0} \
   CONFIG.SUPPORTS_NARROW_BURST {1} \
   CONFIG.WUSER_BITS_PER_BYTE {0} \
   CONFIG.WUSER_WIDTH {0} \
   ] $s_axi_hpc0

  set s_axi_hpc1 [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 s_axi_hpc1 ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {49} \
   CONFIG.ARUSER_WIDTH {1} \
   CONFIG.AWUSER_WIDTH {1} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {128} \
   CONFIG.FREQ_HZ {200000000} \
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
   CONFIG.NUM_READ_OUTSTANDING {16} \
   CONFIG.NUM_READ_THREADS {1} \
   CONFIG.NUM_WRITE_OUTSTANDING {16} \
   CONFIG.NUM_WRITE_THREADS {1} \
   CONFIG.PROTOCOL {AXI4} \
   CONFIG.READ_WRITE_MODE {READ_WRITE} \
   CONFIG.RUSER_BITS_PER_BYTE {0} \
   CONFIG.RUSER_WIDTH {0} \
   CONFIG.SUPPORTS_NARROW_BURST {0} \
   CONFIG.WUSER_BITS_PER_BYTE {0} \
   CONFIG.WUSER_WIDTH {0} \
   ] $s_axi_hpc1

  set s_axis_eth_dma [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:axis_rtl:1.0 s_axis_eth_dma ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {200000000} \
   CONFIG.HAS_TKEEP {1} \
   CONFIG.HAS_TLAST {1} \
   CONFIG.HAS_TREADY {1} \
   CONFIG.HAS_TSTRB {0} \
   CONFIG.LAYERED_METADATA {undef} \
   CONFIG.TDATA_NUM_BYTES {8} \
   CONFIG.TDEST_WIDTH {0} \
   CONFIG.TID_WIDTH {0} \
   CONFIG.TUSER_WIDTH {0} \
   ] $s_axis_eth_dma

  set sysref_rf_in [ create_bd_intf_port -mode Slave -vlnv xilinx.com:display_usp_rf_data_converter:diff_pins_rtl:1.0 sysref_rf_in ]


  # Create ports
  set adc_data_out_resetn_dclk [ create_bd_port -dir O adc_data_out_resetn_dclk ]
  set adc_enable_data_rclk [ create_bd_port -dir O adc_enable_data_rclk ]
  set adc_reset_pulse_dclk [ create_bd_port -dir I -type rst adc_reset_pulse_dclk ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_HIGH} \
 ] $adc_reset_pulse_dclk
  set adc_rfdc_axi_resetn_rclk [ create_bd_port -dir O adc_rfdc_axi_resetn_rclk ]
  set bus_clk [ create_bd_port -dir I -type clk -freq_hz 200000000 bus_clk ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {m_axis_eth_dma:s_axis_eth_dma:s_axi_hpc1} \
   CONFIG.ASSOCIATED_RESET {bus_rstn} \
 ] $bus_clk
  set bus_rstn [ create_bd_port -dir I -type rst bus_rstn ]
  set clk40 [ create_bd_port -dir I -type clk -freq_hz 40000000 clk40 ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {m_axi_app:m_axi_mpm_ep:m_axi_core:m_axi_rpu:m_axi_eth_internal} \
   CONFIG.ASSOCIATED_RESET {clk40_rstn} \
 ] $clk40
  set clk40_rstn [ create_bd_port -dir I -type rst clk40_rstn ]
  set dac_data_in_resetn_dclk [ create_bd_port -dir O dac_data_in_resetn_dclk ]
  set dac_data_in_resetn_dclk2x [ create_bd_port -dir O dac_data_in_resetn_dclk2x ]
  set dac_data_in_resetn_rclk [ create_bd_port -dir O dac_data_in_resetn_rclk ]
  set dac_data_in_resetn_rclk2x [ create_bd_port -dir O dac_data_in_resetn_rclk2x ]
  set dac_reset_pulse_dclk [ create_bd_port -dir I -type rst dac_reset_pulse_dclk ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_HIGH} \
 ] $dac_reset_pulse_dclk
  set data_clk [ create_bd_port -dir O -type clk data_clk ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {122880000} \
 ] $data_clk
  set data_clk_2x [ create_bd_port -dir O -type clk data_clk_2x ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {245760000} \
 ] $data_clk_2x
  set data_clock_locked [ create_bd_port -dir O data_clock_locked ]
  set enable_gated_clocks_clk40 [ create_bd_port -dir I enable_gated_clocks_clk40 ]
  set enable_sysref_rclk [ create_bd_port -dir I enable_sysref_rclk ]
  set fir_resetn_rclk2x [ create_bd_port -dir O fir_resetn_rclk2x ]
  set gated_base_clks_valid_clk40 [ create_bd_port -dir O gated_base_clks_valid_clk40 ]
  set irq0_lpd_rpu_n [ create_bd_port -dir I irq0_lpd_rpu_n ]
  set irq1_lpd_rpu_n [ create_bd_port -dir I irq1_lpd_rpu_n ]
  set jtag0_tck [ create_bd_port -dir IO jtag0_tck ]
  set jtag0_tdi [ create_bd_port -dir IO jtag0_tdi ]
  set jtag0_tdo [ create_bd_port -dir I jtag0_tdo ]
  set jtag0_tms [ create_bd_port -dir IO jtag0_tms ]
  set nco_reset_done_dclk [ create_bd_port -dir O nco_reset_done_dclk ]
  set pl_clk40 [ create_bd_port -dir O -type clk pl_clk40 ]
  set pl_clk100 [ create_bd_port -dir O -type clk pl_clk100 ]
  set pl_clk166 [ create_bd_port -dir O -type clk pl_clk166 ]
  set pl_clk200 [ create_bd_port -dir O -type clk pl_clk200 ]
  set pl_ps_irq0 [ create_bd_port -dir I -from 7 -to 0 -type intr pl_ps_irq0 ]
  set_property -dict [ list \
   CONFIG.PortWidth {8} \
   CONFIG.SENSITIVITY {EDGE_RISING} \
 ] $pl_ps_irq0
  set pl_ps_irq1 [ create_bd_port -dir I -from 5 -to 0 -type intr pl_ps_irq1 ]
  set_property -dict [ list \
   CONFIG.PortWidth {6} \
   CONFIG.SENSITIVITY {LEVEL_HIGH:LEVEL_HIGH} \
 ] $pl_ps_irq1
  set pl_resetn0 [ create_bd_port -dir O -type rst pl_resetn0 ]
  set pl_resetn1 [ create_bd_port -dir O -type rst pl_resetn1 ]
  set pl_resetn2 [ create_bd_port -dir O -type rst pl_resetn2 ]
  set pl_resetn3 [ create_bd_port -dir O -type rst pl_resetn3 ]
  set pll_ref_clk_in [ create_bd_port -dir I -type clk -freq_hz 61440000 pll_ref_clk_in ]
  set pll_ref_clk_out [ create_bd_port -dir O -type clk pll_ref_clk_out ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {61440000} \
 ] $pll_ref_clk_out
  set radio0_invert_adc_iq_r0clk [ create_bd_port -dir O -from 3 -to 0 radio0_invert_adc_iq_r0clk ]
  set radio0_invert_dac_iq_r0clk [ create_bd_port -dir O -from 3 -to 0 radio0_invert_dac_iq_r0clk ]
  set radio1_invert_adc_iq_r1clk [ create_bd_port -dir O -from 3 -to 0 radio1_invert_adc_iq_r1clk ]
  set radio1_invert_dac_iq_r1clk [ create_bd_port -dir O -from 3 -to 0 radio1_invert_dac_iq_r1clk ]
  set rf_axi_status_clk40 [ create_bd_port -dir I -from 31 -to 0 rf_axi_status_clk40 ]
  set rf_dsp_info_clk40 [ create_bd_port -dir I -from 31 -to 0 rf_dsp_info_clk40 ]
  set rf_rfdc_info_clk40 [ create_bd_port -dir I -from 31 -to 0 rf_rfdc_info_clk40 ]
  set rfdc_clk [ create_bd_port -dir O -from 0 -to 0 -type clk rfdc_clk ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {adc_tile224_ch0_dout_i:adc_tile224_ch1_dout_i:adc_tile224_ch1_dout_q:adc_tile224_ch0_dout_q:dac_tile228_ch0_din:dac_tile228_ch1_din:dac_tile229_ch0_din:dac_tile229_ch1_din:adc_tile226_ch0_dout_i:adc_tile226_ch0_dout_q:adc_tile226_ch1_dout_i:adc_tile226_ch1_dout_q} \
   CONFIG.ASSOCIATED_RESET {adc_tile224_axis_resetn_rclk:adc_tile226_axis_resetn_rclk:dac_tile228_axis_resetn_rclk:dac_tile229_axis_resetn_rclk} \
   CONFIG.FREQ_HZ {184320000} \
 ] $rfdc_clk
  set rfdc_clk_2x [ create_bd_port -dir O -from 0 -to 0 -type clk rfdc_clk_2x ]
  set_property -dict [ list \
   CONFIG.FREQ_HZ {368640000} \
 ] $rfdc_clk_2x
  set rfdc_irq [ create_bd_port -dir O -type intr rfdc_irq ]
  set s_axi_hp0_aclk [ create_bd_port -dir I -type clk -freq_hz 40000000 s_axi_hp0_aclk ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {s_axi_hp0} \
   CONFIG.ASSOCIATED_RESET {s_axi_hp0_aresetn} \
 ] $s_axi_hp0_aclk
  set s_axi_hp1_aclk [ create_bd_port -dir I -type clk -freq_hz 40000000 s_axi_hp1_aclk ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {s_axi_hp1} \
 ] $s_axi_hp1_aclk
  set s_axi_hpc0_aclk [ create_bd_port -dir I -type clk s_axi_hpc0_aclk ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {s_axi_hpc0} \
 ] $s_axi_hpc0_aclk
  set start_nco_reset_dclk [ create_bd_port -dir I start_nco_reset_dclk ]
  set sysref_out_pclk [ create_bd_port -dir O sysref_out_pclk ]
  set sysref_out_rclk [ create_bd_port -dir O sysref_out_rclk ]
  set sysref_pl_in [ create_bd_port -dir I sysref_pl_in ]

  # Create instance: ps
  create_hier_cell_ps [current_bd_instance .] ps

  # Create instance: rfdc
  create_hier_cell_rfdc [current_bd_instance .] rfdc

  # Create interface connections
  connect_bd_intf_net -intf_net S00_AXI_1 [get_bd_intf_ports s_axi_hp0] [get_bd_intf_pins ps/s_axi_hp0]
  connect_bd_intf_net -intf_net S_AXIS_S2MM_1_1 [get_bd_intf_ports s_axis_eth_dma] [get_bd_intf_pins ps/s_axis_eth_dma]
  connect_bd_intf_net -intf_net S_AXI_HP1_1 [get_bd_intf_ports s_axi_hp1] [get_bd_intf_pins ps/s_axi_hp1]
  connect_bd_intf_net -intf_net S_AXI_HPC0_FPD_0_1 [get_bd_intf_ports s_axi_hpc0] [get_bd_intf_pins ps/s_axi_hpc0]
  connect_bd_intf_net -intf_net adc0_clk_0_1 [get_bd_intf_ports adc0_clk] [get_bd_intf_pins rfdc/adc0_clk]
  connect_bd_intf_net -intf_net adc2_clk_0_1 [get_bd_intf_ports adc2_clk] [get_bd_intf_pins rfdc/adc2_clk]
  connect_bd_intf_net -intf_net axi_interconnect_common_m_axi_uhd [get_bd_intf_ports m_axi_app] [get_bd_intf_pins ps/m_axi_app]
  connect_bd_intf_net -intf_net dac0_clk_0_1 [get_bd_intf_ports dac0_clk] [get_bd_intf_pins rfdc/dac0_clk]
  connect_bd_intf_net -intf_net dac1_clk_0_1 [get_bd_intf_ports dac1_clk] [get_bd_intf_pins rfdc/dac1_clk]
  connect_bd_intf_net -intf_net inst_zynq_ps_GPIO_0 [get_bd_intf_ports gpio_0] [get_bd_intf_pins ps/gpio_0]
  connect_bd_intf_net -intf_net ps_M07_AXI_0 [get_bd_intf_ports m_axi_eth_internal] [get_bd_intf_pins ps/m_axi_eth_internal]
  connect_bd_intf_net -intf_net ps_M_AXIS_MM2S_1 [get_bd_intf_ports m_axis_eth_dma] [get_bd_intf_pins ps/m_axis_eth_dma]
  connect_bd_intf_net -intf_net ps_M_AXI_0 [get_bd_intf_ports m_axi_rpu] [get_bd_intf_pins ps/m_axi_rpu]
  connect_bd_intf_net -intf_net ps_m_axi_core_0 [get_bd_intf_ports m_axi_core] [get_bd_intf_pins ps/m_axi_core]
  connect_bd_intf_net -intf_net ps_m_axi_mpm_ep [get_bd_intf_ports m_axi_mpm_ep] [get_bd_intf_pins ps/m_axi_mpm_ep]
  connect_bd_intf_net -intf_net ps_m_axi_rf [get_bd_intf_pins ps/m_axi_rf] [get_bd_intf_pins rfdc/s_axi_config]
  connect_bd_intf_net -intf_net rf_data_converter_m00_axis [get_bd_intf_ports adc_tile224_ch0_dout_i] [get_bd_intf_pins rfdc/adc_tile224_ch0_dout_i]
  connect_bd_intf_net -intf_net rf_data_converter_m01_axis [get_bd_intf_ports adc_tile224_ch0_dout_q] [get_bd_intf_pins rfdc/adc_tile224_ch0_dout_q]
  connect_bd_intf_net -intf_net rf_data_converter_m02_axis [get_bd_intf_ports adc_tile224_ch1_dout_i] [get_bd_intf_pins rfdc/adc_tile224_ch1_dout_i]
  connect_bd_intf_net -intf_net rf_data_converter_m03_axis [get_bd_intf_ports adc_tile224_ch1_dout_q] [get_bd_intf_pins rfdc/adc_tile224_ch1_dout_q]
  connect_bd_intf_net -intf_net rf_data_converter_m20_axis [get_bd_intf_ports adc_tile226_ch0_dout_i] [get_bd_intf_pins rfdc/adc_tile226_ch0_dout_i]
  connect_bd_intf_net -intf_net rf_data_converter_m21_axis [get_bd_intf_ports adc_tile226_ch0_dout_q] [get_bd_intf_pins rfdc/adc_tile226_ch0_dout_q]
  connect_bd_intf_net -intf_net rf_data_converter_m22_axis [get_bd_intf_ports adc_tile226_ch1_dout_i] [get_bd_intf_pins rfdc/adc_tile226_ch1_dout_i]
  connect_bd_intf_net -intf_net rf_data_converter_m23_axis [get_bd_intf_ports adc_tile226_ch1_dout_q] [get_bd_intf_pins rfdc/adc_tile226_ch1_dout_q]
  connect_bd_intf_net -intf_net rf_data_converter_vout00 [get_bd_intf_ports dac_tile228_ch0_vout] [get_bd_intf_pins rfdc/dac_tile228_ch0_vout]
  connect_bd_intf_net -intf_net rf_data_converter_vout01 [get_bd_intf_ports dac_tile228_ch1_vout] [get_bd_intf_pins rfdc/dac_tile228_ch1_vout]
  connect_bd_intf_net -intf_net rf_data_converter_vout10 [get_bd_intf_ports dac_tile229_ch0_vout] [get_bd_intf_pins rfdc/dac_tile229_ch0_vout]
  connect_bd_intf_net -intf_net rf_data_converter_vout11 [get_bd_intf_ports dac_tile229_ch1_vout] [get_bd_intf_pins rfdc/dac_tile229_ch1_vout]
  connect_bd_intf_net -intf_net s00_axis_0_1 [get_bd_intf_ports dac_tile228_ch0_din] [get_bd_intf_pins rfdc/dac_tile228_ch0_din]
  connect_bd_intf_net -intf_net s01_axis_0_1 [get_bd_intf_ports dac_tile228_ch1_din] [get_bd_intf_pins rfdc/dac_tile228_ch1_din]
  connect_bd_intf_net -intf_net s10_axis_0_1 [get_bd_intf_ports dac_tile229_ch0_din] [get_bd_intf_pins rfdc/dac_tile229_ch0_din]
  connect_bd_intf_net -intf_net s11_axis_0_1 [get_bd_intf_ports dac_tile229_ch1_din] [get_bd_intf_pins rfdc/dac_tile229_ch1_din]
  connect_bd_intf_net -intf_net s_axi_hpc1 [get_bd_intf_ports s_axi_hpc1] [get_bd_intf_pins ps/s_axi_hpc1]
  connect_bd_intf_net -intf_net sysref_in_0_1 [get_bd_intf_ports sysref_rf_in] [get_bd_intf_pins rfdc/sysref_rf_in]
  connect_bd_intf_net -intf_net vin0_01_0_1 [get_bd_intf_ports adc_tile224_ch0_vin] [get_bd_intf_pins rfdc/adc_tile224_ch0_vin]
  connect_bd_intf_net -intf_net vin0_23_0_1 [get_bd_intf_ports adc_tile224_ch1_vin] [get_bd_intf_pins rfdc/adc_tile224_ch1_vin]
  connect_bd_intf_net -intf_net vin2_01_0_1 [get_bd_intf_ports adc_tile226_ch0_vin] [get_bd_intf_pins rfdc/adc_tile226_ch0_vin]
  connect_bd_intf_net -intf_net vin2_23_0_1 [get_bd_intf_ports adc_tile226_ch1_vin] [get_bd_intf_pins rfdc/adc_tile226_ch1_vin]

  # Create port connections
  connect_bd_net -net Net [get_bd_ports jtag0_tck] [get_bd_pins ps/jtag0_tck]
  connect_bd_net -net Net1 [get_bd_ports jtag0_tdi] [get_bd_pins ps/jtag0_tdi]
  connect_bd_net -net Net2 [get_bd_ports jtag0_tms] [get_bd_pins ps/jtag0_tms]
  connect_bd_net -net S02_ARESETN_0_1 [get_bd_ports bus_rstn] [get_bd_pins ps/bus_rstn]
  connect_bd_net -net adc_reset_pulse_dclk_1 [get_bd_ports adc_reset_pulse_dclk] [get_bd_pins rfdc/adc_reset_pulse_dclk]
  connect_bd_net -net capture_sysref_0_sysref_out_rclk [get_bd_ports sysref_out_rclk] [get_bd_pins rfdc/sysref_out_rclk]
  connect_bd_net -net capture_sysref_sysref_out_pclk [get_bd_ports sysref_out_pclk] [get_bd_pins rfdc/sysref_out_pclk]
  connect_bd_net -net clk40_1 [get_bd_ports clk40] [get_bd_pins ps/clk40] [get_bd_pins rfdc/s_axi_config_clk]
  connect_bd_net -net clk40_rstn_1 [get_bd_ports clk40_rstn] [get_bd_pins ps/clk40_rstn] [get_bd_pins rfdc/s_axi_config_aresetn]
  connect_bd_net -net clk_in1_0_1 [get_bd_ports pll_ref_clk_in] [get_bd_pins rfdc/pll_ref_clk_in]
  connect_bd_net -net dac_reset_pulse_dclk_1 [get_bd_ports dac_reset_pulse_dclk] [get_bd_pins rfdc/dac_reset_pulse_dclk]
  connect_bd_net -net data_clock_mmcm_data_clk [get_bd_ports data_clk] [get_bd_pins rfdc/data_clk]
  connect_bd_net -net data_clock_mmcm_data_clk_2x [get_bd_ports data_clk_2x] [get_bd_pins rfdc/data_clk_2x]
  connect_bd_net -net data_clock_mmcm_locked [get_bd_ports data_clock_locked] [get_bd_pins rfdc/data_clock_locked]
  connect_bd_net -net data_clock_mmcm_tdc_ref_clk [get_bd_ports pll_ref_clk_out] [get_bd_pins rfdc/pll_ref_clk_out]
  connect_bd_net -net enable_gated_clocks_1 [get_bd_ports enable_gated_clocks_clk40] [get_bd_pins rfdc/enable_gated_clocks_clk40]
  connect_bd_net -net enable_rclk_0_1 [get_bd_ports enable_sysref_rclk] [get_bd_pins rfdc/enable_sysref_rclk]
  connect_bd_net -net gpio2_io_i_0_2 [get_bd_ports rf_dsp_info_clk40] [get_bd_pins rfdc/rf_dsp_info_sclk]
  connect_bd_net -net gpio_io_i_0_1 [get_bd_ports rf_axi_status_clk40] [get_bd_pins rfdc/rf_axi_status_sclk]
  connect_bd_net -net inst_zynq_ps_pl_clk0 [get_bd_ports pl_clk100] [get_bd_pins ps/pl_clk100]
  connect_bd_net -net inst_zynq_ps_pl_clk1 [get_bd_ports pl_clk40] [get_bd_pins ps/pl_clk40]
  connect_bd_net -net inst_zynq_ps_pl_clk2 [get_bd_ports pl_clk166] [get_bd_pins ps/pl_clk166]
  connect_bd_net -net inst_zynq_ps_pl_clk3 [get_bd_ports pl_clk200] [get_bd_pins ps/pl_clk200]
  connect_bd_net -net inst_zynq_ps_pl_resetn0 [get_bd_ports pl_resetn0] [get_bd_pins ps/pl_resetn0]
  connect_bd_net -net inst_zynq_ps_pl_resetn1 [get_bd_ports pl_resetn1] [get_bd_pins ps/pl_resetn1]
  connect_bd_net -net inst_zynq_ps_pl_resetn2 [get_bd_ports pl_resetn2] [get_bd_pins ps/pl_resetn2]
  connect_bd_net -net inst_zynq_ps_pl_resetn3 [get_bd_ports pl_resetn3] [get_bd_pins ps/pl_resetn3]
  connect_bd_net -net jtag0_tdo_1 [get_bd_ports jtag0_tdo] [get_bd_pins ps/jtag0_tdo]
  connect_bd_net -net m_axi_sg_aclk_0_1 [get_bd_ports bus_clk] [get_bd_pins ps/bus_clk]
  connect_bd_net -net nirq0_lpd_rpu_0_1 [get_bd_ports irq0_lpd_rpu_n] [get_bd_pins ps/irq0_lpd_rpu_n]
  connect_bd_net -net nirq1_lpd_rpu_0_1 [get_bd_ports irq1_lpd_rpu_n] [get_bd_pins ps/irq1_lpd_rpu_n]
  connect_bd_net -net pl_ps_irq0_1 [get_bd_ports pl_ps_irq0] [get_bd_pins ps/pl_ps_irq0]
  connect_bd_net -net pl_ps_irq1_0_1 [get_bd_ports pl_ps_irq1] [get_bd_pins ps/pl_ps_irq1_1]
  connect_bd_net -net rStartNcoReset_0_1 [get_bd_ports start_nco_reset_dclk] [get_bd_pins rfdc/start_nco_reset_dclk]
  connect_bd_net -net rf_data_converter_irq [get_bd_ports rfdc_irq] [get_bd_pins rfdc/rfdc_irq]
  connect_bd_net -net rf_nco_reset_0_rNcoResetDone [get_bd_ports nco_reset_done_dclk] [get_bd_pins rfdc/nco_reset_done_dclk]
  connect_bd_net -net rf_reset_controller_0_rAdcEnableData [get_bd_ports adc_enable_data_rclk] [get_bd_pins rfdc/adc_enable_data_rclk]
  connect_bd_net -net rf_rfdc_info_clk40_1 [get_bd_ports rf_rfdc_info_clk40] [get_bd_pins rfdc/rf_rfdc_info_sclk]
  connect_bd_net -net rfdc_adc_rfdc_axi_resetn_rclk [get_bd_ports adc_rfdc_axi_resetn_rclk] [get_bd_pins rfdc/adc_rfdc_axi_resetn_rclk]
  connect_bd_net -net rfdc_d2DacFirReset_n_0 [get_bd_ports dac_data_in_resetn_dclk2x] [get_bd_pins rfdc/dac_data_in_resetn_dclk2x]
  connect_bd_net -net rfdc_dAdcDataOutReset_n_0 [get_bd_ports adc_data_out_resetn_dclk] [get_bd_pins rfdc/adc_data_out_resetn_dclk]
  connect_bd_net -net rfdc_dDacDataInReset_n_0 [get_bd_ports dac_data_in_resetn_dclk] [get_bd_pins rfdc/dac_data_in_resetn_dclk]
  connect_bd_net -net rfdc_dac_data_in_resetn_rclk [get_bd_ports dac_data_in_resetn_rclk] [get_bd_pins rfdc/dac_data_in_resetn_rclk]
  connect_bd_net -net rfdc_gated_base_clk_valid [get_bd_ports gated_base_clks_valid_clk40] [get_bd_pins rfdc/gated_base_clks_valid_clk40]
  connect_bd_net -net rfdc_r2AdcFirReset_n_0 [get_bd_ports fir_resetn_rclk2x] [get_bd_pins rfdc/fir_resetn_rclk2x]
  connect_bd_net -net rfdc_r2DacFirReset_n_0 [get_bd_ports dac_data_in_resetn_rclk2x] [get_bd_pins rfdc/dac_data_in_resetn_rclk2x]
  connect_bd_net -net rfdc_radio0_invert_adc_iq_r0clk [get_bd_ports radio0_invert_adc_iq_r0clk] [get_bd_pins rfdc/radio0_invert_adc_iq_r0clk]
  connect_bd_net -net rfdc_radio0_invert_dac_iq_r0clk [get_bd_ports radio0_invert_dac_iq_r0clk] [get_bd_pins rfdc/radio0_invert_dac_iq_r0clk]
  connect_bd_net -net rfdc_radio1_invert_adc_iq_r1clk [get_bd_ports radio1_invert_adc_iq_r1clk] [get_bd_pins rfdc/radio1_invert_adc_iq_r1clk]
  connect_bd_net -net rfdc_radio1_invert_dac_iq_r1clk [get_bd_ports radio1_invert_dac_iq_r1clk] [get_bd_pins rfdc/radio1_invert_dac_iq_r1clk]
  connect_bd_net -net rfdc_rfdc_clk [get_bd_ports rfdc_clk] [get_bd_pins rfdc/rfdc_clk]
  connect_bd_net -net rfdc_rfdc_clk_2x [get_bd_ports rfdc_clk_2x] [get_bd_pins rfdc/rfdc_clk_2x]
  connect_bd_net -net s_axi_hp0_aclk_1 [get_bd_ports s_axi_hp0_aclk] [get_bd_pins ps/s_axi_hp0_aclk]
  connect_bd_net -net s_axi_hp1_aclk_1 [get_bd_ports s_axi_hp1_aclk] [get_bd_pins ps/s_axi_hp1_aclk]
  connect_bd_net -net saxihpc0_fpd_aclk_0_1 [get_bd_ports s_axi_hpc0_aclk] [get_bd_pins ps/s_axi_hpc0_aclk]
  connect_bd_net -net sysref_pl_in_1 [get_bd_ports sysref_pl_in] [get_bd_pins rfdc/sysref_pl_in]

  # Create address segments
  assign_bd_address -offset 0x0010000A4000 -range 0x00004000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs ps/eth_dma_internal/axi_eth_dma_internal/S_AXI_LITE/Reg] -force
  assign_bd_address -offset 0x001000155000 -range 0x00001000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/ThresholdRegister/axi_gpio_0/S_AXI/Reg] -force
  assign_bd_address -offset 0x001000154000 -range 0x00001000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/calibration_muxes/axi_gpio_data/S_AXI/Reg] -force
  assign_bd_address -offset 0x001000000000 -range 0x00001000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs ps/cpld_jtag_engine/S_AXI/reg0] -force
  assign_bd_address -offset 0x001000140000 -range 0x00010000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/data_clock_mmcm/s_axi_lite/Reg] -force
  assign_bd_address -offset 0x001200000000 -range 0x000200000000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs m_axi_app/Reg] -force
  assign_bd_address -offset 0x0010000A0000 -range 0x00004000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs m_axi_core/Reg] -force
  assign_bd_address -offset 0x0010000A8000 -range 0x00004000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs m_axi_eth_internal/Reg] -force
  assign_bd_address -offset 0x001000080000 -range 0x00020000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs m_axi_mpm_ep/Reg] -force
  assign_bd_address -offset 0x80000000 -range 0x00010000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs m_axi_rpu/Reg] -force
  assign_bd_address -offset 0x001000156000 -range 0x00001000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/reg_clock_gate_control/S_AXI/Reg] -force
  assign_bd_address -offset 0x001000150000 -range 0x00000800 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/reg_invert_iq_radio0/S_AXI/Reg] -force
  assign_bd_address -offset 0x001000150800 -range 0x00000800 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/reg_invert_iq_radio1/S_AXI/Reg] -force
  assign_bd_address -offset 0x001000151000 -range 0x00001000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/reg_reset_mmcm/S_AXI/Reg] -force
  assign_bd_address -offset 0x001000153000 -range 0x00001000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/reg_rf_axi_status/S_AXI/Reg] -force
  assign_bd_address -offset 0x001000152000 -range 0x00000800 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/reg_rf_reset_control_radio0/S_AXI/Reg] -force
  assign_bd_address -offset 0x001000152800 -range 0x00000800 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/reg_rf_reset_control_radio1/S_AXI/Reg] -force
  assign_bd_address -offset 0x001000158000 -range 0x00001000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/reg_rfdc_info/S_AXI/Reg] -force
  assign_bd_address -offset 0x001000157000 -range 0x00001000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/reg_rfdc_tile_mapping/S_AXI/Reg] -force
  assign_bd_address -offset 0x001000100000 -range 0x00040000 -target_address_space [get_bd_addr_spaces ps/inst_zynq_ps/Data] [get_bd_addr_segs rfdc/rf_data_converter/s_axi/Reg] -force
  assign_bd_address -offset 0x000800000000 -range 0x000800000000 -target_address_space [get_bd_addr_spaces ps/eth_dma_internal/axi_eth_dma_internal/Data_SG] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP1/HPC1_DDR_HIGH] -force
  assign_bd_address -offset 0x000800000000 -range 0x000800000000 -target_address_space [get_bd_addr_spaces ps/eth_dma_internal/axi_eth_dma_internal/Data_MM2S] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP1/HPC1_DDR_HIGH] -force
  assign_bd_address -offset 0x000800000000 -range 0x000800000000 -target_address_space [get_bd_addr_spaces ps/eth_dma_internal/axi_eth_dma_internal/Data_S2MM] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP1/HPC1_DDR_HIGH] -force
  assign_bd_address -offset 0x00000000 -range 0x80000000 -target_address_space [get_bd_addr_spaces ps/eth_dma_internal/axi_eth_dma_internal/Data_SG] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP1/HPC1_DDR_LOW] -force
  assign_bd_address -offset 0x00000000 -range 0x80000000 -target_address_space [get_bd_addr_spaces ps/eth_dma_internal/axi_eth_dma_internal/Data_MM2S] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP1/HPC1_DDR_LOW] -force
  assign_bd_address -offset 0x00000000 -range 0x80000000 -target_address_space [get_bd_addr_spaces ps/eth_dma_internal/axi_eth_dma_internal/Data_S2MM] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP1/HPC1_DDR_LOW] -force
  assign_bd_address -offset 0x000800000000 -range 0x000800000000 -target_address_space [get_bd_addr_spaces s_axi_hp0] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP2/HP0_DDR_HIGH] -force
  assign_bd_address -offset 0x00000000 -range 0x80000000 -target_address_space [get_bd_addr_spaces s_axi_hp0] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP2/HP0_DDR_LOW] -force
  assign_bd_address -offset 0xFF000000 -range 0x01000000 -target_address_space [get_bd_addr_spaces s_axi_hp0] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP2/HP0_LPS_OCM] -force
  assign_bd_address -offset 0x000800000000 -range 0x000800000000 -target_address_space [get_bd_addr_spaces s_axi_hp1] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP3/HP1_DDR_HIGH] -force
  assign_bd_address -offset 0x00000000 -range 0x80000000 -target_address_space [get_bd_addr_spaces s_axi_hp1] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP3/HP1_DDR_LOW] -force
  assign_bd_address -offset 0xFF000000 -range 0x01000000 -target_address_space [get_bd_addr_spaces s_axi_hp1] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP3/HP1_LPS_OCM] -force
  assign_bd_address -offset 0x000800000000 -range 0x000800000000 -target_address_space [get_bd_addr_spaces s_axi_hpc0] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP0/HPC0_DDR_HIGH] -force
  assign_bd_address -offset 0x00000000 -range 0x80000000 -target_address_space [get_bd_addr_spaces s_axi_hpc0] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP0/HPC0_DDR_LOW] -force
  assign_bd_address -offset 0xFF000000 -range 0x01000000 -target_address_space [get_bd_addr_spaces s_axi_hpc0] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP0/HPC0_LPS_OCM] -force
  assign_bd_address -offset 0x000800000000 -range 0x000800000000 -target_address_space [get_bd_addr_spaces s_axi_hpc1] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP1/HPC1_DDR_HIGH] -force
  assign_bd_address -offset 0x00000000 -range 0x80000000 -target_address_space [get_bd_addr_spaces s_axi_hpc1] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP1/HPC1_DDR_LOW] -force
  assign_bd_address -offset 0xFF000000 -range 0x01000000 -target_address_space [get_bd_addr_spaces s_axi_hpc1] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP1/HPC1_LPS_OCM] -force

  # Exclude Address Segments
  exclude_bd_addr_seg -offset 0xFF000000 -range 0x01000000 -target_address_space [get_bd_addr_spaces ps/eth_dma_internal/axi_eth_dma_internal/Data_MM2S] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP1/HPC1_LPS_OCM]
  exclude_bd_addr_seg -offset 0xFF000000 -range 0x01000000 -target_address_space [get_bd_addr_spaces ps/eth_dma_internal/axi_eth_dma_internal/Data_S2MM] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP1/HPC1_LPS_OCM]
  exclude_bd_addr_seg -offset 0xFF000000 -range 0x01000000 -target_address_space [get_bd_addr_spaces ps/eth_dma_internal/axi_eth_dma_internal/Data_SG] [get_bd_addr_segs ps/inst_zynq_ps/SAXIGP1/HPC1_LPS_OCM]


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


