//
// Copyright 2010-2012 Ettus Research LLC
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

#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/algorithm.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/sensors.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/array.hpp>
#include <boost/math/special_functions/round.hpp>
#include <utility>
#include <cmath>
#include <cfloat>
#include <limits>
#include <tuner_4937di5_regs.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The tvrx constants
 **********************************************************************/
static const freq_range_t tvrx_freq_range(50e6, 860e6);

static const std::vector<std::string> tvrx_antennas = list_of("RX");

static const uhd::dict<std::string, freq_range_t> tvrx_freq_ranges = map_list_of
    ("VHFLO", freq_range_t(50e6, 158e6))
    ("VHFHI", freq_range_t(158e6, 454e6))
    ("UHF"  , freq_range_t(454e6, 860e6))
;

static const boost::array<double, 17> vhflo_gains_db =
    {{-6.00000, -6.00000, -6.00000, -4.00000, 0.00000,
     5.00000, 10.00000, 17.40000, 26.30000, 36.00000,
     43.00000, 48.00000, 49.50000, 50.10000, 50.30000,
     50.30000, 50.30000}};

static const boost::array<double, 17> vhfhi_gains_db =
    {{-13.3000,  -13.3000,  -13.3000,   -1.0000,    7.7000,
    11.0000,   14.7000,   19.3000,   26.1000,   36.0000,
    42.7000,   46.0000,   47.0000,   47.8000,   48.2000,
    48.2000,   48.2000}};

static const boost::array<double, 17> uhf_gains_db =
    {{-8.0000,   -8.0000,   -7.0000,    4.0000,   10.2000,
     14.5000,   17.5000,   20.0000,   24.5000,   30.8000,
     37.0000,   39.8000,   40.7000,   41.6000,   42.6000,
     43.2000,   43.8000}};

static const boost::array<double, 17> tvrx_if_gains_db =
    {{-1.50000,   -1.50000,   -1.50000,   -1.00000,    0.20000,
     2.10000,    4.30000,    6.40000,    9.00000,   12.00000,
     14.80000,   18.20000,   26.10000,   32.50000,  32.50000,
     32.50000,   32.50000}};

//gain linearization data
//this is from the datasheet and is dB vs. volts (below)
//i tried to curve fit this, but it's really just so nonlinear that you'd
//need dang near as many coefficients as to just map it like this and interp.
//these numbers are culled from the 4937DI5 datasheet and are probably totally inaccurate
//but if it's better than the old linear fit i'm happy
static const uhd::dict<std::string, boost::array<double, 17> > tvrx_rf_gains_db = map_list_of
    ("VHFLO", vhflo_gains_db)
    ("VHFHI", vhfhi_gains_db)
    ("UHF"  , uhf_gains_db)
;

//sample voltages for the above points
static const boost::array<double, 17> tvrx_gains_volts =
    {{0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4, 2.6, 2.8, 3.0, 3.2, 3.4, 3.6, 3.8, 4.0}};

static uhd::dict<std::string, gain_range_t> get_tvrx_gain_ranges(void) {
    double rfmax = 0.0, rfmin = FLT_MAX;
    BOOST_FOREACH(const std::string range, tvrx_rf_gains_db.keys()) {
        double my_max = tvrx_rf_gains_db[range].back(); //we're assuming it's monotonic
        double my_min = tvrx_rf_gains_db[range].front(); //if it's not this is wrong wrong wrong
        if(my_max > rfmax) rfmax = my_max;
        if(my_min < rfmin) rfmin = my_min;
    }

    double ifmin = tvrx_if_gains_db.front();
    double ifmax = tvrx_if_gains_db.back();

    return map_list_of
        ("RF", gain_range_t(rfmin, rfmax, (rfmax-rfmin)/4096.0))
        ("IF", gain_range_t(ifmin, ifmax, (ifmax-ifmin)/4096.0))
    ;
}

static const double opamp_gain = 1.22; //onboard DAC opamp gain
static const double tvrx_if_freq = 43.75e6; //IF freq of TVRX module
static const boost::uint16_t reference_divider = 640; //clock reference divider to use
static const double reference_freq = 4.0e6;

/***********************************************************************
 * The tvrx dboard class
 **********************************************************************/
class tvrx : public rx_dboard_base{
public:
    tvrx(ctor_args_t args);
    ~tvrx(void);

private:
    uhd::dict<std::string, double> _gains;
    double _lo_freq;
    tuner_4937di5_regs_t _tuner_4937di5_regs;
    boost::uint8_t _tuner_4937di5_addr(void){
        return (this->get_iface()->get_special_props().mangle_i2c_addrs)? 0x61 : 0x60; //ok really? we could rename that call
    };

