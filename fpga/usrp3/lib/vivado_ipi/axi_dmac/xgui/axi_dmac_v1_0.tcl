
# Loading additional proc with user specified bodies to compute parameter values.
source [file join [file dirname [file dirname [info script]]] gui/axi_dmac_v1_0.gtcl]

# Definitional proc to organize widgets for parameters.
proc init_gui { IPINST } {
  ipgui::add_param $IPINST -name "Component_Name"
  #Adding Page
  set Page_0 [ipgui::add_page $IPINST -name "Page 0"]
  #Adding Group
  set DMA_Endpoint_Configuration [ipgui::add_group $IPINST -name "DMA Endpoint Configuration" -parent ${Page_0} -layout horizontal]
  #Adding Group
  set Source [ipgui::add_group $IPINST -name "Source" -parent ${DMA_Endpoint_Configuration}]
  ipgui::add_param $IPINST -name "DMA_TYPE_SRC" -parent ${Source} -widget comboBox
  ipgui::add_param $IPINST -name "DMA_AXI_PROTOCOL_SRC" -parent ${Source} -widget comboBox
  ipgui::add_param $IPINST -name "DMA_DATA_WIDTH_SRC" -parent ${Source}
  ipgui::add_param $IPINST -name "AXI_SLICE_SRC" -parent ${Source}
  ipgui::add_param $IPINST -name "SYNC_TRANSFER_START" -parent ${Source}

  #Adding Group
  set Destination [ipgui::add_group $IPINST -name "Destination" -parent ${DMA_Endpoint_Configuration}]
  ipgui::add_param $IPINST -name "DMA_TYPE_DEST" -parent ${Destination} -widget comboBox
  ipgui::add_param $IPINST -name "DMA_AXI_PROTOCOL_DEST" -parent ${Destination} -widget comboBox
  ipgui::add_param $IPINST -name "DMA_DATA_WIDTH_DEST" -parent ${Destination}
  ipgui::add_param $IPINST -name "AXI_SLICE_DEST" -parent ${Destination}


  #Adding Group
  set General_Configuration [ipgui::add_group $IPINST -name "General Configuration" -parent ${Page_0}]
  ipgui::add_param $IPINST -name "ID" -parent ${General_Configuration}
  ipgui::add_param $IPINST -name "DMA_LENGTH_WIDTH" -parent ${General_Configuration}
  ipgui::add_param $IPINST -name "FIFO_SIZE" -parent ${General_Configuration}
  ipgui::add_param $IPINST -name "MAX_BYTES_PER_BURST" -parent ${General_Configuration}

  #Adding Group
  set Features [ipgui::add_group $IPINST -name "Features" -parent ${Page_0}]
  ipgui::add_param $IPINST -name "CYCLIC" -parent ${Features}
  ipgui::add_param $IPINST -name "DMA_2D_TRANSFER" -parent ${Features}

  #Adding Group
  set Clock_Domain_Configuration [ipgui::add_group $IPINST -name "Clock Domain Configuration" -parent ${Page_0}]
  ipgui::add_param $IPINST -name "ASYNC_CLK_REQ_SRC" -parent ${Clock_Domain_Configuration}
  ipgui::add_param $IPINST -name "ASYNC_CLK_SRC_DEST" -parent ${Clock_Domain_Configuration}
  ipgui::add_param $IPINST -name "ASYNC_CLK_DEST_REQ" -parent ${Clock_Domain_Configuration}



}

proc update_PARAM_VALUE.DMA_AXI_PROTOCOL_DEST { PARAM_VALUE.DMA_AXI_PROTOCOL_DEST PARAM_VALUE.DMA_TYPE_DEST } {
	# Procedure called to update DMA_AXI_PROTOCOL_DEST when any of the dependent parameters in the arguments change
	
	set DMA_AXI_PROTOCOL_DEST ${PARAM_VALUE.DMA_AXI_PROTOCOL_DEST}
	set DMA_TYPE_DEST ${PARAM_VALUE.DMA_TYPE_DEST}
	set values(DMA_TYPE_DEST) [get_property value $DMA_TYPE_DEST]
	if { [gen_USERPARAMETER_DMA_AXI_PROTOCOL_DEST_ENABLEMENT $values(DMA_TYPE_DEST)] } {
		set_property enabled true $DMA_AXI_PROTOCOL_DEST
	} else {
		set_property enabled false $DMA_AXI_PROTOCOL_DEST
	}
}

proc validate_PARAM_VALUE.DMA_AXI_PROTOCOL_DEST { PARAM_VALUE.DMA_AXI_PROTOCOL_DEST } {
	# Procedure called to validate DMA_AXI_PROTOCOL_DEST
	return true
}

