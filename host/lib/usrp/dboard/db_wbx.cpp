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

static const bool wbx_debug = false;

// Common IO Pins
#define ANTSW_IO        ((1 << 5)|(1 << 15))    // on UNIT_TX, 0 = TX, 1 = RX, on UNIT_RX 0 = main ant, 1 = RX2
#define ADF4350_CE      (1 << 3)
#define ADF4350_PDBRF   (1 << 2)
#define ADF4350_MUXOUT  (1 << 1)                // INPUT!!!
#define LOCKDET_MASK    (1 << 0)                // INPUT!!!

// TX IO Pins
#define TX_PUP_5V       (1 << 7)                // enables 5.0V power supply
#define TX_PUP_3V       (1 << 6)                // enables 3.3V supply
#define TXMOD_EN        (1 << 4)                // on UNIT_TX, 1 enables TX Modulator

// RX IO Pins
#define RX_PUP_5V       (1 << 7)                // enables 5.0V power supply
#define RX_PUP_3V       (1 << 6)                // enables 3.3V supply
#define RXBB_PDB        (1 << 4)                // on UNIT_RX, 1 powers up RX baseband

// RX Attenuator Pins
#define RX_ATTN_SHIFT   8                       // lsb of RX Attenuator Control
#define RX_ATTN_MASK    (63 << RX_ATTN_SHIFT)      // valid bits of RX Attenuator Control

// Mixer functions
#define TX_MIXER_ENB    (TXMOD_EN|ADF4350_PDBRF)
#define TX_MIXER_DIS    0

#define RX_MIXER_ENB    (RXBB_PDB|ADF4350_PDBRF)
#define RX_MIXER_DIS    0

// Pin functions
#define TX_POWER_IO     (TX_PUP_5V|TX_PUP_3V)   // high enables power supply
#define TXIO_MASK       (TX_POWER_IO|ANTSW_IO|ADF4350_CE|ADF4350_PDBRF|TXMOD_EN)

#define RX_POWER_IO     (RX_PUP_5V|RX_PUP_3V)   // high enables power supply
#define RXIO_MASK       (RX_POWER_IO|ANTSW_IO|ADF4350_CE|ADF4350_PDBRF|RXBB_PDB|RX_ATTN_MASK)

// Power functions
#define TX_POWER_UP     (TX_POWER_IO|ADF4350_CE)
#define TX_POWER_DOWN   0

#define RX_POWER_UP     (RX_POWER_IO|ADF4350_CE)
#define RX_POWER_DOWN   0

// Antenna constants
#define ANT_TX          0                       //the tx line is transmitting
#define ANT_RX          ANTSW_IO                //the tx line is receiving
#define ANT_TXRX        0                       //the rx line is on txrx
#define ANT_RX2         ANTSW_IO                //the rx line in on rx2
#define ANT_XX          0                       //dont care how the antenna is set

#include "adf4350_regs.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The WBX dboard
 **********************************************************************/
static const float _max_rx_pga0_gain = 31.5;
static const float _max_tx_pga0_gain = 25;

class wbx_xcvr : public xcvr_dboard_base{
public:
    wbx_xcvr(
        ctor_args_t args,
        const freq_range_t &freq_range
    );
    ~wbx_xcvr(void);

    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);

    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);

private:
    freq_range_t _freq_range;
    uhd::dict<dboard_iface::unit_t, bool> _div2;
    double       _rx_lo_freq, _tx_lo_freq;
    std::string  _rx_ant;
    int          _rx_pga0_attn_iobits;
    float        _rx_pga0_gain;
    float        _tx_pga0_gain;

    void set_rx_lo_freq(double freq);
    void set_tx_lo_freq(double freq);
    void set_rx_ant(const std::string &ant);
    void set_rx_pga0_gain(float gain);
    void set_rx_pga0_attn(float attn);
    void set_tx_pga0_gain(float gain);

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
};

/***********************************************************************
 * Register the WBX dboard (min freq, max freq, rx div2, tx div2)
 **********************************************************************/
static dboard_base::sptr make_wbx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new wbx_xcvr(args, freq_range_t(50e6, 2220e6)));
}

UHD_STATIC_BLOCK(reg_wbx_dboards){
    dboard_manager::register_dboard(0x0052, &make_wbx, "WBX RX");
    dboard_manager::register_dboard(0x0053, &make_wbx, "WBX TX");
}

/***********************************************************************
 * Structors
 **********************************************************************/
wbx_xcvr::wbx_xcvr(
    ctor_args_t args,
    const freq_range_t &freq_range
) : xcvr_dboard_base(args){
    _freq_range = freq_range;

    //enable the clocks that we need
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_TX, true);
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);

    //set the gpio directions
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, TXIO_MASK);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, RXIO_MASK);
    if (wbx_debug) std::cerr << boost::format(
        "WBX GPIO Direction: RX: 0x%08x, TX: 0x%08x"
    ) % RXIO_MASK % TXIO_MASK << std::endl;

    //set some default values
    set_rx_lo_freq((_freq_range.min + _freq_range.max)/2.0);
    set_tx_lo_freq((_freq_range.min + _freq_range.max)/2.0);
    set_rx_ant("RX2");
    set_rx_pga0_gain(0);
    set_tx_pga0_gain(0);
}

