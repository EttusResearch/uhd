//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//


#include "db_sbx_common.hpp"
#include <uhd/types/tune_request.hpp>
#include <boost/algorithm/string.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * Structors
 **********************************************************************/
sbx_xcvr::sbx_version3::sbx_version3(sbx_xcvr *_self_sbx_xcvr) {
    //register the handle to our base SBX class
    self_base = _self_sbx_xcvr;
    _txlo = adf435x_iface::make_adf4350(boost::bind(&sbx_xcvr::sbx_version3::write_lo_regs, this, dboard_iface::UNIT_TX, _1));
    _rxlo = adf435x_iface::make_adf4350(boost::bind(&sbx_xcvr::sbx_version3::write_lo_regs, this, dboard_iface::UNIT_RX, _1));
}

sbx_xcvr::sbx_version3::~sbx_version3(void){
    /* NOP */
}

void sbx_xcvr::sbx_version3::write_lo_regs(dboard_iface::unit_t unit, const std::vector<uint32_t> &regs)
{
    for(uint32_t reg:  regs)
    {
        self_base->get_iface()->write_spi(unit, spi_config_t::EDGE_RISE, reg, 32);
    }
}

/***********************************************************************
 * Tuning
 **********************************************************************/
double sbx_xcvr::sbx_version3::set_lo_freq(dboard_iface::unit_t unit, double target_freq) {
    UHD_LOGGER_TRACE("SBX") << boost::format(
        "SBX tune: target frequency %f MHz"
    ) % (target_freq/1e6) ;

    /*
     * If the user sets 'mode_n=integer' in the tuning args, the user wishes to
     * tune in Integer-N mode, which can result in better spur
     * performance on some mixers. The default is fractional tuning.
     */
    property_tree::sptr subtree = (unit == dboard_iface::UNIT_RX) ? self_base->get_rx_subtree()
                                                                  : self_base->get_tx_subtree();
    device_addr_t tune_args = subtree->access<device_addr_t>("tune_args").get();
    bool is_int_n = boost::iequals(tune_args.get("mode_n",""), "integer");

    //Select the LO
    adf435x_iface::sptr& lo_iface = unit == dboard_iface::UNIT_RX ? _rxlo : _txlo;
    lo_iface->set_feedback_select(adf435x_iface::FB_SEL_DIVIDED);
    lo_iface->set_reference_freq(self_base->get_iface()->get_clock_rate(unit));

    //Use 8/9 prescaler for vco_freq > 3 GHz (pg.18 prescaler)
    lo_iface->set_prescaler(target_freq > 3e9 ? adf435x_iface::PRESCALER_8_9 : adf435x_iface::PRESCALER_4_5);

    //Configure the LO
    double actual_freq = 0.0;
    actual_freq = lo_iface->set_frequency(sbx_freq_range.clip(target_freq), is_int_n);

    if ((unit == dboard_iface::UNIT_TX) and (actual_freq == sbx_tx_lo_2dbm.clip(actual_freq))) {
        lo_iface->set_output_power(adf435x_iface::OUTPUT_POWER_2DBM);
    } else {
        lo_iface->set_output_power(adf435x_iface::OUTPUT_POWER_5DBM);
    }

    //Write to hardware
    lo_iface->commit();

    return actual_freq;
}

