//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_UHD_TRANSPORT_CONVERT_TYPES_IPP
#define INCLUDED_UHD_TRANSPORT_CONVERT_TYPES_IPP

UHD_INLINE void uhd::transport::convert_io_type_to_otw_type(
    const void *io_buff, const io_type_t &io_type,
    void *otw_buff, const otw_type_t &otw_type,
    size_t num_samps
){
    std::vector<const void *> buffs(1, io_buff);
    return uhd::transport::convert_io_type_to_otw_type(
        buffs, io_type, otw_buff, otw_type, num_samps
    );
}

UHD_INLINE void uhd::transport::convert_otw_type_to_io_type(
    const void *otw_buff, const otw_type_t &otw_type,
    void *io_buff, const io_type_t &io_type,
    size_t num_samps
){
    std::vector<void *> buffs(1, io_buff);
    return uhd::transport::convert_otw_type_to_io_type(
        otw_buff, otw_type, buffs, io_type, num_samps
    );
}

#endif /* INCLUDED_UHD_TRANSPORT_CONVERT_TYPES_IPP */
