//
// Copyright 2011 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "db_sbx_common.hpp"
#include "adf4350_regs.hpp"

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

    UHD_LOGV(often) << boost::format(
        "SBX TX Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x"
    ) % attn % attn_code % (iobits & RX_ATTN_MASK) % RX_ATTN_MASK << std::endl;

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

    UHD_LOGV(often) << boost::format(
        "SBX TX Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x"
    ) % attn % attn_code % (iobits & TX_ATTN_MASK) % TX_ATTN_MASK << std::endl;

    //the actual gain setting
    gain = sbx_tx_gain_ranges["PGA0"].stop() - double(attn_code)/2;

    return iobits;
}

void sbx_xcvr::set_tx_gain(double gain, const std::string &name){
    assert_has(sbx_tx_gain_ranges.keys(), name, "sbx tx gain name");
    if(name == "PGA0"){
        tx_pga0_gain_to_iobits(gain);
        _tx_gains[name] = gain;

        //write the new gain to atr regs
        update_atr();
    }
    else UHD_THROW_INVALID_CODE_PATH();
}

void sbx_xcvr::set_rx_gain(double gain, const std::string &name){
    assert_has(sbx_rx_gain_ranges.keys(), name, "sbx rx gain name");
    if(name == "PGA0"){
        rx_pga0_gain_to_iobits(gain);
        _rx_gains[name] = gain;

        //write the new gain to atr regs
        update_atr();
    }
    else UHD_THROW_INVALID_CODE_PATH();
}


/***********************************************************************
 * Structors
 **********************************************************************/
sbx_xcvr::sbx_xcvr(ctor_args_t args) : xcvr_dboard_base(args){
    switch(get_rx_id().to_uint16()) {
        case 0x054:
            db_actual = sbx_versionx_sptr(new sbx_version3(this));
            break;
        case 0x065:
            db_actual = sbx_versionx_sptr(new sbx_version4(this));
            break;
        default:
            /* We didn't recognize the version of the board... */
            UHD_THROW_INVALID_CODE_PATH();
    }

    //enable the clocks that we need
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_TX, true);
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);

    //set the gpio directions and atr controls (identically)
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, (TXIO_MASK|TX_LED_IO));
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, (RXIO_MASK|RX_LED_IO));
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, (TXIO_MASK|TX_LED_IO));
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, (RXIO_MASK|RX_LED_IO));

    //flash LEDs
    flash_leds();

    UHD_LOGV(often) << boost::format(
        "SBX GPIO Direction: RX: 0x%08x, TX: 0x%08x"
    ) % RXIO_MASK % TXIO_MASK << std::endl;

    //set some default values
    set_rx_lo_freq((sbx_freq_range.start() + sbx_freq_range.stop())/2.0);
    set_tx_lo_freq((sbx_freq_range.start() + sbx_freq_range.stop())/2.0);
    set_rx_ant("RX2");

    BOOST_FOREACH(const std::string &name, sbx_tx_gain_ranges.keys()){
        set_tx_gain(sbx_tx_gain_ranges[name].start(), name);
    }
    BOOST_FOREACH(const std::string &name, sbx_rx_gain_ranges.keys()){
        set_rx_gain(sbx_rx_gain_ranges[name].start(), name);
    }
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
    int rx_lo_lpf_en = (_rx_lo_freq == sbx_enable_rx_lo_filter.clip(_rx_lo_freq)) ? LO_LPF_EN : 0;
    int tx_lo_lpf_en = (_tx_lo_freq == sbx_enable_tx_lo_filter.clip(_tx_lo_freq)) ? LO_LPF_EN : 0;
    int rx_ld_led = get_locked(dboard_iface::UNIT_RX) ? 0 : RX_LED_LD;
    int tx_ld_led = get_locked(dboard_iface::UNIT_TX) ? 0 : TX_LED_LD;
    int rx_ant_led = _rx_ant == "TX/RX" ? RX_LED_RX1RX2 : 0;
    int tx_ant_led = _rx_ant == "TX/RX" ? 0 : TX_LED_TXRX;

    //setup the tx atr (this does not change with antenna)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_IDLE,
        tx_pga0_iobits | tx_lo_lpf_en | tx_ld_led | tx_ant_led | TX_POWER_UP | ANT_XX | TX_MIXER_DIS);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_TX_ONLY,
        tx_pga0_iobits | tx_lo_lpf_en | tx_ld_led | tx_ant_led | TX_POWER_UP | ANT_TX | TX_MIXER_ENB);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_FULL_DUPLEX,
        tx_pga0_iobits | tx_lo_lpf_en | tx_ld_led | tx_ant_led | TX_POWER_UP | ANT_TX | TX_MIXER_ENB);

    //setup the rx atr (this does not change with antenna)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_IDLE,
        rx_pga0_iobits | rx_lo_lpf_en | rx_ld_led | rx_ant_led | RX_POWER_UP | ANT_XX | RX_MIXER_DIS);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_TX_ONLY,
        rx_pga0_iobits | rx_lo_lpf_en | rx_ld_led | rx_ant_led | RX_POWER_UP | ANT_RX2 | RX_MIXER_DIS);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_FULL_DUPLEX,
        rx_pga0_iobits | rx_lo_lpf_en | rx_ld_led | rx_ant_led | RX_POWER_UP | ANT_RX2 | RX_MIXER_ENB);

    //set the atr regs that change with antenna setting
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_RX_ONLY,
        tx_pga0_iobits | tx_lo_lpf_en | tx_ld_led | tx_ant_led | TX_POWER_UP | TX_MIXER_DIS |
            ((_rx_ant == "TX/RX")? ANT_RX : ANT_TX));
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_RX_ONLY,
        rx_pga0_iobits | rx_lo_lpf_en | rx_ld_led | rx_ant_led | RX_POWER_UP | RX_MIXER_ENB | 
            ((_rx_ant == "TX/RX")? ANT_TXRX : ANT_RX2));

    UHD_LOGV(often) << boost::format(
        "SBX RXONLY ATR REG: 0x%08x"
    ) % (rx_pga0_iobits | RX_POWER_UP | RX_MIXER_ENB | ((_rx_ant == "TX/RX")? ANT_TXRX : ANT_RX2)) << std::endl;
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
    //only one antenna option, do nothing
}

