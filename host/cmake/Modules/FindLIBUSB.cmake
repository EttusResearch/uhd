
#find libusb 1.0 on various systems
#sets LIBUSB_FOUND, LIBUSB_LIBRARIES, LIBUSB_INCLUDE_DIRS
#override LIBUSB_LIBRARIES LIBUSB_INCLUDE_DIRS to manually set

if(NOT PKG_CONFIG_FOUND)
    include(FindPkgConfig)
endif()
pkg_check_modules(PC_LIBUSB QUIET libusb-1.0)

find_path(LIBUSB_INCLUDE_DIRS
    NAMES libusb.h
    HINTS $ENV{LIBUSB_DIR}/include $ENV{LIBUSB_DIR}/include/libusb-1.0
    ${PC_LIBUSB_INCLUDEDIR} ${PC_LIBUSB_INCLUDEDIR}/libusb-1.0
    PATHS /usr/local/include/libusb-1.0 /usr/local/include
    /usr/include/libusb-1.0 /usr/include
    /opt/local/include/libusb-1.0

    #non-conforming naming convention,
    #backwards compatible with old script
    ${LIBUSB_INCLUDE_DIR}
)

#standard library name for libusb-1.0
set(libusb1_library_names usb-1.0 libusb-1.0)

#libusb-1.0 compatible library on freebsd
if(CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" OR CMAKE_SYSTEM_NAME STREQUAL "kFreeBSD")
    list(APPEND libusb1_library_names usb)
endif()

find_library(LIBUSB_LIBRARIES
    NAMES ${libusb1_library_names}
    HINTS $ENV{LIBUSB_DIR}/lib ${PC_LIBUSB_LIBDIR}
    PATHS /usr/local/lib /usr/lib /opt/local/lib
)

enable_language(C) #needs C support for check below
include(CheckFunctionExists)
if(LIBUSB_INCLUDE_DIRS)
    set(CMAKE_REQUIRED_INCLUDES ${LIBUSB_INCLUDE_DIRS})
endif()
if(LIBUSB_LIBRARIES)
    set(CMAKE_REQUIRED_LIBRARIES ${LIBUSB_LIBRARIES})
endif()

CHECK_FUNCTION_EXISTS("libusb_handle_events_timeout_completed" HAVE_LIBUSB_HANDLE_EVENTS_TIMEOUT_COMPLETED)
if(HAVE_LIBUSB_HANDLE_EVENTS_TIMEOUT_COMPLETED)
    list(APPEND LIBUSB_DEFINITIONS "HAVE_LIBUSB_HANDLE_EVENTS_TIMEOUT_COMPLETED=1")
endif(HAVE_LIBUSB_HANDLE_EVENTS_TIMEOUT_COMPLETED)

CHECK_FUNCTION_EXISTS("libusb_error_name" HAVE_LIBUSB_ERROR_NAME)
if(HAVE_LIBUSB_ERROR_NAME)
    list(APPEND LIBUSB_DEFINITIONS "HAVE_LIBUSB_ERROR_NAME=1")
endif(HAVE_LIBUSB_ERROR_NAME)

CHECK_FUNCTION_EXISTS("libusb_strerror" HAVE_LIBUSB_STRERROR)
if(HAVE_LIBUSB_STRERROR)
    list(APPEND LIBUSB_DEFINITIONS "HAVE_LIBUSB_STRERROR=1")
endif(HAVE_LIBUSB_STRERROR)
set(CMAKE_REQUIRED_LIBRARIES)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBUSB DEFAULT_MSG LIBUSB_LIBRARIES LIBUSB_INCLUDE_DIRS)
mark_as_advanced(LIBUSB_INCLUDE_DIRS LIBUSB_LIBRARIES)
