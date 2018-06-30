//
// Copyright 2013-2014,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "n230_frontend_ctrl.hpp"

#include <uhd/utils/log.hpp>
#include <uhd/exception.hpp>
#include <uhd/types/dict.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include "n230_fpga_defs.h"

namespace uhd { namespace usrp { namespace n230 {

/* ATR Control Bits */
static const uint32_t TX_ENABLE      = (1 << 7);
static const uint32_t SFDX_RX        = (1 << 6);
static const uint32_t SFDX_TX        = (1 << 5);
static const uint32_t SRX_RX         = (1 << 4);
static const uint32_t SRX_TX         = (1 << 3);
static const uint32_t LED_RX         = (1 << 2);
static const uint32_t LED_TXRX_RX    = (1 << 1);
static const uint32_t LED_TXRX_TX    = (1 << 0);

/* ATR State Definitions. */
static const uint32_t STATE_OFF      = 0x00;
static const uint32_t STATE_RX_RX2   = (SFDX_RX
                                                | SFDX_TX
                                                | LED_RX);
static const uint32_t STATE_RX_TXRX  = (SRX_RX
                                                | SRX_TX
                                                | LED_TXRX_RX);
static const uint32_t STATE_FDX_TXRX = (TX_ENABLE
                                                | SFDX_RX
                                                | SFDX_TX
                                                | LED_TXRX_TX
                                                | LED_RX);
static const uint32_t STATE_TX_TXRX  = (TX_ENABLE
                                                | SFDX_RX
                                                | SFDX_TX
                                                | LED_TXRX_TX);

using namespace uhd::usrp;

class n230_frontend_ctrl_impl : public n230_frontend_ctrl
{
public:
    n230_frontend_ctrl_impl(
        radio_ctrl_core_3000::sptr core_ctrl,
        fpga::core_misc_reg_t& core_misc_reg,
        ad9361_ctrl::sptr codec_ctrl,
        const std::vector<gpio_atr::gpio_atr_3000::sptr>& gpio_cores
    ): _core_ctrl(core_ctrl),
       _codec_ctrl(codec_ctrl),
       _gpio_cores(gpio_cores),
       _core_misc_reg(core_misc_reg)
    {
    }

    virtual ~n230_frontend_ctrl_impl()
    {
    }

    void set_antenna_sel(const size_t which, const std::string &ant)
    {
        if (ant != "TX/RX" and ant != "RX2")
            throw uhd::value_error("n230: unknown RX antenna option: " + ant);

        _fe_states[which].rx_ant = ant;
        _flush_atr_state();
    }

    void set_stream_state(const fe_state_t fe0_state_, const fe_state_t fe1_state_)
    {
        //Update soft-state
        _fe_states[0].state = fe0_state_;
        _fe_states[1].state = fe1_state_;

        const fe_state_t fe0_state = _fe_states[0].state;
        const fe_state_t fe1_state = (_gpio_cores.size() > 1) ? _fe_states[1].state : NONE_STREAMING;

        const size_t num_tx = (_is_tx(fe0_state) ? 1 : 0) + (_is_tx(fe1_state) ? 1 : 0);
        const size_t num_rx = (_is_rx(fe0_state) ? 1 : 0) + (_is_rx(fe1_state) ? 1 : 0);

        //setup the active chains in the codec
        if ((num_rx + num_tx) == 0) {
            _codec_ctrl->set_active_chains(
                true, false,
                true, false); //enable something
        } else {
            _codec_ctrl->set_active_chains(
                _is_tx(fe0_state), _is_tx(fe1_state),
                _is_rx(fe0_state), _is_rx(fe1_state));
        }

        _core_misc_reg.flush();
        //atrs change based on enables
        _flush_atr_state();
    }


    void set_stream_state(const size_t which, const fe_state_t state)
    {
        if (which == 0) {
            set_stream_state(state, _fe_states[1].state);
        } else if (which == 1) {
            set_stream_state(_fe_states[0].state, state);
        } else {
            throw uhd::value_error(
                str(boost::format("n230: unknown stream index option: %d") % which)
            );
        }
    }