/***********************************************************************
 * Tuning
 **********************************************************************/
void sbx_xcvr::set_rx_lo_freq(double freq){
    _rx_lo_freq = db_actual->set_lo_freq(dboard_iface::UNIT_RX, freq);
}

void sbx_xcvr::set_tx_lo_freq(double freq){
    _tx_lo_freq = db_actual->set_lo_freq(dboard_iface::UNIT_TX, freq);
}


double sbx_xcvr::set_lo_freq(dboard_iface::unit_t unit, double target_freq) {
    return db_actual->set_lo_freq(unit, target_freq);
}

/***********************************************************************
 * RX Get and Set
 **********************************************************************/
void sbx_xcvr::rx_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = get_rx_id().to_pp_string();
        return;

    case SUBDEV_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_GAIN:
        assert_has(_rx_gains.keys(), key.name, "sbx rx gain name");
        val = _rx_gains[key.name];
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        assert_has(sbx_rx_gain_ranges.keys(), key.name, "sbx rx gain name");
        val = sbx_rx_gain_ranges[key.name];
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(sbx_rx_gain_ranges.keys());
        return;

    case SUBDEV_PROP_FREQ:
        val = _rx_lo_freq;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = sbx_freq_range;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = _rx_ant;
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = sbx_rx_antennas;
        return;

    case SUBDEV_PROP_CONNECTION:
        val = SUBDEV_CONN_COMPLEX_IQ;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
        return;

    case SUBDEV_PROP_ENABLED:
        val = true; //always enabled
        return;

    case SUBDEV_PROP_SENSOR:
        UHD_ASSERT_THROW(key.name == "lo_locked");
        val = sensor_value_t("LO", this->get_locked(dboard_iface::UNIT_RX), "locked", "unlocked");
        return;

    case SUBDEV_PROP_SENSOR_NAMES:
        val = prop_names_t(1, "lo_locked");
        return;

    case SUBDEV_PROP_BANDWIDTH:
        val = 2*20.0e6; //20MHz low-pass, we want complex double-sided
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void sbx_xcvr::rx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_FREQ:
        this->set_rx_lo_freq(val.as<double>());
        return;

    case SUBDEV_PROP_GAIN:
        this->set_rx_gain(val.as<double>(), key.name);
        return;

    case SUBDEV_PROP_ANTENNA:
        this->set_rx_ant(val.as<std::string>());
        return;

    case SUBDEV_PROP_ENABLED:
        return; //always enabled

    case SUBDEV_PROP_BANDWIDTH:
        UHD_MSG(warning) << "SBX: No tunable bandwidth, fixed filtered to 40MHz";
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * TX Get and Set
 **********************************************************************/
