set_module_info(octoclock 4 0 0 0)

set(DISSECTOR_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/packet-octoclock.c
)

set(PLUGIN_FILES
	${CMAKE_CURRENT_BINARY_DIR}/plugin.c
	${DISSECTOR_SRC}
)

register_plugin_files(${CMAKE_CURRENT_BINARY_DIR}/plugin.c
	plugin
	${DISSECTOR_SRC}
)

add_plugin_library(octoclock epan)

set_target_properties(octoclock PROPERTIES PREFIX "")
set_target_properties(octoclock PROPERTIES LINK_FLAGS "${WS_LINK_FLAGS}")
target_link_libraries(octoclock wireshark epan)

install_plugin(octoclock epan)


