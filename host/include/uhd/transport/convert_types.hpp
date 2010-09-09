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

#ifndef INCLUDED_UHD_TRANSPORT_CONVERT_TYPES_HPP
#define INCLUDED_UHD_TRANSPORT_CONVERT_TYPES_HPP

#include <uhd/config.hpp>
#include <uhd/types/io_type.hpp>
#include <uhd/types/otw_type.hpp>
#include <vector>

namespace uhd{ namespace transport{

/*!
 * Convert IO samples to OWT samples.
 *
 * \param io_buff memory containing samples
 * \param io_type the type of these samples
 * \param otw_buff memory to write converted samples
 * \param otw_type the type of these samples
 * \param num_samps the number of samples in io_buff
 */
UHD_API void convert_io_type_to_otw_type(
    const void *io_buff, const io_type_t &io_type,
    void *otw_buff, const otw_type_t &otw_type,
    size_t num_samps
);

/*!
 * Convert IO samples to OWT samples + interleave.
 *
 * \param io_buffs buffers containing samples
 * \param io_type the type of these samples
 * \param otw_buff memory to write converted samples
 * \param otw_type the type of these samples
 * \param nsamps_per_io_buff samples per io_buff
 */
UHD_API void convert_io_type_to_otw_type(
    const std::vector<const void *> &io_buffs,
    const io_type_t &io_type,
    void *otw_buff,
    const otw_type_t &otw_type,
    size_t nsamps_per_io_buff
);

/*!
 * Convert OTW samples to IO samples.
 *
 * \param otw_buff memory containing samples
 * \param otw_type the type of these samples
 * \param io_buff memory to write converted samples
 * \param io_type the type of these samples
 * \param num_samps the number of samples in io_buff
 */
UHD_API void convert_otw_type_to_io_type(
    const void *otw_buff, const otw_type_t &otw_type,
    void *io_buff, const io_type_t &io_type,
    size_t num_samps
);

/*!
 * Convert OTW samples to IO samples + de-interleave.
 *
 * \param otw_buff memory containing samples
 * \param otw_type the type of these samples
 * \param io_buffs buffers to write converted samples
 * \param io_type the type of these samples
 * \param nsamps_per_io_buff samples per io_buff
 */
UHD_API void convert_otw_type_to_io_type(
    const void *otw_buff,
    const otw_type_t &otw_type,
    std::vector<void *> &io_buffs,
    const io_type_t &io_type,
    size_t nsamps_per_io_buff
);

}} //namespace

#include <uhd/transport/convert_types.ipp>

#endif /* INCLUDED_UHD_TRANSPORT_CONVERT_TYPES_HPP */