    double set_gain(double gain, const std::string &name);
    double set_freq(double freq);

    void update_regs(void){
        byte_vector_t regs_vector(4);

        //get the register data
        for(int i=0; i<4; i++){
            regs_vector[i] = _tuner_4937di5_regs.get_reg(i);
            UHD_LOGV(often) << boost::format(
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
    dboard_manager::register_dboard(0x0040, &make_tvrx, "TVRX");
}

/***********************************************************************
 * Structors
 **********************************************************************/
tvrx::tvrx(ctor_args_t args) : rx_dboard_base(args){
    ////////////////////////////////////////////////////////////////////
    // Register properties
    ////////////////////////////////////////////////////////////////////
    this->get_rx_subtree()->create<std::string>("name")
        .set("TVRX");
    this->get_rx_subtree()->create<int>("sensors"); //phony property so this dir exists
    BOOST_FOREACH(const std::string &name, get_tvrx_gain_ranges().keys()){
        this->get_rx_subtree()->create<double>("gains/"+name+"/value")
            .coerce(boost::bind(&tvrx::set_gain, this, _1, name));
        this->get_rx_subtree()->create<meta_range_t>("gains/"+name+"/range")
            .set(get_tvrx_gain_ranges()[name]);
    }
    this->get_rx_subtree()->create<double>("freq/value")
        .coerce(boost::bind(&tvrx::set_freq, this, _1));
    this->get_rx_subtree()->create<meta_range_t>("freq/range")
        .set(tvrx_freq_range);
    this->get_rx_subtree()->create<std::string>("antenna/value")
        .set(tvrx_antennas.at(0));
    this->get_rx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(tvrx_antennas);
    this->get_rx_subtree()->create<std::string>("connection")
        .set("I");
    this->get_rx_subtree()->create<bool>("enabled")
        .set(true); //always enabled
    this->get_rx_subtree()->create<bool>("use_lo_offset")
        .set(false);
    this->get_rx_subtree()->create<double>("bandwidth/value")
        .set(6.0e6);
    this->get_rx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(6.0e6, 6.0e6));

    //enable only the clocks we need
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);

    //set the gpio directions and atr controls (identically)
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, 0x0); // All unused in atr
    if (this->get_iface()->get_special_props().soft_clock_divider){
        this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, 0x1); // GPIO0 is clock
    }
    else{
        this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, 0x0); // All Inputs
    }

    //send initial register settings if necessary

    //set default freq
    _lo_freq = tvrx_freq_range.start() + tvrx_if_freq; //init _lo_freq to a sane default
    this->get_rx_subtree()->access<double>("freq/value").set(tvrx_freq_range.start());

    //set default gains
    BOOST_FOREACH(const std::string &name, get_tvrx_gain_ranges().keys()){
        this->get_rx_subtree()->access<double>("gains/"+name+"/value")
            .set(get_tvrx_gain_ranges()[name].start());
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
        if(freq >= tvrx_freq_ranges[band].start() && freq <= tvrx_freq_ranges[band].stop()){
            UHD_LOGV(often) << "Band: " << band << std::endl;
            return band;
        }
    }
    UHD_THROW_INVALID_CODE_PATH();
}

/***********************************************************************
 * Gain Handling
 **********************************************************************/
/*!
 * Execute a linear interpolation to find the voltage corresponding to a desired gain
 * \param gain the desired gain in dB
 * \param db_vector the vector of dB readings
 * \param volts_vector the corresponding vector of voltages db_vector was sampled at
 * \return a voltage to feed the TVRX analog gain
 */

static double gain_interp(double gain, boost::array<double, 17> db_vector, boost::array<double, 17> volts_vector) {
    double volts;
    gain = uhd::clip<double>(gain, db_vector.front(), db_vector.back()); //let's not get carried away here

    boost::uint8_t gain_step = 0;
    //find which bin we're in
    for(size_t i = 0; i < db_vector.size()-1; i++) {
        if(gain >= db_vector[i] && gain <= db_vector[i+1]) gain_step = i;
    }

    //find the current slope for linear interpolation
    double slope = (volts_vector[gain_step + 1] - volts_vector[gain_step])
                / (db_vector[gain_step + 1] - db_vector[gain_step]);

    //the problem here is that for gains approaching the maximum, the voltage slope becomes infinite
    //i.e., a small change in gain requires an infinite change in voltage
    //to cope, we limit the slope

    if(slope == std::numeric_limits<double>::infinity())
        return volts_vector[gain_step];

    //use the volts per dB slope to find the final interpolated voltage
    volts = volts_vector[gain_step] + (slope * (gain - db_vector[gain_step]));

    UHD_LOGV(often) << "Gain interp: gain: " << gain << ", gain_step: " << int(gain_step) << ", slope: " << slope << ", volts: " << volts << std::endl;

    return volts;
}

