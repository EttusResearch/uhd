find_package(PkgConfig)

PKG_CHECK_MODULES(PC_GR_RFNOC_GAIN gnuradio-rfnoc_gain)

FIND_PATH(
    GR_RFNOC_GAIN_INCLUDE_DIRS
    NAMES gnuradio/rfnoc_gain/api.h
    HINTS $ENV{RFNOC_GAIN_DIR}/include
        ${PC_RFNOC_GAIN_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

FIND_LIBRARY(
    GR_RFNOC_GAIN_LIBRARIES
    NAMES gnuradio-rfnoc_gain
    HINTS $ENV{RFNOC_GAIN_DIR}/lib
        ${PC_RFNOC_GAIN_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include("${CMAKE_CURRENT_LIST_DIR}/gnuradio-rfnoc_gainTarget.cmake")

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(GR_RFNOC_GAIN DEFAULT_MSG GR_RFNOC_GAIN_LIBRARIES GR_RFNOC_GAIN_INCLUDE_DIRS)
MARK_AS_ADVANCED(GR_RFNOC_GAIN_LIBRARIES GR_RFNOC_GAIN_INCLUDE_DIRS)
