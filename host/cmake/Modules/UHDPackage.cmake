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

if(NOT CMAKE_CROSSCOMPILING AND LINUX AND EXISTS "/etc/debian_version")
    set(DEBIAN TRUE)
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
elseif(DEBIAN)
    set(CPACK_GENERATOR DEB)
elseif(REDHAT)
    set(CPACK_GENERATOR RPM)
else()
    set(CPACK_GENERATOR TGZ)
endif()

########################################################################
# Setup package file name
########################################################################
if(DEBIAN AND LIBUHD_PKG)
    set(CPACK_PACKAGE_FILE_NAME "libuhd${UHD_VERSION_MAJOR}_${UHD_VERSION}_${CMAKE_SYSTEM_PROCESSOR}" CACHE INTERNAL "")
elseif(DEBIAN AND LIBUHDDEV_PKG)
    set(CPACK_PACKAGE_FILE_NAME "libuhd-dev_${UHD_VERSION}_${CMAKE_SYSTEM_PROCESSOR}" CACHE INTERNAL "")
elseif(DEBIAN AND UHDHOST_PKG)
    set(CPACK_PACKAGE_FILE_NAME "uhd-host_${UHD_VERSION}_${CMAKE_SYSTEM_PROCESSOR}" CACHE INTERNAL "")
else()
    if(DEBIAN OR REDHAT)
        find_program(LSB_RELEASE_EXECUTABLE lsb_release)

        if(LSB_RELEASE_EXECUTABLE)
            #extract system information by executing the commands
            execute_process(
                COMMAND ${LSB_RELEASE_EXECUTABLE} --short --id
                OUTPUT_VARIABLE LSB_ID OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            execute_process(
                COMMAND ${LSB_RELEASE_EXECUTABLE} --short --release
                OUTPUT_VARIABLE LSB_RELEASE OUTPUT_STRIP_TRAILING_WHITESPACE
            )

            #set a more sensible package name for this system
            set(CPACK_PACKAGE_FILE_NAME "uhd_${UHD_VERSION}_${LSB_ID}-${LSB_RELEASE}-${CMAKE_SYSTEM_PROCESSOR}" CACHE INTERNAL "")
        endif(LSB_RELEASE_EXECUTABLE)
    endif(DEBIAN OR REDHAT)
endif(DEBIAN AND LIBUHD_PKG)

if(${CPACK_GENERATOR} STREQUAL NSIS)

    enable_language(C)

    include(CheckTypeSize)
    check_type_size("void*[8]" BIT_WIDTH BUILTIN_TYPES_ONLY)
    # If CMake option given, specify MSVC version in installer filename
    if(SPECIFY_MSVC_VERSION)
        if(MSVC90) # Visual Studio 2008 (9.0)
            set(MSVC_VERSION "VS2008")
        elseif(MSVC10) # Visual Studio 2010 (10.0)
            set(MSVC_VERSION "VS2010")
        elseif(MSVC11) # Visual Studio 2012 (11.0)
            set(MSVC_VERSION "VS2012")
        elseif(MSVC12) # Visual Studio 2013 (12.0)
            set(MSVC_VERSION "VS2013")
        elseif(MSVC14) # Visual Studio 2015 (14.0)
            set(MSVC_VERSION "VS2015")
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
set(CPACK_RESOURCE_FILE_WELCOME ${CMAKE_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)

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
# Setup CPack Debian
########################################################################
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-all-dev, python-requests")
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "python, python-tk")
foreach(filename preinst postinst prerm postrm)
    list(APPEND CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA ${CMAKE_BINARY_DIR}/debian/${filename})
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/debian)
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/debian/${filename}.in
        ${CMAKE_BINARY_DIR}/debian/${filename}
    @ONLY)
endforeach(filename)
configure_file(
    ${CMAKE_SOURCE_DIR}/cmake/debian/watch
    ${CMAKE_BINARY_DIR}/debian/watch
@ONLY)

########################################################################
# Setup CPack RPM
########################################################################
set(CPACK_RPM_PACKAGE_REQUIRES "boost-devel, python-requests")
set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST_ADDITION "/usr/share/man;/usr/share/man/man1;/usr/lib64/pkgconfig;/usr/lib64/cmake;/usr/lib64/python2.7;/usr/lib64/python2.7/site-packages")
foreach(filename post_install post_uninstall pre_install pre_uninstall)
    string(TOUPPER ${filename} filename_upper)
    list(APPEND CPACK_RPM_${filename_upper}_SCRIPT_FILE ${CMAKE_BINARY_DIR}/redhat/${filename})
    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/redhat)
    configure_file(
        ${CMAKE_SOURCE_DIR}/cmake/redhat/${filename}.in
        ${CMAKE_BINARY_DIR}/redhat/${filename}
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
