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

#ifndef INCLUDED_UHD_TYPES_TUNE_RESULT_HPP
#define INCLUDED_UHD_TYPES_TUNE_RESULT_HPP

#include <uhd/config.hpp>

namespace uhd{

    /*!
     * The tune result struct holds result of a 2-phase tuning:
     * The struct hold the result of tuning the dboard as
     * the target and actual intermediate frequency.
     * The struct hold the result of tuning the DDC/DUC as
     * the target and actual digital converter frequency.
     * It also tell us weather or not the spectrum is inverted.
     */
    struct UHD_API tune_result_t{
        double target_inter_freq;
        double actual_inter_freq;
        double target_dxc_freq;
        double actual_dxc_freq;
        bool spectrum_inverted;
        tune_result_t(void);
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_TUNE_RESULT_HPP */
