#
# Copyright 2010-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Helpful Macros
########################################################################
macro(LIBUHD_APPEND_SOURCES)
    list(APPEND libuhd_sources ${ARGV})
endmacro(LIBUHD_APPEND_SOURCES)

macro(LIBUHD_APPEND_LIBS)
    list(APPEND libuhd_libs ${ARGV})
endmacro(LIBUHD_APPEND_LIBS)

macro(LIBUHD_PYTHON_GEN_SOURCE pyfile outfile)
    # Ensure that the directory exists for outfile
    get_filename_component(outfile_dir ${outfile} PATH)
    file(MAKE_DIRECTORY ${outfile_dir})

    # Make the outfile depend on the python script
    add_custom_command(
        OUTPUT ${outfile} DEPENDS ${pyfile} ${LIBUHD_PYTHON_GEN_SOURCE_DEPS}
        COMMAND ${PYTHON_EXECUTABLE} -B ${pyfile} ${outfile}
        COMMENT "Generating ${outfile}"
    )

    # Make libuhd depend on the outfile
    LIBUHD_APPEND_SOURCES(${outfile})
endmacro(LIBUHD_PYTHON_GEN_SOURCE)

macro(INCLUDE_SUBDIRECTORY subdir)
    # Insert the current directories on the front of the list
    list(INSERT _cmake_source_dirs 0 ${CMAKE_CURRENT_SOURCE_DIR})
    list(INSERT _cmake_binary_dirs 0 ${CMAKE_CURRENT_BINARY_DIR})

    # Set the current directories to the names of the subdirs
    set(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
    set(CMAKE_CURRENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${subdir})

    # Include the subdirectory CMakeLists to run it
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    include(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)

    # Reset the value of the current directories
    list(GET _cmake_source_dirs 0 CMAKE_CURRENT_SOURCE_DIR)
    list(GET _cmake_binary_dirs 0 CMAKE_CURRENT_BINARY_DIR)

    # Pop the subdir names of the front of the list
    list(REMOVE_AT _cmake_source_dirs 0)
    list(REMOVE_AT _cmake_binary_dirs 0)
endmacro(INCLUDE_SUBDIRECTORY)

########################################################################
# Register lower level components
########################################################################
message(STATUS "")
# Dependencies
find_package(LIBUSB)
find_package(DPDK)
LIBUHD_REGISTER_COMPONENT("USB" ENABLE_USB ON "ENABLE_LIBUHD;LIBUSB_FOUND" OFF OFF)
# Devices
LIBUHD_REGISTER_COMPONENT("B100" ENABLE_B100 ON "ENABLE_LIBUHD;ENABLE_USB" OFF OFF)
LIBUHD_REGISTER_COMPONENT("B200" ENABLE_B200 ON "ENABLE_LIBUHD;ENABLE_USB" OFF OFF)
LIBUHD_REGISTER_COMPONENT("USRP1" ENABLE_USRP1 ON "ENABLE_LIBUHD;ENABLE_USB" OFF OFF)
LIBUHD_REGISTER_COMPONENT("USRP2" ENABLE_USRP2 ON "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("X300" ENABLE_X300 ON "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("MPMD" ENABLE_MPMD ON "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("SIM" ENABLE_SIM ON "ENABLE_LIBUHD;ENABLE_MPMD;ENABLE_PYTHON_API" OFF OFF)
LIBUHD_REGISTER_COMPONENT("N300" ENABLE_N300 ON "ENABLE_LIBUHD;ENABLE_MPMD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("N320" ENABLE_N320 ON "ENABLE_LIBUHD;ENABLE_MPMD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("E320" ENABLE_E320 ON "ENABLE_LIBUHD;ENABLE_MPMD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("E300" ENABLE_E300 ON "ENABLE_LIBUHD;ENABLE_MPMD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("X400" ENABLE_X400 ON "ENABLE_LIBUHD;ENABLE_MPMD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("OctoClock" ENABLE_OCTOCLOCK ON "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("DPDK" ENABLE_DPDK ON "ENABLE_MPMD;DPDK_FOUND" OFF OFF)

########################################################################
# Include subdirectories (different than add)
########################################################################
INCLUDE_SUBDIRECTORY(include)
INCLUDE_SUBDIRECTORY(cal)
INCLUDE_SUBDIRECTORY(features)
INCLUDE_SUBDIRECTORY(ic_reg_maps)
INCLUDE_SUBDIRECTORY(types)
INCLUDE_SUBDIRECTORY(convert)
INCLUDE_SUBDIRECTORY(rfnoc)
INCLUDE_SUBDIRECTORY(usrp)
INCLUDE_SUBDIRECTORY(usrp_clock)
INCLUDE_SUBDIRECTORY(utils)
INCLUDE_SUBDIRECTORY(experts)
INCLUDE_SUBDIRECTORY(extension)
INCLUDE_SUBDIRECTORY(transport)

########################################################################
# Build info
########################################################################
include(UHDBuildInfo)
UHD_LOAD_BUILD_INFO()
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/build_info.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/build_info.cpp
@ONLY)

########################################################################
# Setup UHD_VERSION_STRING for version.cpp
########################################################################
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
@ONLY)

########################################################################
# Append to the list of sources for lib uhd
########################################################################
LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_BINARY_DIR}/build_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/exception.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/property_tree.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)

if(ENABLE_C_API)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/error_c.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/version_c.cpp
    )
endif(ENABLE_C_API)

########################################################################
# Add common resource compiler subdirectory
########################################################################
add_subdirectory(rc)
LIBUHD_APPEND_LIBS(uhd_rc)

########################################################################
# Add DLL resource file to Windows build
########################################################################
if(MSVC)
    set(RC_VERSION_PATCH ${UHD_VERSION_PATCH})
    if(UHD_VERSION_DEVEL)
        set(RC_VERSION_PATCH "999")
    endif(UHD_VERSION_DEVEL)

    # Allow a custom .rc template file to be used
    if(CUSTOM_RC_FILE)
        if(IS_ABSOLUTE "${CUSTOM_RC_FILE}")
            set(UHD_RC_IN "${CUSTOM_RC_FILE}")
        else()
            set(UHD_RC_IN "${UHD_BINARY_DIR}/${CUSTOM_RC_FILE}")
        endif(IS_ABSOLUTE "${CUSTOM_RC_FILE}")
        message(STATUS "")
        message(STATUS "Using custom RC template: ${UHD_RC_IN}")
        message(STATUS "")
    else()
        set(UHD_RC_IN "${CMAKE_CURRENT_SOURCE_DIR}/uhd.rc.in")
    endif(CUSTOM_RC_FILE)
    set(UHD_RC_IN ${UHD_RC_IN} CACHE STRING "uhd.rc template filepath")

    configure_file(
        ${UHD_RC_IN}
        ${CMAKE_CURRENT_BINARY_DIR}/uhd.rc
    @ONLY)

    list(APPEND libuhd_sources ${CMAKE_CURRENT_BINARY_DIR}/uhd.rc)
endif(MSVC)

########################################################################
# Setup libuhd library
########################################################################
if(ENABLE_MPMD)
    include_directories(deps/rpclib/include)
    add_subdirectory(deps)
    list(APPEND libuhd_sources $<TARGET_OBJECTS:uhd_rpclib>)
endif()
add_library(uhd SHARED ${libuhd_sources})
target_link_libraries(uhd ${Boost_LIBRARIES} ${libuhd_libs})
if(ENABLE_DPDK)
    add_definitions(-DHAVE_DPDK)
    # The compile flags DPDK_CFLAGS add the DPDK_INCLUDE_DIRS include
    # directories and set the -march=native compiler flag.
    target_compile_options(uhd PRIVATE ${DPDK_CFLAGS})
    # The linker flags DPDK_LDFLAGS contains the list of DPDK libraries
    # as well as the path where to find the libraries
    target_link_options(uhd PUBLIC ${DPDK_LDFLAGS})
    # explicitly link against the DPDK libraries, otherwise there are
    # undefined references when linking libuhd
    target_link_libraries(uhd ${DPDK_LIBRARIES})
endif()
if(APPLE)
    target_link_options(uhd PRIVATE "LINKER:-undefined,dynamic_lookup")
    target_link_options(uhd PRIVATE "-flat_namespace")
endif(APPLE)
set_target_properties(uhd PROPERTIES DEFINE_SYMBOL "UHD_DLL_EXPORTS")
if(NOT LIBUHDDEV_PKG)
    set_target_properties(uhd PROPERTIES SOVERSION "${UHD_ABI_VERSION}")
    set_target_properties(uhd PROPERTIES VERSION "${UHD_ABI_VERSION}")
endif(NOT LIBUHDDEV_PKG)
if(DEFINED LIBUHD_OUTPUT_NAME)
    set_target_properties(uhd PROPERTIES OUTPUT_NAME ${LIBUHD_OUTPUT_NAME})
endif(DEFINED LIBUHD_OUTPUT_NAME)

if(ENABLE_SIM)
    # Get python include dirs
    # This contitional code somewhat duplicates logic also found in 
    # ../python/CMakeLists.txt but omits user messages. This avoid 
    # duplicate output, since SIM mode requires python support. 
    include_directories(${PYTHON_INCLUDE_DIRS})
    if(NOT pybind11_FOUND)
        set(PYBIND11_INCLUDE_DIR
            "${UHD_SOURCE_DIR}/lib/deps/pybind11/include"
            CACHE
            STRING
            "Location of PyBind11 includes"
        )
    endif()
    include_directories(${PYBIND11_INCLUDE_DIR})

    # For PYUHD we don't link against the python libraries, but when calling
    # python instead of being called by it, we have to.
    target_link_libraries(uhd ${PYTHON_LIBRARIES})

endif(ENABLE_SIM)

if(NOT UHDHOST_PKG) # Syntax makes it unusable by UHD_INSTALL
    install(TARGETS uhd
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries # .so file
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT libraries # .lib file
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT libraries # .dll file
    )
endif(NOT UHDHOST_PKG)

#######################################################
# Setup libuhd library (static)
#######################################################
if(ENABLE_STATIC_LIBS)
    add_library(uhd_static STATIC ${libuhd_sources} $<TARGET_OBJECTS:uhd_rc>)
    set_target_properties(uhd_static PROPERTIES OUTPUT_NAME uhd)
    set_target_properties(uhd_static PROPERTIES COMPILE_DEFINITIONS UHD_STATIC_LIB)
    target_link_libraries(uhd_static uhd_rc)
    install(TARGETS uhd_static ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR} # .lib or .a file
    )
