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

#ifndef INCLUDED_UHD_TYPES_HPP
#define INCLUDED_UHD_TYPES_HPP

#include <uhd/config.hpp>
#include <string>

namespace uhd{

    typedef float gain_t; //TODO REMOVE
    typedef double freq_t; //TODO REMOVE

    /*!
     * The gain range struct describes possible gain settings.
     * The mimumum gain, maximum gain, and step size are in dB.
     */
    struct UHD_API gain_range_t{
        float min, max, step;
        gain_range_t(float min = 0.0, float max = 0.0, float step = 0.0);
    };

    /*!
     * The frequency range struct describes possible frequency settings.
     * Because tuning is very granular (sub-Hz), step size is not listed.
     * The mimumum frequency and maximum frequency are in Hz.
     */
    struct UHD_API freq_range_t{
        double min, max;
        freq_range_t(double min = 0.0, double max = 0.0);
    };

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

    /*!
     * Clock configuration settings:
     * The source for the 10MHz reference clock.
     * The source and polarity for the PPS clock.
     * Possible settings for the reference and pps source
     * are implementation specific motherboard properties.
     * See the MBOARD_PROP_XXX_SOURCE_NAMES properties.
     */
    struct clock_config_t{
        enum polarity_t {POLARITY_NEG, POLARITY_POS};
        std::string ref_source;
        std::string pps_source;
        polarity_t  pps_polarity;
        clock_config_t(void);
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_HPP */
