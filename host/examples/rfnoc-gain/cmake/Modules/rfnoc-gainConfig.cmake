find_package(PkgConfig)
PKG_CHECK_MODULES(PC_RFNOC_GAIN rfnoc-gain)

find_path(
    RFNOC_GAIN_INCLUDE_DIRS
    NAMES rfnoc/gain/config.hpp
    HINTS $ENV{RFNOC_GAIN_DIR}/include
        ${PC_RFNOC_GAIN_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

find_library(
    RFNOC_GAIN_LIBRARIES
    NAMES rfnoc-gain
    HINTS $ENV{RFNOC_GAIN_DIR}/lib
        ${PC_RFNOC_GAIN_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
          )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(rfnoc-gain
    DEFAULT_MSG
    RFNOC_GAIN_LIBRARIES
    RFNOC_GAIN_INCLUDE_DIRS)
mark_as_advanced(
    RFNOC_GAIN_LIBRARIES
    RFNOC_GAIN_INCLUDE_DIRS)
