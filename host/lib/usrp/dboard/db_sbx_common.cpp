//
// Copyright 2011-2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "db_sbx_common.hpp"

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;


/***********************************************************************
 * Register the SBX dboard (min freq, max freq, rx div2, tx div2)
 **********************************************************************/
static dboard_base::sptr make_sbx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new sbx_xcvr(args));
}

UHD_STATIC_BLOCK(reg_sbx_dboards){
    dboard_manager::register_dboard(0x0054, 0x0055, &make_sbx, "SBX");
    dboard_manager::register_dboard(0x0065, 0x0064, &make_sbx, "SBX v4");
    dboard_manager::register_dboard(0x0067, 0x0066, &make_sbx, "CBX");
    dboard_manager::register_dboard(0x0069, 0x0068, &make_sbx, "SBX v5");
    dboard_manager::register_dboard(0x0083, 0x0082, &make_sbx, "SBX-120");
    dboard_manager::register_dboard(0x0085, 0x0084, &make_sbx, "CBX-120");
}


/***********************************************************************
 * Gain Handling
 **********************************************************************/
static int rx_pga0_gain_to_iobits(double &gain){
    //clip the input
    gain = sbx_rx_gain_ranges["PGA0"].clip(gain);

    //convert to attenuation and update iobits for atr
    double attn = sbx_rx_gain_ranges["PGA0"].stop() - gain;

    //calculate the RX attenuation
    int attn_code = int(floor(attn*2));
    int iobits = ((~attn_code) << RX_ATTN_SHIFT) & RX_ATTN_MASK;

    UHD_LOGGER_TRACE("SBX") << boost::format(
        "SBX RX Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x"
    ) % attn % attn_code % (iobits & RX_ATTN_MASK) % RX_ATTN_MASK ;

    //the actual gain setting
    gain = sbx_rx_gain_ranges["PGA0"].stop() - double(attn_code)/2;

    return iobits;
}

static int tx_pga0_gain_to_iobits(double &gain){
    //clip the input
    gain = sbx_tx_gain_ranges["PGA0"].clip(gain);

    //convert to attenuation and update iobits for atr
    double attn = sbx_tx_gain_ranges["PGA0"].stop() - gain;

    //calculate the TX attenuation
    int attn_code = int(floor(attn*2));
    int iobits = ((~attn_code) << TX_ATTN_SHIFT) & TX_ATTN_MASK;

    UHD_LOGGER_TRACE("SBX") << boost::format(
        "SBX TX Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x"
    ) % attn % attn_code % (iobits & TX_ATTN_MASK) % TX_ATTN_MASK ;

    //the actual gain setting
    gain = sbx_tx_gain_ranges["PGA0"].stop() - double(attn_code)/2;

    return iobits;
}

double sbx_xcvr::set_tx_gain(double gain, const std::string &name){
    assert_has(sbx_tx_gain_ranges.keys(), name, "sbx tx gain name");
    if(name == "PGA0"){
        tx_pga0_gain_to_iobits(gain);
        _tx_gains[name] = gain;

        //write the new gain to atr regs
        update_atr();
    }
    else UHD_THROW_INVALID_CODE_PATH();
    return _tx_gains[name];
}

double sbx_xcvr::set_rx_gain(double gain, const std::string &name){
    assert_has(sbx_rx_gain_ranges.keys(), name, "sbx rx gain name");
    if(name == "PGA0"){
        rx_pga0_gain_to_iobits(gain);
        _rx_gains[name] = gain;

        //write the new gain to atr regs
        update_atr();
    }
    else UHD_THROW_INVALID_CODE_PATH();
    return _rx_gains[name];
}


/***********************************************************************
 * Structors
 **********************************************************************/
