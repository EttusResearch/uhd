`pragma protect version = 2
`pragma protect begin_commonblock
`pragma protect control error_handling = "delegated"
`pragma protect control runtime_visibility = "delegated"
`pragma protect control child_visibility = "delegated"
`pragma protect control decryption=(activity==simulation) ? "false" : "true"
`pragma protect end_commonblock
`pragma protect begin_toolblock
`pragma protect rights_digest_method="sha256"
${ args['key_info'] }\
`pragma protect control xilinx_configuration_visible = "false"
`pragma protect control xilinx_enable_modification = "false"
`pragma protect control xilinx_enable_probing = "false"
`pragma protect control xilinx_enable_netlist_export = "false"
`pragma protect control xilinx_enable_bitstream = "true"
`pragma protect control xilinx_schematic_visibility="false"
`pragma protect control decryption=(xilinx_activity==simulation) ? "false" : "true"
`pragma protect end_toolblock = ""
