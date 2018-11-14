project(UHD_RPCLIB CXX)

set(RPCLIB_DEFAULT_BUFFER_SIZE "1024 << 10" CACHE STRING "Default buffer size")

set(RPCLIB_VERISION_MAJOR 1)
set(RPCLIB_VERSION_MINOR 0)
set(RPCLIB_VERSION_PATCH 0)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rpc/version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/version.h)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/rpc/config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.h)

set(RPCLIB_COMPILE_DEFINITIONS "")
if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")

elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")

elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "MSVC")
    set(RPCLIB_COMPILE_DEFINITIONS
        "${RPCLIB_COMPILE_DEFINITIONS}"
        "WIN32_LEAN_AND_MEAN"
        "NOMINMAX"
        "VC_EXTRALEAN"
        "_CRT_SECURE_NO_WARNINGS"
        "_CRT_NONSTDC_NO_DEPRECATE"
        "_WIN32_WINNT=0x0501"
        "_GNU_SOURCE"
        "ASIO_HAS_STD_ADDRESSOF"
        "ASIO_HAS_STD_ARRAY"
        "ASIO_HAS_CSTDINT"
        "ASIO_HAS_STD_SHARED_PTR"
        "ASIO_HAS_STD_TYPE_TRAITS"
    )
endif()

include_directories(include)
include_directories(include/rpc)
add_library(uhd_rpclib OBJECT
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
set_target_properties(uhd_rpclib
    PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    COMPILE_DEFINITIONS "${RPCLIB_COMPILE_DEFINITIONS}"
)

