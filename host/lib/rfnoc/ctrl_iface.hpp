//
// Copyright 2012-2016 Ettus Research LLC
//
// SPDX-License-Identifier: GPL-3.0
//

#ifndef INCLUDED_LIBUHD_RFNOC_CTRL_IFACE_HPP
#define INCLUDED_LIBUHD_RFNOC_CTRL_IFACE_HPP

#include <uhd/utils/msg_task.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/wb_iface.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>
#include <string>

namespace uhd { namespace rfnoc {

/*!
 * Provide access to peek, poke for the radio ctrl module
 */
class ctrl_iface : public uhd::timed_wb_iface
{
public:
    typedef boost::shared_ptr<ctrl_iface> sptr;

    virtual ~ctrl_iface(void) = 0;

    //! Make a new control object
    static sptr make(
        const bool big_endian,
        uhd::transport::zero_copy_if::sptr ctrl_xport,
        uhd::transport::zero_copy_if::sptr resp_xport,
        const uint32_t sid,
        const std::string &name = "0"
    );

    //! Set the tick rate (converting time into ticks)
    virtual void set_tick_rate(const double rate) = 0;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_CTRL_IFACE_HPP */
