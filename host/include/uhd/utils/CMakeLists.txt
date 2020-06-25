#
# Copyright 2010-2012,2015 Ettus Research LLC
# Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

add_subdirectory(chdr)

UHD_INSTALL(FILES
    algorithm.hpp
    assert_has.hpp
    assert_has.ipp
    byteswap.hpp
    byteswap.ipp
    cast.hpp
    csv.hpp
    dirty_tracked.hpp
    fp_compare_delta.ipp
    fp_compare_epsilon.ipp
    gain_group.hpp
    graph_utils.hpp
    interpolation.hpp
    log.hpp
    log_add.hpp
    math.hpp
    msg_task.hpp
    noncopyable.hpp
    paths.hpp
    pimpl.hpp
    platform.hpp
    pybind_adaptors.hpp
    safe_call.hpp
    safe_main.hpp
    scope_exit.hpp
    static.hpp
    tasks.hpp
    thread_priority.hpp
    thread.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/utils
    COMPONENT headers
)

if(ENABLE_C_API)
    UHD_INSTALL(FILES
        thread_priority.h
        log.h
        DESTINATION ${INCLUDE_DIR}/uhd/utils
        COMPONENT headers
    )
endif(ENABLE_C_API)
