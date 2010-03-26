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
#include <uhd/time_spec.hpp>
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
     */
    struct UHD_API clock_config_t{
        enum ref_source_t {
            REF_INT, //internal reference
            REF_SMA, //external sma port
            REF_MIMO //mimo cable (usrp2 only)
        } ref_source;
        enum pps_source_t {
            PPS_INT, //there is no internal
            PPS_SMA, //external sma port
            PPS_MIMO //mimo cable (usrp2 only)
        } pps_source;
        enum pps_polarity_t {
            PPS_NEG, //negative edge
            PPS_POS  //positive edge
        } pps_polarity;
        clock_config_t(void);
    };

    /*!
     * Command struct for configuration and control of streaming:
     *
     * A stream command defines how the device sends samples to the host.
     * Streaming is controlled by submitting a stream command to the rx dsp.
     * Granular control over what the device streams to the host can be
     * achieved through submission of multiple (carefully-crafted) commands.
     *
     * The stream_now parameter controls when the stream begins.
     * When true, the device will begin streaming ASAP. When false,
     * the device will begin streaming at a time specified by time_spec.
     *
     * The continuous parameter controls the number of samples received.
     * When true, the device continues streaming indefinitely. When false,
     * the device will stream the number of samples specified by num_samps.
     *
     * Standard usage case:
     * To start continuous streaming, set stream_now to true and continuous to true.
     * To end continuous streaming, set stream_now to true and continuous to false.
     */
    struct UHD_API stream_cmd_t{
        bool stream_now;
        time_spec_t time_spec;
        bool continuous;
        size_t num_samps;
        stream_cmd_t(void);
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_HPP */
