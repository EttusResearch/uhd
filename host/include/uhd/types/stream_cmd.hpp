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

#endif /* INCLUDED_UHD_TYPES_STREAM_CMD_HPP */
