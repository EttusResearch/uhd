#
# Copyright 2010-2012 Ettus Research LLC
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

INSTALL(FILES
    algorithm.hpp
    assert_has.hpp
    assert_has.ipp
    atomic.hpp
    byteswap.hpp
    byteswap.ipp
    csv.hpp
    gain_group.hpp
    images.hpp
    log.hpp
    msg.hpp
    paths.hpp
    pimpl.hpp
    safe_call.hpp
    safe_main.hpp
    static.hpp
    tasks.hpp
    thread_priority.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/utils
    COMPONENT headers
)
