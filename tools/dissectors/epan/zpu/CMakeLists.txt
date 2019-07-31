set_module_info(zpu 4 0 0 0)

set(DISSECTOR_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/packet-zpu.c
)

set(PLUGIN_FILES
	${CMAKE_CURRENT_BINARY_DIR}/plugin.c
	${DISSECTOR_SRC}
)

register_plugin_files(${CMAKE_CURRENT_BINARY_DIR}/plugin.c
	plugin
	${DISSECTOR_SRC}
)

add_plugin_library(zpu epan)

set_target_properties(zpu PROPERTIES PREFIX "")
set_target_properties(zpu PROPERTIES LINK_FLAGS "${WS_LINK_FLAGS}")
target_link_libraries(zpu wireshark epan)

install_plugin(zpu epan)


