# Hierarchical cell: mtu
proc create_hier_cell_mtu { parentCell nameHier numPorts } {

  if { $parentCell eq "" || $nameHier eq "" } {
     puts "ERROR: create_hier_cell_mtu() - Empty argument(s)!"
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

  # Create cells and wire everything up
  create_bd_pin -dir I -from [expr $numPorts * 32 - 1] -to 0 mtu_regs
  connect_bd_net -net mtu_regs_1 [get_bd_pins mtu_regs]
  # BUG: Vivado 2015.4 does not connect nets the first time with just the driver
  connect_bd_net -quiet -net mtu_regs_1 [get_bd_pins mtu_regs]

  for {set i 0} {$i < $numPorts} {incr i} {
     # Create instance: xlslice_0, and set properties
     set xlslice [ create_bd_cell -type ip -vlnv xilinx.com:ip:xlslice:1.0 xlslice_$i ]
     set_property -dict [ list \
        CONFIG.DIN_FROM [expr $i * 32 + 15] \
        CONFIG.DIN_TO [expr $i * 32] \
        CONFIG.DIN_WIDTH [expr $numPorts * 32] \
        CONFIG.DOUT_WIDTH {16} \
     ] $xlslice

     connect_bd_net -net mtu_regs_1 [get_bd_pins $xlslice/Din]

     create_bd_pin -dir O -from 15 -to 0 mtu$i
     connect_bd_net [get_bd_pins mtu$i] [get_bd_pins $xlslice/Dout]
  }

  # Restore current instance
  current_bd_instance $oldCurInst
}


