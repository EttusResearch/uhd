#
# Copyright 2010 Ettus Research LLC
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


ADD_LIBRARY(uhd SHARED
    device.cpp
    device_addr.cpp
    gain_handler.cpp
    shared_iovec.cpp
    uhd.cpp
    wax.cpp
    transport/udp.cpp
    usrp/usrp.cpp
    usrp/dboard/basic.cpp
    usrp/dboard/base.cpp
    usrp/dboard/id.cpp
    usrp/dboard/interface.cpp
    usrp/dboard/manager.cpp
    usrp/mboard/base.cpp
    usrp/mboard/usrp2.cpp
    usrp/mboard/usrp2/impl_base.cpp
    usrp/mboard/usrp2/dboard_impl.cpp
    usrp/mboard/usrp2/dboard_interface.cpp
)

TARGET_LINK_LIBRARIES(uhd ${Boost_LIBRARIES})

INSTALL(TARGETS uhd LIBRARY DESTINATION ${LIBRARY_DIR})
