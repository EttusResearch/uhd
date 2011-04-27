//
// Copyright 2010 Ettus Research LLC
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

// Common IO Pins
#define LO_LPF_EN       (1 << 15)
#define ADF4350_CE      (1 << 3)
#define ADF4350_PDBRF   (1 << 2)
#define ADF4350_MUXOUT  (1 << 1)                // INPUT!!!
#define LOCKDET_MASK    (1 << 0)                // INPUT!!!

// TX IO Pins
#define TRSW            (1 << 14)               // 0 = TX, 1 = RX 
#define TX_LED_TXRX     (1 << 7)                // LED for TX Antenna Selection TX/RX
#define TX_LED_LD       (1 << 6)                // LED for TX Lock Detect
#define DIS_POWER_TX    (1 << 5)                // on UNIT_TX, 0 powers up TX
#define TX_ENABLE       (1 << 4)                // on UNIT_TX, 0 disables TX Mixer

// RX IO Pins
#define LNASW           (1 << 14)               // 0 = TX/RX, 1 = RX2
#define RX_LED_RX1RX2   (1 << 7)                // LED for RX Antenna Selection RX1/RX2
#define RX_LED_LD       (1 << 6)                // LED for RX Lock Detect
#define DIS_POWER_RX    (1 << 5)                // on UNIT_RX, 0 powers up RX
#define RX_DISABLE      (1 << 4)                // on UNIT_RX, 1 disables RX Mixer and Baseband

// RX Attenuator Pins
#define RX_ATTN_SHIFT   8                       // lsb of RX Attenuator Control
#define RX_ATTN_MASK    (63 << RX_ATTN_SHIFT)   // valid bits of RX Attenuator Control

// TX Attenuator Pins
#define TX_ATTN_SHIFT   8                       // lsb of RX Attenuator Control
#define TX_ATTN_MASK    (63 << TX_ATTN_SHIFT)   // valid bits of RX Attenuator Control

// Mixer functions
#define TX_MIXER_ENB    (ADF4350_PDBRF)
#define TX_MIXER_DIS    0

#define RX_MIXER_ENB    (ADF4350_PDBRF)
#define RX_MIXER_DIS    0

// Pin functions
#define TX_LED_IO       (TX_LED_TXRX|TX_LED_LD)     // LED gpio lines, pull down for LED
#define TXIO_MASK       (LO_LPF_EN|TRSW|ADF4350_CE|ADF4350_PDBRF|TX_ATTN_MASK|DIS_POWER_TX|TX_ENABLE)

#define RX_LED_IO       (RX_LED_RX1RX2|RX_LED_LD)   // LED gpio lines, pull down for LED
#define RXIO_MASK       (LO_LPF_EN|LNASW|ADF4350_CE|ADF4350_PDBRF|RX_ATTN_MASK|DIS_POWER_RX|RX_DISABLE)

// Power functions
#define TX_POWER_UP     (ADF4350_CE|TX_ENABLE)
#define TX_POWER_DOWN   (DIS_POWER_TX)

#define RX_POWER_UP     (ADF4350_CE)
#define RX_POWER_DOWN   (DIS_POWER_RX)

// Antenna constants
#define ANT_TX          TRSW                    //the tx line is transmitting
#define ANT_RX          0                       //the tx line is receiving
#define ANT_TXRX        0                       //the rx line is on txrx
#define ANT_RX2         LNASW                   //the rx line in on rx2
#define ANT_XX          LNASW                   //dont care how the antenna is set

#include "adf4350_regs.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/warning.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>
#include <boost/thread.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The SBX dboard constants
 **********************************************************************/
static const bool sbx_debug = false;

static const freq_range_t sbx_freq_range(68.75e6, 4.4e9);

static const freq_range_t sbx_tx_lo_2dbm = list_of
    (range_t(0.35e9, 0.37e9))
;

static const freq_range_t sbx_enable_tx_lo_filter = list_of
    (range_t(0.4e9, 1.5e9))
;

static const freq_range_t sbx_enable_rx_lo_filter = list_of
    (range_t(0.4e9, 1.5e9))
;

static const prop_names_t sbx_tx_antennas = list_of("TX/RX");

static const prop_names_t sbx_rx_antennas = list_of("TX/RX")("RX2");

static const uhd::dict<std::string, gain_range_t> sbx_tx_gain_ranges = map_list_of
    ("PGA0", gain_range_t(0, 31.5, double(0.5)))
;

static const uhd::dict<std::string, gain_range_t> sbx_rx_gain_ranges = map_list_of
    ("PGA0", gain_range_t(0, 31.5, double(0.5)))
;

/***********************************************************************
 * The SBX dboard
 **********************************************************************/
