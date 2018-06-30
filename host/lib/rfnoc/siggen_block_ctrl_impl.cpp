//
// Copyright 2016-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/format.hpp>
#include <uhd/utils/log.hpp>
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
        UHD_LOGGER_TRACE(unique_id()) << "issue_stream_cmd()" << std::endl;
        if (not stream_cmd.stream_now) {
            throw uhd::not_implemented_error(
                "siggen_block does not support timed commands.");
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
                throw uhd::not_implemented_error(
                    "siggen_block does not support streaming modes other than CONTINUOUS");

            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }

};

UHD_RFNOC_BLOCK_REGISTER(siggen_block_ctrl, "SigGen");