wbx_xcvr::~wbx_xcvr(void){
    /* NOP */
}

/***********************************************************************
 * Helper Methods
 **********************************************************************/
void wbx_xcvr::update_atr(void){
    //calculate atr pins

    //setup the tx atr (this does not change with antenna)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_IDLE,        TX_POWER_UP | ANT_XX | TX_MIXER_DIS);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_RX_ONLY,     TX_POWER_UP | ANT_RX | TX_MIXER_DIS);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_TX_ONLY,     TX_POWER_UP | ANT_TX | TX_MIXER_ENB);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_FULL_DUPLEX, TX_POWER_UP | ANT_TX | TX_MIXER_ENB);

    //setup the rx atr (this does not change with antenna)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_IDLE,
        _rx_pga0_attn_iobits | RX_POWER_UP | ANT_XX | RX_MIXER_DIS);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_TX_ONLY,
        _rx_pga0_attn_iobits | RX_POWER_UP | ANT_XX | RX_MIXER_DIS);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_FULL_DUPLEX,
        _rx_pga0_attn_iobits | RX_POWER_UP | ANT_RX2| RX_MIXER_ENB);

    //set the rx atr regs that change with antenna setting
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_RX_ONLY,
        _rx_pga0_attn_iobits | RX_POWER_UP | RX_MIXER_ENB | ((_rx_ant == "TX/RX")? ANT_TXRX : ANT_RX2));
    if (wbx_debug) std::cerr << boost::format(
        "WBX RXONLY ATR REG: 0x%08x"
    ) % (_rx_pga0_attn_iobits | RX_POWER_UP | RX_MIXER_ENB | ((_rx_ant == "TX/RX")? ANT_TXRX : ANT_RX2)) << std::endl;
}

void wbx_xcvr::set_rx_lo_freq(double freq){
    _rx_lo_freq = set_lo_freq(dboard_iface::UNIT_RX, freq);
}

void wbx_xcvr::set_tx_lo_freq(double freq){
    _tx_lo_freq = set_lo_freq(dboard_iface::UNIT_TX, freq);
}

void wbx_xcvr::set_rx_ant(const std::string &ant){
    //validate input
    UHD_ASSERT_THROW(ant == "TX/RX" or ant == "RX2");

    //shadow the setting
    _rx_ant = ant;

    //write the new antenna setting to atr regs
    update_atr();
}

void wbx_xcvr::set_rx_pga0_gain(float gain){
    //clip the input
    gain = std::clip<float>(gain, 0, _max_rx_pga0_gain);

    //shadow the setting (does not account for precision loss)
    _rx_pga0_gain = gain;

    //convert to attenuation and update iobits for atr
    set_rx_pga0_attn(_max_rx_pga0_gain - gain);

    //write the new gain to atr regs
    update_atr();
}

void wbx_xcvr::set_rx_pga0_attn(float attn)
{
    int attn_code = int(floor(attn*2));
    _rx_pga0_attn_iobits = ((~attn_code) << RX_ATTN_SHIFT) & RX_ATTN_MASK;
    if (wbx_debug) std::cerr << boost::format(
        "WBX Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x"
    ) % attn % attn_code % (_rx_pga0_attn_iobits & RX_ATTN_MASK) % RX_ATTN_MASK << std::endl;
}

void wbx_xcvr::set_tx_pga0_gain(float gain){
    //clip the input
    gain = std::clip<float>(gain, 0, _max_tx_pga0_gain);

    //voltage level constants
    static const float max_volts = float(0.5), min_volts = float(1.4);
    static const float slope = (max_volts-min_volts)/_max_rx_pga0_gain;

    //calculate the voltage for the aux dac
    float dac_volts = gain*slope + min_volts;

    if (wbx_debug) std::cerr << boost::format(
        "WBX TX Gain: %f dB, dac_volts: %f V"
    ) % gain % dac_volts << std::endl;

    //write the new voltage to the aux dac
    this->get_iface()->write_aux_dac(dboard_iface::UNIT_TX, 0, dac_volts);

    //shadow the setting (does not account for precision loss)
    _tx_pga0_gain = gain;
}

