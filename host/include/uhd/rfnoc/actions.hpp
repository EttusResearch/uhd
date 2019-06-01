//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_ACTIONS_HPP
#define INCLUDED_LIBUHD_RFNOC_ACTIONS_HPP

#include <uhd/config.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <string>
#include <vector>
#include <memory>

namespace uhd { namespace rfnoc {

/*! Container for an action
 *
 * In the RFNoC context, an action is comparable to a command. Nodes in the
 * graph can send each other actions. action_info is the payload of such an
 * action message. Nodes pass shared pointers to action_info objects between
 * each other to avoid costly copies of large action_info objects.
 */
struct UHD_API action_info
{
public:
    virtual ~action_info() {}

    using sptr = std::shared_ptr<action_info>;
    //! A unique counter for this action
    const size_t id;
    //! A string identifier for this action
    std::string key;
    //! An arbitrary payload. It is up to consumers and producers to
    // (de-)serialize it.
    std::vector<uint8_t> payload;

    //! Factory function
    static sptr make(const std::string& key="");

protected:
    action_info(const std::string& key);
};

struct UHD_API stream_cmd_action_info : public action_info
{
public:
    using sptr = std::shared_ptr<stream_cmd_action_info>;

    uhd::stream_cmd_t stream_cmd;

    //! Factory function
    static sptr make(const uhd::stream_cmd_t::stream_mode_t stream_mode);

private:
    stream_cmd_action_info(const uhd::stream_cmd_t::stream_mode_t stream_mode);
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_ACTIONS_HPP */

