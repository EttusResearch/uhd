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
    ("RF", gain_range_t(0, 60, float(60.0/4096))) //both gains are analog and controlled by DAC
    ("IF", gain_range_t(0, 35, float(35.0/4096))) //the old driver used 1dB for both; i don't think that's right?
;

static const uhd::dict<std::string, freq_range_t> tvrx_freq_ranges = map_list_of
    ("VHFLO", freq_range_t(50e6, 158e6)) //this isn't used outside this driver. just for us.
    ("VHFHI", freq_range_t(158e6, 454e6))
    ("UHF"  , freq_range_t(454e6, 860e6))
;

//we might need to spec the various bands for band selection.

static const double tvrx_lo_freq = 36e6; //LO freq of TVRX module

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
    dtt75403_regs_t _dtt75403_regs;
    boost::uint8_t _dtt75403_addr(void){
        return (this->get_iface()->get_special_props().mangle_i2c_addrs)? 0x60 : 0x61; //ok really? we could rename that call
    };

    void set_gain(float gain, const std::string &name);

    void send_reg(boost::uint8_t start_reg, boost::uint8_t stop_reg){
        //why is this an inline anyway? bring this out
        start_reg = boost::uint8_t(std::clip(int(start_reg), 0x0, 0x5));
        stop_reg = boost::uint8_t(std::clip(int(stop_reg), 0x0, 0x5));

        for(boost::uint8_t start_addr=start_reg; start_addr <= stop_reg; start_addr += sizeof(boost::uint32_t) - 1){
            int num_bytes = int(stop_reg - start_addr + 1) > int(sizeof(boost::uint32_t)) - 1 ? sizeof(boost::uint32_t) - 1 : stop_reg - start_addr + 1;

            //create buffer for register data (+1 for start address)
            byte_vector_t regs_vector(num_bytes + 1);

            //first byte is the address of first register
            regs_vector[0] = start_addr;

            //get the register data
            for(int i=0; i<num_bytes; i++){
                regs_vector[1+i] = _max2118_write_regs.get_reg(start_addr+i);
                if(tvrx_debug) std::cerr << boost::format(
                    "tvrx: send reg 0x%02x, value 0x%04x, start_addr = 0x%04x, num_bytes %d"
                ) % int(start_addr+i) % int(regs_vector[1+i]) % int(start_addr) % num_bytes << std::endl;
            }

            //send the data
            this->get_iface()->write_i2c(
                _max2118_addr(), regs_vector
            );
        }
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
    //enable only the clocks we need
    
    //TODO: YOU GOT SOME CLOCK SETUP TO DO HERE, DOGGGG
    
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

/***********************************************************************
 * Gain Handling
 **********************************************************************/
/*!
 * Convert a requested gain for the GC2 vga into the integer register value.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return 5 bit the register value
 */

//yo this should look the same as the below one for the IF, dogg
static int gain_to_gc2_vga_reg(float &gain){
    int reg = 0;
    gain = std::clip<float>(float(boost::math::iround(gain)), tvrx_gain_ranges["GC2"].min, tvrx_gain_ranges["GC2"].max);

    // Half dB steps from 0-5dB, 1dB steps from 5-24dB
    if (gain < 5) {
        reg = boost::math::iround(31.0 - gain/0.5);
        gain = float(boost::math::iround(gain) * 0.5);
    } else {
        reg = boost::math::iround(22.0 - (gain - 4.0));
        gain = float(boost::math::iround(gain));
    }

    if (tvrx_debug) std::cerr << boost::format(
        "tvrx GC2 Gain: %f dB, reg: %d"
    ) % gain % reg << std::endl;

    return reg;
}

/*!
 * Convert a requested gain for the RF gain into the dac_volts value.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return dac voltage value
 */

//yo what about the 1.22 opamp gain of the USRP1 -- is this replicated on U2?
static float gain_to_rfgain_dac(float &gain){
    //clip the input
    gain = std::clip<float>(gain, tvrx_gain_ranges["RF"].min, tvrx_gain_ranges["RF"].max);

    //voltage level constants
    static const float max_volts = float(0), min_volts = float(3.3);
    static const float slope = (max_volts-min_volts)/tvrx_gain_ranges["RF"].max;

    //calculate the voltage for the aux dac
    float dac_volts = gain*slope + min_volts;

    if (tvrx_debug) std::cerr << boost::format(
        "tvrx RFAGC Gain: %f dB, dac_volts: %f V"
    ) % gain % dac_volts << std::endl;

    //the actual gain setting
    gain = (dac_volts - min_volts)/slope;

    return dac_volts;
}

void tvrx::set_gain(float gain, const std::string &name){
    assert_has(tvrx_gain_ranges.keys(), name, "tvrx gain name");
    if (name == "RF"){
//should look the same as below
    }
    else if(name == "IF"){
        //write the new voltage to the aux dac                                CHECK THIS
        this->get_iface()->write_aux_dac(dboard_iface::UNIT_RX, dboard_iface::AUX_DAC_A, gain_to_gc1_rfvga_dac(gain));
    }
    else UHD_THROW_INVALID_CODE_PATH();
    _gains[name] = gain;
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

//for smaaht kids to use to set advanced props
void tvrx::rx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){
    case SUBDEV_PROP_GAIN:
        this->set_gain(val.as<float>(), key.name);
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