/*!
 * Convert a requested gain for the RF gain into a DAC voltage.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return dac voltage value
 */

static double rf_gain_to_voltage(double gain, double lo_freq){
    //clip the input
    gain = get_tvrx_gain_ranges()["RF"].clip(gain);

    //first we need to find out what band we're in, because gains are different across different bands
    std::string band = get_band(lo_freq - tvrx_if_freq);

    //this is the voltage at the TVRX gain input
    double gain_volts = gain_interp(gain, tvrx_rf_gains_db[band], tvrx_gains_volts);
    //this is the voltage at the USRP DAC output
    double dac_volts = gain_volts / opamp_gain;

    dac_volts = uhd::clip<double>(dac_volts, 0.0, 3.3);

    UHD_LOGV(often) << boost::format(
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

static double if_gain_to_voltage(double gain){
    //clip the input
    gain = get_tvrx_gain_ranges()["IF"].clip(gain);

    double gain_volts = gain_interp(gain, tvrx_if_gains_db, tvrx_gains_volts);
    double dac_volts = gain_volts / opamp_gain;

    dac_volts = uhd::clip<double>(dac_volts, 0.0, 3.3);

    UHD_LOGV(often) << boost::format(
        "tvrx IF AGC gain: %f dB, dac_volts: %f V"
    ) % gain % dac_volts << std::endl;

    return dac_volts;
}

double tvrx::set_gain(double gain, const std::string &name){
    assert_has(get_tvrx_gain_ranges().keys(), name, "tvrx gain name");
    if (name == "RF"){
        this->get_iface()->write_aux_dac(dboard_iface::UNIT_RX, dboard_iface::AUX_DAC_B, rf_gain_to_voltage(gain, _lo_freq));
    }
    else if(name == "IF"){
        this->get_iface()->write_aux_dac(dboard_iface::UNIT_RX, dboard_iface::AUX_DAC_A, if_gain_to_voltage(gain));
    }
    else UHD_THROW_INVALID_CODE_PATH();
    _gains[name] = gain;

    return gain;
}

/*!
 * Set the tuner to center the desired frequency at 43.75MHz
 * \param freq the requested frequency
 */

double tvrx::set_freq(double freq) {
    freq = tvrx_freq_range.clip(freq);
    std::string prev_band = get_band(_lo_freq - tvrx_if_freq);
    std::string new_band = get_band(freq);

    double target_lo_freq = freq + tvrx_if_freq; //the desired LO freq for high-side mixing
    double f_ref = reference_freq / double(reference_divider); //your tuning step size

    int divisor = int((target_lo_freq + (f_ref * 4.0)) / (f_ref * 8)); //the divisor we'll use
    double actual_lo_freq = (f_ref * 8 * divisor); //the LO freq we'll actually get

    if((divisor & ~0x7fff)) UHD_THROW_INVALID_CODE_PATH();

    //now we update the registers
    _tuner_4937di5_regs.db1 = (divisor >> 8) & 0xff;
    _tuner_4937di5_regs.db2 = divisor & 0xff;

    if(new_band == "VHFLO") _tuner_4937di5_regs.bandsel = tuner_4937di5_regs_t::BANDSEL_VHFLO;
    else if(new_band == "VHFHI") _tuner_4937di5_regs.bandsel = tuner_4937di5_regs_t::BANDSEL_VHFHI;
    else if(new_band == "UHF") _tuner_4937di5_regs.bandsel = tuner_4937di5_regs_t::BANDSEL_UHF;
    else UHD_THROW_INVALID_CODE_PATH();

    _tuner_4937di5_regs.power = tuner_4937di5_regs_t::POWER_OFF;
    update_regs();

    //ok don't forget to reset RF gain here if the new band != the old band
    //we do this because the gains are different for different band settings
    //not FAR off, but we do this to be consistent
    if(prev_band != new_band) set_gain(_gains["RF"], "RF");

    UHD_LOGV(often) << boost::format("set_freq: target LO: %f f_ref: %f divisor: %i actual LO: %f") % target_lo_freq % f_ref % divisor % actual_lo_freq << std::endl;

    _lo_freq = actual_lo_freq; //for rx props

    //Check the the IF if larger than the dsp rate and apply a corrective adjustment
    //so that the cordic will be tuned to a possible rate within its range.
    const double codec_rate = this->get_iface()->get_codec_rate(dboard_iface::UNIT_RX);
    if (tvrx_if_freq >= codec_rate/2){
        return _lo_freq - codec_rate;
    }

    return _lo_freq;
}
