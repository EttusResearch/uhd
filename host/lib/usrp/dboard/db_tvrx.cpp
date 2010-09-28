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

// No RX IO Pins Used

// RX IO Functions

//ADC/DAC functions:
//DAC 1: RF AGC
//DAC 2: IF AGC

//min freq: 50e6
//max freq: 860e6
//gain range: [0:1dB:115dB]

#include <uhd/utils/static.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/warning.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/usrp/subdev_props.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/math/special_functions/round.hpp>
#include <utility>
#include <cmath>
#include <tuner_4937di5_regs.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The tvrx constants
 **********************************************************************/
static const bool tvrx_debug = true;

static const freq_range_t tvrx_freq_range(50e6, 860e6);

static const prop_names_t tvrx_antennas = ""; //only got one

static const uhd::dict<std::string, gain_range_t> tvrx_gain_ranges = map_list_of
    ("RF", gain_range_t(-13.3, 50.3, float(50.3/4096))) //both gains are analog and controlled by DAC
    ("IF", gain_range_t(-1.5, 32.5, float(32.5/4096))) //the old driver used 1dB for both; i don't think that's right?
;

static const uhd::dict<std::string, freq_range_t> tvrx_freq_ranges = map_list_of
    ("VHFLO", freq_range_t(50e6, 158e6))
    ("VHFHI", freq_range_t(158e6, 454e6))
    ("UHF"  , freq_range_t(454e6, 860e6))
;

//gain linearization data
//this is from the datasheet and is dB vs. volts (below)
//i tried to curve fit this, but it's really just so nonlinear that you'd
//need dang near as many coefficients as to just map it like this and interp.
//these numbers are culled from the 4937DI5 datasheet and are probably totally inaccurate
//but if it's better than the old linear fit i'm happy
static const uhd::dict<std::string, std::vector<float> > tvrx_rf_gains_db = map_list_of
    ("VHFLO", std::vector<float>(-6.00000, -6.00000, -6.00000, -4.00000, 0.00000,
                                 5.00000, 10.00000, 17.40000, 26.30000, 36.00000,
                                 43.00000, 48.00000, 49.50000, 50.10000, 50.30000,
                                 50.30000, 50.30000))
    ("VHFHI", std::vector<float>(-13.3000,  -13.3000,  -13.3000,   -1.0000,    7.7000,
                                 11.0000,   14.7000,   19.3000,   26.1000,   36.0000,
                                 42.7000,   46.0000,   47.0000,   47.8000,   48.2000,
                                 48.2000,   48.2000))
    ("UHF"  , std::vector<float>(-8.0000,   -8.0000,   -7.0000,    4.0000,   10.2000,
                                 14.5000,   17.5000,   20.0000,   24.5000,   30.8000,
                                 37.0000,   39.8000,   40.7000,   41.6000,   42.6000,
                                 43.2000,   43.8000))
;

static const std::vector<float> tvrx_if_gains_db = 
              std::vector<float>(-1.50000,   -1.50000,   -1.50000,   -1.00000,    0.20000,
                                 2.10000,    4.30000,    6.40000,    9.00000,   12.00000,
                                 14.80000,   18.20000,   26.10000,   32.50000,  32.50000,
                                 32.50000,   32.50000)
;

//sample voltages for the above points
static const std::vector<float> tvrx_rf_gains_volts = 
              std::vector<float>(0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4, 2.6, 2.8, 3.0, 3.2, 3.4, 3.6, 3.8, 4.0);


/*!
 * Execute a linear interpolation to find the voltage corresponding to a desired gain
 * \param gain the desired gain in dB
 * \param db_vector the vector of dB readings
 * \param volts_vector the corresponding vector of voltages db_vector was sampled at
 * \return a voltage to feed the TVRX analog gain
 */

static float gain_interp(float gain, std::vector<float> db_vector, std::vector<float> volts_vector) {
    float volts;
    gain = std::clip<float>(gain, db_vector.front(), db_vector.back()); //let's not get carried away here
    
    boost::uint8_t gain_step = 0;
    for(int i = 0; i < db_vector.size()-1; i++) {
        if(gain > db_vector[i] && gain < db_vector.[i+1]) gain_step = i;
    }
        
    
    return volts;
}

static const double opamp_gain = 1.22; //onboard DAC opamp gain
static const double tvrx_lo_freq = 43.75e6; //LO freq of TVRX module
static const boost::uint16_t reference_divider = 640; //clock reference divider to use

/***********************************************************************
 * The tvrx dboard class
 **********************************************************************/
class tvrx : public rx_dboard_base{
public:
    tvrx(ctor_args_t args);
    ~tvrx(void);
    
    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);

private:
    uhd::dict<std::string, float> _gains;
    tuner_4937di5_regs_t _tuner_4937di5_regs;
    boost::uint8_t _tuner_4937di5_addr(void){
        return (this->get_iface()->get_special_props().mangle_i2c_addrs)? 0x60 : 0x61; //ok really? we could rename that call
    };

    void set_gain(float gain, const std::string &name);

    void update_regs(void){
        byte_vector_t regs_vector(4);

        //get the register data
        for(int i=0; i<4; i++){
            regs_vector[i] = _tuner_4937di5_regs.get_reg(i);
            if(tvrx_debug) std::cerr << boost::format(
                "tvrx: send reg 0x%02x, value 0x%04x"
            ) % int(i) % int(regs_vector[i]) << std::endl;
        }

        //send the data
        this->get_iface()->write_i2c(
            _tuner_4937di5_addr(), regs_vector
        );
    }

};

/***********************************************************************
 * Register the tvrx dboard
 **********************************************************************/
static dboard_base::sptr make_tvrx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new tvrx(args));
}

