//
// Copyright 2013,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/safe_call.hpp>
#include <uhdlib/usrp/cores/tx_vita_core_3000.hpp>

#define REG_CTRL_ERROR_POLICY       (_base + 0)
#define REG_FC_PRE_RADIO_RESP_BASE  (_base + 2*4)
#define REG_FC_PRE_FIFO_RESP_BASE   (_base + 4*4)
#define REG_CTRL_FC_CYCLE_OFFSET    (0*4)
#define REG_CTRL_FC_PACKET_OFFSET   (1*4)

using namespace uhd;

tx_vita_core_3000::~tx_vita_core_3000(void){
    /* NOP */
}

struct tx_vita_core_3000_impl : tx_vita_core_3000
{
    tx_vita_core_3000_impl(
        wb_iface::sptr iface,
        const size_t base,
        fc_monitor_loc fc_location
    ):
        _iface(iface),
        _base(base),
        _fc_base((fc_location==FC_PRE_RADIO or fc_location==FC_DEFAULT) ?
                    REG_FC_PRE_RADIO_RESP_BASE : REG_FC_PRE_FIFO_RESP_BASE),
        _fc_location(fc_location)
    {
        if (fc_location != FC_DEFAULT) {
            //Turn off the other FC monitoring module
            const size_t other_fc_base = (fc_location==FC_PRE_RADIO) ?
                    REG_FC_PRE_FIFO_RESP_BASE : REG_FC_PRE_RADIO_RESP_BASE;
            _iface->poke32(other_fc_base + REG_CTRL_FC_CYCLE_OFFSET, 0);
            _iface->poke32(other_fc_base + REG_CTRL_FC_PACKET_OFFSET, 0);
        }
        this->set_underflow_policy("next_packet");
        this->clear();
    }

    ~tx_vita_core_3000_impl(void)
    {
        UHD_SAFE_CALL
        (
            this->clear();
        )
    }

    void clear(void)
    {
        this->configure_flow_control(0, 0);
        this->set_underflow_policy(_policy); //clears the seq
    }

    void set_underflow_policy(const std::string &policy)
    {
        if (policy == "next_packet")
        {
            _iface->poke32(REG_CTRL_ERROR_POLICY, (1 << 1));
        }
        else if (policy == "next_burst")
        {
            _iface->poke32(REG_CTRL_ERROR_POLICY, (1 << 2));
        }
        else if (policy == "wait")
        {
            _iface->poke32(REG_CTRL_ERROR_POLICY, (1 << 0));
        }
        else throw uhd::value_error("USRP TX cannot handle requested underflow policy: " + policy);
        _policy = policy;
    }

    void setup(const uhd::stream_args_t &stream_args)
    {
        if (stream_args.args.has_key("underflow_policy"))
        {
            this->set_underflow_policy(stream_args.args["underflow_policy"]);
        }
    }

    void configure_flow_control(const size_t cycs_per_up, const size_t pkts_per_up)
    {
        if (cycs_per_up == 0) _iface->poke32(_fc_base + REG_CTRL_FC_CYCLE_OFFSET, 0);
        else _iface->poke32(_fc_base + REG_CTRL_FC_CYCLE_OFFSET, (1 << 31) | ((cycs_per_up) & 0xffffff));

        if (pkts_per_up == 0) _iface->poke32(_fc_base + REG_CTRL_FC_PACKET_OFFSET, 0);
        else _iface->poke32(_fc_base + REG_CTRL_FC_PACKET_OFFSET, (1 << 31) | ((pkts_per_up) & 0xffff));
    }

    wb_iface::sptr  _iface;
    const size_t    _base;
    const size_t    _fc_base;
    std::string     _policy;
    fc_monitor_loc  _fc_location;

};

tx_vita_core_3000::sptr tx_vita_core_3000::make(
    wb_iface::sptr iface,
    const size_t base,
    fc_monitor_loc fc_location
)
{
    return tx_vita_core_3000::sptr(new tx_vita_core_3000_impl(iface, base, fc_location));
}

tx_vita_core_3000::sptr tx_vita_core_3000::make_no_radio_buff(
    wb_iface::sptr iface,
    const size_t base
)
{
    //No internal radio buffer so only pre-radio monitoring is supported.
    return tx_vita_core_3000::sptr(new tx_vita_core_3000_impl(iface, base, FC_DEFAULT));
}
