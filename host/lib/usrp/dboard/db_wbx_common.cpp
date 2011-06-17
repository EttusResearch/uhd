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

// Common IO Pins
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

// TX Attenuator Pins (v3 only)
#define TX_ATTN_16      (1 << 14)
#define TX_ATTN_8       (1 << 5)
#define TX_ATTN_4       (1 << 4)
#define TX_ATTN_2       (1 << 3)
#define TX_ATTN_1       (1 << 1)
#define TX_ATTN_MASK    (TX_ATTN_16|TX_ATTN_8|TX_ATTN_4|TX_ATTN_2|TX_ATTN_1)      // valid bits of TX Attenuator Control

// Mixer functions
#define TX_MIXER_ENB    (TXMOD_EN|ADF4350_PDBRF)
#define TX_MIXER_DIS    0

#define RX_MIXER_ENB    (RXBB_PDB|ADF4350_PDBRF)
#define RX_MIXER_DIS    0

// Power functions
#define TX_POWER_UP     (TX_PUP_5V|TX_PUP_3V) // high enables power supply
#define TX_POWER_DOWN   0

#define RX_POWER_UP     (RX_PUP_5V|RX_PUP_3V|ADF4350_CE) // high enables power supply
#define RX_POWER_DOWN   0

#include "db_wbx_common.hpp"
#include "adf4350_regs.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The WBX Common dboard constants
 **********************************************************************/
static const uhd::dict<std::string, gain_range_t> wbx_tx_gain_ranges = map_list_of
    ("PGA0", gain_range_t(0, 25, 0.05))
;

static const uhd::dict<std::string, gain_range_t> wbx_v3_tx_gain_ranges = map_list_of
    ("PGA0", gain_range_t(0, 31, 1.0))
;

static const uhd::dict<std::string, gain_range_t> wbx_rx_gain_ranges = map_list_of
    ("PGA0", gain_range_t(0, 31.5, 0.5))
;

/***********************************************************************
 * WBX Common Implementation
 **********************************************************************/
wbx_base::wbx_base(ctor_args_t args) : xcvr_dboard_base(args){

    //enable the clocks that we need
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_TX, true);
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);

    //v3 has different io bits for attenuator control
    int v3_iobits = is_v3() ? TX_ATTN_MASK : ADF4350_CE;

    //set the gpio directions and atr controls
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, TXMOD_EN|ADF4350_PDBRF);
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, RXBB_PDB|ADF4350_PDBRF);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, TX_PUP_5V|TX_PUP_3V|TXMOD_EN|ADF4350_PDBRF|v3_iobits);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, RX_PUP_5V|RX_PUP_3V|ADF4350_CE|RXBB_PDB|ADF4350_PDBRF|RX_ATTN_MASK);

    //setup ATR for the mixer enables (always enabled to prevent phase slip between bursts)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_IDLE,        TX_MIXER_ENB, TX_MIXER_DIS | TX_MIXER_ENB);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_RX_ONLY,     TX_MIXER_ENB, TX_MIXER_DIS | TX_MIXER_ENB);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_TX_ONLY,     TX_MIXER_ENB, TX_MIXER_DIS | TX_MIXER_ENB);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_FULL_DUPLEX, TX_MIXER_ENB, TX_MIXER_DIS | TX_MIXER_ENB);

    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_IDLE,        RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_TX_ONLY,     RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_RX_ONLY,     RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_FULL_DUPLEX, RX_MIXER_ENB, RX_MIXER_DIS | RX_MIXER_ENB);

    //set some default values
    if (is_v3()) {
        BOOST_FOREACH(const std::string &name, wbx_v3_tx_gain_ranges.keys()){
            set_tx_gain(wbx_v3_tx_gain_ranges[name].start(), name);
        }
    }
    else {
        BOOST_FOREACH(const std::string &name, wbx_tx_gain_ranges.keys()){
            set_tx_gain(wbx_tx_gain_ranges[name].start(), name);
        }
    }

    BOOST_FOREACH(const std::string &name, wbx_rx_gain_ranges.keys()){
        set_rx_gain(wbx_rx_gain_ranges[name].start(), name);
    }
    set_rx_enabled(false);
    set_tx_enabled(false);
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

void wbx_base::set_tx_enabled(bool enb){
    this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX,
        (enb)? TX_POWER_UP | ADF4350_CE : TX_POWER_DOWN, TX_POWER_UP | TX_POWER_DOWN | (is_v3() ? 0 : ADF4350_CE)
    );
}

/***********************************************************************
 * Gain Handling
 **********************************************************************/
static int rx_pga0_gain_to_iobits(double &gain){
    //clip the input
    gain = wbx_rx_gain_ranges["PGA0"].clip(gain);

    //convert to attenuation
    double attn = wbx_rx_gain_ranges["PGA0"].stop() - gain;

    //calculate the attenuation
    int attn_code = boost::math::iround(attn*2);
    int iobits = ((~attn_code) << RX_ATTN_SHIFT) & RX_ATTN_MASK;

    UHD_LOGV(often) << boost::format(
        "WBX RX Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x"
    ) % attn % attn_code % (iobits & RX_ATTN_MASK) % RX_ATTN_MASK << std::endl;

    //the actual gain setting
    gain = wbx_rx_gain_ranges["PGA0"].stop() - double(attn_code)/2;

    return iobits;
}

