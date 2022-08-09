set(DISSECTOR_NAME "rfnoc${RFNOC_CHDR_WIDTH}")

set_module_info(${DISSECTOR_NAME} 4 0 0 0)

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

add_plugin_library(${DISSECTOR_NAME} epan)

set_target_properties(${DISSECTOR_NAME} PROPERTIES PREFIX ""
    CXX_STANDARD 14
    CXX_STANDARD_REQUIRED YES
)
set_target_properties(${DISSECTOR_NAME} PROPERTIES LINK_FLAGS "${WS_LINK_FLAGS}")
target_link_libraries(${DISSECTOR_NAME} wireshark epan)

install_plugin(${DISSECTOR_NAME} epan)