class sbx_xcvr : public xcvr_dboard_base{
public:
    sbx_xcvr(ctor_args_t args);
    ~sbx_xcvr(void);

    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);

    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);

private:
    uhd::dict<std::string, double> _tx_gains, _rx_gains;
    double       _rx_lo_freq, _tx_lo_freq;
    std::string  _tx_ant, _rx_ant;

    void set_rx_lo_freq(double freq);
    void set_tx_lo_freq(double freq);
    void set_rx_ant(const std::string &ant);
    void set_tx_ant(const std::string &ant);
    void set_rx_gain(double gain, const std::string &name);
    void set_tx_gain(double gain, const std::string &name);

    void update_atr(void);

    /*!
     * Set the LO frequency for the particular dboard unit.
     * \param unit which unit rx or tx
     * \param target_freq the desired frequency in Hz
     * \return the actual frequency in Hz
     */
    double set_lo_freq(dboard_iface::unit_t unit, double target_freq);

    /*!
     * Get the lock detect status of the LO.
     * \param unit which unit rx or tx
     * \return true for locked
     */
    bool get_locked(dboard_iface::unit_t unit){
        return (this->get_iface()->read_gpio(unit) & LOCKDET_MASK) != 0;
    }

    /*!
     * Flash the LEDs
     */
    void flash_leds(void) {
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

};

/***********************************************************************
 * Register the SBX dboard (min freq, max freq, rx div2, tx div2)
 **********************************************************************/
static dboard_base::sptr make_sbx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new sbx_xcvr(args));
}

UHD_STATIC_BLOCK(reg_sbx_dboards){
    dboard_manager::register_dboard(0x0054, 0x0055, &make_sbx, "SBX");
}

/***********************************************************************
 * Structors
 **********************************************************************/
