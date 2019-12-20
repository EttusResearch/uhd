set_module_info(zpu 3 15 0 0)

set(DISSECTOR_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/packet-zpu.c
)

set(PLUGIN_C_FILE
	${CMAKE_CURRENT_BINARY_DIR}/plugin-zpu.c
)

set(PLUGIN_FILES
	${PLUGIN_C_FILE}
	${DISSECTOR_SRC}
)

register_plugin_files(
	zpu
        ${PLUGIN_C_FILE}
	plugin
	${DISSECTOR_SRC}
)

add_plugin_library(zpu epan)

set_target_properties(zpu PROPERTIES PREFIX "")
set_target_properties(zpu PROPERTIES LINK_FLAGS "${WS_LINK_FLAGS}")
target_link_libraries(zpu ${WIRESHARK_LIBRARIES})

install_plugin(zpu epan)


