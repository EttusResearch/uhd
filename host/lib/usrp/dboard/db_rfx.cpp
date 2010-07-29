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

// IO Pin functions
#define POWER_IO     (1 << 7)   // Low enables power supply
#define ANTSW_IO     (1 << 6)   // On TX DB, 0 = TX, 1 = RX, on RX DB 0 = main ant, 1 = RX2
#define MIXER_IO     (1 << 5)   // Enable appropriate mixer
#define LOCKDET_MASK (1 << 2)   // Input pin

// Mixer constants
#define MIXER_ENB    MIXER_IO
#define MIXER_DIS    0

// Power constants
#define POWER_UP     0
#define POWER_DOWN   POWER_IO

// Antenna constants
#define ANT_TX       0          //the tx line is transmitting
#define ANT_RX       ANTSW_IO   //the tx line is receiving
#define ANT_TXRX     0          //the rx line is on txrx
#define ANT_RX2      ANTSW_IO   //the rx line in on rx2
#define ANT_XX       0          //dont care how the antenna is set

#include "adf4360_regs.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/math/special_functions/round.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The RFX Series constants
 **********************************************************************/
static const bool rfx_debug = false;

static const prop_names_t rfx_tx_antennas = list_of("TX/RX");

static const prop_names_t rfx_rx_antennas = list_of("TX/RX")("RX2");

static const uhd::dict<std::string, gain_range_t> rfx_tx_gain_ranges; //empty

static const uhd::dict<std::string, gain_range_t> rfx_rx_gain_ranges = map_list_of
    ("PGA0", gain_range_t(0, 70, float(0.022)))
;

static const uhd::dict<std::string, gain_range_t> rfx400_rx_gain_ranges = map_list_of
    ("PGA0", gain_range_t(0, 45, float(0.022)))
;

/***********************************************************************
 * The RFX series of dboards
 **********************************************************************/
class rfx_xcvr : public xcvr_dboard_base{
public:
    rfx_xcvr(
        ctor_args_t args,
        const freq_range_t &freq_range,
        bool rx_div2, bool tx_div2
    );
    ~rfx_xcvr(void);

    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);

    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);

private:
    freq_range_t _freq_range;
    uhd::dict<std::string, gain_range_t> _rx_gain_ranges;
    uhd::dict<dboard_iface::unit_t, bool> _div2;
    double       _rx_lo_freq, _tx_lo_freq;
    std::string  _rx_ant;
    uhd::dict<std::string, float> _rx_gains;

    void set_rx_lo_freq(double freq);
    void set_tx_lo_freq(double freq);
    void set_rx_ant(const std::string &ant);
    void set_tx_ant(const std::string &ant);
    void set_rx_gain(float gain, const std::string &name);
    void set_tx_gain(float gain, const std::string &name);

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
 * Register the RFX dboards (min freq, max freq, rx div2, tx div2)
 **********************************************************************/
static dboard_base::sptr make_rfx_flex400(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new rfx_xcvr(args, freq_range_t(400e6, 500e6), false, true));
}

static dboard_base::sptr make_rfx_flex900(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new rfx_xcvr(args, freq_range_t(750e6, 1050e6), true, true));
}

static dboard_base::sptr make_rfx_flex1800(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new rfx_xcvr(args, freq_range_t(1500e6, 2100e6), false, false));
}

static dboard_base::sptr make_rfx_flex1200(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new rfx_xcvr(args, freq_range_t(1150e6, 1450e6), true, true));
}

static dboard_base::sptr make_rfx_flex2200(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new rfx_xcvr(args, freq_range_t(2000e6, 2400e6), false, false));
}

static dboard_base::sptr make_rfx_flex2400(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new rfx_xcvr(args, freq_range_t(2300e6, 2900e6), false, false));
}

UHD_STATIC_BLOCK(reg_rfx_dboards){
    dboard_manager::register_dboard(0x0024, 0x0028, &make_rfx_flex400,  "Flex 400 MIMO B");
    dboard_manager::register_dboard(0x0025, 0x0029, &make_rfx_flex900,  "Flex 900 MIMO B");
    dboard_manager::register_dboard(0x0034, 0x0035, &make_rfx_flex1800, "Flex 1800 MIMO B");
    dboard_manager::register_dboard(0x0026, 0x002a, &make_rfx_flex1200, "Flex 1200 MIMO B");
    dboard_manager::register_dboard(0x002c, 0x002d, &make_rfx_flex2200, "Flex 2200 MIMO B");
    dboard_manager::register_dboard(0x0027, 0x002b, &make_rfx_flex2400, "Flex 2400 MIMO B");
}

