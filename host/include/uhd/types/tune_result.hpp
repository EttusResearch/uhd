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
#include <string>

namespace uhd{

    /*!
     * The tune result struct holds result of a 2-phase tuning.
     */
    struct UHD_API tune_result_t{
        double target_rf_freq;
        double actual_rf_freq;
        double target_dsp_freq;
        double actual_dsp_freq;

        /*!
         * Create a pretty print string for this tune result struct.
         * \return the printable string
         */
        std::string to_pp_string(void) const;
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_TUNE_RESULT_HPP */
