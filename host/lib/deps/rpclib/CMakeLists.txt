PROJECT(UHD_RPCLIB CXX)

SET(RPCLIB_DEFAULT_BUFFER_SIZE "1024 << 10" CACHE STRING "Default buffer size")

SET(RPCLIB_VERISION_MAJOR 1)
SET(RPCLIB_VERSION_MINOR 0)
SET(RPCLIB_VERSION_PATCH 0)

CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rpc/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h)

CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rpc/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h)

INCLUDE_DIRECTORIES(include)
INCLUDE_DIRECTORIES(include/rpc)
ADD_LIBRARY(uhd_rpclib OBJECT
    lib/rpc/dispatcher.cc
    lib/rpc/server.cc
    lib/rpc/client.cc
    lib/rpc/this_handler.cc
    lib/rpc/this_session.cc
    lib/rpc/this_server.cc
    lib/rpc/rpc_error.cc
    lib/rpc/detail/server_session.cc
    lib/rpc/detail/response.cc
    lib/rpc/detail/client_error.cc
)
SET_PROPERTY(TARGET uhd_rpclib PROPERTY POSITION_INDEPENDENT_CODE ON)