/***********************************************************************
 * Structors
 **********************************************************************/
rfx_xcvr::rfx_xcvr(
    ctor_args_t args,
    const freq_range_t &freq_range,
    bool rx_div2, bool tx_div2
) : xcvr_dboard_base(args){
    _freq_range = freq_range;
    _div2[dboard_iface::UNIT_RX] = rx_div2;
    _div2[dboard_iface::UNIT_TX] = tx_div2;

    if(this->get_rx_id() == 0x0024) { //RFX400
        _rx_gain_ranges = rfx400_rx_gain_ranges;
    }
    else {
        _rx_gain_ranges = rfx_rx_gain_ranges;
    }


    //enable the clocks that we need
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_TX, true);
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);

    //set the gpio directions and atr controls (identically)
    boost::uint16_t output_enables = POWER_IO | ANTSW_IO | MIXER_IO;
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, output_enables);
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, output_enables);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, output_enables);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, output_enables);

    //setup the tx atr (this does not change with antenna)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_IDLE,        POWER_UP | ANT_XX | MIXER_DIS);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_RX_ONLY,     POWER_UP | ANT_RX | MIXER_DIS);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_TX_ONLY,     POWER_UP | ANT_TX | MIXER_ENB);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_TX, dboard_iface::ATR_REG_FULL_DUPLEX, POWER_UP | ANT_TX | MIXER_ENB);

    //setup the rx atr (this does not change with antenna)
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_IDLE,        POWER_UP | ANT_XX | MIXER_DIS);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_TX_ONLY,     POWER_UP | ANT_XX | MIXER_DIS);
    this->get_iface()->set_atr_reg(dboard_iface::UNIT_RX, dboard_iface::ATR_REG_FULL_DUPLEX, POWER_UP | ANT_RX2| MIXER_ENB);

    //set some default values
    set_rx_lo_freq((_freq_range.min + _freq_range.max)/2.0);
    set_tx_lo_freq((_freq_range.min + _freq_range.max)/2.0);
    set_rx_ant("RX2");

    BOOST_FOREACH(const std::string &name, _rx_gain_ranges.keys()){
        set_rx_gain(_rx_gain_ranges[name].min, name);
    }
}

rfx_xcvr::~rfx_xcvr(void){
    /* NOP */
}

/***********************************************************************
 * Antenna Handling
 **********************************************************************/
void rfx_xcvr::set_rx_ant(const std::string &ant){
    //validate input
    assert_has(rfx_rx_antennas, ant, "rfx rx antenna name");

    //set the rx atr regs that change with antenna setting
    this->get_iface()->set_atr_reg(
        dboard_iface::UNIT_RX, dboard_iface::ATR_REG_RX_ONLY,
        POWER_UP | MIXER_ENB | ((ant == "TX/RX")? ANT_TXRX : ANT_RX2)
    );

    //shadow the setting
    _rx_ant = ant;
}

void rfx_xcvr::set_tx_ant(const std::string &ant){
    assert_has(rfx_tx_antennas, ant, "rfx tx antenna name");
    //only one antenna option, do nothing
}

/***********************************************************************
 * Gain Handling
 **********************************************************************/
static float rx_pga0_gain_to_dac_volts(float &gain){
    //voltage level constants (negative slope)
    static const float max_volts = float(.2), min_volts = float(1.2);
    static const float slope = (max_volts-min_volts)/45;

    //calculate the voltage for the aux dac
    float dac_volts = std::clip<float>(gain*slope + min_volts, max_volts, min_volts);

    //the actual gain setting
    gain = (dac_volts - min_volts)/slope;

    return dac_volts;
}

void rfx_xcvr::set_tx_gain(float, const std::string &name){
    assert_has(rfx_tx_gain_ranges.keys(), name, "rfx tx gain name");
    UHD_THROW_INVALID_CODE_PATH(); //no gains to set
}

