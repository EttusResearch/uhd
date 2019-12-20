set_module_info(chdr 3 15 0 0)

set(DISSECTOR_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/packet-chdr.c
)

set(PLUGIN_C_FILE
	${CMAKE_CURRENT_BINARY_DIR}/plugin-chdr.c
)

set(PLUGIN_FILES
	${PLUGIN_C_FILE}
	${DISSECTOR_SRC}
)

register_plugin_files(
	chdr
	${PLUGIN_C_FILE}
	plugin
	${DISSECTOR_SRC}
)

add_plugin_library(chdr epan)

set_target_properties(chdr PROPERTIES PREFIX "")
set_target_properties(chdr PROPERTIES LINK_FLAGS "${WS_LINK_FLAGS}")
target_link_libraries(chdr ${WIRESHARK_LIBRARIES})

install_plugin(chdr epan)
