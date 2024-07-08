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
find_package(Boost 1.65 REQUIRED ${BOOST_REQUIRED_COMPONENTS})

add_executable(probe_gain_block
    probe_gain_block.cpp
)
target_include_directories(
    probe_gain_block
    PRIVATE ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(probe_gain_block
    ${UHD_LIBRARIES}
    ${Boost_LIBRARIES}
    -Wl,--no-as-needed
    rfnoc-gain
)

### Once it's built... ########################################################
# Typically you would install the apps into $PATH, which is what we do here.
install(
    TARGETS probe_gain_block
    RUNTIME
    DESTINATION ${RUNTIME_DIR}
    COMPONENT apps
)