proc update_PARAM_VALUE.DMA_AXI_PROTOCOL_SRC { PARAM_VALUE.DMA_AXI_PROTOCOL_SRC PARAM_VALUE.DMA_TYPE_SRC } {
	# Procedure called to update DMA_AXI_PROTOCOL_SRC when any of the dependent parameters in the arguments change
	
	set DMA_AXI_PROTOCOL_SRC ${PARAM_VALUE.DMA_AXI_PROTOCOL_SRC}
	set DMA_TYPE_SRC ${PARAM_VALUE.DMA_TYPE_SRC}
	set values(DMA_TYPE_SRC) [get_property value $DMA_TYPE_SRC]
	if { [gen_USERPARAMETER_DMA_AXI_PROTOCOL_SRC_ENABLEMENT $values(DMA_TYPE_SRC)] } {
		set_property enabled true $DMA_AXI_PROTOCOL_SRC
	} else {
		set_property enabled false $DMA_AXI_PROTOCOL_SRC
	}
}

proc validate_PARAM_VALUE.DMA_AXI_PROTOCOL_SRC { PARAM_VALUE.DMA_AXI_PROTOCOL_SRC } {
	# Procedure called to validate DMA_AXI_PROTOCOL_SRC
	return true
}

proc update_PARAM_VALUE.SYNC_TRANSFER_START { PARAM_VALUE.SYNC_TRANSFER_START PARAM_VALUE.DMA_TYPE_SRC } {
	# Procedure called to update SYNC_TRANSFER_START when any of the dependent parameters in the arguments change
	
	set SYNC_TRANSFER_START ${PARAM_VALUE.SYNC_TRANSFER_START}
	set DMA_TYPE_SRC ${PARAM_VALUE.DMA_TYPE_SRC}
	set values(DMA_TYPE_SRC) [get_property value $DMA_TYPE_SRC]
	if { [gen_USERPARAMETER_SYNC_TRANSFER_START_ENABLEMENT $values(DMA_TYPE_SRC)] } {
		set_property enabled true $SYNC_TRANSFER_START
	} else {
		set_property enabled false $SYNC_TRANSFER_START
	}
}

proc validate_PARAM_VALUE.SYNC_TRANSFER_START { PARAM_VALUE.SYNC_TRANSFER_START } {
	# Procedure called to validate SYNC_TRANSFER_START
	return true
}

proc update_PARAM_VALUE.ASYNC_CLK_DEST_REQ { PARAM_VALUE.ASYNC_CLK_DEST_REQ } {
	# Procedure called to update ASYNC_CLK_DEST_REQ when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.ASYNC_CLK_DEST_REQ { PARAM_VALUE.ASYNC_CLK_DEST_REQ } {
	# Procedure called to validate ASYNC_CLK_DEST_REQ
	return true
}

proc update_PARAM_VALUE.ASYNC_CLK_REQ_SRC { PARAM_VALUE.ASYNC_CLK_REQ_SRC } {
	# Procedure called to update ASYNC_CLK_REQ_SRC when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.ASYNC_CLK_REQ_SRC { PARAM_VALUE.ASYNC_CLK_REQ_SRC } {
	# Procedure called to validate ASYNC_CLK_REQ_SRC
	return true
}

proc update_PARAM_VALUE.ASYNC_CLK_SRC_DEST { PARAM_VALUE.ASYNC_CLK_SRC_DEST } {
	# Procedure called to update ASYNC_CLK_SRC_DEST when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.ASYNC_CLK_SRC_DEST { PARAM_VALUE.ASYNC_CLK_SRC_DEST } {
	# Procedure called to validate ASYNC_CLK_SRC_DEST
	return true
}

proc update_PARAM_VALUE.AXI_SLICE_DEST { PARAM_VALUE.AXI_SLICE_DEST } {
	# Procedure called to update AXI_SLICE_DEST when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.AXI_SLICE_DEST { PARAM_VALUE.AXI_SLICE_DEST } {
	# Procedure called to validate AXI_SLICE_DEST
	return true
}

proc update_PARAM_VALUE.AXI_SLICE_SRC { PARAM_VALUE.AXI_SLICE_SRC } {
	# Procedure called to update AXI_SLICE_SRC when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.AXI_SLICE_SRC { PARAM_VALUE.AXI_SLICE_SRC } {
	# Procedure called to validate AXI_SLICE_SRC
	return true
}

proc update_PARAM_VALUE.CYCLIC { PARAM_VALUE.CYCLIC } {
	# Procedure called to update CYCLIC when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.CYCLIC { PARAM_VALUE.CYCLIC } {
	# Procedure called to validate CYCLIC
	return true
}

