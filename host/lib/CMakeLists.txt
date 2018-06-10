#
# Copyright 2010-2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#

########################################################################
# Helpful Macros
########################################################################
MACRO(LIBUHD_APPEND_SOURCES)
    LIST(APPEND libuhd_sources ${ARGV})
ENDMACRO(LIBUHD_APPEND_SOURCES)

MACRO(LIBUHD_APPEND_LIBS)
    LIST(APPEND libuhd_libs ${ARGV})
ENDMACRO(LIBUHD_APPEND_LIBS)

MACRO(LIBUHD_PYTHON_GEN_SOURCE pyfile outfile)
    #ensure that the directory exists for outfile
    GET_FILENAME_COMPONENT(outfile_dir ${outfile} PATH)
    FILE(MAKE_DIRECTORY ${outfile_dir})

    #make the outfile depend on the python script
    ADD_CUSTOM_COMMAND(
        OUTPUT ${outfile} DEPENDS ${pyfile} ${LIBUHD_PYTHON_GEN_SOURCE_DEPS}
        COMMAND ${PYTHON_EXECUTABLE} -B ${pyfile} ${outfile}
        COMMENT "Generating ${outfile}"
    )

    #make libuhd depend on the outfile
    LIBUHD_APPEND_SOURCES(${outfile})
ENDMACRO(LIBUHD_PYTHON_GEN_SOURCE)