void sbx_xcvr::tx_get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = get_tx_id().to_pp_string();
        return;

    case SUBDEV_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_GAIN:
        assert_has(_tx_gains.keys(), key.name, "sbx tx gain name");
        val = _tx_gains[key.name];
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        assert_has(sbx_tx_gain_ranges.keys(), key.name, "sbx tx gain name");
        val = sbx_tx_gain_ranges[key.name];
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(sbx_tx_gain_ranges.keys());
        return;

    case SUBDEV_PROP_FREQ:
        val = _tx_lo_freq;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = sbx_freq_range;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("TX/RX");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = sbx_tx_antennas;
        return;

    case SUBDEV_PROP_CONNECTION:
        val = SUBDEV_CONN_COMPLEX_QI;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
        return;

    case SUBDEV_PROP_ENABLED:
        val = true; //always enabled
        return;

    case SUBDEV_PROP_SENSOR:
        UHD_ASSERT_THROW(key.name == "lo_locked");
        val = sensor_value_t("LO", this->get_locked(dboard_iface::UNIT_TX), "locked", "unlocked");
        return;

    case SUBDEV_PROP_SENSOR_NAMES:
        val = prop_names_t(1, "lo_locked");
        return;

    case SUBDEV_PROP_BANDWIDTH:
        val = 2*20.0e6; //20MHz low-pass, we want complex double-sided
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void sbx_xcvr::tx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_FREQ:
        this->set_tx_lo_freq(val.as<double>());
        return;

    case SUBDEV_PROP_GAIN:
        this->set_tx_gain(val.as<double>(), key.name);
        return;

    case SUBDEV_PROP_ANTENNA:
        this->set_tx_ant(val.as<std::string>());
        return;

    case SUBDEV_PROP_ENABLED:
        return; //always enabled

    case SUBDEV_PROP_BANDWIDTH:
        UHD_MSG(warning) << "SBX: No tunable bandwidth, fixed filtered to 40MHz";
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}


bool sbx_xcvr::get_locked(dboard_iface::unit_t unit) {
    return (this->get_iface()->read_gpio(unit) & LOCKDET_MASK) != 0;
}


void sbx_xcvr::flash_leds(void) {
    //Remove LED gpios from ATR control temporarily and set to outputs
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, TXIO_MASK);
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, RXIO_MASK);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, (TXIO_MASK|RX_LED_IO));
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, (RXIO_MASK|RX_LED_IO));

    /*
    //flash All LEDs
    for (int i = 0; i < 3; i++) {
        this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, RX_LED_IO, RX_LED_IO);
        this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, TX_LED_IO, TX_LED_IO);

        boost::this_thread::sleep(boost::posix_time::milliseconds(100));

        this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, 0, RX_LED_IO);
        this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, 0, TX_LED_IO);

        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    }
    */

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, TX_LED_LD, TX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, TX_LED_TXRX|TX_LED_LD, TX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, RX_LED_LD, RX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, RX_LED_RX1RX2|RX_LED_LD, RX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, RX_LED_LD, RX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, 0, RX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, TX_LED_LD, TX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, 0, TX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    /*
    //flash All LEDs
    for (int i = 0; i < 3; i++) {
        this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, 0, RX_LED_IO);
        this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, 0, TX_LED_IO);

        boost::this_thread::sleep(boost::posix_time::milliseconds(100));

        this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, RX_LED_IO, RX_LED_IO);
        this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, TX_LED_IO, TX_LED_IO);

        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    }
    */
    //Put LED gpios back in ATR control and update atr
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, (TXIO_MASK|TX_LED_IO));
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, (RXIO_MASK|RX_LED_IO));
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, (TXIO_MASK|TX_LED_IO));
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, (RXIO_MASK|RX_LED_IO));
}

