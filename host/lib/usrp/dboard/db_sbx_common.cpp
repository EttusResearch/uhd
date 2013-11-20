//
// Copyright 2011-2012 Ettus Research LLC
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

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * ADF 4350/4351 Tuning Utility
 **********************************************************************/
sbx_xcvr::sbx_versionx::adf435x_tuning_settings sbx_xcvr::sbx_versionx::_tune_adf435x_synth(
    double target_freq,
    double ref_freq,
    const adf435x_tuning_constraints& constraints,
    double& actual_freq)
{
    //Default invalid value for actual_freq
    actual_freq = 0;

    double pfd_freq = 0;
    boost::uint16_t R = 0, BS = 0, N = 0, FRAC = 0, MOD = 0;
    boost::uint16_t RFdiv = static_cast<boost::uint16_t>(constraints.rf_divider_range.start());
    bool D = false, T = false;

    //Reference doubler for 50% duty cycle
    //If ref_freq < 12.5MHz enable the reference doubler
    D = (ref_freq <= constraints.ref_doubler_threshold);

    static const double MIN_VCO_FREQ = 2.2e9;
    static const double MAX_VCO_FREQ = 4.4e9;

    //increase RF divider until acceptable VCO frequency
    double vco_freq = target_freq;
    while (vco_freq < MIN_VCO_FREQ && RFdiv < static_cast<boost::uint16_t>(constraints.rf_divider_range.stop())) {
        vco_freq *= 2;
        RFdiv *= 2;
    }

    /*
     * The goal here is to loop though possible R dividers,
     * band select clock dividers, N (int) dividers, and FRAC
     * (frac) dividers.
     *
     * Calculate the N and F dividers for each set of values.
     * The loop exits when it meets all of the constraints.
     * The resulting loop values are loaded into the registers.
     *
     * from pg.21
     *
     * f_pfd = f_ref*(1+D)/(R*(1+T))
     * f_vco = (N + (FRAC/MOD))*f_pfd
     *    N = f_vco/f_pfd - FRAC/MOD = f_vco*((R*(T+1))/(f_ref*(1+D))) - FRAC/MOD
     * f_rf = f_vco/RFdiv)
     * f_actual = f_rf/2
     */
    for(R = 1; R <= 1023; R+=1){
        //PFD input frequency = f_ref/R ... ignoring Reference doubler/divide-by-2 (D & T)
        pfd_freq = ref_freq*(D?2:1)/(R*(T?2:1));

        //keep the PFD frequency at or below 25MHz (Loop Filter Bandwidth)
        if (pfd_freq > constraints.pfd_freq_max) continue;

        //ignore fractional part of tuning
        //N is computed from target_freq and not vco_freq because the feedback
        //mode is set to FEEDBACK_SELECT_DIVIDED
        N = boost::uint16_t(std::floor(target_freq/pfd_freq));

        //keep N > minimum int divider requirement
        if (N < static_cast<boost::uint16_t>(constraints.int_range.start())) continue;

        for(BS=1; BS <= 255; BS+=1){
            //keep the band select frequency at or below band_sel_freq_max
            //constraint on band select clock
            if (pfd_freq/BS > constraints.band_sel_freq_max) continue;
            goto done_loop;
        }
    } done_loop:

    //Fractional-N calculation
    MOD = 4095; //max fractional accuracy
    //N is computed from target_freq and not vco_freq because the feedback
    //mode is set to FEEDBACK_SELECT_DIVIDED
    FRAC = static_cast<boost::uint16_t>((target_freq/pfd_freq - N)*MOD);
    if (constraints.force_frac0) {
        if (FRAC > (MOD / 2)) { //Round integer such that actual freq is closest to target
            N++;
        }
        FRAC = 0;
    }

    //Reference divide-by-2 for 50% duty cycle
    // if R even, move one divide by 2 to to regs.reference_divide_by_2
    if(R % 2 == 0) {
        T = true;
        R /= 2;
    }

    //Typical phase resync time documented in data sheet pg.24
    static const double PHASE_RESYNC_TIME = 400e-6;

    //actual frequency calculation
    actual_freq = double((N + (double(FRAC)/double(MOD)))*ref_freq*(D?2:1)/(R*(T?2:1)));

    //load the settings
    adf435x_tuning_settings settings;
    settings.frac_12_bit = FRAC;
    settings.int_16_bit = N;
    settings.mod_12_bit = MOD;
    settings.clock_divider_12_bit = std::max<boost::uint16_t>(1, std::ceil(PHASE_RESYNC_TIME*pfd_freq/MOD));
    settings.r_counter_10_bit = R;
    settings.r_divide_by_2_en = T;
    settings.r_doubler_en = D;
    settings.band_select_clock_div = BS;
    settings.rf_divider = RFdiv;
    settings.feedback_after_divider = true;

    UHD_LOGV(often)
        << boost::format("ADF 435X Frequencies (MHz): REQUESTED=%0.9f, ACTUAL=%0.9f"
        ) % (target_freq/1e6) % (actual_freq/1e6) << std::endl
        << boost::format("ADF 435X Intermediates (MHz): VCO=%0.2f, PFD=%0.2f, BAND=%0.2f, REF=%0.2f"
        ) % (vco_freq/1e6) % (pfd_freq/1e6) % (pfd_freq/BS/1e6) % (ref_freq/1e6) << std::endl
        << boost::format("ADF 435X Settings: R=%d, BS=%d, N=%d, FRAC=%d, MOD=%d, T=%d, D=%d, RFdiv=%d"
        ) % R % BS % N % FRAC % MOD % T % D % RFdiv << std::endl;

    UHD_ASSERT_THROW((settings.frac_12_bit          & ((boost::uint16_t)~0xFFF)) == 0);
    UHD_ASSERT_THROW((settings.mod_12_bit           & ((boost::uint16_t)~0xFFF)) == 0);
    UHD_ASSERT_THROW((settings.clock_divider_12_bit & ((boost::uint16_t)~0xFFF)) == 0);
    UHD_ASSERT_THROW((settings.r_counter_10_bit     & ((boost::uint16_t)~0x3FF)) == 0);

    UHD_ASSERT_THROW(vco_freq >= MIN_VCO_FREQ and vco_freq <= MAX_VCO_FREQ);
    UHD_ASSERT_THROW(settings.rf_divider >= static_cast<boost::uint16_t>(constraints.rf_divider_range.start()));
    UHD_ASSERT_THROW(settings.rf_divider <= static_cast<boost::uint16_t>(constraints.rf_divider_range.stop()));
    UHD_ASSERT_THROW(settings.int_16_bit >= static_cast<boost::uint16_t>(constraints.int_range.start()));
    UHD_ASSERT_THROW(settings.int_16_bit <= static_cast<boost::uint16_t>(constraints.int_range.stop()));

    return settings;
}


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
        case 0x054:
            db_actual = sbx_versionx_sptr(new sbx_version3(this));
            freq_range = sbx_freq_range;
            break;
        case 0x065:
            db_actual = sbx_versionx_sptr(new sbx_version4(this));
            freq_range = sbx_freq_range;
            break;
        case 0x067:
            db_actual = sbx_versionx_sptr(new cbx(this));
            freq_range = cbx_freq_range;
            break;
        default:
            /* We didn't recognize the version of the board... */
            UHD_THROW_INVALID_CODE_PATH();
    }

    ////////////////////////////////////////////////////////////////////
    // Register RX properties
    ////////////////////////////////////////////////////////////////////
    if(get_rx_id() == 0x054) this->get_rx_subtree()->create<std::string>("name").set("SBXv3 RX");
    else if(get_rx_id() == 0x065) this->get_rx_subtree()->create<std::string>("name").set("SBXv4 RX");
    else if(get_rx_id() == 0x067) this->get_rx_subtree()->create<std::string>("name").set("CBX RX");
    else this->get_rx_subtree()->create<std::string>("name").set("SBX/CBX RX");

    this->get_rx_subtree()->create<sensor_value_t>("sensors/lo_locked")
        .publish(boost::bind(&sbx_xcvr::get_locked, this, dboard_iface::UNIT_RX));
    BOOST_FOREACH(const std::string &name, sbx_rx_gain_ranges.keys()){
        this->get_rx_subtree()->create<double>("gains/"+name+"/value")
            .coerce(boost::bind(&sbx_xcvr::set_rx_gain, this, _1, name))
            .set(sbx_rx_gain_ranges[name].start());
        this->get_rx_subtree()->create<meta_range_t>("gains/"+name+"/range")
            .set(sbx_rx_gain_ranges[name]);
    }
    this->get_rx_subtree()->create<double>("freq/value")
        .coerce(boost::bind(&sbx_xcvr::set_lo_freq, this, dboard_iface::UNIT_RX, _1))
        .set((freq_range.start() + freq_range.stop())/2.0);
    this->get_rx_subtree()->create<meta_range_t>("freq/range").set(freq_range);
    this->get_rx_subtree()->create<std::string>("antenna/value")
        .subscribe(boost::bind(&sbx_xcvr::set_rx_ant, this, _1))
        .set("RX2");
    this->get_rx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(sbx_rx_antennas);
    this->get_rx_subtree()->create<std::string>("connection").set("IQ");
    this->get_rx_subtree()->create<bool>("enabled").set(true); //always enabled
    this->get_rx_subtree()->create<bool>("use_lo_offset").set(false);
    this->get_rx_subtree()->create<double>("bandwidth/value").set(2*20.0e6); //20MHz low-pass, we want complex double-sided
    this->get_rx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(2*20.0e6, 2*20.0e6));

    ////////////////////////////////////////////////////////////////////
    // Register TX properties
    ////////////////////////////////////////////////////////////////////
    if(get_tx_id() == 0x055) this->get_tx_subtree()->create<std::string>("name").set("SBXv3 TX");
    else if(get_tx_id() == 0x064) this->get_tx_subtree()->create<std::string>("name").set("SBXv4 TX");
    else if(get_tx_id() == 0x066) this->get_tx_subtree()->create<std::string>("name").set("CBX TX");
    else this->get_tx_subtree()->create<std::string>("name").set("SBX/CBX TX");

    this->get_tx_subtree()->create<sensor_value_t>("sensors/lo_locked")
        .publish(boost::bind(&sbx_xcvr::get_locked, this, dboard_iface::UNIT_TX));
    BOOST_FOREACH(const std::string &name, sbx_tx_gain_ranges.keys()){
        this->get_tx_subtree()->create<double>("gains/"+name+"/value")
            .coerce(boost::bind(&sbx_xcvr::set_tx_gain, this, _1, name))
            .set(sbx_tx_gain_ranges[name].start());
        this->get_tx_subtree()->create<meta_range_t>("gains/"+name+"/range")
            .set(sbx_tx_gain_ranges[name]);
    }
    this->get_tx_subtree()->create<double>("freq/value")
        .coerce(boost::bind(&sbx_xcvr::set_lo_freq, this, dboard_iface::UNIT_TX, _1))
        .set((freq_range.start() + freq_range.stop())/2.0);
    this->get_tx_subtree()->create<meta_range_t>("freq/range").set(freq_range);
    this->get_tx_subtree()->create<std::string>("antenna/value")
        .subscribe(boost::bind(&sbx_xcvr::set_tx_ant, this, _1))
        .set(sbx_tx_antennas.at(0));
    this->get_tx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(sbx_tx_antennas);
    this->get_tx_subtree()->create<std::string>("connection").set("QI");
    this->get_tx_subtree()->create<bool>("enabled").set(true); //always enabled
    this->get_tx_subtree()->create<bool>("use_lo_offset").set(false);
    this->get_tx_subtree()->create<double>("bandwidth/value").set(2*20.0e6); //20MHz low-pass, we want complex double-sided
    this->get_tx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(2*20.0e6, 2*20.0e6));

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
    int rx_ld_led = _rx_lo_lock_cache ? 0 : RX_LED_LD;
    int tx_ld_led = _tx_lo_lock_cache ? 0 : TX_LED_LD;
    int rx_ant_led = _rx_ant == "TX/RX" ? RX_LED_RX1RX2 : 0;
    int tx_ant_led = _tx_ant == "TX/RX" ? 0 : TX_LED_TXRX;

    //setup the tx atr (this does not change with antenna)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            dboard_iface::ATR_REG_IDLE, 0 | tx_lo_lpf_en \
            | tx_ld_led | tx_ant_led | TX_POWER_UP | ANT_XX | TX_MIXER_DIS);

    //setup the rx atr (this does not change with antenna)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            dboard_iface::ATR_REG_IDLE, rx_pga0_iobits | rx_lo_lpf_en \
            | rx_ld_led | rx_ant_led | RX_POWER_UP | ANT_XX | RX_MIXER_DIS);

    //set the RX atr regs that change with antenna setting
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            dboard_iface::ATR_REG_RX_ONLY, rx_pga0_iobits | rx_lo_lpf_en \
            | rx_ld_led | rx_ant_led | RX_POWER_UP | RX_MIXER_ENB \
            | ((_rx_ant != "RX2")? ANT_TXRX : ANT_RX2));
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            dboard_iface::ATR_REG_TX_ONLY, rx_pga0_iobits | rx_lo_lpf_en \
            | rx_ld_led | rx_ant_led | RX_POWER_UP | RX_MIXER_DIS \
            | ((_rx_ant == "CAL")? ANT_TXRX : ANT_RX2));
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, \
            dboard_iface::ATR_REG_FULL_DUPLEX, rx_pga0_iobits | rx_lo_lpf_en \
            | rx_ld_led | rx_ant_led | RX_POWER_UP | RX_MIXER_ENB \
            | ((_rx_ant == "CAL")? ANT_TXRX : ANT_RX2));

    //set the TX atr regs that change with antenna setting
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            dboard_iface::ATR_REG_RX_ONLY, 0 | tx_lo_lpf_en \
            | tx_ld_led | tx_ant_led | TX_POWER_UP | TX_MIXER_DIS \
            | ((_rx_ant != "RX2")? ANT_RX : ANT_TX));
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            dboard_iface::ATR_REG_TX_ONLY, tx_pga0_iobits | tx_lo_lpf_en \
            | tx_ld_led | tx_ant_led | TX_POWER_UP | TX_MIXER_ENB \
            | ((_tx_ant == "CAL")? ANT_RX : ANT_TX));
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, \
            dboard_iface::ATR_REG_FULL_DUPLEX, tx_pga0_iobits | tx_lo_lpf_en \
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


void sbx_xcvr::flash_leds(void) {
    //Remove LED gpios from ATR control temporarily and set to outputs
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, TXIO_MASK);
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, RXIO_MASK);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, (TXIO_MASK|RX_LED_IO));
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, (RXIO_MASK|RX_LED_IO));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, TX_LED_LD, TX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, \
            TX_LED_TXRX|TX_LED_LD, TX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, RX_LED_LD, RX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, \
            RX_LED_RX1RX2|RX_LED_LD, RX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, RX_LED_LD, RX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, 0, RX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, TX_LED_LD, TX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, 0, TX_LED_IO);
    boost::this_thread::sleep(boost::posix_time::milliseconds(100));

    //Put LED gpios back in ATR control and update atr
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, (TXIO_MASK|TX_LED_IO));
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, (RXIO_MASK|RX_LED_IO));
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, (TXIO_MASK|TX_LED_IO));
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, (RXIO_MASK|RX_LED_IO));
}
