########################################################################
# Find the library for ORC development files
########################################################################

INCLUDE(FindPkgConfig)
PKG_CHECK_MODULES(PC_ORC "orc-0.4 > 0.4.11")

FIND_PATH(
    ORC_INCLUDE_DIRS
    NAMES orc/orc.h
    HINTS $ENV{ORC_DIR}/include/orc-0.4
        ${PC_ORC_INCLUDEDIR}
    PATHS /usr/local/include/orc-0.4
          /usr/include/orc-0.4
)

FIND_LIBRARY(
    ORC_LIBRARIES
    NAMES orc-0.4
    HINTS $ENV{ORC_DIR}/lib
        ${PC_ORC_LIBDIR}
    PATHS /usr/local/lib
          /usr/lib
)

INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ORC DEFAULT_MSG ORC_LIBRARIES ORC_INCLUDE_DIRS)
MARK_AS_ADVANCED(ORC_LIBRARIES ORC_INCLUDE_DIRS)
