//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "db_wbx_common.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/log.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;


/***********************************************************************
 * Gain-related functions
 **********************************************************************/
static int rx_pga0_gain_to_iobits(double &gain){
    //clip the input
    gain = wbx_rx_gain_ranges["PGA0"].clip(gain);

    //convert to attenuation
    double attn = wbx_rx_gain_ranges["PGA0"].stop() - gain;

    //calculate the attenuation
    int attn_code = boost::math::iround(attn*2);
    int iobits = ((~attn_code) << RX_ATTN_SHIFT) & RX_ATTN_MASK;

    UHD_LOGGER_TRACE("WBX") << boost::format(
        "WBX RX Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x"
    ) % attn % attn_code % (iobits & RX_ATTN_MASK) % RX_ATTN_MASK ;

    //the actual gain setting
    gain = wbx_rx_gain_ranges["PGA0"].stop() - double(attn_code)/2;

    return iobits;
}


/***********************************************************************
 * WBX Common Implementation
 **********************************************************************/
wbx_base::wbx_base(ctor_args_t args) : xcvr_dboard_base(args){

    //enable the clocks that we need
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_TX, true);
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);

    ////////////////////////////////////////////////////////////////////
    // Register RX and TX properties
    ////////////////////////////////////////////////////////////////////
    uint16_t rx_id = this->get_rx_id().to_uint16();

    this->get_rx_subtree()->create<device_addr_t>("tune_args").set(device_addr_t());
    this->get_rx_subtree()->create<sensor_value_t>("sensors/lo_locked")
        .set_publisher(boost::bind(&wbx_base::get_locked, this, dboard_iface::UNIT_RX));
    for(const std::string &name:  wbx_rx_gain_ranges.keys()){
        this->get_rx_subtree()->create<double>("gains/"+name+"/value")
            .set_coercer(boost::bind(&wbx_base::set_rx_gain, this, _1, name))
            .set(wbx_rx_gain_ranges[name].start());
        this->get_rx_subtree()->create<meta_range_t>("gains/"+name+"/range")
            .set(wbx_rx_gain_ranges[name]);
    }
    this->get_rx_subtree()->create<std::string>("connection").set("IQ");
    this->get_rx_subtree()->create<bool>("enabled")
        .add_coerced_subscriber(boost::bind(&wbx_base::set_rx_enabled, this, _1))
        .set(true); //start enabled
    this->get_rx_subtree()->create<bool>("use_lo_offset").set(false);

    //Value of bw low-pass dependent on board, we want complex double-sided
    double bw = (rx_id != 0x0081) ? 20.0e6 : 60.0e6;
    this->get_rx_subtree()->create<double>("bandwidth/value").set(2*bw);
    this->get_rx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(2*bw, 2*bw));
    this->get_tx_subtree()->create<double>("bandwidth/value").set(2*bw);
    this->get_tx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(2*bw, 2*bw));

    this->get_tx_subtree()->create<device_addr_t>("tune_args").set(device_addr_t());
    this->get_tx_subtree()->create<sensor_value_t>("sensors/lo_locked")
        .set_publisher(boost::bind(&wbx_base::get_locked, this, dboard_iface::UNIT_TX));
    this->get_tx_subtree()->create<std::string>("connection").set("IQ");
    this->get_tx_subtree()->create<bool>("use_lo_offset").set(false);

    // instantiate subclass foo
    switch(rx_id) {
        case 0x0053:
            db_actual = wbx_versionx_sptr(new wbx_version2(this));
            return;
        case 0x0057:
            db_actual = wbx_versionx_sptr(new wbx_version3(this));
            return;
        case 0x0063:
            db_actual = wbx_versionx_sptr(new wbx_version4(this));
            return;
        case 0x0081:
            db_actual = wbx_versionx_sptr(new wbx_version4(this));
            return;
        default:
            /* We didn't recognize the version of the board... */
            UHD_THROW_INVALID_CODE_PATH();
    }

}


wbx_base::~wbx_base(void){
    /* NOP */
}

/***********************************************************************
 * Enables
 **********************************************************************/
void wbx_base::set_rx_enabled(bool enb){
    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX,
        (enb)? RX_POWER_UP : RX_POWER_DOWN, RX_POWER_UP | RX_POWER_DOWN
    );
}

/***********************************************************************
 * Gain Handling
 **********************************************************************/
double wbx_base::set_rx_gain(double gain, const std::string &name){
    assert_has(wbx_rx_gain_ranges.keys(), name, "wbx rx gain name");
    if(name == "PGA0"){
        uint16_t io_bits = rx_pga0_gain_to_iobits(gain);
        _rx_gains[name] = gain;

        //write the new gain to rx gpio outputs
        this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, io_bits, RX_ATTN_MASK);
    }
    else UHD_THROW_INVALID_CODE_PATH();
    return _rx_gains[name]; //returned shadowed
}

/***********************************************************************
 * Tuning
 **********************************************************************/
sensor_value_t wbx_base::get_locked(dboard_iface::unit_t unit){
    const bool locked = (this->get_iface()->read_gpio(unit) & LOCKDET_MASK) != 0;
    return sensor_value_t("LO", locked, "locked", "unlocked");
}

void wbx_base::wbx_versionx::write_lo_regs(dboard_iface::unit_t unit, const std::vector<uint32_t> &regs) {
    for(uint32_t reg:  regs) {
        self_base->get_iface()->write_spi(unit, spi_config_t::EDGE_RISE, reg, 32);
    }
}
