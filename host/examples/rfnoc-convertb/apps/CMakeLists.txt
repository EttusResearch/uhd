#
# Copyright 2019 Ettus Research, a National Instruments Brand
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

# This app needs Boost
set(BOOST_REQUIRED_COMPONENTS
    program_options
    system
    python
)
set(THREADS_PREFER_PTHREAD_FLAG ON)#added for pthread
find_package(Threads REQUIRED)#added for pthread
find_package(PythonLibs REQUIRED)
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
    ${CMAKE_SOURCE_DIR}/include
    ${PYTHON_INCLUDE_DIRS}
)

add_executable(usrp_record_samples
    usrp_record_samples.cpp
)

add_executable(init_convert_block
    init_convert_block.cpp
)
target_link_libraries(usrp_record_samples
    ${UHD_LIBRARIES}
    ${Boost_LIBRARIES}
    -Wl,--no-as-needed
    rfnoc-convertb
    Threads::Threads  #added for pthread
    ${PYTHON_LIBRARIES}
)

target_link_libraries(init_convert_block
    ${UHD_LIBRARIES}
    ${Boost_LIBRARIES}
    -Wl,--no-as-needed
    rfnoc-convertb
)
### Once it's built... ########################################################
# Here, you would have commands to install your program.
# We will skip these in this example.
#install(
#    TARGETS init_convert_block
#    RUNTIME
#    DESTINATION ${PROJECT_DATA_DIR}
#    COMPONENT apps
#)
