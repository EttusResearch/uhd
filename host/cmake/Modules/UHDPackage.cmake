#
# Copyright 2010-2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
include(UHDVersion) #sets version information

set(UHD_RELEASE_MODE "${UHD_RELEASE_MODE}" CACHE BOOL "set UHD to release mode to build installers")

########################################################################
# Setup additional defines for OS types
########################################################################
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(LINUX TRUE)
endif()

if(NOT CMAKE_CROSSCOMPILING AND LINUX AND EXISTS "/etc/redhat-release")
    set(REDHAT TRUE)
endif()

########################################################################
# Set generator type for recognized systems
########################################################################
if(CPACK_GENERATOR)
    #already set
elseif(APPLE)
    set(CPACK_GENERATOR PackageMaker)
elseif(WIN32)
    set(CPACK_GENERATOR NSIS)
elseif(REDHAT)
    set(CPACK_GENERATOR RPM)
else()
    set(CPACK_GENERATOR TGZ)
endif()

########################################################################
# Setup package file name
########################################################################
if(REDHAT)
    #extract system information by executing the commands
    execute_process(
        COMMAND bash -c "source /etc/os-release && echo $ID"
        OUTPUT_VARIABLE OS_ID OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND bash -c "source /etc/os-release && echo $VERSION_ID"
        OUTPUT_VARIABLE VERSION_ID OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    #set a more sensible package name for this system
    if(UHD_RELEASE_MODE)
        string(REGEX REPLACE "(.release)$" "" uhd_vers_pretty ${UHD_VERSION})
        set(CPACK_PACKAGE_FILE_NAME "uhd-${uhd_vers_pretty}-${OS_ID}${VERSION_ID}" CACHE INTERNAL "")
    else()
        set(CPACK_PACKAGE_FILE_NAME "uhd-${UHD_VERSION}-${OS_ID}${VERSION_ID}" CACHE INTERNAL "")
    endif()
endif(REDHAT)

if(${CPACK_GENERATOR} STREQUAL NSIS)

    enable_language(C)

    include(CheckTypeSize)
    check_type_size("void*[8]" BIT_WIDTH BUILTIN_TYPES_ONLY)
    # If CMake option given, specify MSVC version in installer filename
    if(SPECIFY_MSVC_VERSION)
        if(MSVC_TOOLSET_VERSION EQUAL 90) # Visual Studio 2008 (9.0)
            set(MSVC_VERSION "VS2008")
        elseif(MSVC_TOOLSET_VERSION EQUAL 100) # Visual Studio 2010 (10.0)
            set(MSVC_VERSION "VS2010")
        elseif(MSVC_TOOLSET_VERSION EQUAL 110) # Visual Studio 2012 (11.0)
            set(MSVC_VERSION "VS2012")
        elseif(MSVC_TOOLSET_VERSION EQUAL 120) # Visual Studio 2013 (12.0)
            set(MSVC_VERSION "VS2013")
        elseif(MSVC_TOOLSET_VERSION EQUAL 140) # Visual Studio 2015 (14.0)
            set(MSVC_VERSION "VS2015")
        elseif(MSVC_TOOLSET_VERSION EQUAL 141) # Visual Studio 2017 (14.1)
            set(MSVC_VERSION "VS2017")
        elseif(MSVC_TOOLSET_VERSION EQUAL 142) # Visual Studio 2019 (14.2)
            set(MSVC_VERSION "VS2019")
        endif()
        set(CPACK_PACKAGE_FILE_NAME "uhd_${UHD_VERSION}_Win${BIT_WIDTH}_${MSVC_VERSION}" CACHE INTERNAL "")
    else()
        set(CPACK_PACKAGE_FILE_NAME "uhd_${UHD_VERSION}_Win${BIT_WIDTH}" CACHE INTERNAL "")
    endif(SPECIFY_MSVC_VERSION)

    set(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")
endif()

########################################################################
# Setup CPack General
########################################################################
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ettus Research - USRP Hardware Driver")
set(CPACK_PACKAGE_VENDOR              "Ettus Research (National Instruments)")
set(CPACK_PACKAGE_CONTACT             "Ettus Research <support@ettus.com>")
set(CPACK_PACKAGE_VERSION "${UHD_VERSION}")
set(CPACK_RESOURCE_FILE_WELCOME ${UHD_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_LICENSE ${UHD_SOURCE_DIR}/LICENSE)

########################################################################
# Setup CPack Source
########################################################################

set(CPACK_SOURCE_PACKAGE_FILE_NAME "uhd-${UHD_VERSION}" CACHE INTERNAL "")
set(CPACK_SOURCE_IGNORE_FILES "\\\\.git*;\\\\.swp$")

########################################################################
# Setup CPack Components
########################################################################
set(CPACK_COMPONENT_LIBRARIES_GROUP      "Development")
set(CPACK_COMPONENT_PYTHONAPI_GROUP      "Development")
set(CPACK_COMPONENT_HEADERS_GROUP        "Development")
set(CPACK_COMPONENT_UTILITIES_GROUP      "Runtime")
set(CPACK_COMPONENT_EXAMPLES_GROUP       "Runtime")
set(CPACK_COMPONENT_MANUAL_GROUP         "Documentation")
set(CPACK_COMPONENT_DOXYGEN_GROUP        "Documentation")
set(CPACK_COMPONENT_README_GROUP         "Documentation")

set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME      "Libraries")
set(CPACK_COMPONENT_PYTHONAPI_DISPLAY_NAME      "UHD Python API")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME        "C++ Headers")
set(CPACK_COMPONENT_UTILITIES_DISPLAY_NAME      "Utilities")
set(CPACK_COMPONENT_EXAMPLES_DISPLAY_NAME       "Examples")
set(CPACK_COMPONENT_MANUAL_DISPLAY_NAME         "Manual")
set(CPACK_COMPONENT_DOXYGEN_DISPLAY_NAME        "Doxygen")
set(CPACK_COMPONENT_README_DISPLAY_NAME         "Readme")
set(CPACK_COMPONENT_IMAGES_DISPLAY_NAME         "Images")

set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION     "Dynamic link library")
set(CPACK_COMPONENT_PYTHONAPI_DESCRIPTION     "UHD Python API")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION       "C++ development headers")
set(CPACK_COMPONENT_UTILITIES_DESCRIPTION     "Utility executables and python scripts")
set(CPACK_COMPONENT_EXAMPLES_DESCRIPTION      "Example executables")
set(CPACK_COMPONENT_MANUAL_DESCRIPTION        "Manual/application notes (rst and html)")
set(CPACK_COMPONENT_DOXYGEN_DESCRIPTION       "API documentation (html)")
set(CPACK_COMPONENT_README_DESCRIPTION        "Readme files (txt)")
set(CPACK_COMPONENT_IMAGES_DESCRIPTION        "FPGA and firmware images")

