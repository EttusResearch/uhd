########################################################################
# Toolchain file for cross building for ARM Cortex A8 w/ NEON
# Usage:  cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchains/arm_cortex_a8_\
cross.cmake -DENABLE_E100=ON -DENABLE_USRP_E_UTILS=TRUE -DENABLE_ORC=ON \
-DCAMKE_INSTALL_PREFIX=./install ../
########################################################################
set( CMAKE_SYSTEM_NAME Linux )
set( CMAKE_CXX_COMPILER arm-angstrom-linux-gnueabi-g++ )
set( CMAKE_C_COMPILER  arm-angstrom-linux-gnueabi-gcc )
set( CMAKE_CXX_FLAGS "-march=armv7-a -mtune=cortex-a8 -mfpu=neon -mfloat-abi=softfp"  CACHE STRING "" FORCE )
set( CMAKE_C_FLAGS ${CMAKE_CXX_FLAGS} CACHE STRING "" FORCE ) #same flags for C sources

set( CMAKE_FIND_ROOT_PATH /usr/local/angstrom/arm/
/usr/local/angstrom/arm/arm-angstrom-linux-gnueabi )

#set( BOOST_ROOT ${CMAKE_FIND_ROOT_PATH} )
#set( BOOST_INCLUDEDIR ${CMAKE_FIND_ROOT_PATH}/usr/include/boost )
#set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