    void set_bandsel(const std::string& which, double freq)
    {
        using namespace n230::fpga;

        if(which[0] == 'R') {
            if(freq < 2.2e9) {
                _core_misc_reg.set(core_misc_reg_t::RX_BANDSEL_A, 0);
                _core_misc_reg.set(core_misc_reg_t::RX_BANDSEL_B, 0);
                _core_misc_reg.set(core_misc_reg_t::RX_BANDSEL_C, 1);
            } else if((freq >= 2.2e9) && (freq < 4e9)) {
                _core_misc_reg.set(core_misc_reg_t::RX_BANDSEL_A, 0);
                _core_misc_reg.set(core_misc_reg_t::RX_BANDSEL_B, 1);
                _core_misc_reg.set(core_misc_reg_t::RX_BANDSEL_C, 0);
            } else if((freq >= 4e9) && (freq <= 6e9)) {
                _core_misc_reg.set(core_misc_reg_t::RX_BANDSEL_A, 1);
                _core_misc_reg.set(core_misc_reg_t::RX_BANDSEL_B, 0);
                _core_misc_reg.set(core_misc_reg_t::RX_BANDSEL_C, 0);
            } else {
                UHD_THROW_INVALID_CODE_PATH();
            }
        } else if(which[0] == 'T') {
            if(freq < 2.5e9) {
                _core_misc_reg.set(core_misc_reg_t::TX_BANDSEL_A, 0);
                _core_misc_reg.set(core_misc_reg_t::TX_BANDSEL_B, 1);
            } else if((freq >= 2.5e9) && (freq <= 6e9)) {
                _core_misc_reg.set(core_misc_reg_t::TX_BANDSEL_A, 1);
                _core_misc_reg.set(core_misc_reg_t::TX_BANDSEL_B, 0);
            } else {
                UHD_THROW_INVALID_CODE_PATH();
            }
        } else {
            UHD_THROW_INVALID_CODE_PATH();
        }

        _core_misc_reg.flush();
    }

    void set_self_test_mode(self_test_mode_t mode)
    {
        switch (mode) {
            case LOOPBACK_RADIO: {
                _core_ctrl->poke32(fpga::sr_addr(fpga::SR_CORE_LOOPBACK), 0x1);
            } break;
            case LOOPBACK_CODEC: {
                _core_ctrl->poke32(fpga::sr_addr(fpga::SR_CORE_LOOPBACK), 0x0);
                _codec_ctrl->data_port_loopback(true);
            } break;
            //Default = disable
            default:
            case LOOPBACK_DISABLED: {
                _core_ctrl->poke32(fpga::sr_addr(fpga::SR_CORE_LOOPBACK), 0x0);
                _codec_ctrl->data_port_loopback(false);
            } break;
        }
    }

private:
    void _flush_atr_state()
    {
        for (size_t i = 0; i < _gpio_cores.size(); i++) {
            const fe_state_cache_t& fe_state_cache = _fe_states[i];
            const bool enb_rx = _is_rx(fe_state_cache.state);
            const bool enb_tx = _is_tx(fe_state_cache.state);
            const bool is_rx2 = (fe_state_cache.rx_ant == "RX2");
            const size_t rxonly = (enb_rx)? ((is_rx2)? STATE_RX_RX2 : STATE_RX_TXRX) : STATE_OFF;
            const size_t txonly = (enb_tx)? (STATE_TX_TXRX) : STATE_OFF;
            size_t fd = STATE_OFF;
            if (enb_rx and enb_tx) fd = STATE_FDX_TXRX;
            if (enb_rx and not enb_tx) fd = rxonly;
            if (not enb_rx and enb_tx) fd = txonly;
            _gpio_cores[i]->set_atr_reg(gpio_atr::ATR_REG_IDLE, STATE_OFF);
            _gpio_cores[i]->set_atr_reg(gpio_atr::ATR_REG_RX_ONLY, rxonly);
            _gpio_cores[i]->set_atr_reg(gpio_atr::ATR_REG_TX_ONLY, txonly);
            _gpio_cores[i]->set_atr_reg(gpio_atr::ATR_REG_FULL_DUPLEX, fd);
        }
    }

    inline static bool _is_tx(const fe_state_t state)
    {
        return state == TX_STREAMING || state == TXRX_STREAMING;
    }

    inline static bool _is_rx(const fe_state_t state)
    {
        return state == RX_STREAMING || state == TXRX_STREAMING;
    }

private:
    struct fe_state_cache_t {
        fe_state_cache_t() : state(NONE_STREAMING), rx_ant("RX2")
        {}
        fe_state_t state;
        std::string rx_ant;
    };

    radio_ctrl_core_3000::sptr              _core_ctrl;
    ad9361_ctrl::sptr                       _codec_ctrl;
    std::vector<gpio_atr::gpio_atr_3000::sptr>   _gpio_cores;
    fpga::core_misc_reg_t&                  _core_misc_reg;
    uhd::dict<size_t, fe_state_cache_t>     _fe_states;
};

}}} //namespace

using namespace uhd::usrp::n230;

n230_frontend_ctrl::sptr n230_frontend_ctrl::make(
        radio_ctrl_core_3000::sptr core_ctrl,
        fpga::core_misc_reg_t& core_misc_reg,
        ad9361_ctrl::sptr codec_ctrl,
        const std::vector<gpio_atr::gpio_atr_3000::sptr>& gpio_cores)
{
    return sptr(new n230_frontend_ctrl_impl(core_ctrl, core_misc_reg, codec_ctrl, gpio_cores));
}