sbx_xcvr::sbx_xcvr(ctor_args_t args) : xcvr_dboard_base(args){
    switch(get_rx_id().to_uint16()) {
        case 0x0054:
            db_actual = sbx_versionx_sptr(new sbx_version3(this));
            freq_range =          sbx_freq_range;
            enable_rx_lo_filter = sbx_enable_rx_lo_filter;
            enable_tx_lo_filter = sbx_enable_tx_lo_filter;
            break;
        case 0x0065:
        case 0x0069:
        case 0x0083:
            db_actual = sbx_versionx_sptr(new sbx_version4(this));
            freq_range =          sbx_freq_range;
            enable_rx_lo_filter = sbx_enable_rx_lo_filter;
            enable_tx_lo_filter = sbx_enable_tx_lo_filter;
            break;
        case 0x0067:
        case 0x0085:
            db_actual = sbx_versionx_sptr(new cbx(this));
            freq_range =          cbx_freq_range;
            enable_rx_lo_filter = cbx_enable_rx_lo_filter;
            enable_tx_lo_filter = cbx_enable_tx_lo_filter;
            break;
        default:
            /* We didn't recognize the version of the board... */
            UHD_THROW_INVALID_CODE_PATH();
    }

    ////////////////////////////////////////////////////////////////////
    // Register RX properties
    ////////////////////////////////////////////////////////////////////
    this->get_rx_subtree()->create<device_addr_t>("tune_args").set(device_addr_t());

    uint16_t rx_id = get_rx_id().to_uint16();
    if(rx_id == 0x0054) this->get_rx_subtree()->create<std::string>("name").set("SBXv3 RX");
    else if(rx_id == 0x0065) this->get_rx_subtree()->create<std::string>("name").set("SBXv4 RX");
    else if(rx_id == 0x0067) this->get_rx_subtree()->create<std::string>("name").set("CBX RX");
    else if(rx_id == 0x0083) this->get_rx_subtree()->create<std::string>("name").set("SBX-120 RX");
    else if(rx_id == 0x0085) this->get_rx_subtree()->create<std::string>("name").set("CBX-120 RX");
    else this->get_rx_subtree()->create<std::string>("name").set("SBX/CBX RX");

    this->get_rx_subtree()->create<sensor_value_t>("sensors/lo_locked")
        .set_publisher(boost::bind(&sbx_xcvr::get_locked, this, dboard_iface::UNIT_RX));
    for(const std::string &name:  sbx_rx_gain_ranges.keys()){
        this->get_rx_subtree()->create<double>("gains/"+name+"/value")
            .set_coercer(boost::bind(&sbx_xcvr::set_rx_gain, this, _1, name))
            .set(sbx_rx_gain_ranges[name].start());
        this->get_rx_subtree()->create<meta_range_t>("gains/"+name+"/range")
            .set(sbx_rx_gain_ranges[name]);
    }
    this->get_rx_subtree()->create<double>("freq/value")
        .set_coercer(boost::bind(&sbx_xcvr::set_lo_freq, this, dboard_iface::UNIT_RX, _1))
        .set((freq_range.start() + freq_range.stop())/2.0);
    this->get_rx_subtree()->create<meta_range_t>("freq/range").set(freq_range);
    this->get_rx_subtree()->create<std::string>("antenna/value")
        .add_coerced_subscriber(boost::bind(&sbx_xcvr::set_rx_ant, this, _1))
        .set("RX2");
    this->get_rx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(sbx_rx_antennas);
    this->get_rx_subtree()->create<std::string>("connection").set("IQ");
    this->get_rx_subtree()->create<bool>("enabled").set(true); //always enabled
    this->get_rx_subtree()->create<bool>("use_lo_offset").set(false);

    //Value of bw low-pass dependent on board, we want complex double-sided
    double rx_bw = ((rx_id != 0x0083) && (rx_id != 0x0085)) ? 20.0e6 : 60.0e6;
    this->get_rx_subtree()->create<double>("bandwidth/value").set(2*rx_bw);
    this->get_rx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(2*rx_bw, 2*rx_bw));

    ////////////////////////////////////////////////////////////////////
    // Register TX properties
    ////////////////////////////////////////////////////////////////////
    this->get_tx_subtree()->create<device_addr_t>("tune_args").set(device_addr_t());

    uint16_t tx_id = get_tx_id().to_uint16();
    if(tx_id == 0x0055) this->get_tx_subtree()->create<std::string>("name").set("SBXv3 TX");
    else if(tx_id == 0x0064) this->get_tx_subtree()->create<std::string>("name").set("SBXv4 TX");
    else if(tx_id == 0x0066) this->get_tx_subtree()->create<std::string>("name").set("CBX TX");
    else if(tx_id == 0x0082) this->get_tx_subtree()->create<std::string>("name").set("SBX-120 TX");
    else if(tx_id == 0x0084) this->get_tx_subtree()->create<std::string>("name").set("CBX-120 TX");
    else this->get_tx_subtree()->create<std::string>("name").set("SBX/CBX TX");

    this->get_tx_subtree()->create<sensor_value_t>("sensors/lo_locked")
        .set_publisher(boost::bind(&sbx_xcvr::get_locked, this, dboard_iface::UNIT_TX));
    for(const std::string &name:  sbx_tx_gain_ranges.keys()){
        this->get_tx_subtree()->create<double>("gains/"+name+"/value")
            .set_coercer(boost::bind(&sbx_xcvr::set_tx_gain, this, _1, name))
            .set(sbx_tx_gain_ranges[name].start());
        this->get_tx_subtree()->create<meta_range_t>("gains/"+name+"/range")
            .set(sbx_tx_gain_ranges[name]);
    }
    this->get_tx_subtree()->create<double>("freq/value")
        .set_coercer(boost::bind(&sbx_xcvr::set_lo_freq, this, dboard_iface::UNIT_TX, _1))
        .set((freq_range.start() + freq_range.stop())/2.0);
    this->get_tx_subtree()->create<meta_range_t>("freq/range").set(freq_range);
    this->get_tx_subtree()->create<std::string>("antenna/value")
        .add_coerced_subscriber(boost::bind(&sbx_xcvr::set_tx_ant, this, _1))
        .set(sbx_tx_antennas.at(0));
    this->get_tx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(sbx_tx_antennas);
    this->get_tx_subtree()->create<std::string>("connection").set("QI");
    this->get_tx_subtree()->create<bool>("enabled").set(true); //always enabled
    this->get_tx_subtree()->create<bool>("use_lo_offset").set(false);

    //Value of bw low-pass dependent on board, we want complex double-sided
    double tx_bw = ((tx_id != 0x0082) && (tx_id != 0x0084)) ? 20.0e6 : 60.0e6;
    this->get_tx_subtree()->create<double>("bandwidth/value").set(2*tx_bw);
    this->get_tx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(2*tx_bw, 2*tx_bw));

    //enable the clocks that we need
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_TX, true);
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);

    //set the gpio directions and atr controls (identically)
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, (TXIO_MASK|TX_LED_IO));
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, (RXIO_MASK|RX_LED_IO));
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, (TXIO_MASK|TX_LED_IO));
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, (RXIO_MASK|RX_LED_IO));

    //Initialize ATR registers after direction and pin ctrl configuration
    update_atr();

    UHD_LOGGER_TRACE("SBX") << boost::format(
        "SBX GPIO Direction: RX: 0x%08x, TX: 0x%08x"
    ) % RXIO_MASK % TXIO_MASK ;
}