void rfx_xcvr::set_rx_gain(float gain, const std::string &name){
    assert_has(_rx_gain_ranges.keys(), name, "rfx rx gain name");
    if(name == "PGA0"){
        float dac_volts = rx_pga0_gain_to_dac_volts(gain);
        _rx_gains[name] = gain;

        //write the new voltage to the aux dac
        this->get_iface()->write_aux_dac(dboard_iface::UNIT_RX, dboard_iface::AUX_DAC_A, dac_volts);
    }
    else UHD_THROW_INVALID_CODE_PATH();
}

/***********************************************************************
 * Tuning
 **********************************************************************/
void rfx_xcvr::set_rx_lo_freq(double freq){
    _rx_lo_freq = set_lo_freq(dboard_iface::UNIT_RX, freq);
}

void rfx_xcvr::set_tx_lo_freq(double freq){
    _tx_lo_freq = set_lo_freq(dboard_iface::UNIT_TX, freq);
}

double rfx_xcvr::set_lo_freq(
    dboard_iface::unit_t unit,
    double target_freq
){
    if (rfx_debug) std::cerr << boost::format(
        "RFX tune: target frequency %f Mhz"
    ) % (target_freq/1e6) << std::endl;

    //clip the input
    target_freq = std::clip(target_freq, _freq_range.min, _freq_range.max);
    if (_div2[unit]) target_freq *= 2;

    //map prescalers to the register enums
    static const uhd::dict<int, adf4360_regs_t::prescaler_value_t> prescaler_to_enum = map_list_of
        (8,  adf4360_regs_t::PRESCALER_VALUE_8_9)
        (16, adf4360_regs_t::PRESCALER_VALUE_16_17)
        (32, adf4360_regs_t::PRESCALER_VALUE_32_33)
    ;

    //map band select clock dividers to enums
    static const uhd::dict<int, adf4360_regs_t::band_select_clock_div_t> bandsel_to_enum = map_list_of
        (1, adf4360_regs_t::BAND_SELECT_CLOCK_DIV_1)
        (2, adf4360_regs_t::BAND_SELECT_CLOCK_DIV_2)
        (4, adf4360_regs_t::BAND_SELECT_CLOCK_DIV_4)
        (8, adf4360_regs_t::BAND_SELECT_CLOCK_DIV_8)
    ;

    double actual_freq=0, ref_freq = this->get_iface()->get_clock_rate(unit);
    int R=0, BS=0, P=0, B=0, A=0;

    /*
     * The goal here to to loop though possible R dividers,
     * band select clock dividers, and prescaler values.
     * Calculate the A and B counters for each set of values.
     * The loop exists when it meets all of the constraints.
     * The resulting loop values are loaded into the registers.
     *
     * fvco = [P*B + A] * fref/R
     * fvco*R/fref = P*B + A = N
     */
    for(R = 2; R <= 32; R+=2){
        BOOST_FOREACH(BS, bandsel_to_enum.keys()){
            if (ref_freq/R/BS > 1e6) continue; //constraint on band select clock
            BOOST_FOREACH(P, prescaler_to_enum.keys()){
                //calculate B and A from N
                double N = target_freq*R/ref_freq;
                B = int(std::floor(N/P));
                A = boost::math::iround(N - P*B);
                if (B < A or B > 8191 or B < 3 or A > 31) continue; //constraints on A, B
                //calculate the actual frequency
                actual_freq = double(P*B + A)*ref_freq/R;
                if (actual_freq/P > 300e6) continue; //constraint on prescaler output
                //constraints met: exit loop
                goto done_loop;
            }
        }
    } done_loop:

    if (rfx_debug) std::cerr << boost::format(
        "RFX tune: R=%d, BS=%d, P=%d, B=%d, A=%d"
    ) % R % BS % P % B % A << std::endl;

    //load the register values
    adf4360_regs_t regs;
    regs.core_power_level        = adf4360_regs_t::CORE_POWER_LEVEL_10MA;
    regs.counter_operation       = adf4360_regs_t::COUNTER_OPERATION_NORMAL;
    regs.muxout_control          = adf4360_regs_t::MUXOUT_CONTROL_DLD;
    regs.phase_detector_polarity = adf4360_regs_t::PHASE_DETECTOR_POLARITY_POS;
    regs.charge_pump_output      = adf4360_regs_t::CHARGE_PUMP_OUTPUT_NORMAL;
    regs.cp_gain_0               = adf4360_regs_t::CP_GAIN_0_SET1;
    regs.mute_till_ld            = adf4360_regs_t::MUTE_TILL_LD_ENB;
    regs.output_power_level      = adf4360_regs_t::OUTPUT_POWER_LEVEL_3_5MA;
    regs.current_setting1        = adf4360_regs_t::CURRENT_SETTING1_0_31MA;
    regs.current_setting2        = adf4360_regs_t::CURRENT_SETTING2_0_31MA;
    regs.power_down              = adf4360_regs_t::POWER_DOWN_NORMAL_OP;
    regs.prescaler_value         = prescaler_to_enum[P];
    regs.a_counter               = A;
    regs.b_counter               = B;
    regs.cp_gain_1               = adf4360_regs_t::CP_GAIN_1_SET1;
    regs.divide_by_2_output      = (_div2[unit])?
                                    adf4360_regs_t::DIVIDE_BY_2_OUTPUT_DIV2 :
                                    adf4360_regs_t::DIVIDE_BY_2_OUTPUT_FUND ;
    regs.divide_by_2_prescaler   = adf4360_regs_t::DIVIDE_BY_2_PRESCALER_FUND;
    regs.r_counter               = R;
    regs.ablpw                   = adf4360_regs_t::ABLPW_3_0NS;
    regs.lock_detect_precision   = adf4360_regs_t::LOCK_DETECT_PRECISION_5CYCLES;
    regs.test_mode_bit           = 0;
    regs.band_select_clock_div   = bandsel_to_enum[BS];

    //write the registers
    std::vector<adf4360_regs_t::addr_t> addrs = list_of //correct power-up sequence to write registers (R, C, N)
        (adf4360_regs_t::ADDR_RCOUNTER)
        (adf4360_regs_t::ADDR_CONTROL)
        (adf4360_regs_t::ADDR_NCOUNTER)
    ;
    BOOST_FOREACH(adf4360_regs_t::addr_t addr, addrs){
        this->get_iface()->write_spi(
            unit, spi_config_t::EDGE_RISE,
            regs.get_reg(addr), 24
        );
    }

    //return the actual frequency
    if (_div2[unit]) actual_freq /= 2;
    if (rfx_debug) std::cerr << boost::format(
        "RFX tune: actual frequency %f Mhz"
    ) % (actual_freq/1e6) << std::endl;
    return actual_freq;
}

