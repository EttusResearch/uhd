#
# Copyright 2011-2013,2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# This file included, use CMake directory variables
########################################################################
if(ENABLE_USB)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/fx2_ctrl.cpp
    )
endif(ENABLE_USB)

if(ENABLE_E300 OR ENABLE_B200 OR ENABLE_E320)
    include_directories("${CMAKE_CURRENT_SOURCE_DIR}/ad9361_driver")
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/ad9361_ctrl.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ad936x_manager.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ad9361_driver/ad9361_device.cpp
    )
endif(ENABLE_E300 OR ENABLE_B200 OR ENABLE_E320)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_SOURCE_DIR}/adf4001_ctrl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/adf435x.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/adf535x.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lmx2592.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/lmx2572.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/tmp468.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/apply_corrections.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/validate_subdev_spec.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/recv_packet_demuxer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/io_service_mgr.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/io_service_args.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/pwr_cal_mgr.cpp
)

if(ENABLE_X400)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/admv1320.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/admv1420.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/ltc5594.cpp
    )
endif(ENABLE_X400)

if(ENABLE_DPDK)
    set_property(
        SOURCE
        ${CMAKE_CURRENT_SOURCE_DIR}/io_service_mgr.cpp
        PROPERTY COMPILE_DEFINITIONS HAVE_DPDK
    )
endif(ENABLE_DPDK)

# Generate protobuf stubs for MPM server
if(ENABLE_MPMD)
    if(WIN32)
        # On Windows: use CONFIG mode to get full dependency chain (utf8_range etc.).
        # Use CACHE FORCE so that option() inside protobuf-config.cmake (CMP0077 old
        # behavior) does not clear our hint before it is evaluated.
        set(protobuf_MODULE_COMPATIBLE ON CACHE BOOL "" FORCE)
        find_package(Protobuf CONFIG REQUIRED)
    else()
        find_package(Protobuf REQUIRED)
    endif()
    find_package(gRPC REQUIRED)

    if(NOT Protobuf_PROTOC_EXECUTABLE)
        message(FATAL_ERROR "protobuf compiler (protoc) not found! Install protobuf-compiler or set ENABLE_MPMD=OFF.")
    endif()

    protobuf_generate(
        OUT_VAR MPM_SERVER_PROTO_CPP_FILES
        PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/mpm_server.proto
        LANGUAGE cpp
    )
    if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.21")
        protobuf_generate(
            OUT_VAR MPM_SERVER_PROTO_GRPC_CPP_FILES
            LANGUAGE grpc_cpp
            PLUGIN protoc-gen-grpc_cpp=${GRPC_CPP_PLUGIN}
            PROTOS ${CMAKE_CURRENT_SOURCE_DIR}/mpm_server.proto
            GENERATE_EXTENSIONS .grpc.pb.h .grpc.pb.cc
        )
    else()
        add_custom_command(
            OUTPUT
                ${CMAKE_CURRENT_BINARY_DIR}/mpm_server.grpc.pb.cc
                ${CMAKE_CURRENT_BINARY_DIR}/mpm_server.grpc.pb.h
            COMMAND
                ${Protobuf_PROTOC_EXECUTABLE} --proto_path=${CMAKE_CURRENT_SOURCE_DIR} --grpc_cpp_out=${CMAKE_CURRENT_BINARY_DIR} --plugin=protoc-gen-grpc_cpp=${GRPC_CPP_PLUGIN} ${CMAKE_CURRENT_SOURCE_DIR}/mpm_server.proto
            DEPENDS
                ${CMAKE_CURRENT_SOURCE_DIR}/mpm_server.proto
            COMMENT "Generating gRPC C++ stubs for MPM server (CMake < 3.21)"
            VERBATIM
        )
        set(MPM_SERVER_PROTO_GRPC_CPP_FILES ${CMAKE_CURRENT_BINARY_DIR}/mpm_server.grpc.pb.cc ${CMAKE_CURRENT_BINARY_DIR}/mpm_server.grpc.pb.h)
    endif()

    # Generate C++ client files using mpm_client_generate_cpp.py
    add_custom_command(
        OUTPUT
            ${CMAKE_CURRENT_BINARY_DIR}/mpm_client.hpp
            ${CMAKE_CURRENT_BINARY_DIR}/mpm_client.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/mpm_client_python.cpp
        COMMAND
            ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_SOURCE_DIR}/host/python
            ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/mpm_client_generate_cpp.py
            ${CMAKE_CURRENT_SOURCE_DIR}/mpm_server.proto
            ${CMAKE_CURRENT_BINARY_DIR}/mpm_client.hpp
            ${CMAKE_CURRENT_BINARY_DIR}/mpm_client.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mpm_client.hpp.mako
            ${CMAKE_CURRENT_SOURCE_DIR}/mpm_client.cpp.mako
            ${CMAKE_CURRENT_BINARY_DIR}/mpm_client_python.cpp
            ${CMAKE_CURRENT_SOURCE_DIR}/mpm_client_python.cpp.mako
        DEPENDS
            ${CMAKE_CURRENT_SOURCE_DIR}/mpm_client_generate_cpp.py
            ${CMAKE_CURRENT_SOURCE_DIR}/mpm_server.proto
            ${CMAKE_CURRENT_SOURCE_DIR}/mpm_client.hpp.mako
            ${CMAKE_CURRENT_SOURCE_DIR}/mpm_client.cpp.mako
            ${CMAKE_CURRENT_SOURCE_DIR}/mpm_client_python.cpp.mako
            ${MPM_SERVER_PROTO_CPP_FILES}
            ${MPM_SERVER_PROTO_GRPC_CPP_FILES}
        COMMENT "Generating C++ client files for MPM server"
        VERBATIM
    )

    add_custom_target(
        generate_mpm_stubs
        DEPENDS
            ${MPM_SERVER_PROTO_CPP_FILES}
            ${MPM_SERVER_PROTO_GRPC_CPP_FILES}
            ${CMAKE_CURRENT_BINARY_DIR}/mpm_client.hpp
            ${CMAKE_CURRENT_BINARY_DIR}/mpm_client.cpp
            ${CMAKE_CURRENT_BINARY_DIR}/mpm_client_python.cpp
    )

    # Add the generated files to the library sources
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_BINARY_DIR}/mpm_server.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/mpm_server.grpc.pb.cc
        ${CMAKE_CURRENT_BINARY_DIR}/mpm_client.cpp
    )
    
    # Mark files as generated so CMake doesn't complain at configure time
    set_source_files_properties(
        ${CMAKE_CURRENT_BINARY_DIR}/mpm_client.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/mpm_client_python.cpp
        PROPERTIES GENERATED TRUE
    )
    
    # Add binary directory to include path for generated headers
    include_directories(${CMAKE_CURRENT_BINARY_DIR})
    # Add Protobuf include directories
    include_directories(${Protobuf_INCLUDE_DIRS})

    # Disable array-bounds warning for generated protobuf files (false positive with GCC 9)
    # See https://github.com/protocolbuffers/protobuf/issues/7140
    if(${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 10.0)
        message(STATUS "Disable array-bounds warning for GCC ${CMAKE_CXX_COMPILER_VERSION} on protobuf-generated files")
        set_source_files_properties(
            ${CMAKE_CURRENT_BINARY_DIR}/mpm_server.pb.cc
            ${CMAKE_CURRENT_BINARY_DIR}/mpm_server.grpc.pb.cc
            PROPERTIES COMPILE_FLAGS "-Wno-error=array-bounds"
        )
    endif()
endif(ENABLE_MPMD)
