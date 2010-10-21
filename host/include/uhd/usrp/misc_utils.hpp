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

#ifndef INCLUDED_UHD_USRP_MISC_UTILS_HPP
#define INCLUDED_UHD_USRP_MISC_UTILS_HPP

#include <uhd/config.hpp>
#include <uhd/wax.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/utils/gain_group.hpp>

namespace uhd{ namespace usrp{

    /*!
     * Different policies for gain group prioritization.
     */
    enum gain_group_policy_t{
        GAIN_GROUP_POLICY_RX = 'R',
        GAIN_GROUP_POLICY_TX = 'T'
    };

    /*!
     * Create a gain group that represents the subdevice and its codec.
     * \param dboard_id the dboard id for this subdevice
     * \param subdev the object with subdevice properties
     * \param codec the object with codec properties
     * \param gain_group_policy the policy to use
     */
    UHD_API gain_group::sptr make_gain_group(
        const dboard_id_t &dboard_id,
        wax::obj subdev, wax::obj codec,
        gain_group_policy_t gain_group_policy
    );

    /*!
     * Verify the rx subdevice specification.
     * If the subdev spec if empty, automatically fill it.
     * \param subdev_spec the subdev spec to verify/fill
     * \param mboard the motherboard properties object
     * \throw exception when the subdev spec is invalid
     */
    UHD_API void verify_rx_subdev_spec(subdev_spec_t &subdev_spec, wax::obj mboard);

    /*!
     * Verify the tx subdevice specification.
     * If the subdev spec if empty, automatically fill it.
     * \param subdev_spec the subdev spec to verify/fill
     * \param mboard the motherboard properties object
     * \throw exception when the subdev spec is invalid
     */
    UHD_API void verify_tx_subdev_spec(subdev_spec_t &subdev_spec, wax::obj mboard);

}} //namespace

#endif /* INCLUDED_UHD_USRP_MISC_UTILS_HPP */