sbx_xcvr::~sbx_xcvr(void){
    /* NOP */
}

/***********************************************************************
 * Antenna Handling
 **********************************************************************/
void sbx_xcvr::update_atr(void){
    //calculate atr pins
    int rx_pga0_iobits = rx_pga0_gain_to_iobits(_rx_gains["PGA0"]);
    int tx_pga0_iobits = tx_pga0_gain_to_iobits(_tx_gains["PGA0"]);
    int rx_lo_lpf_en = (_rx_lo_freq == enable_rx_lo_filter.clip(_rx_lo_freq)) ? LO_LPF_EN : 0;
    int tx_lo_lpf_en = (_tx_lo_freq == enable_tx_lo_filter.clip(_tx_lo_freq)) ? LO_LPF_EN : 0;
    int rx_ld_led = _rx_lo_lock_cache ? 0 : RX_LED_LD;
    int tx_ld_led = _tx_lo_lock_cache ? 0 : TX_LED_LD;
    int rx_ant_led = _rx_ant == "TX/RX" ? RX_LED_RX1RX2 : 0;
    int tx_ant_led = _tx_ant == "TX/RX" ? 0 : TX_LED_TXRX;

    //setup the tx atr (this does not change with antenna)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            gpio_atr::ATR_REG_IDLE, 0 | tx_lo_lpf_en \
            | tx_ld_led | tx_ant_led | TX_POWER_UP | ANT_XX | TX_MIXER_DIS);

    //setup the rx atr (this does not change with antenna)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            gpio_atr::ATR_REG_IDLE, rx_pga0_iobits | rx_lo_lpf_en \
            | rx_ld_led | rx_ant_led | RX_POWER_UP | ANT_XX | RX_MIXER_DIS);

    //set the RX atr regs that change with antenna setting
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            gpio_atr::ATR_REG_RX_ONLY, rx_pga0_iobits | rx_lo_lpf_en \
            | rx_ld_led | rx_ant_led | RX_POWER_UP | RX_MIXER_ENB \
            | ((_rx_ant != "RX2")? ANT_TXRX : ANT_RX2));
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            gpio_atr::ATR_REG_TX_ONLY, rx_pga0_iobits | rx_lo_lpf_en \
            | rx_ld_led | rx_ant_led | RX_POWER_UP | RX_MIXER_DIS \
            | ((_rx_ant == "CAL")? ANT_TXRX : ANT_RX2));
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            gpio_atr::ATR_REG_FULL_DUPLEX, rx_pga0_iobits | rx_lo_lpf_en \
            | rx_ld_led | rx_ant_led | RX_POWER_UP | RX_MIXER_ENB \
            | ((_rx_ant == "CAL")? ANT_TXRX : ANT_RX2));

    //set the TX atr regs that change with antenna setting
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            gpio_atr::ATR_REG_RX_ONLY, 0 | tx_lo_lpf_en \
            | tx_ld_led | tx_ant_led | TX_POWER_UP | TX_MIXER_DIS \
            | ((_rx_ant != "RX2")? ANT_RX : ANT_TX));
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            gpio_atr::ATR_REG_TX_ONLY, tx_pga0_iobits | tx_lo_lpf_en \
            | tx_ld_led | tx_ant_led | TX_POWER_UP | TX_MIXER_ENB \
            | ((_tx_ant == "CAL")? ANT_RX : ANT_TX));
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            gpio_atr::ATR_REG_FULL_DUPLEX, tx_pga0_iobits | tx_lo_lpf_en \
            | tx_ld_led | tx_ant_led | TX_POWER_UP | TX_MIXER_ENB \
            | ((_tx_ant == "CAL")? ANT_RX : ANT_TX));
}