double wbx_xcvr::set_lo_freq(
    dboard_iface::unit_t unit,
    double target_freq
){
    if (wbx_debug) std::cerr << boost::format(
        "WBX tune: target frequency %f Mhz"
    ) % (target_freq/1e6) << std::endl;

    //clip the input
    target_freq = std::clip(target_freq, _freq_range.min, _freq_range.max);

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
    double vco_freq = target_freq*2;
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
    actual_freq = double((N + (double(FRAC)/double(MOD)))*ref_freq*(1+int(D))/(R*(1+int(T)))/RFdiv/2);


    if (wbx_debug) {
        std::cerr << boost::format("WBX Intermediates: ref=%0.2f, outdiv=%f, fbdiv=%f") % (ref_freq*(1+int(D))/(R*(1+int(T)))) % double(RFdiv*2) % double(N + double(FRAC)/double(MOD)) << std::endl;

        std::cerr << boost::format("WBX tune: R=%d, BS=%d, N=%d, FRAC=%d, MOD=%d, T=%d, D=%d, RFdiv=%d, LD=%d"
            ) % R % BS % N % FRAC % MOD % T % D % RFdiv % get_locked(unit)<< std::endl
        << boost::format("WBX Frequencies (MHz): REQ=%0.2f, ACT=%0.2f, VCO=%0.2f, PFD=%0.2f, BAND=%0.2f"
            ) % (target_freq/1e6) % (actual_freq/1e6) % (vco_freq/1e6) % (pfd_freq/1e6) % (pfd_freq/BS/1e6) << std::endl;
    }

    //load the register values
    adf4350_regs_t regs;

    regs.frac_12_bit = FRAC;
    regs.int_16_bit = N;
    regs.mod_12_bit = MOD;
    regs.prescaler = prescaler;
    regs.r_counter_10_bit = R;
    regs.reference_divide_by_2 = T;
    regs.reference_doubler = D;
    regs.band_select_clock_div = BS;
    regs.rf_divider_select = rfdivsel_to_enum[RFdiv];

    //write the registers
    //correct power-up sequence to write registers (5, 4, 3, 2, 1, 0)
    int addr;

    for(addr=5; addr>=0; addr--){
        if (wbx_debug) std::cerr << boost::format(
            "WBX SPI Reg (0x%02x): 0x%08x"
        ) % addr % regs.get_reg(addr) << std::endl;
        this->get_iface()->write_spi(
            unit, spi_config_t::EDGE_RISE,
            regs.get_reg(addr), 32
        );
    }

    //return the actual frequency
    if (wbx_debug) std::cerr << boost::format(
        "WBX tune: actual frequency %f Mhz"
    ) % (actual_freq/1e6) << std::endl;
    return actual_freq;
}

/***********************************************************************
 * RX Get and Set
 **********************************************************************/
void wbx_xcvr::rx_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = get_rx_id().to_pp_string();
        return;

    case SUBDEV_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_GAIN:
        UHD_ASSERT_THROW(name == "PGA0");
        val = _rx_pga0_gain;
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        UHD_ASSERT_THROW(name == "PGA0");
        val = gain_range_t(0, _max_rx_pga0_gain, float(0.5));
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(1, "PGA0");
        return;

    case SUBDEV_PROP_FREQ:
        val = _rx_lo_freq;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = _freq_range;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = _rx_ant;
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:{
            prop_names_t ants = list_of("TX/RX")("RX2");
            val = ants;
        }
        return;

    case SUBDEV_PROP_QUADRATURE:
        val = true;
        return;

    case SUBDEV_PROP_IQ_SWAPPED:
        val = false;
        return;

    case SUBDEV_PROP_SPECTRUM_INVERTED:
        val = false;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
        return;

    case SUBDEV_PROP_LO_LOCKED:
        val = this->get_locked(dboard_iface::UNIT_RX);
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void wbx_xcvr::rx_set(const wax::obj &key_, const wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_FREQ:
        set_rx_lo_freq(val.as<double>());
        return;

    case SUBDEV_PROP_GAIN:
        UHD_ASSERT_THROW(name == "PGA0");
        set_rx_pga0_gain(val.as<float>());
        return;

    case SUBDEV_PROP_ANTENNA:
        set_rx_ant(val.as<std::string>());
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * TX Get and Set
 **********************************************************************/
void wbx_xcvr::tx_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_NAME:
        val = get_tx_id().to_pp_string();
        return;

    case SUBDEV_PROP_OTHERS:
        val = prop_names_t(); //empty
        return;

    case SUBDEV_PROP_GAIN:
        UHD_ASSERT_THROW(name == "PGA0");
        val = _tx_pga0_gain;
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        UHD_ASSERT_THROW(name == "PGA0");
        val = gain_range_t(0, _max_tx_pga0_gain, float(0.05));
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(1, "PGA0");
        return;

    case SUBDEV_PROP_FREQ:
        val = _tx_lo_freq;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = _freq_range;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("TX/RX");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = prop_names_t(1, "TX/RX");
        return;

    case SUBDEV_PROP_QUADRATURE:
        val = true;
        return;

    case SUBDEV_PROP_IQ_SWAPPED:
        val = false;
        return;

    case SUBDEV_PROP_SPECTRUM_INVERTED:
        val = false;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
        return;

    case SUBDEV_PROP_LO_LOCKED:
        val = this->get_locked(dboard_iface::UNIT_TX);
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void wbx_xcvr::tx_set(const wax::obj &key_, const wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_FREQ:
        set_tx_lo_freq(val.as<double>());
        return;

    case SUBDEV_PROP_GAIN:
        UHD_ASSERT_THROW(name == "PGA0");
        set_tx_pga0_gain(val.as<float>());
        return;

    case SUBDEV_PROP_ANTENNA:
        //its always set to tx/rx, so we only allow this value
        UHD_ASSERT_THROW(val.as<std::string>() == "TX/RX");
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
