set_module_info(octoclock 3 15 0 0)

set(DISSECTOR_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/packet-octoclock.c
)

set(PLUGIN_C_FILE
	${CMAKE_CURRENT_BINARY_DIR}/plugin-octoclock.c
)

set(PLUGIN_FILES
	${PLUGIN_C_FILE}
	${DISSECTOR_SRC}
)

register_plugin_files(
	octoclock
        ${PLUGIN_C_FILE}
	plugin
	${DISSECTOR_SRC}
)

add_plugin_library(octoclock epan)

set_target_properties(octoclock PROPERTIES PREFIX "")
set_target_properties(octoclock PROPERTIES LINK_FLAGS "${WS_LINK_FLAGS}")
target_link_libraries(octoclock ${WIRESHARK_LIBRARIES})

install_plugin(octoclock epan)
