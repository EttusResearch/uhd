#
# Copyright 2010-2011,2015 Ettus Research LLC
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
    byte_vector.hpp
    clock_config.hpp
    device_addr.hpp
    dict.ipp
    dict.hpp
    direction.hpp
    endianness.hpp
    io_type.hpp
    mac_addr.hpp
    metadata.hpp
    otw_type.hpp
    ranges.hpp
    ref_vector.hpp
    sensors.hpp
    serial.hpp
    sid.hpp
    stream_cmd.hpp
    time_spec.hpp
    tune_request.hpp
    tune_result.hpp
    wb_iface.hpp
    filters.hpp
    DESTINATION ${INCLUDE_DIR}/uhd/types
    COMPONENT headers
)

IF(ENABLE_C_API)
    UHD_INSTALL(FILES
        metadata.h
        ranges.h
        sensors.h
        string_vector.h
        tune_request.h
        tune_result.h
        usrp_info.h
        DESTINATION ${INCLUDE_DIR}/uhd/types
        COMPONENT headers
    )
ENDIF(ENABLE_C_API)