void sbx_xcvr::set_rx_ant(const std::string &ant){
    //validate input
    assert_has(sbx_rx_antennas, ant, "sbx rx antenna name");

    //shadow the setting
    _rx_ant = ant;

    //write the new antenna setting to atr regs
    update_atr();
}

void sbx_xcvr::set_tx_ant(const std::string &ant){
    assert_has(sbx_tx_antennas, ant, "sbx tx antenna name");

    //shadow the setting
    _tx_ant = ant;

    //write the new antenna setting to atr regs
    update_atr();
}

/***********************************************************************
 * Tuning
 **********************************************************************/
double sbx_xcvr::set_lo_freq(dboard_iface::unit_t unit, double target_freq) {
    const double actual = db_actual->set_lo_freq(unit, target_freq);
    if (unit == dboard_iface::UNIT_RX){
        _rx_lo_lock_cache = false;
        _rx_lo_freq = actual;
    }
    if (unit == dboard_iface::UNIT_TX){
        _tx_lo_lock_cache = false;
        _tx_lo_freq = actual;
    }
    update_atr();
    return actual;
}


sensor_value_t sbx_xcvr::get_locked(dboard_iface::unit_t unit) {
    const bool locked = (this->get_iface()->read_gpio(unit) & LOCKDET_MASK) != 0;

    if (unit == dboard_iface::UNIT_RX) _rx_lo_lock_cache = locked;
    if (unit == dboard_iface::UNIT_TX) _tx_lo_lock_cache = locked;

    //write the new lock cache setting to atr regs
    update_atr();

    return sensor_value_t("LO", locked, "locked", "unlocked");
}