//v3 TX gains
static int tx_pga0_gain_to_iobits(double &gain){
    //clip the input
    gain = wbx_v3_tx_gain_ranges["PGA0"].clip(gain);

    //convert to attenuation
    double attn = wbx_v3_tx_gain_ranges["PGA0"].stop() - gain;

    //calculate the attenuation
    int attn_code = boost::math::iround(attn*2);
    int iobits = (
            (attn_code & 16 ? 0 : TX_ATTN_16) |
            (attn_code &  8 ? 0 : TX_ATTN_8) |
            (attn_code &  4 ? 0 : TX_ATTN_4) |
            (attn_code &  2 ? 0 : TX_ATTN_2) |
            (attn_code &  1 ? 0 : TX_ATTN_1) 
        ) & TX_ATTN_MASK;

    UHD_LOGV(often) << boost::format(
        "WBX TX Attenuation: %f dB, Code: %d, IO Bits %x, Mask: %x"
    ) % attn % attn_code % (iobits & TX_ATTN_MASK) % TX_ATTN_MASK << std::endl;

    //the actual gain setting
    gain = wbx_v3_tx_gain_ranges["PGA0"].stop() - double(attn_code)/2;

    return iobits;
}

//Pre v3 TX gains
static double tx_pga0_gain_to_dac_volts(double &gain){
    //clip the input
    gain = wbx_tx_gain_ranges["PGA0"].clip(gain);

    //voltage level constants
    static const double max_volts = 0.5, min_volts = 1.4;
    static const double slope = (max_volts-min_volts)/wbx_tx_gain_ranges["PGA0"].stop();

    //calculate the voltage for the aux dac
    double dac_volts = gain*slope + min_volts;

    UHD_LOGV(often) << boost::format(
        "WBX TX Gain: %f dB, dac_volts: %f V"
    ) % gain % dac_volts << std::endl;

    //the actual gain setting
    gain = (dac_volts - min_volts)/slope;

    return dac_volts;
}

void wbx_base::set_tx_gain(double gain, const std::string &name){
    if (is_v3()) {
        assert_has(wbx_v3_tx_gain_ranges.keys(), name, "wbx tx gain name");
        if(name == "PGA0"){
            double dac_volts = tx_pga0_gain_to_iobits(gain);
            _tx_gains[name] = gain;

            //write the new voltage to the aux dac
            this->get_iface()->write_aux_dac(dboard_iface::UNIT_TX, dboard_iface::AUX_DAC_A, dac_volts);
        }
        else UHD_THROW_INVALID_CODE_PATH();
    }
    else {
        assert_has(wbx_tx_gain_ranges.keys(), name, "wbx tx gain name");
        if(name == "PGA0"){
            double dac_volts = tx_pga0_gain_to_dac_volts(gain);
            _tx_gains[name] = gain;

            //write the new voltage to the aux dac
            this->get_iface()->write_aux_dac(dboard_iface::UNIT_TX, dboard_iface::AUX_DAC_A, dac_volts);
        }
        else UHD_THROW_INVALID_CODE_PATH();
    }
}

void wbx_base::set_rx_gain(double gain, const std::string &name){
    assert_has(wbx_rx_gain_ranges.keys(), name, "wbx rx gain name");
    if(name == "PGA0"){
        boost::uint16_t io_bits = rx_pga0_gain_to_iobits(gain);
        _rx_gains[name] = gain;

        //write the new gain to rx gpio outputs
        this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, io_bits, RX_ATTN_MASK);
    }
    else UHD_THROW_INVALID_CODE_PATH();
}

/***********************************************************************
 * Tuning
 **********************************************************************/
