# Function definitions
proc ::connect_server { {hostname localhost} {port 3121} } {
    if { [string compare [current_hw_server -quiet] ""] != 0 } {
        disconnect_server        
    }    
    connect_hw_server -url $hostname:$port
}

proc ::disconnect_server { } {
    disconnect_hw_server [current_hw_server]
}

proc ::jtag_list {} {
    # Iterate through all hardware targets
    set hw_targets [get_hw_targets -of_objects [current_hw_server -quiet] -quiet]
    set idx_t 0
    foreach hw_target $hw_targets {
        puts "== Target${idx_t}: $hw_target =="
        open_hw_target $hw_target -quiet
        # Iterate through all hardware devices
        set hw_devices [get_hw_devices]
        set idx_d 0
        foreach hw_device $hw_devices {
            puts "--- Device${idx_d}: $hw_device (Address = ${idx_t}:${idx_d})"
            set idx_d [expr $idx_d + 1]
        }
        close_hw_target -quiet
        set idx_t [expr $idx_t + 1]
    }
}

proc ::jtag_program { filepath {serial "."} {address "0:0"} } {
    set idx_t [lindex [split $address :] 0]
    set idx_d [lindex [split $address :] 1]

    set hw_targets [get_hw_targets -of_objects [current_hw_server]]
    set hw_targets_regexp {}

    foreach target $hw_targets {
        if { [regexp $serial $target] } {
            set hw_targets_regexp [concat $hw_targets_regexp $target]
        }
    }

    set hw_target [lindex $hw_targets_regexp $idx_t]

    if { [string compare $hw_target ""] == 0 } {
        error "ERROR: Could not open hw_target $idx_t. Either the address $address is incorrect or the device is not connected."
    } else {
        open_hw_target $hw_target -quiet
    }

    set hw_device [lindex [get_hw_devices] $idx_d]
    if { [string compare $hw_device ""] == 0 } {
        close_hw_target -quiet
        error "ERROR: Could not open hw_device $idx_d. Either the address $address is incorrect or the device is not connected."
    } else {
        puts "- Target: $hw_target"
        puts "- Device: $hw_device"
        puts "- Filename: $filepath"
        puts "Programming..."
        current_hw_device $hw_device
        set_property PROBES.FILE {} [current_hw_device]
        set_property PROGRAM.FILE $filepath [current_hw_device]
        program_hw_devices [current_hw_device]
        close_hw_target -quiet
        puts "Programming DONE"
    }
}

# Initialization sequence
open_hw
connect_server

if [expr $argc > 0] {
    #Execute a command and exit
    set cmd [lindex $argv 0]
    if { [string compare $cmd "list"] == 0 } {
        jtag_list        
    } elseif { [string compare $cmd "program"] == 0 } {
        set filepath [lindex $argv 1]
        if [expr $argc == 3] {
            set serial [lindex $argv 2]
            jtag_program $filepath $serial
        } elseif [expr $argc > 3] {
            set serial [lindex $argv 2]
            set devaddr [lindex $argv 3]
            jtag_program $filepath $serial $devaddr
        } else {
            jtag_program $filepath
        }
    } else {
        error "Invalid command: $cmd"
    }
    disconnect_server
    exit
}