endif(ENABLE_STATIC_LIBS)

if(ENABLE_EXTEND_WIN_PACKAGING)
    if(ENABLE_USB AND NOT ENABLE_STATIC_LIBS)
        include(UHDLIBUSB)

        if(DEFINED UHD_LIBUSBDLLPATH)
            # UHD_INSTALL(FILES
            #     ${UHD_LIBUSBDLLPATH}
            #     DESTINATION ${CMAKE_INSTALL_BINDIR}
            #     COMPONENT winlibusb)
        install(FILES
                ${UHD_LIBUSBDLLPATH}
                DESTINATION ${CMAKE_INSTALL_BINDIR}
                COMPONENT libraries)
        else()
            message(WARNING "Unable to locate libusb dynamic link library from LIBUSB_LIBRARIES: ${LIBUSB_LIBRARIES}")
        endif(DEFINED UHD_LIBUSBDLLPATH)

        # Variable LIBUSB_LICENSE_FILE is defined in top level CMakeLists.txt,
        # and the files gets generated in the UHDLIBUSB module.
        if(EXISTS ${UHD_LIBUSB_LICENSEFILE})
            UHD_INSTALL(FILES
                ${UHD_LIBUSB_LICENSEFILE}
                DESTINATION ${PKG_DOC_DIR}
                COMPONENT readme
            )
        else()
            message(WARNING "Did not find generated libusb license files at ${UHD_LIBUSB_LICENSEFILE}")
        endif(EXISTS ${UHD_LIBUSB_LICENSEFILE})
    endif(ENABLE_USB AND NOT ENABLE_STATIC_LIBS)
endif(ENABLE_EXTEND_WIN_PACKAGING)
