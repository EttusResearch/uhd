//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/actions.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <atomic>

using namespace uhd::rfnoc;

namespace {
    // A static counter, which we use to uniquely label actions
    std::atomic<size_t> action_counter{0};
}

action_info::action_info(const std::string& key, const uhd::device_addr_t& args)
    : id(action_counter++), key(key), args(args)
{
    // nop
}

//! Factory function
action_info::sptr action_info::make(
    const std::string& key, const uhd::device_addr_t& args)
{
    struct action_info_make_shared : public action_info
    {
    };
    if (key == ACTION_KEY_STREAM_CMD) {
        return stream_cmd_action_info::make(
            uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
    }
    // return std::make_shared<action_info_make_shared>(key, args);
    return sptr(new action_info(key, args));
}

/*** Stream Command Action Info **********************************************/
stream_cmd_action_info::stream_cmd_action_info(
    const uhd::stream_cmd_t::stream_mode_t stream_mode)
    : action_info(ACTION_KEY_STREAM_CMD), stream_cmd(stream_mode)
{
    // nop
}

stream_cmd_action_info::sptr stream_cmd_action_info::make(
    const uhd::stream_cmd_t::stream_mode_t stream_mode)
{
    return sptr(new stream_cmd_action_info(stream_mode));
}

/*** RX Metadata Action Info *************************************************/
rx_event_action_info::rx_event_action_info() : action_info(ACTION_KEY_RX_EVENT)
{
    // nop
}

rx_event_action_info::sptr rx_event_action_info::make()
{
    struct rx_event_action_info_make_shared : public rx_event_action_info
    {
    };
    return std::make_shared<rx_event_action_info_make_shared>();
}
