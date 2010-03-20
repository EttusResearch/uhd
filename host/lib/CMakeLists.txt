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

########################################################################
# Create a list of libuhd sources
########################################################################
SET(libuhd_sources
    device.cpp
    device_addr.cpp
    gain_handler.cpp
    metadata.cpp
    simple_device.cpp
    time_spec.cpp
    wax.cpp
    transport/udp_simple.cpp
    transport/vrt.cpp
    usrp/dboard/basic.cpp
    usrp/dboard_base.cpp
    usrp/dboard_interface.cpp
    usrp/dboard_manager.cpp
    usrp/usrp2/dboard_impl.cpp
    usrp/usrp2/dboard_interface.cpp
    usrp/usrp2/dsp_impl.cpp
    usrp/usrp2/io_impl.cpp
    usrp/usrp2/mboard_impl.cpp
    usrp/usrp2/usrp2_impl.cpp
)

########################################################################
# Conditionally add the udp sources
########################################################################
LIST(APPEND libuhd_sources
    transport/udp_zero_copy_none.cpp
)

########################################################################
# Conditionally add the usrp1e sources
########################################################################
LIST(APPEND libuhd_sources
    usrp/usrp1e/usrp1e_none.cpp
)

########################################################################
# Setup libuhd library
########################################################################
ADD_LIBRARY(uhd SHARED ${libuhd_sources})

TARGET_LINK_LIBRARIES(uhd ${Boost_LIBRARIES})

SET_TARGET_PROPERTIES(uhd PROPERTIES DEFINE_SYMBOL "UHD_DLL_EXPORTS")

INSTALL(TARGETS uhd
    LIBRARY DESTINATION ${LIBRARY_DIR} # .so file
    ARCHIVE DESTINATION ${LIBRARY_DIR} # .lib file
    RUNTIME DESTINATION ${LIBRARY_DIR} # .dll file
)