set(CPACK_COMPONENT_README_REQUIRED TRUE)

set(CPACK_COMPONENT_UTILITIES_DEPENDS libraries)
set(CPACK_COMPONENT_EXAMPLES_DEPENDS libraries)
set(CPACK_COMPONENT_TESTS_DEPENDS libraries)

set(CPACK_COMPONENTS_ALL libraries pythonapi headers utilities examples manual doxygen readme images)

########################################################################
# Setup CPack RPM
########################################################################
set(CPACK_RPM_SPEC_MORE_DEFINE "%global __python %{__python3}")
set(CPACK_RPM_PACKAGE_REQUIRES "boost-devel, python3-requests")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/usr/share/man;/usr/share/man/man1;/usr/lib64/pkgconfig;/usr/lib64/cmake;/usr/lib64/python2.7;/usr/lib64/python2.7/site-packages")
set(CPACK_RPM_FILE_NAME "$CACHE{CPACK_PACKAGE_FILE_NAME}.rpm")
foreach(filename post_install post_uninstall pre_install pre_uninstall)
    string(TOUPPER ${filename} filename_upper)
    list(APPEND CPACK_RPM_${filename_upper}_SCRIPT_FILE ${UHD_BINARY_DIR}/redhat/${filename})
    file(MAKE_DIRECTORY ${UHD_BINARY_DIR}/redhat)
    configure_file(
        ${UHD_SOURCE_DIR}/cmake/redhat/${filename}.in
        ${UHD_BINARY_DIR}/redhat/${filename}
    @ONLY)
endforeach(filename)

########################################################################
# Setup CPack NSIS
########################################################################
set(CPACK_NSIS_MODIFY_PATH ON)

set(HLKM_ENV "\\\"SYSTEM\\\\CurrentControlSet\\\\Control\\\\Session Manager\\\\Environment\\\"")

set(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    WriteRegStr HKLM ${HLKM_ENV} \\\"UHD_PKG_PATH\\\" \\\"$INSTDIR\\\"
")

set(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
    DeleteRegValue HKLM ${HLKM_ENV} \\\"UHD_PKG_PATH\\\"
")

if(WIN32)
    #Install necessary runtime DLL's
    include(InstallRequiredSystemLibraries)
endif(WIN32)

########################################################################
include(CPack) #include after setting vars
