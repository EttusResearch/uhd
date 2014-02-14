package require cmdline
package require xilinxTGI 1.0
package require xilinxGeneratorUtils 1.0
namespace import ::xilinx::GeneratorUtils::getConstant

proc main { argc argv } {
   namespace import -force ::xilinx::TGI::message
   message info "Running axi_interconnect_v1_06_a_ucfgen.tcl"


   # Parse the contents of argv into an array called 'parameters'
   set options {
      {componentInstanceID.arg ""    "Element ID to generate"}
      {verbose                       "Enable verbose messaging"}      
   }
   set usage ": axi_interconnect_v1_06_a_ucfgen.tcl \[options]...\noptions:"
   array set parameters [::cmdline::getoptions argv $options $usage]
   
   # Ensure that the componentInstanceID has been specified in the array 'parameters'
   set componentInstanceID $parameters(componentInstanceID)
   if { [string equal $componentInstanceID ""] } {
      message error "Missing required parameter: -componentInstanceID"
      return $::xilinx::coreu::ERROR
   }
  
   set instname ""
   set num_s 0
   set ict_period ""
   set any_async 0
   set any_sync 0
   set s_async [list 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
   set s_ratio [list 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0]
   set m_async 0
   set m_ratio 0

   foreach parameterID [::xilinx::TGI::getParameterIDs $componentInstanceID] {
      set parameterName [::xilinx::TGI::getName $parameterID]
      set parameterValue [::xilinx::TGI::getValue $parameterID]
      
      if {[string equal -nocase $parameterName "Component_Name"]} {
        set instname [string tolower $parameterValue]
        message DEBUG "Set Component Name = $instname"
      } elseif {[string equal $parameterName "NUM_SLAVE_PORTS"]} {
        set num_s $parameterValue
        message DEBUG "Set $parameterName = $parameterValue"
      } elseif {[string equal $parameterName "ACLK_PERIOD"]} {
        set ict_period $parameterValue
        message DEBUG "Set $parameterName = $parameterValue"
      } elseif {[string equal $parameterName "M00_AXI_IS_ACLK_ASYNC"]} {
        message DEBUG "Set $parameterName = $parameterValue"
        if {$parameterValue} {
          set m_async 1
          set any_async 1
        }
      } elseif {[string equal $parameterName "M00_AXI_ACLK_RATIO"]} {
        message DEBUG "Set $parameterName = $parameterValue"
        if {![string equal "1:1" $parameterValue]} {
          if {[string first "1:" $parameterValue] == 0} {
            set m_ratio [string range $parameterValue 2 end]
          } else {
            set m_ratio 1
          }
          set any_sync 1
          message DEBUG "Set M00 ratio = $m_ratio"
        }
      } elseif {[string match "S*_AXI_IS_ACLK_ASYNC" $parameterName]} {
        message DEBUG "Set $parameterName = $parameterValue"
        if {$parameterValue} {
          if {[string index $parameterName 1] == "0"} {
            set i [string range $parameterName 2 2]
          } else {
            set i [string range $parameterName 1 2]
          }
          set s_async [lreplace $s_async $i $i 1]
          set any_async 1
        }
      } elseif {[string match "S*_AXI_ACLK_RATIO" $parameterName]} {
        message DEBUG "Set $parameterName = $parameterValue"
        if {![string equal "1:1" $parameterValue]} {
          if {[string index $parameterName 1] == "0"} {
            set i [string range $parameterName 2 2]
          } else {
            set i [string range $parameterName 1 2]
          }
          if {[string first "1:" $parameterValue] == 0} {
            set ratio [string range $parameterValue 2 end]
          } else {
            set ratio 1
          }
          set s_ratio [lreplace $s_ratio $i $i $ratio]
          set any_sync 1
          message DEBUG "Set [string range $parameterName 0 2] ratio = $ratio"
        }
      } else {
        message DEBUG "Ignoring $parameterName"
      }
   }
   
   if {[string equal $instname ""]} {
      message error "Missing required parameter: Component_Name"
      return $::xilinx::coreu::ERROR
   }
      set elaborationDirectory [::xilinx::GeneratorUtils::getElaborationDirectory          $componentInstanceID true]
   file mkdir $elaborationDirectory
   set filename [file join $elaborationDirectory "${instname}.ucf"]
   set outputFile [open $filename "w"]
   message DEBUG "Opened file $filename"
   

   if {$num_s == 0} {
      message error "Missing positive value required for parameter: NUM_SLAVE_PORTS"
      return $::xilinx::coreu::ERROR
   }

  # INTERCONNECT_ARESETN input is resynchronized and is excluded from timing analysis.
    puts $outputFile "NET \"*_resync*\" TNM = FFS \"${instname}_reset_resync\";"
    puts $outputFile "NET \"*INTERCONNECT_ARESETN\" TNM = FFS \"${instname}_reset_resync\";"
    puts $outputFile "TIMESPEC \"TS_${instname}_reset_resync\" = TO \"${instname}_reset_resync\" TIG;"
    
  # Sync clock conversions no longer require timing constraints, as of v1.06.a.

  # Write tspecs for async clock conversion (if any)
    if {$any_async} {
      message info "Generating core-level timing constraints for asynchronous clock conversions in ${instname}."
      puts $outputFile "INST \"*_converter_bank/*clock_conv_inst/*asyncfifo_*/*mem/*dout_i_?\" TNM = \"${instname}_async_clock_conv_FFDEST\";"
      puts $outputFile "INST \"*_converter_bank/*clock_conv_inst/*asyncfifo_*/*mem/*dout_i_??\" TNM = \"${instname}_async_clock_conv_FFDEST\";"
      puts $outputFile "INST \"*_converter_bank/*clock_conv_inst/*asyncfifo_*/*mem/*dout_i_???\" TNM = \"${instname}_async_clock_conv_FFDEST\";"
      puts $outputFile "INST \"*_converter_bank/*clock_conv_inst/*asyncfifo_*/*mem/*dout_i_????\" TNM = \"${instname}_async_clock_conv_FFDEST\";"
      if {$ict_period > 0} {
        set period "${ict_period} PS"
        puts $outputFile "TIMESPEC \"TS_${instname}_async_clock_conv\" = FROM RAMS TO \"${instname}_async_clock_conv_FFDEST\" ${period} DATAPATHONLY;"
      } else {
        message info "Parameter ACLK_PERIOD not specified; generating TIG constraint for async clock-converter pathways."
        puts $outputFile "TIMESPEC \"TS_${instname}_async_clock_conv\" = FROM RAMS TO \"${instname}_async_clock_conv_FFDEST\" TIG;"
      }
      puts $outputFile "NET \"*_converter_bank/*clock_conv_inst/*asyncfifo_*/wr_pntr_gc*\" TIG;"
      puts $outputFile "NET \"*_converter_bank/*clock_conv_inst/*asyncfifo_*/rd_pntr_gc*\" TIG;"
      puts $outputFile "NET \"*_converter_bank/*clock_conv_inst/*asyncfifo_*/*clkx/wr_q*\" TIG;"
      puts $outputFile "NET \"*_converter_bank/*clock_conv_inst/*asyncfifo_*/*clkx/rd_q*\" TIG;"
      puts $outputFile "NET \"*_converter_bank/*clock_conv_inst/*async_conv_reset\" TIG;"
    } else {
      message info "No asynchronous clock conversions in ${instname}."
    }
    
   close $outputFile
   message info "Writing core-level timing constraint file $filename."
   
   namespace import -force ::xilinx::TGI::message
   return $::xilinx::coreu::OK 
}

return [main $argc $argv]