double wbx_base::set_lo_freq(
    dboard_iface::unit_t unit,
    double target_freq
){
    UHD_LOGV(often) << boost::format(
        "WBX tune: target frequency %f Mhz"
    ) % (target_freq/1e6) << std::endl;

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


    UHD_LOGV(often)
        << boost::format("WBX Intermediates: ref=%0.2f, outdiv=%f, fbdiv=%f") % (ref_freq*(1+int(D))/(R*(1+int(T)))) % double(RFdiv*2) % double(N + double(FRAC)/double(MOD)) << std::endl

        << boost::format("WBX tune: R=%d, BS=%d, N=%d, FRAC=%d, MOD=%d, T=%d, D=%d, RFdiv=%d, LD=%d"
            ) % R % BS % N % FRAC % MOD % T % D % RFdiv % get_locked(unit)<< std::endl
        << boost::format("WBX Frequencies (MHz): REQ=%0.2f, ACT=%0.2f, VCO=%0.2f, PFD=%0.2f, BAND=%0.2f"
            ) % (target_freq/1e6) % (actual_freq/1e6) % (vco_freq/1e6) % (pfd_freq/1e6) % (pfd_freq/BS/1e6) << std::endl;

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
    UHD_ASSERT_THROW(rfdivsel_to_enum.has_key(RFdiv));
    regs.rf_divider_select = rfdivsel_to_enum[RFdiv];

    if (unit == dboard_iface::UNIT_RX) {
        freq_range_t rx_lo_5dbm = list_of
            (range_t(0.05e9, 1.4e9))
        ;

        freq_range_t rx_lo_2dbm = list_of
            (range_t(1.4e9, 2.2e9))
        ;

        if (actual_freq == rx_lo_5dbm.clip(actual_freq)) regs.output_power = adf4350_regs_t::OUTPUT_POWER_5DBM;

        if (actual_freq == rx_lo_2dbm.clip(actual_freq)) regs.output_power = adf4350_regs_t::OUTPUT_POWER_2DBM;

    } else if (unit == dboard_iface::UNIT_TX) {
        freq_range_t tx_lo_5dbm = list_of
            (range_t(0.05e9, 1.7e9))
            (range_t(1.9e9, 2.2e9))
        ;

        freq_range_t tx_lo_m1dbm = list_of
            (range_t(1.7e9, 1.9e9))
        ;

        if (actual_freq == tx_lo_5dbm.clip(actual_freq)) regs.output_power = adf4350_regs_t::OUTPUT_POWER_5DBM;

        if (actual_freq == tx_lo_m1dbm.clip(actual_freq)) regs.output_power = adf4350_regs_t::OUTPUT_POWER_M1DBM;

    }

    //write the registers
    //correct power-up sequence to write registers (5, 4, 3, 2, 1, 0)
    int addr;

    for(addr=5; addr>=0; addr--){
        UHD_LOGV(often) << boost::format(
            "WBX SPI Reg (0x%02x): 0x%08x"
        ) % addr % regs.get_reg(addr) << std::endl;
        this->get_iface()->write_spi(
            unit, spi_config_t::EDGE_RISE,
            regs.get_reg(addr), 32
        );
    }

    //return the actual frequency
    UHD_LOGV(often) << boost::format(
        "WBX tune: actual frequency %f Mhz"
    ) % (actual_freq/1e6) << std::endl;
    return actual_freq;
}

bool wbx_base::get_locked(dboard_iface::unit_t unit){
    return (this->get_iface()->read_gpio(unit) & LOCKDET_MASK) != 0;
}

bool wbx_base::is_v3(void){
    return get_rx_id() == 0x057;
}

/***********************************************************************
 * RX Get and Set
 **********************************************************************/
void wbx_base::rx_get(const wax::obj &key_, wax::obj &val){
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
        assert_has(_rx_gains.keys(), key.name, "wbx rx gain name");
        val = _rx_gains[key.name];
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        assert_has(wbx_rx_gain_ranges.keys(), key.name, "wbx rx gain name");
        val = wbx_rx_gain_ranges[key.name];
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(wbx_rx_gain_ranges.keys());
        return;

    case SUBDEV_PROP_FREQ:
        val = 0.0;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = freq_range_t(0.0, 0.0, 0.0);;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = prop_names_t(1, "");
        return;

    case SUBDEV_PROP_CONNECTION:
        val = SUBDEV_CONN_COMPLEX_IQ;
        return;

    case SUBDEV_PROP_ENABLED:
        val = _rx_enabled;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
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

void wbx_base::rx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_GAIN:
        this->set_rx_gain(val.as<double>(), key.name);
        return;

    case SUBDEV_PROP_ENABLED:
        _rx_enabled = val.as<bool>();
        this->set_rx_enabled(_rx_enabled);
        return;

    case SUBDEV_PROP_BANDWIDTH:
        UHD_MSG(warning) << "WBX: No tunable bandwidth, fixed filtered to 40MHz";
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * TX Get and Set
 **********************************************************************/
void wbx_base::tx_get(const wax::obj &key_, wax::obj &val){
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
        assert_has(_tx_gains.keys(), key.name, "wbx tx gain name");
        val = _tx_gains[key.name];
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        assert_has(wbx_tx_gain_ranges.keys(), key.name, "wbx tx gain name");
        val = wbx_tx_gain_ranges[key.name];
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(wbx_tx_gain_ranges.keys());
        return;

    case SUBDEV_PROP_FREQ:
        val = 0.0;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = freq_range_t(0.0, 0.0, 0.0);
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = prop_names_t(1, "");
        return;

    case SUBDEV_PROP_CONNECTION:
        val = SUBDEV_CONN_COMPLEX_IQ;
        return;

    case SUBDEV_PROP_ENABLED:
        val = _tx_enabled;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
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

void wbx_base::tx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_GAIN:
        this->set_tx_gain(val.as<double>(), key.name);
        return;

    case SUBDEV_PROP_ENABLED:
        _tx_enabled = val.as<bool>();
        this->set_tx_enabled(_tx_enabled);
        return;

    case SUBDEV_PROP_BANDWIDTH:
        UHD_MSG(warning) << "WBX: No tunable bandwidth, fixed filtered to 40MHz";
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
