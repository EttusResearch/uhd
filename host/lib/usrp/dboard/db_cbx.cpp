//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "db_sbx_common.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/math/special_functions/round.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * Structors
 **********************************************************************/
sbx_xcvr::cbx::cbx(sbx_xcvr *_self_sbx_xcvr) {
    //register the handle to our base CBX class
    self_base = _self_sbx_xcvr;
    _txlo = max287x_iface::make<max2870>(boost::bind(&sbx_xcvr::cbx::write_lo_regs, this, dboard_iface::UNIT_TX, _1));
    _rxlo = max287x_iface::make<max2870>(boost::bind(&sbx_xcvr::cbx::write_lo_regs, this, dboard_iface::UNIT_RX, _1));
}


sbx_xcvr::cbx::~cbx(void){
    /* NOP */
}

void sbx_xcvr::cbx::write_lo_regs(dboard_iface::unit_t unit, const std::vector<uint32_t> &regs)
{
    for(uint32_t reg:  regs)
    {
        self_base->get_iface()->write_spi(unit, spi_config_t::EDGE_RISE, reg, 32);
    }
}


/***********************************************************************
 * Tuning
 **********************************************************************/
double sbx_xcvr::cbx::set_lo_freq(dboard_iface::unit_t unit, double target_freq) {
    UHD_LOGGER_TRACE("CBX") << boost::format(
        "CBX tune: target frequency %f MHz"
    ) % (target_freq/1e6) ;

    //clip the input
    target_freq = cbx_freq_range.clip(target_freq);

    double ref_freq = self_base->get_iface()->get_clock_rate(unit);
    double target_pfd_freq = 25e6;
    double actual_freq = 0.0;

    /*
     * If the user sets 'mode_n=integer' in the tuning args, the user wishes to
     * tune in Integer-N mode, which can result in better spur
     * performance on some mixers. The default is fractional tuning.
     */
    property_tree::sptr subtree = (unit == dboard_iface::UNIT_RX) ? self_base->get_rx_subtree()
                                                                  : self_base->get_tx_subtree();
    device_addr_t tune_args = subtree->access<device_addr_t>("tune_args").get();
    bool is_int_n = boost::iequals(tune_args.get("mode_n",""), "integer");

    if (unit == dboard_iface::UNIT_RX)
    {
        actual_freq = _rxlo->set_frequency(target_freq, ref_freq, target_pfd_freq, is_int_n);
        _rxlo->commit();
    } else {
        actual_freq = _txlo->set_frequency(target_freq, ref_freq, target_pfd_freq, is_int_n);
        _txlo->set_output_power((actual_freq == sbx_tx_lo_2dbm.clip(actual_freq))
                                ? max287x_iface::OUTPUT_POWER_2DBM
                                : max287x_iface::OUTPUT_POWER_5DBM);
        _txlo->commit();
    }
    return actual_freq;
}

