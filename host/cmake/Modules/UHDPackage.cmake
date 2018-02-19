#
# Copyright 2010-2016 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
INCLUDE(UHDVersion) #sets version information

SET(UHD_RELEASE_MODE "${UHD_RELEASE_MODE}" CACHE BOOL "set UHD to release mode to build installers")

########################################################################
# Setup additional defines for OS types
########################################################################
IF(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    SET(LINUX TRUE)
ENDIF()

IF(NOT CMAKE_CROSSCOMPILING AND LINUX AND EXISTS "/etc/debian_version")
    SET(DEBIAN TRUE)
ENDIF()

IF(NOT CMAKE_CROSSCOMPILING AND LINUX AND EXISTS "/etc/redhat-release")
    SET(REDHAT TRUE)
ENDIF()

########################################################################
# Set generator type for recognized systems
########################################################################
IF(CPACK_GENERATOR)
    #already set
ELSEIF(APPLE)
    SET(CPACK_GENERATOR PackageMaker)
ELSEIF(WIN32)
    SET(CPACK_GENERATOR NSIS)
ELSEIF(DEBIAN)
    SET(CPACK_GENERATOR DEB)
ELSEIF(REDHAT)
    SET(CPACK_GENERATOR RPM)
ELSE()
    SET(CPACK_GENERATOR TGZ)
ENDIF()

########################################################################
# Setup package file name
########################################################################
IF(DEBIAN AND LIBUHD_PKG)
    SET(CPACK_PACKAGE_FILE_NAME "libuhd${UHD_VERSION_MAJOR}_${UHD_VERSION}_${CMAKE_SYSTEM_PROCESSOR}" CACHE INTERNAL "")
ELSEIF(DEBIAN AND LIBUHDDEV_PKG)
    SET(CPACK_PACKAGE_FILE_NAME "libuhd-dev_${UHD_VERSION}_${CMAKE_SYSTEM_PROCESSOR}" CACHE INTERNAL "")
ELSEIF(DEBIAN AND UHDHOST_PKG)
    SET(CPACK_PACKAGE_FILE_NAME "uhd-host_${UHD_VERSION}_${CMAKE_SYSTEM_PROCESSOR}" CACHE INTERNAL "")
ELSE()
    IF(DEBIAN OR REDHAT)
        FIND_PROGRAM(LSB_RELEASE_EXECUTABLE lsb_release)

        IF(LSB_RELEASE_EXECUTABLE)
            #extract system information by executing the commands
            EXECUTE_PROCESS(
                COMMAND ${LSB_RELEASE_EXECUTABLE} --short --id
                OUTPUT_VARIABLE LSB_ID OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            EXECUTE_PROCESS(
                COMMAND ${LSB_RELEASE_EXECUTABLE} --short --release
                OUTPUT_VARIABLE LSB_RELEASE OUTPUT_STRIP_TRAILING_WHITESPACE
            )

            #set a more sensible package name for this system
            SET(CPACK_PACKAGE_FILE_NAME "uhd_${UHD_VERSION}_${LSB_ID}-${LSB_RELEASE}-${CMAKE_SYSTEM_PROCESSOR}" CACHE INTERNAL "")
        ENDIF(LSB_RELEASE_EXECUTABLE)
    ENDIF(DEBIAN OR REDHAT)
ENDIF(DEBIAN AND LIBUHD_PKG)

IF(${CPACK_GENERATOR} STREQUAL NSIS)

    ENABLE_LANGUAGE(C)

    include(CheckTypeSize)
    check_type_size("void*[8]" BIT_WIDTH BUILTIN_TYPES_ONLY)
    # If CMake option given, specify MSVC version in installer filename
    IF(SPECIFY_MSVC_VERSION)
        IF(MSVC90) # Visual Studio 2008 (9.0)
            SET(MSVC_VERSION "VS2008")
        ELSEIF(MSVC10) # Visual Studio 2010 (10.0)
            SET(MSVC_VERSION "VS2010")
        ELSEIF(MSVC11) # Visual Studio 2012 (11.0)
            SET(MSVC_VERSION "VS2012")
        ELSEIF(MSVC12) # Visual Studio 2013 (12.0)
            SET(MSVC_VERSION "VS2013")
        ELSEIF(MSVC14) # Visual Studio 2015 (14.0)
            SET(MSVC_VERSION "VS2015")
        ENDIF()
        SET(CPACK_PACKAGE_FILE_NAME "uhd_${UHD_VERSION}_Win${BIT_WIDTH}_${MSVC_VERSION}" CACHE INTERNAL "")
    ELSE()
        SET(CPACK_PACKAGE_FILE_NAME "uhd_${UHD_VERSION}_Win${BIT_WIDTH}" CACHE INTERNAL "")
    ENDIF(SPECIFY_MSVC_VERSION)

    SET(CPACK_PACKAGE_INSTALL_DIRECTORY "${CMAKE_PROJECT_NAME}")
ENDIF()

########################################################################
# Setup CPack General
########################################################################
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Ettus Research - USRP Hardware Driver")
SET(CPACK_PACKAGE_VENDOR              "Ettus Research (National Instruments)")
SET(CPACK_PACKAGE_CONTACT             "Ettus Research <support@ettus.com>")
SET(CPACK_PACKAGE_VERSION "${UHD_VERSION}")
SET(CPACK_RESOURCE_FILE_WELCOME ${CMAKE_SOURCE_DIR}/README.md)
SET(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/LICENSE)

########################################################################
# Setup CPack Source
########################################################################

SET(CPACK_SOURCE_PACKAGE_FILE_NAME "uhd-${UHD_VERSION}" CACHE INTERNAL "")
SET(CPACK_SOURCE_IGNORE_FILES "\\\\.git*;\\\\.swp$")

########################################################################
# Setup CPack Components
########################################################################
SET(CPACK_COMPONENT_LIBRARIES_GROUP      "Development")
SET(CPACK_COMPONENT_HEADERS_GROUP        "Development")
SET(CPACK_COMPONENT_UTILITIES_GROUP      "Runtime")
SET(CPACK_COMPONENT_EXAMPLES_GROUP       "Runtime")
SET(CPACK_COMPONENT_MANUAL_GROUP         "Documentation")
SET(CPACK_COMPONENT_DOXYGEN_GROUP        "Documentation")
SET(CPACK_COMPONENT_README_GROUP         "Documentation")

SET(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME      "Libraries")
SET(CPACK_COMPONENT_HEADERS_DISPLAY_NAME        "C++ Headers")
SET(CPACK_COMPONENT_UTILITIES_DISPLAY_NAME      "Utilities")
SET(CPACK_COMPONENT_EXAMPLES_DISPLAY_NAME       "Examples")
SET(CPACK_COMPONENT_MANUAL_DISPLAY_NAME         "Manual")
SET(CPACK_COMPONENT_DOXYGEN_DISPLAY_NAME        "Doxygen")
SET(CPACK_COMPONENT_README_DISPLAY_NAME         "Readme")
SET(CPACK_COMPONENT_IMAGES_DISPLAY_NAME         "Images")

SET(CPACK_COMPONENT_LIBRARIES_DESCRIPTION     "Dynamic link library")
SET(CPACK_COMPONENT_HEADERS_DESCRIPTION       "C++ development headers")
SET(CPACK_COMPONENT_UTILITIES_DESCRIPTION     "Utility executables and python scripts")
SET(CPACK_COMPONENT_EXAMPLES_DESCRIPTION      "Example executables")
SET(CPACK_COMPONENT_MANUAL_DESCRIPTION        "Manual/application notes (rst and html)")
SET(CPACK_COMPONENT_DOXYGEN_DESCRIPTION       "API documentation (html)")
SET(CPACK_COMPONENT_README_DESCRIPTION        "Readme files (txt)")
SET(CPACK_COMPONENT_IMAGES_DESCRIPTION        "FPGA and firmware images")

SET(CPACK_COMPONENT_README_REQUIRED TRUE)

SET(CPACK_COMPONENT_UTILITIES_DEPENDS libraries)
SET(CPACK_COMPONENT_EXAMPLES_DEPENDS libraries)
SET(CPACK_COMPONENT_TESTS_DEPENDS libraries)

SET(CPACK_COMPONENTS_ALL libraries headers utilities examples manual doxygen readme images)

########################################################################
# Setup CPack Debian
########################################################################
SET(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-all-dev, python-requests")
SET(CPACK_DEBIAN_PACKAGE_RECOMMENDS "python, python-tk")
FOREACH(filename preinst postinst prerm postrm)
    LIST(APPEND CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA ${CMAKE_BINARY_DIR}/debian/${filename})
    FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/debian)
    CONFIGURE_FILE(
        ${CMAKE_SOURCE_DIR}/cmake/debian/${filename}.in
        ${CMAKE_BINARY_DIR}/debian/${filename}
    @ONLY)
ENDFOREACH(filename)
CONFIGURE_FILE(
    ${CMAKE_SOURCE_DIR}/cmake/debian/watch
    ${CMAKE_BINARY_DIR}/debian/watch
@ONLY)

########################################################################
# Setup CPack RPM
########################################################################
SET(CPACK_RPM_PACKAGE_REQUIRES "boost-devel, python-requests")

FOREACH(filename post_install post_uninstall pre_install pre_uninstall)
    STRING(TOUPPER ${filename} filename_upper)
    LIST(APPEND CPACK_RPM_${filename_upper}_SCRIPT_FILE ${CMAKE_BINARY_DIR}/redhat/${filename})
    FILE(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/redhat)
    CONFIGURE_FILE(
        ${CMAKE_SOURCE_DIR}/cmake/redhat/${filename}.in
        ${CMAKE_BINARY_DIR}/redhat/${filename}
    @ONLY)
ENDFOREACH(filename)

########################################################################
# Setup CPack NSIS
########################################################################
SET(CPACK_NSIS_MODIFY_PATH ON)

SET(HLKM_ENV "\\\"SYSTEM\\\\CurrentControlSet\\\\Control\\\\Session Manager\\\\Environment\\\"")

SET(CPACK_NSIS_EXTRA_INSTALL_COMMANDS "
    WriteRegStr HKLM ${HLKM_ENV} \\\"UHD_PKG_PATH\\\" \\\"$INSTDIR\\\"
")

SET(CPACK_NSIS_EXTRA_UNINSTALL_COMMANDS "
    DeleteRegValue HKLM ${HLKM_ENV} \\\"UHD_PKG_PATH\\\"
")

IF(WIN32)
    #Install necessary runtime DLL's
    INCLUDE(InstallRequiredSystemLibraries)
ENDIF(WIN32)

########################################################################
INCLUDE(CPack) #include after setting vars
