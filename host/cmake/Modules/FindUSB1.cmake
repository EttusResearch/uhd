
#find libusb 1.0 on various systems
#sets LIBUSB_FOUND, LIBUSB_LIBRARIES, LIBUSB_INCLUDE_DIRS
#override LIBUSB_LIBRARIES LIBUSB_INCLUDE_DIRS to manually set

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_LIBUSB QUIET libusb-1.0)

FIND_PATH(LIBUSB_INCLUDE_DIRS
    NAMES libusb.h
    HINTS $ENV{LIBUSB_DIR}/include ${PC_LIBUSB_INCLUDEDIR}
    PATHS /usr/local/include/libusb-1.0 /usr/local/include
    /usr/include/libusb-1.0 /usr/include
    /opt/local/include/libusb-1.0

    #non-conforming naming convention,
    #backwards compatible with old script
    ${LIBUSB_INCLUDE_DIR}
)

#standard library name for libusb-1.0
set(libusb1_library_names usb-1.0)

#libusb-1.0 compatible library on freebsd
if((CMAKE_SYSTEM_NAME STREQUAL "FreeBSD") OR (CMAKE_SYSTEM_NAME STREQUAL "kFreeBSD"))
    list(APPEND libusb1_library_names usb)
endif()

FIND_LIBRARY(LIBUSB_LIBRARIES
    NAMES ${libusb1_library_names}
    HINTS $ENV{LIBUSB_DIR}/lib ${PC_LIBUSB_LIBDIR}
    PATHS /usr/local/lib /usr/lib /opt/local/lib
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(LIBUSB DEFAULT_MSG LIBUSB_LIBRARIES LIBUSB_INCLUDE_DIRS)
MARK_AS_ADVANCED(LIBUSB_INCLUDE_DIRS LIBUSB_LIBRARIES)

