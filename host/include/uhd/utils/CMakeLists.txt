#
# Copyright 2010-2012,2015 Ettus Research LLC
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

UHD_INSTALL(FILES
    algorithm.hpp
    assert_has.hpp
    assert_has.ipp
    atomic.hpp
    byteswap.hpp
    byteswap.ipp
    cast.hpp
    csv.hpp
    fp_compare_delta.ipp
    fp_compare_epsilon.ipp
    gain_group.hpp
    log.hpp
    math.hpp
    msg.hpp
    msg_task.hpp
    paths.hpp
    pimpl.hpp
    platform.hpp
    safe_call.hpp
    safe_main.hpp
    static.hpp
    tasks.hpp
    thread_priority.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/utils
    COMPONENT headers
)

IF(ENABLE_C_API)
    UHD_INSTALL(FILES
        thread_priority.h
        DESTINATION ${INCLUDE_DIR}/uhd/utils
        COMPONENT headers
    )
ENDIF(ENABLE_C_API)
