#
# Copyright 2008 Ettus Research LLC
# Copyright 2015 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: LGPL-3.0-or-later
#

proc set_props {process options} {
	if ![string compare $options ""] {
		return
	}
	set state 1
	foreach opt $options {
		if $state {
			set key $opt
			set state 0
		} else {
			puts ">>> Setting: $process\[$key\] = $opt"
			if ![string compare $process "Project"] {
				project set $key $opt
			} else {
				project set $key $opt -process $process
			}
			set state 1
		}
	}
}

if [file isfile $env(ISE_FILE)] {
	puts ">>> Opening project: $env(ISE_FILE)"
	project open $env(ISE_FILE)
} else {
	puts ">>> Creating project: $env(ISE_FILE)"
	project new $env(ISE_FILE)
	
	##################################################
	# Set the project properties
	##################################################
	set_props "Project" $env(PROJECT_PROPERTIES)
	
	##################################################
	# Add the sources
	##################################################
	foreach source $env(SOURCES) {
		puts ">>> Adding source to project: $source"
		xfile add $source
	}
	
	##################################################
	# Add the custom sources
	##################################################
	foreach source $env(CUSTOM_SRCS) {
		puts ">>> Adding custom source to project: $source"
		xfile add $source -include_global
	}
	
	##################################################
	# Set the top level module
	##################################################
	project set top $env(TOP_MODULE)
	
	##################################################
	# Set the process properties
	##################################################
	set_props "Synthesize - XST" $env(SYNTHESIZE_PROPERTIES)
	set_props "Translate" $env(TRANSLATE_PROPERTIES)
	set_props "Map" $env(MAP_PROPERTIES)
	set_props "Place & Route" $env(PLACE_ROUTE_PROPERTIES)
	set_props "Generate Post-Place & Route Static Timing" $env(STATIC_TIMING_PROPERTIES)
	set_props "Generate Programming File" $env(GEN_PROG_FILE_PROPERTIES)
	set_props "Generate Post-Place & Route Simulation Model" $env(SIM_MODEL_PROPERTIES)
}

if [string compare [lindex $argv 0] ""] {
	puts ">>> Running Process: [lindex $argv 0]"
	process run [lindex $argv 0]
}

project close
exit


