# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  ipgui::add_param $IPINST -name "NUM_REGS" -parent ${Page_0}


}

proc update_PARAM_VALUE.NUM_REGS { PARAM_VALUE.NUM_REGS } {
	# Procedure called to update NUM_REGS when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.NUM_REGS { PARAM_VALUE.NUM_REGS } {
	# Procedure called to validate NUM_REGS
	return true
}


proc update_MODELPARAM_VALUE.NUM_REGS { MODELPARAM_VALUE.NUM_REGS PARAM_VALUE.NUM_REGS } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.NUM_REGS}] ${MODELPARAM_VALUE.NUM_REGS}
}

