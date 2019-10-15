#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

# This app needs Boost
set(BOOST_REQUIRED_COMPONENTS
    program_options
    system
)
if(MSVC)
    set(BOOST_ALL_DYN_LINK "${BOOST_ALL_DYN_LINK}" CACHE BOOL "boost enable dynamic linking")
    if(BOOST_ALL_DYN_LINK)
        add_definitions(-DBOOST_ALL_DYN_LINK) #setup boost auto-linking in msvc
    else(BOOST_ALL_DYN_LINK)
        set(BOOST_REQUIRED_COMPONENTS) #empty components list for static link
    endif(BOOST_ALL_DYN_LINK)
endif(MSVC)
find_package(Boost 1.58 REQUIRED ${BOOST_REQUIRED_COMPONENTS})

include_directories(
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_BINARY_DIR}/lib
    ${CMAKE_BINARY_DIR}/include
    ${UHD_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIR}
)
link_directories(
    ${Boost_LIBRARY_DIRS}
)

add_executable(init_gain_block
    init_gain_block.cpp
)
target_link_libraries(init_gain_block
    ${UHD_LIBRARIES}
    ${Boost_LIBRARIES}
)