proc update_PARAM_VALUE.DMA_2D_TRANSFER { PARAM_VALUE.DMA_2D_TRANSFER } {
	# Procedure called to update DMA_2D_TRANSFER when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.DMA_2D_TRANSFER { PARAM_VALUE.DMA_2D_TRANSFER } {
	# Procedure called to validate DMA_2D_TRANSFER
	return true
}

proc update_PARAM_VALUE.DMA_DATA_WIDTH_DEST { PARAM_VALUE.DMA_DATA_WIDTH_DEST } {
	# Procedure called to update DMA_DATA_WIDTH_DEST when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.DMA_DATA_WIDTH_DEST { PARAM_VALUE.DMA_DATA_WIDTH_DEST } {
	# Procedure called to validate DMA_DATA_WIDTH_DEST
	return true
}

proc update_PARAM_VALUE.DMA_DATA_WIDTH_SRC { PARAM_VALUE.DMA_DATA_WIDTH_SRC } {
	# Procedure called to update DMA_DATA_WIDTH_SRC when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.DMA_DATA_WIDTH_SRC { PARAM_VALUE.DMA_DATA_WIDTH_SRC } {
	# Procedure called to validate DMA_DATA_WIDTH_SRC
	return true
}

proc update_PARAM_VALUE.DMA_LENGTH_WIDTH { PARAM_VALUE.DMA_LENGTH_WIDTH } {
	# Procedure called to update DMA_LENGTH_WIDTH when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.DMA_LENGTH_WIDTH { PARAM_VALUE.DMA_LENGTH_WIDTH } {
	# Procedure called to validate DMA_LENGTH_WIDTH
	return true
}

proc update_PARAM_VALUE.DMA_TYPE_DEST { PARAM_VALUE.DMA_TYPE_DEST } {
	# Procedure called to update DMA_TYPE_DEST when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.DMA_TYPE_DEST { PARAM_VALUE.DMA_TYPE_DEST } {
	# Procedure called to validate DMA_TYPE_DEST
	return true
}

proc update_PARAM_VALUE.DMA_TYPE_SRC { PARAM_VALUE.DMA_TYPE_SRC } {
	# Procedure called to update DMA_TYPE_SRC when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.DMA_TYPE_SRC { PARAM_VALUE.DMA_TYPE_SRC } {
	# Procedure called to validate DMA_TYPE_SRC
	return true
}

proc update_PARAM_VALUE.FIFO_SIZE { PARAM_VALUE.FIFO_SIZE } {
	# Procedure called to update FIFO_SIZE when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.FIFO_SIZE { PARAM_VALUE.FIFO_SIZE } {
	# Procedure called to validate FIFO_SIZE
	return true
}

proc update_PARAM_VALUE.ID { PARAM_VALUE.ID } {
	# Procedure called to update ID when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.ID { PARAM_VALUE.ID } {
	# Procedure called to validate ID
	return true
}

proc update_PARAM_VALUE.MAX_BYTES_PER_BURST { PARAM_VALUE.MAX_BYTES_PER_BURST } {
	# Procedure called to update MAX_BYTES_PER_BURST when any of the dependent parameters in the arguments change
}

proc validate_PARAM_VALUE.MAX_BYTES_PER_BURST { PARAM_VALUE.MAX_BYTES_PER_BURST } {
	# Procedure called to validate MAX_BYTES_PER_BURST
	return true
}


proc update_MODELPARAM_VALUE.ID { MODELPARAM_VALUE.ID PARAM_VALUE.ID } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.ID}] ${MODELPARAM_VALUE.ID}
}

proc update_MODELPARAM_VALUE.DMA_DATA_WIDTH_SRC { MODELPARAM_VALUE.DMA_DATA_WIDTH_SRC PARAM_VALUE.DMA_DATA_WIDTH_SRC } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DMA_DATA_WIDTH_SRC}] ${MODELPARAM_VALUE.DMA_DATA_WIDTH_SRC}
}

proc update_MODELPARAM_VALUE.DMA_DATA_WIDTH_DEST { MODELPARAM_VALUE.DMA_DATA_WIDTH_DEST PARAM_VALUE.DMA_DATA_WIDTH_DEST } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DMA_DATA_WIDTH_DEST}] ${MODELPARAM_VALUE.DMA_DATA_WIDTH_DEST}
}

proc update_MODELPARAM_VALUE.DMA_LENGTH_WIDTH { MODELPARAM_VALUE.DMA_LENGTH_WIDTH PARAM_VALUE.DMA_LENGTH_WIDTH } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DMA_LENGTH_WIDTH}] ${MODELPARAM_VALUE.DMA_LENGTH_WIDTH}
}

