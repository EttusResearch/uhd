//
// Copyright 2016 Ettus Research LLC
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

#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/rfnoc/siggen_block_ctrl.hpp>

using namespace uhd::rfnoc;

class siggen_block_ctrl_impl : public siggen_block_ctrl
{
public:
    UHD_RFNOC_BLOCK_CONSTRUCTOR(siggen_block_ctrl)
    {
        // nop
    }

    void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd, const size_t)
    {
        UHD_RFNOC_BLOCK_TRACE() << "issue_stream_cmd() " << std::endl;
        if (not stream_cmd.stream_now) {
            throw uhd::not_implemented_error("siggen_block does not support timed commands.");
        }
        switch (stream_cmd.stream_mode) {
            case uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS:
                sr_write("ENABLE", true);
                break;

            case uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS:
                sr_write("ENABLE", false);
                break;

            case uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE:
            case uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE:
                throw uhd::not_implemented_error("siggen_block does not support streaming modes other than CONTINUOUS");

            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

};

UHD_RFNOC_BLOCK_REGISTER(siggen_block_ctrl, "SigGen");