UHD_STATIC_BLOCK(reg_tvrx_dboard){
    //register the factory function for the rx dbid
    dboard_manager::register_dboard(0x0003, &make_tvrx, "tvrx");
}

/***********************************************************************
 * Structors
 **********************************************************************/
tvrx::tvrx(ctor_args_t args) : rx_dboard_base(args){
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);

    //set the gpio directions and atr controls (identically)
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, 0x0); // All unused in atr
    if (this->get_iface()->get_special_props().soft_clock_divider){
        this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, 0x1); // GPIO0 is clock
    }
    else{
        this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, 0x0); // All Inputs
    }

    //send initial register settings
    //HAAAAY GUYYYYYSSS
    
    //set default gains
    BOOST_FOREACH(const std::string &name, tvrx_gain_ranges.keys()){
        set_gain(tvrx_gain_ranges[name].min, name);
    }
}

tvrx::~tvrx(void){
}

/*! Return a string corresponding to the relevant band
 * \param freq the frequency of interest
 * \return a string corresponding to the band
 */

static std::string get_band(double freq) {
    BOOST_FOREACH(const std::string &band, tvrx_freq_ranges.keys()) {
        if(freq >= tvrx_freq_ranges[band].min && freq <= tvrx_freq_ranges[band].max)
            return name;
    }
    UHD_THROW_INVALID_CODE_PATH();
}

/***********************************************************************
 * Gain Handling
 **********************************************************************/
/* 
 * Gain accuracy on this thing is basically a crap shoot. In other words, there is none.
 * The old driver basically picked a linear approximation of the median gain, but the actual
 * gain slope is heavily nonlinear and varies both with gain and with frequency.
 * It also probably varies markedly over temperature, but there's only so much we can do.
 * The best approximation to the RF gain data is fifth-order. If we ignore the curve portions
 * below 0dB (yes, it's a pad below a certain gain value) and if we ignore the curve portions
 * near the max-gain asymptotes, it looks pretty close to third-order. This is less insane to
 * approximate.
 */

/*!
 * Convert a requested gain for the RF gain into a DAC voltage.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return dac voltage value
 */

static float rf_gain_to_voltage(float &gain){
    //clip the input
    gain = std::clip<float>(gain, tvrx_gain_ranges["RF"].min, tvrx_gain_ranges["RF"].max);

    //voltage level constants
    static const float max_volts = float(4.0), min_volts = float(0.6);
    static const float slope = (max_volts-min_volts)/tvrx_gain_ranges["RF"].max;

    //this is the voltage at the TVRX gain input
    float gain_volts = gain*slope + min_volts
    //this is the voltage at the USRP DAC output
    float dac_volts = gain_volts / opamp_gain;

    //the actual gain setting
    gain = (dac_volts - min_volts)/slope;

    if (tvrx_debug) std::cerr << boost::format(
        "tvrx RF AGC gain: %f dB, dac_volts: %f V"
    ) % gain % dac_volts << std::endl;

    return dac_volts;
}

/*!
 * Convert a requested gain for the IF gain into a DAC voltage.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return dac voltage value
 */

static float if_gain_to_voltage(float &gain){
    //clip the input
    gain = std::clip<float>(gain, tvrx_gain_ranges["IF"].min, tvrx_gain_ranges["IF"].max);

    //voltage level constants
    static const float max_volts = float(4.0), min_volts = float(0.0);
    static const float slope = (max_volts-min_volts)/tvrx_gain_ranges["IF"].max;

    //this is the voltage at the TVRX gain input
    float gain_volts = gain*slope + 1.25;
    //this is the voltage at the USRP DAC output
    float dac_volts = gain_volts / opamp_gain;

    //the actual gain setting
    gain = (dac_volts - min_volts)/slope;

    if (tvrx_debug) std::cerr << boost::format(
        "tvrx IF AGC gain: %f dB, dac_volts: %f V"
    ) % gain % dac_volts << std::endl;

    return dac_volts;
}

void tvrx::set_gain(float gain, const std::string &name){
    assert_has(tvrx_gain_ranges.keys(), name, "tvrx gain name");
    if (name == "RF"){
        this->get_iface()->write_aux_dac(dboard_iface::UNIT_RX, dboard_iface::AUX_DAC_A, rf_gain_to_voltage(gain));
    }
    else if(name == "IF"){
        this->get_iface()->write_aux_dac(dboard_iface::UNIT_RX, dboard_iface::AUX_DAC_B, if_gain_to_voltage(gain));
    }
    else UHD_THROW_INVALID_CODE_PATH();
    _gains[name] = gain;
}

/*!
 * Set the tuner to center the desired frequency at 43.75MHz
 * \param freq the requested frequency
 */

void tvrx::set_freq(double freq) {
    
    
    
}

/***********************************************************************
 * RX Get and Set
 **********************************************************************/
void tvrx::rx_get(const wax::obj &key_, wax::obj &val){
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
        assert_has(_gains.keys(), key.name, "tvrx gain name");
        val = _gains[key.name];
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        assert_has(tvrx_gain_ranges.keys(), key.name, "tvrx gain name");
        val = tvrx_gain_ranges[key.name];
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(tvrx_gain_ranges.keys());
        return;

    case SUBDEV_PROP_FREQ:
        val = tvrx_lo_freq;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = tvrx_freq_range;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string(tvrx_antennas);
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = tvrx_antennas;
        return;

    case SUBDEV_PROP_CONNECTION:
        val = SUBDEV_CONN_COMPLEX_IQ;
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
        return;

    case SUBDEV_PROP_LO_LOCKED:
        val = true;
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void tvrx::rx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_GAIN:
        this->set_gain(val.as<float>(), key.name);
        return;
    case SUBDEV_PROP_FREQ:
        this->set_freq(val.as<double>());
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