MACRO(INCLUDE_SUBDIRECTORY subdir)
    #insert the current directories on the front of the list
    LIST(INSERT _cmake_source_dirs 0 ${CMAKE_CURRENT_SOURCE_DIR})
    LIST(INSERT _cmake_binary_dirs 0 ${CMAKE_CURRENT_BINARY_DIR})

    #set the current directories to the names of the subdirs
    SET(CMAKE_CURRENT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${subdir})
    SET(CMAKE_CURRENT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/${subdir})

    #include the subdirectory CMakeLists to run it
    FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    INCLUDE(${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt)

    #reset the value of the current directories
    LIST(GET _cmake_source_dirs 0 CMAKE_CURRENT_SOURCE_DIR)
    LIST(GET _cmake_binary_dirs 0 CMAKE_CURRENT_BINARY_DIR)

    #pop the subdir names of the front of the list
    LIST(REMOVE_AT _cmake_source_dirs 0)
    LIST(REMOVE_AT _cmake_binary_dirs 0)
ENDMACRO(INCLUDE_SUBDIRECTORY)

########################################################################
# Register lower level components
########################################################################
MESSAGE(STATUS "")
# Dependencies
FIND_PACKAGE(USB1)
FIND_PACKAGE(GPSD)
FIND_PACKAGE(LIBERIO)
LIBUHD_REGISTER_COMPONENT("LIBERIO" ENABLE_LIBERIO ON "ENABLE_LIBUHD;LIBERIO_FOUND" OFF OFF)
LIBUHD_REGISTER_COMPONENT("USB" ENABLE_USB ON "ENABLE_LIBUHD;LIBUSB_FOUND" OFF OFF)
LIBUHD_REGISTER_COMPONENT("GPSD" ENABLE_GPSD OFF "ENABLE_LIBUHD;ENABLE_GPSD;LIBGPS_FOUND" OFF OFF)
# Devices
LIBUHD_REGISTER_COMPONENT("B100" ENABLE_B100 ON "ENABLE_LIBUHD;ENABLE_USB" OFF OFF)
LIBUHD_REGISTER_COMPONENT("B200" ENABLE_B200 ON "ENABLE_LIBUHD;ENABLE_USB" OFF OFF)
LIBUHD_REGISTER_COMPONENT("E300" ENABLE_E300 OFF "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("USRP1" ENABLE_USRP1 ON "ENABLE_LIBUHD;ENABLE_USB" OFF OFF)
LIBUHD_REGISTER_COMPONENT("USRP2" ENABLE_USRP2 ON "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("X300" ENABLE_X300 ON "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("N230" ENABLE_N230 ON "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("MPMD" ENABLE_MPMD ON "ENABLE_LIBUHD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("N300" ENABLE_N300 ON "ENABLE_LIBUHD;ENABLE_MPMD" OFF OFF)
LIBUHD_REGISTER_COMPONENT("OctoClock" ENABLE_OCTOCLOCK ON "ENABLE_LIBUHD" OFF OFF)

########################################################################
# Include subdirectories (different than add)
########################################################################
INCLUDE_SUBDIRECTORY(include)
INCLUDE_SUBDIRECTORY(ic_reg_maps)
INCLUDE_SUBDIRECTORY(types)
INCLUDE_SUBDIRECTORY(convert)
INCLUDE_SUBDIRECTORY(rfnoc)
INCLUDE_SUBDIRECTORY(cal)
INCLUDE_SUBDIRECTORY(usrp)
INCLUDE_SUBDIRECTORY(usrp_clock)
INCLUDE_SUBDIRECTORY(utils)
INCLUDE_SUBDIRECTORY(experts)
INCLUDE_SUBDIRECTORY(transport)

########################################################################
# Build info
########################################################################
INCLUDE(UHDBuildInfo)
UHD_LOAD_BUILD_INFO()
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/build_info.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/build_info.cpp
@ONLY)

########################################################################
# Setup UHD_VERSION_STRING for version.cpp
########################################################################
CONFIGURE_FILE(
    ${CMAKE_CURRENT_SOURCE_DIR}/version.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
@ONLY)

########################################################################
# Append to the list of sources for lib uhd
########################################################################
LIBUHD_APPEND_SOURCES(
    ${CMAKE_CURRENT_BINARY_DIR}/build_info.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/deprecated.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/device.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/device3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/image_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/exception.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/property_tree.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/version.cpp
)

IF(ENABLE_C_API)
    LIBUHD_APPEND_SOURCES(
        ${CMAKE_CURRENT_SOURCE_DIR}/error_c.cpp
    )
ENDIF(ENABLE_C_API)

########################################################################
# Add DLL resource file to Windows build
########################################################################
IF(MSVC)
    MATH(EXPR RC_VERSION_MAJOR_API "${UHD_VERSION_MAJOR} * 1000 + ${UHD_VERSION_API}")
    SET(RC_VERSION_PATCH ${UHD_VERSION_PATCH})
    IF(UHD_VERSION_DEVEL)
        SET(RC_VERSION_PATCH "999")
    ENDIF(UHD_VERSION_DEVEL)

    # Allow a custom .rc template file to be used
    IF(CUSTOM_RC_FILE)
        IF(IS_ABSOLUTE "${CUSTOM_RC_FILE}")
            SET(UHD_RC_IN "${CUSTOM_RC_FILE}")
        ELSE()
            SET(UHD_RC_IN "${CMAKE_BINARY_DIR}/${CUSTOM_RC_FILE}")
        ENDIF(IS_ABSOLUTE "${CUSTOM_RC_FILE}")
        MESSAGE(STATUS "")
        MESSAGE(STATUS "Using custom RC template: ${UHD_RC_IN}")
        MESSAGE(STATUS "")
    ELSE()
        SET(UHD_RC_IN "${CMAKE_CURRENT_SOURCE_DIR}/uhd.rc.in")
    ENDIF(CUSTOM_RC_FILE)
    SET(UHD_RC_IN ${UHD_RC_IN} CACHE STRING "uhd.rc template filepath")

    CONFIGURE_FILE(
        ${UHD_RC_IN}
        ${CMAKE_CURRENT_BINARY_DIR}/uhd.rc
    @ONLY)

    LIST(APPEND libuhd_sources ${CMAKE_CURRENT_BINARY_DIR}/uhd.rc)
ENDIF(MSVC)

########################################################################
# Setup libuhd library
########################################################################
IF(ENABLE_MPMD)
    INCLUDE_DIRECTORIES(deps/rpclib/include)
    ADD_SUBDIRECTORY(deps)
    LIST(APPEND libuhd_sources $<TARGET_OBJECTS:uhd_rpclib>)
ENDIF()
ADD_LIBRARY(uhd SHARED ${libuhd_sources})
TARGET_LINK_LIBRARIES(uhd ${Boost_LIBRARIES} ${libuhd_libs})
SET_TARGET_PROPERTIES(uhd PROPERTIES DEFINE_SYMBOL "UHD_DLL_EXPORTS")
IF(NOT LIBUHDDEV_PKG)
    SET_TARGET_PROPERTIES(uhd PROPERTIES SOVERSION "${UHD_VERSION_MAJOR}")
    SET_TARGET_PROPERTIES(uhd PROPERTIES VERSION "${UHD_VERSION_MAJOR}.${UHD_VERSION_API}")
ENDIF(NOT LIBUHDDEV_PKG)
IF(DEFINED LIBUHD_OUTPUT_NAME)
    SET_TARGET_PROPERTIES(uhd PROPERTIES OUTPUT_NAME ${LIBUHD_OUTPUT_NAME})
ENDIF(DEFINED LIBUHD_OUTPUT_NAME)

IF(NOT UHDHOST_PKG) #Syntax makes it unusable by UHD_INSTALL
    INSTALL(TARGETS uhd
        LIBRARY DESTINATION ${LIBRARY_DIR} COMPONENT libraries # .so file
        ARCHIVE DESTINATION ${LIBRARY_DIR} COMPONENT libraries # .lib file
        RUNTIME DESTINATION ${RUNTIME_DIR} COMPONENT libraries # .dll file
    )
ENDIF(NOT UHDHOST_PKG)

#######################################################
# Setup libuhd library (static)
#######################################################
IF(ENABLE_STATIC_LIBS)
    ADD_LIBRARY(uhd_static STATIC ${libuhd_sources})
    SET_TARGET_PROPERTIES(uhd_static PROPERTIES OUTPUT_NAME uhd)
    INSTALL(TARGETS uhd_static
        ARCHIVE DESTINATION lib${LIB_SUFFIX} # .lib or .a file
    )
ENDIF(ENABLE_STATIC_LIBS)
