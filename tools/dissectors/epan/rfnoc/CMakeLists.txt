set_module_info(rfnoc 4 0 0 0)

set(DISSECTOR_SRC
	${CMAKE_CURRENT_SOURCE_DIR}/packet-rfnoc.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../host/lib/exception.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../host/lib/rfnoc/chdr_types.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/../../../../host/lib/rfnoc/chdr_packet_writer.cpp
)

set(PLUGIN_FILES
	${CMAKE_CURRENT_BINARY_DIR}/plugin.c
	${DISSECTOR_SRC}
)

register_plugin_files(${CMAKE_CURRENT_BINARY_DIR}/plugin.c
	plugin
	${DISSECTOR_SRC}
)

add_plugin_library(rfnoc epan)

set_target_properties(rfnoc PROPERTIES PREFIX ""
	CXX_STANDARD 14
	CXX_STANDARD_REQUIRED YES
)
set_target_properties(rfnoc PROPERTIES LINK_FLAGS "${WS_LINK_FLAGS}")
target_link_libraries(rfnoc wireshark epan)

install_plugin(rfnoc epan)