proc update_MODELPARAM_VALUE.DMA_2D_TRANSFER { MODELPARAM_VALUE.DMA_2D_TRANSFER PARAM_VALUE.DMA_2D_TRANSFER } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DMA_2D_TRANSFER}] ${MODELPARAM_VALUE.DMA_2D_TRANSFER}
}

proc update_MODELPARAM_VALUE.ASYNC_CLK_REQ_SRC { MODELPARAM_VALUE.ASYNC_CLK_REQ_SRC PARAM_VALUE.ASYNC_CLK_REQ_SRC } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.ASYNC_CLK_REQ_SRC}] ${MODELPARAM_VALUE.ASYNC_CLK_REQ_SRC}
}

proc update_MODELPARAM_VALUE.ASYNC_CLK_SRC_DEST { MODELPARAM_VALUE.ASYNC_CLK_SRC_DEST PARAM_VALUE.ASYNC_CLK_SRC_DEST } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.ASYNC_CLK_SRC_DEST}] ${MODELPARAM_VALUE.ASYNC_CLK_SRC_DEST}
}

proc update_MODELPARAM_VALUE.ASYNC_CLK_DEST_REQ { MODELPARAM_VALUE.ASYNC_CLK_DEST_REQ PARAM_VALUE.ASYNC_CLK_DEST_REQ } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.ASYNC_CLK_DEST_REQ}] ${MODELPARAM_VALUE.ASYNC_CLK_DEST_REQ}
}

proc update_MODELPARAM_VALUE.AXI_SLICE_DEST { MODELPARAM_VALUE.AXI_SLICE_DEST PARAM_VALUE.AXI_SLICE_DEST } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.AXI_SLICE_DEST}] ${MODELPARAM_VALUE.AXI_SLICE_DEST}
}

proc update_MODELPARAM_VALUE.AXI_SLICE_SRC { MODELPARAM_VALUE.AXI_SLICE_SRC PARAM_VALUE.AXI_SLICE_SRC } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.AXI_SLICE_SRC}] ${MODELPARAM_VALUE.AXI_SLICE_SRC}
}

proc update_MODELPARAM_VALUE.SYNC_TRANSFER_START { MODELPARAM_VALUE.SYNC_TRANSFER_START PARAM_VALUE.SYNC_TRANSFER_START } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.SYNC_TRANSFER_START}] ${MODELPARAM_VALUE.SYNC_TRANSFER_START}
}

proc update_MODELPARAM_VALUE.CYCLIC { MODELPARAM_VALUE.CYCLIC PARAM_VALUE.CYCLIC } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.CYCLIC}] ${MODELPARAM_VALUE.CYCLIC}
}

proc update_MODELPARAM_VALUE.DMA_AXI_PROTOCOL_DEST { MODELPARAM_VALUE.DMA_AXI_PROTOCOL_DEST PARAM_VALUE.DMA_AXI_PROTOCOL_DEST } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DMA_AXI_PROTOCOL_DEST}] ${MODELPARAM_VALUE.DMA_AXI_PROTOCOL_DEST}
}

proc update_MODELPARAM_VALUE.DMA_AXI_PROTOCOL_SRC { MODELPARAM_VALUE.DMA_AXI_PROTOCOL_SRC PARAM_VALUE.DMA_AXI_PROTOCOL_SRC } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DMA_AXI_PROTOCOL_SRC}] ${MODELPARAM_VALUE.DMA_AXI_PROTOCOL_SRC}
}

proc update_MODELPARAM_VALUE.DMA_TYPE_DEST { MODELPARAM_VALUE.DMA_TYPE_DEST PARAM_VALUE.DMA_TYPE_DEST } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DMA_TYPE_DEST}] ${MODELPARAM_VALUE.DMA_TYPE_DEST}
}

proc update_MODELPARAM_VALUE.DMA_TYPE_SRC { MODELPARAM_VALUE.DMA_TYPE_SRC PARAM_VALUE.DMA_TYPE_SRC } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.DMA_TYPE_SRC}] ${MODELPARAM_VALUE.DMA_TYPE_SRC}
}

proc update_MODELPARAM_VALUE.MAX_BYTES_PER_BURST { MODELPARAM_VALUE.MAX_BYTES_PER_BURST PARAM_VALUE.MAX_BYTES_PER_BURST } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.MAX_BYTES_PER_BURST}] ${MODELPARAM_VALUE.MAX_BYTES_PER_BURST}
}

proc update_MODELPARAM_VALUE.FIFO_SIZE { MODELPARAM_VALUE.FIFO_SIZE PARAM_VALUE.FIFO_SIZE } {
	# Procedure called to set VHDL generic/Verilog parameter value(s) based on TCL parameter value
	set_property value [get_property value ${PARAM_VALUE.FIFO_SIZE}] ${MODELPARAM_VALUE.FIFO_SIZE}
}

