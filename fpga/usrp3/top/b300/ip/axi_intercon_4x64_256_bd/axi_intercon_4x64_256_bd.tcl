
################################################################
# This is a generated script based on design: axi_intercon_4x64_256_bd
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
# source axi_intercon_4x64_256_bd_script.tcl

# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./myproj/project_1.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
   create_project project_1 myproj -part xc7k325tfbg676-1
}


# CHANGE DESIGN NAME HERE
variable design_name
set design_name axi_intercon_4x64_256_bd

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
xilinx.com:ip:axi_register_slice:2.1\
xilinx.com:ip:axi_dwidth_converter:2.1\
xilinx.com:ip:axi_crossbar:2.1\
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
  set M00_AXI [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M00_AXI ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.DATA_WIDTH {256} \
   CONFIG.NUM_READ_OUTSTANDING {2} \
   CONFIG.NUM_WRITE_OUTSTANDING {2} \
   CONFIG.PROTOCOL {AXI4} \
   ] $M00_AXI

  set S00_AXI [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S00_AXI ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.ARUSER_WIDTH {0} \
   CONFIG.AWUSER_WIDTH {0} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {64} \
   CONFIG.FREQ_HZ {1000000000} \
   CONFIG.HAS_BRESP {1} \
   CONFIG.HAS_BURST {1} \
   CONFIG.HAS_CACHE {1} \
   CONFIG.HAS_LOCK {1} \
   CONFIG.HAS_PROT {1} \
   CONFIG.HAS_QOS {1} \
   CONFIG.HAS_REGION {1} \
   CONFIG.HAS_RRESP {1} \
   CONFIG.HAS_WSTRB {1} \
   CONFIG.ID_WIDTH {1} \
   CONFIG.MAX_BURST_LENGTH {256} \
   CONFIG.NUM_READ_OUTSTANDING {2} \
   CONFIG.NUM_READ_THREADS {1} \
   CONFIG.NUM_WRITE_OUTSTANDING {2} \
   CONFIG.NUM_WRITE_THREADS {1} \
   CONFIG.PROTOCOL {AXI4} \
   CONFIG.READ_WRITE_MODE {READ_WRITE} \
   CONFIG.RUSER_BITS_PER_BYTE {0} \
   CONFIG.RUSER_WIDTH {0} \
   CONFIG.SUPPORTS_NARROW_BURST {1} \
   CONFIG.WUSER_BITS_PER_BYTE {0} \
   CONFIG.WUSER_WIDTH {0} \
   ] $S00_AXI

  set S01_AXI [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S01_AXI ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.ARUSER_WIDTH {0} \
   CONFIG.AWUSER_WIDTH {0} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {64} \
   CONFIG.FREQ_HZ {1000000000} \
   CONFIG.HAS_BRESP {1} \
   CONFIG.HAS_BURST {1} \
   CONFIG.HAS_CACHE {1} \
   CONFIG.HAS_LOCK {1} \
   CONFIG.HAS_PROT {1} \
   CONFIG.HAS_QOS {1} \
   CONFIG.HAS_REGION {1} \
   CONFIG.HAS_RRESP {1} \
   CONFIG.HAS_WSTRB {1} \
   CONFIG.ID_WIDTH {1} \
   CONFIG.MAX_BURST_LENGTH {256} \
   CONFIG.NUM_READ_OUTSTANDING {2} \
   CONFIG.NUM_READ_THREADS {1} \
   CONFIG.NUM_WRITE_OUTSTANDING {2} \
   CONFIG.NUM_WRITE_THREADS {1} \
   CONFIG.PROTOCOL {AXI4} \
   CONFIG.READ_WRITE_MODE {READ_WRITE} \
   CONFIG.RUSER_BITS_PER_BYTE {0} \
   CONFIG.RUSER_WIDTH {0} \
   CONFIG.SUPPORTS_NARROW_BURST {1} \
   CONFIG.WUSER_BITS_PER_BYTE {0} \
   CONFIG.WUSER_WIDTH {0} \
   ] $S01_AXI

  set S02_AXI [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S02_AXI ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.ARUSER_WIDTH {0} \
   CONFIG.AWUSER_WIDTH {0} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {64} \
   CONFIG.FREQ_HZ {1000000000} \
   CONFIG.HAS_BRESP {1} \
   CONFIG.HAS_BURST {1} \
   CONFIG.HAS_CACHE {1} \
   CONFIG.HAS_LOCK {1} \
   CONFIG.HAS_PROT {1} \
   CONFIG.HAS_QOS {1} \
   CONFIG.HAS_REGION {1} \
   CONFIG.HAS_RRESP {1} \
   CONFIG.HAS_WSTRB {1} \
   CONFIG.ID_WIDTH {1} \
   CONFIG.MAX_BURST_LENGTH {256} \
   CONFIG.NUM_READ_OUTSTANDING {2} \
   CONFIG.NUM_READ_THREADS {1} \
   CONFIG.NUM_WRITE_OUTSTANDING {2} \
   CONFIG.NUM_WRITE_THREADS {1} \
   CONFIG.PROTOCOL {AXI4} \
   CONFIG.READ_WRITE_MODE {READ_WRITE} \
   CONFIG.RUSER_BITS_PER_BYTE {0} \
   CONFIG.RUSER_WIDTH {0} \
   CONFIG.SUPPORTS_NARROW_BURST {1} \
   CONFIG.WUSER_BITS_PER_BYTE {0} \
   CONFIG.WUSER_WIDTH {0} \
   ] $S02_AXI

  set S03_AXI [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S03_AXI ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.ARUSER_WIDTH {0} \
   CONFIG.AWUSER_WIDTH {0} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {64} \
   CONFIG.FREQ_HZ {1000000000} \
   CONFIG.HAS_BRESP {1} \
   CONFIG.HAS_BURST {1} \
   CONFIG.HAS_CACHE {1} \
   CONFIG.HAS_LOCK {1} \
   CONFIG.HAS_PROT {1} \
   CONFIG.HAS_QOS {1} \
   CONFIG.HAS_REGION {1} \
   CONFIG.HAS_RRESP {1} \
   CONFIG.HAS_WSTRB {1} \
   CONFIG.ID_WIDTH {1} \
   CONFIG.MAX_BURST_LENGTH {256} \
   CONFIG.NUM_READ_OUTSTANDING {2} \
   CONFIG.NUM_READ_THREADS {1} \
   CONFIG.NUM_WRITE_OUTSTANDING {2} \
   CONFIG.NUM_WRITE_THREADS {1} \
   CONFIG.PROTOCOL {AXI4} \
   CONFIG.READ_WRITE_MODE {READ_WRITE} \
   CONFIG.RUSER_BITS_PER_BYTE {0} \
   CONFIG.RUSER_WIDTH {0} \
   CONFIG.SUPPORTS_NARROW_BURST {1} \
   CONFIG.WUSER_BITS_PER_BYTE {0} \
   CONFIG.WUSER_WIDTH {0} \
   ] $S03_AXI


  # Create ports
  set M00_AXI_ACLK [ create_bd_port -dir I -type clk -freq_hz 1000000000 M00_AXI_ACLK ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_RESET {M00_AXI_ARESETN} \
   CONFIG.CLK_DOMAIN {axi_intercon_4x64_256_bd_M00_ACLK} \
 ] $M00_AXI_ACLK
  set M00_AXI_ARESETN [ create_bd_port -dir I -type rst M00_AXI_ARESETN ]
  set S00_AXI_ACLK [ create_bd_port -dir I -type clk -freq_hz 2000000000 S00_AXI_ACLK ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_RESET {S00_AXI_ARESETN} \
   CONFIG.CLK_DOMAIN {axi_intercon_4x64_256_bd_S00_ACLK} \
 ] $S00_AXI_ACLK
  set S00_AXI_ARESETN [ create_bd_port -dir I -type rst S00_AXI_ARESETN ]
  set S01_AXI_ACLK [ create_bd_port -dir I -type clk -freq_hz 2000000000 S01_AXI_ACLK ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_RESET {S01_AXI_ARESETN} \
   CONFIG.CLK_DOMAIN {axi_intercon_4x64_256_bd_S01_ACLK} \
 ] $S01_AXI_ACLK
  set S01_AXI_ARESETN [ create_bd_port -dir I -type rst S01_AXI_ARESETN ]
  set S02_AXI_ACLK [ create_bd_port -dir I -type clk -freq_hz 2000000000 S02_AXI_ACLK ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_RESET {S02_AXI_ARESETN} \
   CONFIG.CLK_DOMAIN {axi_intercon_4x64_256_bd_S02_ACLK} \
 ] $S02_AXI_ACLK
  set S02_AXI_ARESETN [ create_bd_port -dir I -type rst S02_AXI_ARESETN ]
  set S03_AXI_ACLK [ create_bd_port -dir I -type clk -freq_hz 2000000000 S03_AXI_ACLK ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_RESET {S03_AXI_ARESETN} \
   CONFIG.CLK_DOMAIN {axi_intercon_4x64_256_bd_S03_ACLK} \
 ] $S03_AXI_ACLK
  set S03_AXI_ARESETN [ create_bd_port -dir I -type rst S03_AXI_ARESETN ]

  # Create instance: m00_rs, and set properties
  set m00_rs [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 m00_rs ]
  set_property -dict [ list \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
 ] $m00_rs

  # Create instance: s00_rs_0, and set properties
  set s00_rs_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 s00_rs_0 ]
  set_property -dict [ list \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
   CONFIG.REG_R {1} \
   CONFIG.REG_W {1} \
 ] $s00_rs_0

  # Create instance: s00_rs_1, and set properties
  set s00_rs_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 s00_rs_1 ]
  set_property -dict [ list \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
   CONFIG.REG_R {1} \
   CONFIG.REG_W {1} \
 ] $s00_rs_1

  # Create instance: s00_width_conv, and set properties
  set s00_width_conv [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dwidth_converter:2.1 s00_width_conv ]
  set_property -dict [ list \
   CONFIG.ACLK_ASYNC {1} \
   CONFIG.FIFO_MODE {2} \
   CONFIG.MI_DATA_WIDTH {256} \
   CONFIG.SI_DATA_WIDTH {64} \
   CONFIG.SYNCHRONIZATION_STAGES {2} \
 ] $s00_width_conv

  # Create instance: s01_rs_0, and set properties
  set s01_rs_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 s01_rs_0 ]
  set_property -dict [ list \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
   CONFIG.REG_R {1} \
   CONFIG.REG_W {1} \
 ] $s01_rs_0

  # Create instance: s01_rs_1, and set properties
  set s01_rs_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 s01_rs_1 ]
  set_property -dict [ list \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
   CONFIG.REG_R {1} \
   CONFIG.REG_W {1} \
 ] $s01_rs_1

  # Create instance: s01_width_conv, and set properties
  set s01_width_conv [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dwidth_converter:2.1 s01_width_conv ]
  set_property -dict [ list \
   CONFIG.ACLK_ASYNC {1} \
   CONFIG.FIFO_MODE {2} \
   CONFIG.MI_DATA_WIDTH {256} \
   CONFIG.SI_DATA_WIDTH {64} \
   CONFIG.SYNCHRONIZATION_STAGES {2} \
 ] $s01_width_conv

  # Create instance: s02_rs_0, and set properties
  set s02_rs_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 s02_rs_0 ]
  set_property -dict [ list \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
   CONFIG.REG_R {1} \
   CONFIG.REG_W {1} \
 ] $s02_rs_0

  # Create instance: s02_rs_1, and set properties
  set s02_rs_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 s02_rs_1 ]
  set_property -dict [ list \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
   CONFIG.REG_R {1} \
   CONFIG.REG_W {1} \
 ] $s02_rs_1

  # Create instance: s02_width_conv, and set properties
  set s02_width_conv [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dwidth_converter:2.1 s02_width_conv ]
  set_property -dict [ list \
   CONFIG.ACLK_ASYNC {1} \
   CONFIG.FIFO_MODE {2} \
   CONFIG.MI_DATA_WIDTH {256} \
   CONFIG.SI_DATA_WIDTH {64} \
   CONFIG.SYNCHRONIZATION_STAGES {2} \
 ] $s02_width_conv

  # Create instance: s03_rs_0, and set properties
  set s03_rs_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 s03_rs_0 ]
  set_property -dict [ list \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
   CONFIG.REG_R {1} \
   CONFIG.REG_W {1} \
 ] $s03_rs_0

  # Create instance: s03_rs_1, and set properties
  set s03_rs_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 s03_rs_1 ]
  set_property -dict [ list \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
   CONFIG.REG_R {1} \
   CONFIG.REG_W {1} \
 ] $s03_rs_1

  # Create instance: s03_width_conv, and set properties
  set s03_width_conv [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dwidth_converter:2.1 s03_width_conv ]
  set_property -dict [ list \
   CONFIG.ACLK_ASYNC {1} \
   CONFIG.FIFO_MODE {2} \
   CONFIG.MI_DATA_WIDTH {256} \
   CONFIG.SI_DATA_WIDTH {64} \
   CONFIG.SYNCHRONIZATION_STAGES {2} \
 ] $s03_width_conv

  # Create instance: xbar, and set properties
  set xbar [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_crossbar:2.1 xbar ]
  set_property -dict [ list \
   CONFIG.DATA_WIDTH {256} \
   CONFIG.ID_WIDTH {2} \
   CONFIG.NUM_MI {1} \
   CONFIG.NUM_SI {4} \
   CONFIG.S01_BASE_ID {0x00000001} \
   CONFIG.S02_BASE_ID {0x00000002} \
   CONFIG.S03_BASE_ID {0x00000003} \
   CONFIG.S04_BASE_ID {0x00000004} \
   CONFIG.S05_BASE_ID {0x00000005} \
   CONFIG.S06_BASE_ID {0x00000006} \
   CONFIG.S07_BASE_ID {0x00000007} \
   CONFIG.S08_BASE_ID {0x00000008} \
   CONFIG.S09_BASE_ID {0x00000009} \
   CONFIG.S10_BASE_ID {0x0000000a} \
   CONFIG.S11_BASE_ID {0x0000000b} \
   CONFIG.S12_BASE_ID {0x0000000c} \
   CONFIG.S13_BASE_ID {0x0000000d} \
   CONFIG.S14_BASE_ID {0x0000000e} \
   CONFIG.S15_BASE_ID {0x0000000f} \
   CONFIG.STRATEGY {2} \
 ] $xbar

  # Create interface connections
  connect_bd_intf_net -intf_net S00_AXI_1 [get_bd_intf_ports S00_AXI] [get_bd_intf_pins s00_rs_0/S_AXI]
  connect_bd_intf_net -intf_net S01_AXI_1 [get_bd_intf_ports S01_AXI] [get_bd_intf_pins s01_rs_0/S_AXI]
  connect_bd_intf_net -intf_net S02_AXI_1 [get_bd_intf_ports S02_AXI] [get_bd_intf_pins s02_rs_0/S_AXI]
  connect_bd_intf_net -intf_net S03_AXI_1 [get_bd_intf_ports S03_AXI] [get_bd_intf_pins s03_rs_0/S_AXI]
  connect_bd_intf_net -intf_net m00_rs_M_AXI [get_bd_intf_ports M00_AXI] [get_bd_intf_pins m00_rs/M_AXI]
  connect_bd_intf_net -intf_net s00_rs_1_M_AXI [get_bd_intf_pins s00_rs_1/M_AXI] [get_bd_intf_pins xbar/S00_AXI]
  connect_bd_intf_net -intf_net s00_rs_M_AXI [get_bd_intf_pins s00_rs_0/M_AXI] [get_bd_intf_pins s00_width_conv/S_AXI]
  connect_bd_intf_net -intf_net s00_width_conv_M_AXI [get_bd_intf_pins s00_rs_1/S_AXI] [get_bd_intf_pins s00_width_conv/M_AXI]
  connect_bd_intf_net -intf_net s01_rs_1_M_AXI [get_bd_intf_pins s01_rs_1/M_AXI] [get_bd_intf_pins xbar/S01_AXI]
  connect_bd_intf_net -intf_net s01_rs_M_AXI [get_bd_intf_pins s01_rs_0/M_AXI] [get_bd_intf_pins s01_width_conv/S_AXI]
  connect_bd_intf_net -intf_net s01_width_conv_M_AXI [get_bd_intf_pins s01_rs_1/S_AXI] [get_bd_intf_pins s01_width_conv/M_AXI]
  connect_bd_intf_net -intf_net s02_rs_1_M_AXI [get_bd_intf_pins s02_rs_1/M_AXI] [get_bd_intf_pins xbar/S02_AXI]
  connect_bd_intf_net -intf_net s02_rs_M_AXI [get_bd_intf_pins s02_rs_0/M_AXI] [get_bd_intf_pins s02_width_conv/S_AXI]
  connect_bd_intf_net -intf_net s02_width_conv_M_AXI [get_bd_intf_pins s02_rs_1/S_AXI] [get_bd_intf_pins s02_width_conv/M_AXI]
  connect_bd_intf_net -intf_net s03_rs_1_M_AXI [get_bd_intf_pins s03_rs_1/M_AXI] [get_bd_intf_pins xbar/S03_AXI]
  connect_bd_intf_net -intf_net s03_rs_M_AXI [get_bd_intf_pins s03_rs_0/M_AXI] [get_bd_intf_pins s03_width_conv/S_AXI]
  connect_bd_intf_net -intf_net s03_width_conv_M_AXI [get_bd_intf_pins s03_rs_1/S_AXI] [get_bd_intf_pins s03_width_conv/M_AXI]
  connect_bd_intf_net -intf_net xbar_M00_AXI [get_bd_intf_pins m00_rs/S_AXI] [get_bd_intf_pins xbar/M00_AXI]

  # Create port connections
  connect_bd_net -net M00_AXI_ACLK_1 [get_bd_ports M00_AXI_ACLK] [get_bd_pins m00_rs/aclk] [get_bd_pins s00_rs_1/aclk] [get_bd_pins s00_width_conv/m_axi_aclk] [get_bd_pins s01_rs_1/aclk] [get_bd_pins s01_width_conv/m_axi_aclk] [get_bd_pins s02_rs_1/aclk] [get_bd_pins s02_width_conv/m_axi_aclk] [get_bd_pins s03_rs_1/aclk] [get_bd_pins s03_width_conv/m_axi_aclk] [get_bd_pins xbar/aclk]
  connect_bd_net -net M00_AXI_ARESETN_1 [get_bd_ports M00_AXI_ARESETN] [get_bd_pins m00_rs/aresetn] [get_bd_pins s00_rs_1/aresetn] [get_bd_pins s00_width_conv/m_axi_aresetn] [get_bd_pins s01_rs_1/aresetn] [get_bd_pins s01_width_conv/m_axi_aresetn] [get_bd_pins s02_rs_1/aresetn] [get_bd_pins s02_width_conv/m_axi_aresetn] [get_bd_pins s03_rs_1/aresetn] [get_bd_pins s03_width_conv/m_axi_aresetn] [get_bd_pins xbar/aresetn]
  connect_bd_net -net S00_AXI_ACLK_1 [get_bd_ports S00_AXI_ACLK] [get_bd_pins s00_rs_0/aclk] [get_bd_pins s00_width_conv/s_axi_aclk]
  connect_bd_net -net S00_AXI_ARESETN_1 [get_bd_ports S00_AXI_ARESETN] [get_bd_pins s00_rs_0/aresetn] [get_bd_pins s00_width_conv/s_axi_aresetn]
  connect_bd_net -net S01_AXI_ACLK_1 [get_bd_ports S01_AXI_ACLK] [get_bd_pins s01_rs_0/aclk] [get_bd_pins s01_width_conv/s_axi_aclk]
  connect_bd_net -net S01_AXI_ARESETN_1 [get_bd_ports S01_AXI_ARESETN] [get_bd_pins s01_rs_0/aresetn] [get_bd_pins s01_width_conv/s_axi_aresetn]
  connect_bd_net -net S02_AXI_ACLK_1 [get_bd_ports S02_AXI_ACLK] [get_bd_pins s02_rs_0/aclk] [get_bd_pins s02_width_conv/s_axi_aclk]
  connect_bd_net -net S02_AXI_ARESETN_1 [get_bd_ports S02_AXI_ARESETN] [get_bd_pins s02_rs_0/aresetn] [get_bd_pins s02_width_conv/s_axi_aresetn]
  connect_bd_net -net S03_AXI_ACLK_1 [get_bd_ports S03_AXI_ACLK] [get_bd_pins s03_rs_0/aclk] [get_bd_pins s03_width_conv/s_axi_aclk]
  connect_bd_net -net S03_AXI_ARESETN_1 [get_bd_ports S03_AXI_ARESETN] [get_bd_pins s03_rs_0/aresetn] [get_bd_pins s03_width_conv/s_axi_aresetn]

  # Create address segments
  assign_bd_address -offset 0x00000000 -range 0x40000000 -target_address_space [get_bd_addr_spaces S00_AXI] [get_bd_addr_segs M00_AXI/Reg] -force
  assign_bd_address -offset 0x00000000 -range 0x40000000 -target_address_space [get_bd_addr_spaces S01_AXI] [get_bd_addr_segs M00_AXI/Reg] -force
  assign_bd_address -offset 0x00000000 -range 0x40000000 -target_address_space [get_bd_addr_spaces S02_AXI] [get_bd_addr_segs M00_AXI/Reg] -force
  assign_bd_address -offset 0x00000000 -range 0x40000000 -target_address_space [get_bd_addr_spaces S03_AXI] [get_bd_addr_segs M00_AXI/Reg] -force


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


