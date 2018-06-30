//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_RFNOC_WB_IFACE_ADAPTER_HPP
#define INCLUDED_RFNOC_WB_IFACE_ADAPTER_HPP

#include <uhd/config.hpp>
#include "ctrl_iface.hpp"
#include <uhd/types/wb_iface.hpp>
#include <boost/function.hpp>

namespace uhd {
    namespace rfnoc {

/*! wb_iface control into RFNoC block registers.
 *
 * This is specifically for mimicking a wb_iface that talks to an RFNoC block.
 * It assumes an underlying ctrl_iface is talking to an RFNoC block.
 */
class UHD_API wb_iface_adapter : public uhd::timed_wb_iface
{
public:
    typedef boost::function<double()> gettickrate_type;
    typedef boost::function<uhd::time_spec_t()> gettime_type;
    typedef boost::function<void(const uhd::time_spec_t &)> settime_type;

    wb_iface_adapter(
        ctrl_iface::sptr iface,
        const gettickrate_type &,
        const settime_type &,
        const gettime_type &
    );

    virtual ~wb_iface_adapter(void) {}

    void poke32(const wb_addr_type addr, const uint32_t data);
    uint32_t peek32(const wb_addr_type addr);
    uint64_t peek64(const wb_addr_type addr);
    time_spec_t get_time() { return gettime_functor(); }
    void set_time(const uhd::time_spec_t& t) { settime_functor(t); }

private:
    ctrl_iface::sptr       _iface;
    const gettickrate_type gettickrate_functor;
    const settime_type     settime_functor;
    const gettime_type     gettime_functor;

    inline uint64_t get_timestamp() { return gettime_functor().to_ticks(gettickrate_functor()); }
};

}} // namespace uhd::rfnoc

#endif /* INCLUDED_RFNOC_WB_IFACE_ADAPTER_HPP */
