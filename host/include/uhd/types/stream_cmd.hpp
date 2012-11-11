//
// Copyright 2010-2012 Ettus Research LLC
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

#ifndef INCLUDED_UHD_TYPES_STREAM_CMD_HPP
#define INCLUDED_UHD_TYPES_STREAM_CMD_HPP

#include <uhd/config.hpp>
#include <uhd/types/time_spec.hpp>

namespace uhd{

    /*!
     * Command struct for configuration and control of streaming:
     *
     * A stream command defines how the device sends samples to the host.
     * Streaming is controlled by submitting a stream command to the rx dsp.
     * Granular control over what the device streams to the host can be
     * achieved through submission of multiple (carefully-crafted) commands.
     *
     * The mode parameter controls how streaming is issued to the device:
     *   - "Start continuous" tells the device to stream samples indefinitely.
     *   - "Stop continuous" tells the device to end continuous streaming.
     *   - "Num samps and done" tells the device to stream num samps and
     *      to not expect a future stream command for contiguous samples.
     *   - "Num samps and more" tells the device to stream num samps and
     *      to expect a future stream command for contiguous samples.
     *
     * The stream now parameter controls when the stream begins.
     * When true, the device will begin streaming ASAP. When false,
     * the device will begin streaming at a time specified by time_spec.
     */
    struct UHD_API stream_cmd_t{

        enum stream_mode_t {
            STREAM_MODE_START_CONTINUOUS   = int('a'),
            STREAM_MODE_STOP_CONTINUOUS    = int('o'),
            STREAM_MODE_NUM_SAMPS_AND_DONE = int('d'),
            STREAM_MODE_NUM_SAMPS_AND_MORE = int('m')
        } stream_mode;
        size_t num_samps;

        bool stream_now;
        time_spec_t time_spec;

        stream_cmd_t(const stream_mode_t &stream_mode);
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_STREAM_CMD_HPP */
