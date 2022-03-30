
################################################################
# This is a generated script based on design: axi_inter_4x64_512_bd
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
# source axi_inter_4x64_512_bd_script.tcl

# If there is no project opened, this script will create a
# project, but make sure you do not have an existing project
# <./myproj/project_1.xpr> in the current working folder.

set list_projs [get_projects -quiet]
if { $list_projs eq "" } {
   create_project project_1 myproj -part xczu28dr-ffvg1517-1-e
}


# CHANGE DESIGN NAME HERE
variable design_name
set design_name axi_inter_4x64_512_bd

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
  set_property USER_COMMENTS.comment_0 "Note 1: The ID width of the AXI interconnect (xbar) master changes depending on the axi_interconnect optimization strategy. It will be zero for 'minimize area' and non-zero for 'maximize performance'.
Note 2: I have explicitly set ID width to 1 on the input ports to match our AXI master and to 4 on the output block to match the DRAM slave." [get_bd_designs $design_name]

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
xilinx.com:ip:axi_crossbar:2.1\
xilinx.com:ip:axi_dwidth_converter:2.1\
xilinx.com:ip:axi_register_slice:2.1\
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
  set M0_AXI [ create_bd_intf_port -mode Master -vlnv xilinx.com:interface:aximm_rtl:1.0 M0_AXI ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.DATA_WIDTH {512} \
   CONFIG.FREQ_HZ {350000000} \
   CONFIG.NUM_READ_OUTSTANDING {2} \
   CONFIG.NUM_WRITE_OUTSTANDING {2} \
   CONFIG.PROTOCOL {AXI4} \
   ] $M0_AXI

  set S0_AXI [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S0_AXI ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.ARUSER_WIDTH {0} \
   CONFIG.AWUSER_WIDTH {0} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {64} \
   CONFIG.FREQ_HZ {350000000} \
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
   ] $S0_AXI

  set S1_AXI [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S1_AXI ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.ARUSER_WIDTH {0} \
   CONFIG.AWUSER_WIDTH {0} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {64} \
   CONFIG.FREQ_HZ {350000000} \
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
   ] $S1_AXI

  set S2_AXI [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S2_AXI ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.ARUSER_WIDTH {0} \
   CONFIG.AWUSER_WIDTH {0} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {64} \
   CONFIG.FREQ_HZ {350000000} \
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
   ] $S2_AXI

  set S3_AXI [ create_bd_intf_port -mode Slave -vlnv xilinx.com:interface:aximm_rtl:1.0 S3_AXI ]
  set_property -dict [ list \
   CONFIG.ADDR_WIDTH {32} \
   CONFIG.ARUSER_WIDTH {0} \
   CONFIG.AWUSER_WIDTH {0} \
   CONFIG.BUSER_WIDTH {0} \
   CONFIG.DATA_WIDTH {64} \
   CONFIG.FREQ_HZ {350000000} \
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
   ] $S3_AXI


  # Create ports
  set M0_AXI_ACLK [ create_bd_port -dir I -type clk -freq_hz 350000000 M0_AXI_ACLK ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {M0_AXI} \
   CONFIG.ASSOCIATED_RESET {M0_AXI_ARESETN} \
 ] $M0_AXI_ACLK
  set M0_AXI_ARESETN [ create_bd_port -dir I -type rst M0_AXI_ARESETN ]
  set_property -dict [ list \
   CONFIG.POLARITY {ACTIVE_LOW} \
 ] $M0_AXI_ARESETN
  set S0_AXI_ACLK [ create_bd_port -dir I -type clk -freq_hz 350000000 S0_AXI_ACLK ]
  set_property -dict [ list \
   CONFIG.ASSOCIATED_BUSIF {S0_AXI:S1_AXI:S2_AXI:S3_AXI} \
   CONFIG.ASSOCIATED_RESET {S0_AXI_ARESETN} \
 ] $S0_AXI_ACLK
  set S0_AXI_ARESETN [ create_bd_port -dir I -type rst S0_AXI_ARESETN ]

  # Create instance: axi_crossbar_0, and set properties
  set axi_crossbar_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_crossbar:2.1 axi_crossbar_0 ]
  set_property -dict [ list \
   CONFIG.DATA_WIDTH {256} \
   CONFIG.ID_WIDTH {4} \
   CONFIG.M00_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M00_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A00_ADDR_WIDTH {0} \
   CONFIG.M01_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M01_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A00_ADDR_WIDTH {0} \
   CONFIG.M02_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M02_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A00_ADDR_WIDTH {0} \
   CONFIG.M03_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M03_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A00_ADDR_WIDTH {0} \
   CONFIG.M04_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M04_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A00_ADDR_WIDTH {0} \
   CONFIG.M05_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M05_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A00_ADDR_WIDTH {0} \
   CONFIG.M06_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M06_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A00_ADDR_WIDTH {0} \
   CONFIG.M07_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M07_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A00_ADDR_WIDTH {0} \
   CONFIG.M08_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M08_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A00_ADDR_WIDTH {0} \
   CONFIG.M09_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M09_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A00_ADDR_WIDTH {0} \
   CONFIG.M10_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M10_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A00_ADDR_WIDTH {0} \
   CONFIG.M11_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M11_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A00_ADDR_WIDTH {0} \
   CONFIG.M12_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M12_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A00_ADDR_WIDTH {0} \
   CONFIG.M13_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M13_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A00_ADDR_WIDTH {0} \
   CONFIG.M14_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M14_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A00_ADDR_WIDTH {0} \
   CONFIG.M15_A00_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A01_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A02_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A03_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A04_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A05_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A06_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A07_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A08_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A09_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A10_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A11_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A12_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A13_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A14_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.M15_A15_BASE_ADDR {0xffffffffffffffff} \
   CONFIG.NUM_MI {1} \
   CONFIG.NUM_SI {4} \
   CONFIG.S01_BASE_ID {0x00000004} \
   CONFIG.S02_BASE_ID {0x00000008} \
   CONFIG.S03_BASE_ID {0x0000000c} \
   CONFIG.S04_BASE_ID {0x00000010} \
   CONFIG.S05_BASE_ID {0x00000014} \
   CONFIG.S06_BASE_ID {0x00000018} \
   CONFIG.S07_BASE_ID {0x0000001c} \
   CONFIG.S08_BASE_ID {0x00000020} \
   CONFIG.S09_BASE_ID {0x00000024} \
   CONFIG.S10_BASE_ID {0x00000028} \
   CONFIG.S11_BASE_ID {0x0000002c} \
   CONFIG.S12_BASE_ID {0x00000030} \
   CONFIG.S13_BASE_ID {0x00000034} \
   CONFIG.S14_BASE_ID {0x00000038} \
   CONFIG.S15_BASE_ID {0x0000003c} \
 ] $axi_crossbar_0

  # Create instance: axi_dwidth_converter_0, and set properties
  set axi_dwidth_converter_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dwidth_converter:2.1 axi_dwidth_converter_0 ]
  set_property -dict [ list \
   CONFIG.FIFO_MODE {1} \
   CONFIG.MI_DATA_WIDTH {256} \
   CONFIG.SI_DATA_WIDTH {64} \
   CONFIG.SI_ID_WIDTH {1} \
   CONFIG.SYNCHRONIZATION_STAGES {3} \
 ] $axi_dwidth_converter_0

  # Create instance: axi_dwidth_converter_1, and set properties
  set axi_dwidth_converter_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dwidth_converter:2.1 axi_dwidth_converter_1 ]
  set_property -dict [ list \
   CONFIG.FIFO_MODE {1} \
   CONFIG.MI_DATA_WIDTH {256} \
   CONFIG.SI_DATA_WIDTH {64} \
   CONFIG.SI_ID_WIDTH {1} \
   CONFIG.SYNCHRONIZATION_STAGES {3} \
 ] $axi_dwidth_converter_1

  # Create instance: axi_dwidth_converter_2, and set properties
  set axi_dwidth_converter_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dwidth_converter:2.1 axi_dwidth_converter_2 ]
  set_property -dict [ list \
   CONFIG.FIFO_MODE {1} \
   CONFIG.MI_DATA_WIDTH {256} \
   CONFIG.SI_DATA_WIDTH {64} \
   CONFIG.SI_ID_WIDTH {1} \
   CONFIG.SYNCHRONIZATION_STAGES {3} \
 ] $axi_dwidth_converter_2

  # Create instance: axi_dwidth_converter_3, and set properties
  set axi_dwidth_converter_3 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dwidth_converter:2.1 axi_dwidth_converter_3 ]
  set_property -dict [ list \
   CONFIG.FIFO_MODE {1} \
   CONFIG.MI_DATA_WIDTH {256} \
   CONFIG.SI_DATA_WIDTH {64} \
   CONFIG.SI_ID_WIDTH {1} \
   CONFIG.SYNCHRONIZATION_STAGES {3} \
 ] $axi_dwidth_converter_3

  # Create instance: axi_dwidth_converter_4, and set properties
  set axi_dwidth_converter_4 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_dwidth_converter:2.1 axi_dwidth_converter_4 ]
  set_property -dict [ list \
   CONFIG.ACLK_ASYNC {1} \
   CONFIG.FIFO_MODE {2} \
   CONFIG.MI_DATA_WIDTH {512} \
   CONFIG.SI_DATA_WIDTH {256} \
   CONFIG.SI_ID_WIDTH {4} \
   CONFIG.SYNCHRONIZATION_STAGES {2} \
 ] $axi_dwidth_converter_4

  # Create instance: axi_register_slice_0, and set properties
  set axi_register_slice_0 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 axi_register_slice_0 ]
  set_property -dict [ list \
   CONFIG.ID_WIDTH {1} \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
 ] $axi_register_slice_0

  # Create instance: axi_register_slice_1, and set properties
  set axi_register_slice_1 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 axi_register_slice_1 ]
  set_property -dict [ list \
   CONFIG.ID_WIDTH {1} \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
 ] $axi_register_slice_1

  # Create instance: axi_register_slice_2, and set properties
  set axi_register_slice_2 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 axi_register_slice_2 ]
  set_property -dict [ list \
   CONFIG.ID_WIDTH {1} \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
 ] $axi_register_slice_2

  # Create instance: axi_register_slice_3, and set properties
  set axi_register_slice_3 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 axi_register_slice_3 ]
  set_property -dict [ list \
   CONFIG.ID_WIDTH {1} \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
 ] $axi_register_slice_3

  # Create instance: axi_register_slice_4, and set properties
  set axi_register_slice_4 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 axi_register_slice_4 ]
  set_property -dict [ list \
   CONFIG.ID_WIDTH {1} \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
 ] $axi_register_slice_4

  # Create instance: axi_register_slice_5, and set properties
  set axi_register_slice_5 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 axi_register_slice_5 ]
  set_property -dict [ list \
   CONFIG.ID_WIDTH {1} \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
 ] $axi_register_slice_5

  # Create instance: axi_register_slice_6, and set properties
  set axi_register_slice_6 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 axi_register_slice_6 ]
  set_property -dict [ list \
   CONFIG.ID_WIDTH {1} \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
 ] $axi_register_slice_6

  # Create instance: axi_register_slice_7, and set properties
  set axi_register_slice_7 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 axi_register_slice_7 ]
  set_property -dict [ list \
   CONFIG.ID_WIDTH {1} \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
 ] $axi_register_slice_7

  # Create instance: axi_register_slice_8, and set properties
  set axi_register_slice_8 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 axi_register_slice_8 ]
  set_property -dict [ list \
   CONFIG.ID_WIDTH {4} \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
 ] $axi_register_slice_8

  # Create instance: axi_register_slice_9, and set properties
  set axi_register_slice_9 [ create_bd_cell -type ip -vlnv xilinx.com:ip:axi_register_slice:2.1 axi_register_slice_9 ]
  set_property -dict [ list \
   CONFIG.ID_WIDTH {4} \
   CONFIG.REG_AR {1} \
   CONFIG.REG_AW {1} \
   CONFIG.REG_B {1} \
 ] $axi_register_slice_9

  # Create interface connections
  connect_bd_intf_net -intf_net S0_AXI_1 [get_bd_intf_ports S0_AXI] [get_bd_intf_pins axi_register_slice_0/S_AXI]
  connect_bd_intf_net -intf_net S1_AXI_1 [get_bd_intf_ports S1_AXI] [get_bd_intf_pins axi_register_slice_1/S_AXI]
  connect_bd_intf_net -intf_net S2_AXI_1 [get_bd_intf_ports S2_AXI] [get_bd_intf_pins axi_register_slice_2/S_AXI]
  connect_bd_intf_net -intf_net S3_AXI_1 [get_bd_intf_ports S3_AXI] [get_bd_intf_pins axi_register_slice_3/S_AXI]
  connect_bd_intf_net -intf_net axi_crossbar_0_M00_AXI [get_bd_intf_pins axi_crossbar_0/M00_AXI] [get_bd_intf_pins axi_register_slice_8/S_AXI]
  connect_bd_intf_net -intf_net axi_dwidth_converter_0_M_AXI [get_bd_intf_pins axi_dwidth_converter_0/M_AXI] [get_bd_intf_pins axi_register_slice_4/S_AXI]
  connect_bd_intf_net -intf_net axi_dwidth_converter_1_M_AXI [get_bd_intf_pins axi_dwidth_converter_1/M_AXI] [get_bd_intf_pins axi_register_slice_5/S_AXI]
  connect_bd_intf_net -intf_net axi_dwidth_converter_2_M_AXI1 [get_bd_intf_pins axi_dwidth_converter_2/M_AXI] [get_bd_intf_pins axi_register_slice_6/S_AXI]
  connect_bd_intf_net -intf_net axi_dwidth_converter_3_M_AXI [get_bd_intf_pins axi_dwidth_converter_3/M_AXI] [get_bd_intf_pins axi_register_slice_7/S_AXI]
  connect_bd_intf_net -intf_net axi_dwidth_converter_4_M_AXI [get_bd_intf_pins axi_dwidth_converter_4/M_AXI] [get_bd_intf_pins axi_register_slice_9/S_AXI]
  connect_bd_intf_net -intf_net axi_register_slice_0_M_AXI [get_bd_intf_pins axi_dwidth_converter_0/S_AXI] [get_bd_intf_pins axi_register_slice_0/M_AXI]
  connect_bd_intf_net -intf_net axi_register_slice_1_M_AXI [get_bd_intf_pins axi_dwidth_converter_1/S_AXI] [get_bd_intf_pins axi_register_slice_1/M_AXI]
  connect_bd_intf_net -intf_net axi_register_slice_2_M_AXI [get_bd_intf_pins axi_dwidth_converter_2/S_AXI] [get_bd_intf_pins axi_register_slice_2/M_AXI]
  connect_bd_intf_net -intf_net axi_register_slice_3_M_AXI [get_bd_intf_pins axi_dwidth_converter_3/S_AXI] [get_bd_intf_pins axi_register_slice_3/M_AXI]
  connect_bd_intf_net -intf_net axi_register_slice_4_M_AXI [get_bd_intf_pins axi_crossbar_0/S00_AXI] [get_bd_intf_pins axi_register_slice_4/M_AXI]
  connect_bd_intf_net -intf_net axi_register_slice_5_M_AXI [get_bd_intf_pins axi_crossbar_0/S01_AXI] [get_bd_intf_pins axi_register_slice_5/M_AXI]
  connect_bd_intf_net -intf_net axi_register_slice_6_M_AXI [get_bd_intf_pins axi_crossbar_0/S02_AXI] [get_bd_intf_pins axi_register_slice_6/M_AXI]
  connect_bd_intf_net -intf_net axi_register_slice_7_M_AXI [get_bd_intf_pins axi_crossbar_0/S03_AXI] [get_bd_intf_pins axi_register_slice_7/M_AXI]
  connect_bd_intf_net -intf_net axi_register_slice_8_M_AXI [get_bd_intf_pins axi_dwidth_converter_4/S_AXI] [get_bd_intf_pins axi_register_slice_8/M_AXI]
  connect_bd_intf_net -intf_net axi_register_slice_9_M_AXI [get_bd_intf_ports M0_AXI] [get_bd_intf_pins axi_register_slice_9/M_AXI]

  # Create port connections
  connect_bd_net -net M0_AXI_ACLK_1 [get_bd_ports M0_AXI_ACLK] [get_bd_pins axi_dwidth_converter_4/m_axi_aclk] [get_bd_pins axi_register_slice_9/aclk]
  connect_bd_net -net M0_AXI_ARESETN_1 [get_bd_ports M0_AXI_ARESETN] [get_bd_pins axi_dwidth_converter_4/m_axi_aresetn] [get_bd_pins axi_register_slice_9/aresetn]
  connect_bd_net -net S0_AXI_ACLK_1 [get_bd_ports S0_AXI_ACLK] [get_bd_pins axi_crossbar_0/aclk] [get_bd_pins axi_dwidth_converter_0/s_axi_aclk] [get_bd_pins axi_dwidth_converter_1/s_axi_aclk] [get_bd_pins axi_dwidth_converter_2/s_axi_aclk] [get_bd_pins axi_dwidth_converter_3/s_axi_aclk] [get_bd_pins axi_dwidth_converter_4/s_axi_aclk] [get_bd_pins axi_register_slice_0/aclk] [get_bd_pins axi_register_slice_1/aclk] [get_bd_pins axi_register_slice_2/aclk] [get_bd_pins axi_register_slice_3/aclk] [get_bd_pins axi_register_slice_4/aclk] [get_bd_pins axi_register_slice_5/aclk] [get_bd_pins axi_register_slice_6/aclk] [get_bd_pins axi_register_slice_7/aclk] [get_bd_pins axi_register_slice_8/aclk]
  connect_bd_net -net S0_AXI_ARESETN_1 [get_bd_ports S0_AXI_ARESETN] [get_bd_pins axi_crossbar_0/aresetn] [get_bd_pins axi_dwidth_converter_0/s_axi_aresetn] [get_bd_pins axi_dwidth_converter_1/s_axi_aresetn] [get_bd_pins axi_dwidth_converter_2/s_axi_aresetn] [get_bd_pins axi_dwidth_converter_3/s_axi_aresetn] [get_bd_pins axi_dwidth_converter_4/s_axi_aresetn] [get_bd_pins axi_register_slice_0/aresetn] [get_bd_pins axi_register_slice_1/aresetn] [get_bd_pins axi_register_slice_2/aresetn] [get_bd_pins axi_register_slice_3/aresetn] [get_bd_pins axi_register_slice_4/aresetn] [get_bd_pins axi_register_slice_5/aresetn] [get_bd_pins axi_register_slice_6/aresetn] [get_bd_pins axi_register_slice_7/aresetn] [get_bd_pins axi_register_slice_8/aresetn]

  # Create address segments
  assign_bd_address -offset 0x00000000 -range 0x000100000000 -target_address_space [get_bd_addr_spaces S0_AXI] [get_bd_addr_segs M0_AXI/Reg] -force
  assign_bd_address -offset 0x00000000 -range 0x000100000000 -target_address_space [get_bd_addr_spaces S1_AXI] [get_bd_addr_segs M0_AXI/Reg] -force
  assign_bd_address -offset 0x00000000 -range 0x000100000000 -target_address_space [get_bd_addr_spaces S2_AXI] [get_bd_addr_segs M0_AXI/Reg] -force
  assign_bd_address -offset 0x00000000 -range 0x000100000000 -target_address_space [get_bd_addr_spaces S3_AXI] [get_bd_addr_segs M0_AXI/Reg] -force


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


