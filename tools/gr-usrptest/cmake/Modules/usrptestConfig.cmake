include(FindPkgConfig)
PKG_CHECK_MODULES(PC_USRPTEST usrptest)

find_path(
    USRPTEST_INCLUDE_DIRS
    NAMES usrptest/api.h
    HINTS $ENV{USRPTEST_DIR}/include
        ${PC_USRPTEST_INCLUDEDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/include
          /usr/local/include
          /usr/include
)

find_library(
    USRPTEST_LIBRARIES
    NAMES gnuradio-usrptest
    HINTS $ENV{USRPTEST_DIR}/lib
        ${PC_USRPTEST_LIBDIR}
    PATHS ${CMAKE_INSTALL_PREFIX}/lib
          ${CMAKE_INSTALL_PREFIX}/lib64
          /usr/local/lib
          /usr/local/lib64
          /usr/lib
          /usr/lib64
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(USRPTEST DEFAULT_MSG USRPTEST_LIBRARIES USRPTEST_INCLUDE_DIRS)
mark_as_advanced(USRPTEST_LIBRARIES USRPTEST_INCLUDE_DIRS)