/***********************************************************************
 * RX Get and Set
 **********************************************************************/
void rfx_xcvr::rx_get(const wax::obj &key_, wax::obj &val){
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
        assert_has(_rx_gains.keys(), name, "rfx rx gain name");
        val = _rx_gains[name];
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        assert_has(_rx_gain_ranges.keys(), name, "rfx rx gain name");
        val = _rx_gain_ranges[name];
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(_rx_gain_ranges.keys());
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

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = rfx_rx_antennas;
        return;

    case SUBDEV_PROP_CONNECTION:
        val = SUBDEV_CONN_COMPLEX_QI;
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

void rfx_xcvr::rx_set(const wax::obj &key_, const wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_FREQ:
        this->set_rx_lo_freq(val.as<double>());
        return;

    case SUBDEV_PROP_GAIN:
        this->set_rx_gain(val.as<float>(), name);
        return;

    case SUBDEV_PROP_ANTENNA:
        this->set_rx_ant(val.as<std::string>());
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

/***********************************************************************
 * TX Get and Set
 **********************************************************************/
void rfx_xcvr::tx_get(const wax::obj &key_, wax::obj &val){
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
    case SUBDEV_PROP_GAIN_RANGE:
        assert_has(rfx_tx_gain_ranges.keys(), name, "rfx tx gain name");
        //no controllable tx gains, will not get here
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(rfx_tx_gain_ranges.keys());
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
        val = rfx_tx_antennas;
        return;

    case SUBDEV_PROP_CONNECTION:
        val = SUBDEV_CONN_COMPLEX_IQ;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = true;
        return;

    case SUBDEV_PROP_LO_LOCKED:
        val = this->get_locked(dboard_iface::UNIT_TX);
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void rfx_xcvr::tx_set(const wax::obj &key_, const wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_FREQ:
        this->set_tx_lo_freq(val.as<double>());
        return;

    case SUBDEV_PROP_GAIN:
        this->set_tx_gain(val.as<float>(), name);
        return;

    case SUBDEV_PROP_ANTENNA:
        this->set_tx_ant(val.as<std::string>());
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