sbx_xcvr::sbx_xcvr(ctor_args_t args) : xcvr_dboard_base(args){

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

    if (sbx_debug) std::cerr << boost::format(
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

    
    if (sbx_debug) std::cerr << boost::format(
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

    
    if (sbx_debug) std::cerr << boost::format(
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

    if (sbx_debug) std::cerr << boost::format(
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
    _rx_lo_freq = set_lo_freq(dboard_iface::UNIT_RX, freq);
}

void sbx_xcvr::set_tx_lo_freq(double freq){
    _tx_lo_freq = set_lo_freq(dboard_iface::UNIT_TX, freq);
}

double sbx_xcvr::set_lo_freq(
    dboard_iface::unit_t unit,
    double target_freq
){
    if (sbx_debug) std::cerr << boost::format(
        "SBX tune: target frequency %f Mhz"
    ) % (target_freq/1e6) << std::endl;

    //clip the input
    target_freq = sbx_freq_range.clip(target_freq);

    //map prescaler setting to mininmum integer divider (N) values (pg.18 prescaler)
    static const uhd::dict<int, int> prescaler_to_min_int_div = map_list_of
        (0,23) //adf4350_regs_t::PRESCALER_4_5
        (1,75) //adf4350_regs_t::PRESCALER_8_9
    ;

    //map rf divider select output dividers to enums
    static const uhd::dict<int, adf4350_regs_t::rf_divider_select_t> rfdivsel_to_enum = map_list_of
        (1,  adf4350_regs_t::RF_DIVIDER_SELECT_DIV1)
        (2,  adf4350_regs_t::RF_DIVIDER_SELECT_DIV2)
        (4,  adf4350_regs_t::RF_DIVIDER_SELECT_DIV4)
        (8,  adf4350_regs_t::RF_DIVIDER_SELECT_DIV8)
        (16, adf4350_regs_t::RF_DIVIDER_SELECT_DIV16)
    ;

    double actual_freq, pfd_freq;
    double ref_freq = this->get_iface()->get_clock_rate(unit);
    int R=0, BS=0, N=0, FRAC=0, MOD=0;
    int RFdiv = 1;
    adf4350_regs_t::reference_divide_by_2_t T     = adf4350_regs_t::REFERENCE_DIVIDE_BY_2_DISABLED;
    adf4350_regs_t::reference_doubler_t     D     = adf4350_regs_t::REFERENCE_DOUBLER_DISABLED;    

    //Reference doubler for 50% duty cycle
    // if ref_freq < 12.5MHz enable regs.reference_divide_by_2
    if(ref_freq <= 12.5e6) D = adf4350_regs_t::REFERENCE_DOUBLER_ENABLED;

    //increase RF divider until acceptable VCO frequency
    //start with target_freq*2 because mixer has divide by 2
    double vco_freq = target_freq;
    while (vco_freq < 2.2e9) {
        vco_freq *= 2;
        RFdiv *= 2;
    }

    //use 8/9 prescaler for vco_freq > 3 GHz (pg.18 prescaler)
    adf4350_regs_t::prescaler_t prescaler = vco_freq > 3e9 ? adf4350_regs_t::PRESCALER_8_9 : adf4350_regs_t::PRESCALER_4_5;

    /*
     * The goal here is to loop though possible R dividers,
     * band select clock dividers, N (int) dividers, and FRAC 
     * (frac) dividers.
     *
     * Calculate the N and F dividers for each set of values.
     * The loop exists when it meets all of the constraints.
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
        pfd_freq = ref_freq*(1+D)/(R*(1+T));

        //keep the PFD frequency at or below 25MHz (Loop Filter Bandwidth)
        if (pfd_freq > 25e6) continue;

        //ignore fractional part of tuning
        N = int(std::floor(vco_freq/pfd_freq));

        //keep N > minimum int divider requirement
        if (N < prescaler_to_min_int_div[prescaler]) continue;

        for(BS=1; BS <= 255; BS+=1){
            //keep the band select frequency at or below 100KHz
            //constraint on band select clock
            if (pfd_freq/BS > 100e3) continue;
            goto done_loop;
        }
    } done_loop:

    //Fractional-N calculation
    MOD = 4095; //max fractional accuracy
    FRAC = int((vco_freq/pfd_freq - N)*MOD);

    //Reference divide-by-2 for 50% duty cycle
    // if R even, move one divide by 2 to to regs.reference_divide_by_2
    if(R % 2 == 0){
        T = adf4350_regs_t::REFERENCE_DIVIDE_BY_2_ENABLED;
        R /= 2;
    }

    //actual frequency calculation
    actual_freq = double((N + (double(FRAC)/double(MOD)))*ref_freq*(1+int(D))/(R*(1+int(T)))/RFdiv);

    if (sbx_debug) {
        std::cerr << boost::format("SBX Intermediates: ref=%0.2f, outdiv=%f, fbdiv=%f") % (ref_freq*(1+int(D))/(R*(1+int(T)))) % double(RFdiv*2) % double(N + double(FRAC)/double(MOD)) << std::endl;

        std::cerr << boost::format("SBX tune: R=%d, BS=%d, N=%d, FRAC=%d, MOD=%d, T=%d, D=%d, RFdiv=%d, LD=%d"
            ) % R % BS % N % FRAC % MOD % T % D % RFdiv % get_locked(unit)<< std::endl
        << boost::format("SBX Frequencies (MHz): REQ=%0.2f, ACT=%0.2f, VCO=%0.2f, PFD=%0.2f, BAND=%0.2f"
            ) % (target_freq/1e6) % (actual_freq/1e6) % (vco_freq/1e6) % (pfd_freq/1e6) % (pfd_freq/BS/1e6) << std::endl;
    }

    //load the register values
    adf4350_regs_t regs;

    if ((unit == dboard_iface::UNIT_TX) and (actual_freq == sbx_tx_lo_2dbm.clip(actual_freq))) 
        regs.output_power = adf4350_regs_t::OUTPUT_POWER_2DBM;
    else
        regs.output_power = adf4350_regs_t::OUTPUT_POWER_5DBM;

    regs.frac_12_bit = FRAC;
    regs.int_16_bit = N;
    regs.mod_12_bit = MOD;
    regs.prescaler = prescaler;
    regs.r_counter_10_bit = R;
    regs.reference_divide_by_2 = T;
    regs.reference_doubler = D;
    regs.band_select_clock_div = BS;
    UHD_ASSERT_THROW(rfdivsel_to_enum.has_key(RFdiv));
    regs.rf_divider_select = rfdivsel_to_enum[RFdiv];

    //write the registers
    //correct power-up sequence to write registers (5, 4, 3, 2, 1, 0)
    int addr;

    for(addr=5; addr>=0; addr--){
        if (sbx_debug) std::cerr << boost::format(
            "SBX SPI Reg (0x%02x): 0x%08x"
        ) % addr % regs.get_reg(addr) << std::endl;
        this->get_iface()->write_spi(
            unit, spi_config_t::EDGE_RISE,
            regs.get_reg(addr), 32
        );
    }

    //return the actual frequency
    if (sbx_debug) std::cerr << boost::format(
        "SBX tune: actual frequency %f Mhz"
    ) % (actual_freq/1e6) << std::endl;
    return actual_freq;
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
        uhd::warning::post(
            str(boost::format("SBX: No tunable bandwidth, fixed filtered to 40MHz"))
        );
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
        uhd::warning::post(
            str(boost::format("SBX: No tunable bandwidth, fixed filtered to 40MHz"))
        );
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
