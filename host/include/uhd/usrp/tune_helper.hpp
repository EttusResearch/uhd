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

#ifndef INCLUDED_UHD_USRP_TUNE_HELPER_HPP
#define INCLUDED_UHD_USRP_TUNE_HELPER_HPP

#include <uhd/config.hpp>
#include <uhd/wax.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/types/tune_result.hpp>

namespace uhd{ namespace usrp{

    /*!
     * Tune a rx chain to the desired frequency:
     * The IF of the subdevice is set as close as possible to
     * the given target frequency + the LO offset (when applicable).
     * The ddc cordic is setup to bring the IF down to baseband.
     * \param subdev the dboard subdevice object with properties
     * \param ddc the mboard dsp object with properties
     * \param tune_request tune request instructions
     * \return a tune result struct
     */
    UHD_API tune_result_t tune_rx_subdev_and_dsp(
        wax::obj subdev, wax::obj ddc, const tune_request_t &tune_request
    );

    /*!
     * Calculate the overall frequency from the combination of dboard IF and DDC shift.
     * \param subdev the dboard subdevice object with properties
     * \param ddc the mboard dsp object with properties
     * \return the overall tune frequency of the system in Hz
     */
    UHD_API double derive_freq_from_rx_subdev_and_dsp(
        wax::obj subdev, wax::obj ddc
    );

    /*!
     * Tune a tx chain to the desired frequency:
     * The IF of the subdevice is set as close as possible to
     * the given target frequency + the LO offset (when applicable).
     * The duc cordic is setup to bring the baseband up to IF.
     * \param subdev the dboard subdevice object with properties
     * \param duc the mboard dsp object with properties
     * \param tune_request tune request instructions
     * \return a tune result struct
     */
    UHD_API tune_result_t tune_tx_subdev_and_dsp(
        wax::obj subdev, wax::obj duc, const tune_request_t &tune_request
    );

    /*!
     * Calculate the overall frequency from the combination of dboard IF and DUC shift.
     * \param subdev the dboard subdevice object with properties
     * \param duc the mboard dsp object with properties
     * \return the overall tune frequency of the system in Hz
     */
    UHD_API double derive_freq_from_tx_subdev_and_dsp(
        wax::obj subdev, wax::obj duc
    );

}}

#endif /* INCLUDED_UHD_USRP_TUNE_HELPER_HPP */
