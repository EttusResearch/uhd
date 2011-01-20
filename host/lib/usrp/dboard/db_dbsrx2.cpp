//
// Copyright 2010-2011 Ettus Research LLC
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

#include "max2112_regs.hpp"
#include <uhd/utils/static.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/algorithm.hpp>
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

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * The DBSRX2 constants
 **********************************************************************/
static const bool dbsrx2_debug = false;

static const freq_range_t dbsrx2_freq_range(0.8e9, 2.4e9);

static const int dbsrx2_ref_divider = 4; // Hitachi HMC426 divider (U7)

static const prop_names_t dbsrx2_antennas = list_of("J3");

static const uhd::dict<std::string, gain_range_t> dbsrx2_gain_ranges = map_list_of
    ("GC1", gain_range_t(0, 73, 0.05))
    ("BBG", gain_range_t(0, 15, 1))
;

/***********************************************************************
 * The DBSRX2 dboard class
 **********************************************************************/
class dbsrx2 : public rx_dboard_base{
public:
    dbsrx2(ctor_args_t args);
    ~dbsrx2(void);

    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);

private:
    double _lo_freq;
    double _bandwidth;
    uhd::dict<std::string, double> _gains;
    max2112_write_regs_t _max2112_write_regs;
    max2112_read_regs_t _max2112_read_regs;
    boost::uint8_t _max2112_addr(){ //0x60 or 0x61 depending on which side
        return (this->get_iface()->get_special_props().mangle_i2c_addrs)? 0x60 : 0x61;
    }

    void set_lo_freq(double target_freq);
    void set_gain(double gain, const std::string &name);
    void set_bandwidth(double bandwidth);

    void send_reg(boost::uint8_t start_reg, boost::uint8_t stop_reg){
        start_reg = boost::uint8_t(std::clip(int(start_reg), 0x0, 0xB));
        stop_reg = boost::uint8_t(std::clip(int(stop_reg), 0x0, 0xB));

        for(boost::uint8_t start_addr=start_reg; start_addr <= stop_reg; start_addr += sizeof(boost::uint32_t) - 1){
            int num_bytes = int(stop_reg - start_addr + 1) > int(sizeof(boost::uint32_t)) - 1 ? sizeof(boost::uint32_t) - 1 : stop_reg - start_addr + 1;

            //create buffer for register data (+1 for start address)
            byte_vector_t regs_vector(num_bytes + 1);

            //first byte is the address of first register
            regs_vector[0] = start_addr;

            //get the register data
            for(int i=0; i<num_bytes; i++){
                regs_vector[1+i] = _max2112_write_regs.get_reg(start_addr+i);
                if(dbsrx2_debug) std::cerr << boost::format(
                    "DBSRX2: send reg 0x%02x, value 0x%04x, start_addr = 0x%04x, num_bytes %d"
                ) % int(start_addr+i) % int(regs_vector[1+i]) % int(start_addr) % num_bytes << std::endl;
            }

            //send the data
            this->get_iface()->write_i2c(
                _max2112_addr(), regs_vector
            );
        }
    }

    void read_reg(boost::uint8_t start_reg, boost::uint8_t stop_reg){
        static const boost::uint8_t status_addr = 0xC;
        start_reg = boost::uint8_t(std::clip(int(start_reg), 0x0, 0xD));
        stop_reg = boost::uint8_t(std::clip(int(stop_reg), 0x0, 0xD));

        for(boost::uint8_t start_addr=start_reg; start_addr <= stop_reg; start_addr += sizeof(boost::uint32_t)){
            int num_bytes = int(stop_reg - start_addr + 1) > int(sizeof(boost::uint32_t)) ? sizeof(boost::uint32_t) : stop_reg - start_addr + 1;

            //create address to start reading register data
            byte_vector_t address_vector(1);
            address_vector[0] = start_addr;

            //send the address
            this->get_iface()->write_i2c(
                _max2112_addr(), address_vector
            );

            //create buffer for register data
            byte_vector_t regs_vector(num_bytes);

            //read from i2c
            regs_vector = this->get_iface()->read_i2c(
                _max2112_addr(), num_bytes
            );

            for(boost::uint8_t i=0; i < num_bytes; i++){
                if (i + start_addr >= status_addr){
                    _max2112_read_regs.set_reg(i + start_addr, regs_vector[i]);
                    /*
                    if(dbsrx2_debug) std::cerr << boost::format(
                        "DBSRX2: set reg 0x%02x, value 0x%04x"
                    ) % int(i + start_addr) % int(_max2112_read_regs.get_reg(i + start_addr)) << std::endl;
                    */
                }
                if(dbsrx2_debug) std::cerr << boost::format(
                    "DBSRX2: read reg 0x%02x, value 0x%04x, start_addr = 0x%04x, num_bytes %d"
                ) % int(start_addr+i) % int(regs_vector[i]) % int(start_addr) % num_bytes << std::endl;
            }
        }
    }

    /*!
     * Is the LO locked?
     * \return true for locked
     */
    bool get_locked(void){
        read_reg(0xC, 0xD);

        //mask and return lock detect
        bool locked = (_max2112_read_regs.ld & _max2112_read_regs.vasa & _max2112_read_regs.vase) != 0;

        if(dbsrx2_debug) std::cerr << boost::format(
            "DBSRX2 locked: %d"
        ) % locked << std::endl;

        return locked;
    }

};

/***********************************************************************
 * Register the DBSRX2 dboard
 **********************************************************************/
// FIXME 0x67 is the default i2c address on USRP2
//       need to handle which side for USRP1 with different address
static dboard_base::sptr make_dbsrx2(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new dbsrx2(args));
}

UHD_STATIC_BLOCK(reg_dbsrx2_dboard){
    //register the factory function for the rx dbid
    dboard_manager::register_dboard(0x0012, &make_dbsrx2, "DBSRX2");
}

/***********************************************************************
 * Structors
 **********************************************************************/
dbsrx2::dbsrx2(ctor_args_t args) : rx_dboard_base(args){
    //enable only the clocks we need
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);

    //set the gpio directions and atr controls (identically)
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, 0x0); // All unused in atr
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, 0x0); // All Inputs

    //send initial register settings
    send_reg(0x0, 0xB);
    //for (boost::uint8_t addr=0; addr<=12; addr++) this->send_reg(addr, addr);

    //set defaults for LO, gains
    set_lo_freq(dbsrx2_freq_range.start());
    BOOST_FOREACH(const std::string &name, dbsrx2_gain_ranges.keys()){
        set_gain(dbsrx2_gain_ranges[name].start(), name);
    }

    set_bandwidth(40e6); // default bandwidth from datasheet
    get_locked();

    _max2112_write_regs.bbg = boost::math::iround(dbsrx2_gain_ranges["BBG"].start());
    send_reg(0x9, 0x9);
}

dbsrx2::~dbsrx2(void){
}


/***********************************************************************
 * Tuning
 **********************************************************************/
void dbsrx2::set_lo_freq(double target_freq){
    //target_freq = std::clip(target_freq, dbsrx2_freq_range.min, dbsrx2_freq_range.max);

    //variables used in the calculation below
    int scaler = target_freq > 1125e6 ? 2 : 4;
    double ref_freq = this->get_iface()->get_clock_rate(dboard_iface::UNIT_RX);
    int R, intdiv, fracdiv, ext_div;
    double N;

    //compute tuning variables
    ext_div = dbsrx2_ref_divider; // 12MHz < ref_freq/ext_divider < 30MHz

    R = 1; //Divide by 1 is the only tested value

    N = (target_freq*R*ext_div)/(ref_freq); //actual spec range is (19, 251)
    intdiv = int(std::floor(N)); //  if (intdiv < 19  or intdiv > 251) continue;
    fracdiv = boost::math::iround((N - intdiv)*double(1 << 20));

    //calculate the actual freq from the values above
    N = double(intdiv) + double(fracdiv)/double(1 << 20);
    _lo_freq = (N*ref_freq)/(R*ext_div);

    //load new counters into registers
    _max2112_write_regs.set_n_divider(intdiv);
    _max2112_write_regs.set_f_divider(fracdiv);
    _max2112_write_regs.r_divider = R;
    _max2112_write_regs.d24 = scaler == 4 ? max2112_write_regs_t::D24_DIV4 : max2112_write_regs_t::D24_DIV2;

    //debug output of calculated variables
    if (dbsrx2_debug) std::cerr
        << boost::format("DBSRX2 tune:\n")
        << boost::format("    R=%d, N=%f, scaler=%d, ext_div=%d\n") % R % N % scaler % ext_div
        << boost::format("    int=%d, frac=%d, d24=%d\n") % intdiv % fracdiv % int(_max2112_write_regs.d24)
        << boost::format("    Ref    Freq=%fMHz\n") % (ref_freq/1e6)
        << boost::format("    Target Freq=%fMHz\n") % (target_freq/1e6)
        << boost::format("    Actual Freq=%fMHz\n") % (_lo_freq/1e6)
        << std::endl;

    //send the registers
    send_reg(0x0, 0x7);

    //FIXME: probably unnecessary to call get_locked here
    //get_locked();

}

/***********************************************************************
 * Gain Handling
 **********************************************************************/
/*!
 * Convert a requested gain for the BBG vga into the integer register value.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return 4 bit the register value
 */
static int gain_to_bbg_vga_reg(double &gain){
    int reg = boost::math::iround(dbsrx2_gain_ranges["BBG"].clip(gain));

    gain = double(reg);

    if (dbsrx2_debug) std::cerr 
        << boost::format("DBSRX2 BBG Gain:\n")
        << boost::format("    %f dB, bbg: %d") % gain % reg 
        << std::endl;

    return reg;
}

/*!
 * Convert a requested gain for the GC1 rf vga into the dac_volts value.
 * The gain passed into the function will be set to the actual value.
 * \param gain the requested gain in dB
 * \return dac voltage value
 */
static double gain_to_gc1_rfvga_dac(double &gain){
    //clip the input
    gain = dbsrx2_gain_ranges["GC1"].clip(gain);

    //voltage level constants
    static const double max_volts = 0.5, min_volts = 2.7;
    static const double slope = (max_volts-min_volts)/dbsrx2_gain_ranges["GC1"].stop();

    //calculate the voltage for the aux dac
    double dac_volts = gain*slope + min_volts;

    if (dbsrx2_debug) std::cerr 
        << boost::format("DBSRX2 GC1 Gain:\n")
        << boost::format("    %f dB, dac_volts: %f V") % gain % dac_volts 
        << std::endl;

    //the actual gain setting
    gain = (dac_volts - min_volts)/slope;

    return dac_volts;
}

void dbsrx2::set_gain(double gain, const std::string &name){
    assert_has(dbsrx2_gain_ranges.keys(), name, "dbsrx2 gain name");
    if (name == "BBG"){
        _max2112_write_regs.bbg = gain_to_bbg_vga_reg(gain);
        send_reg(0x9, 0x9);
    }
    else if(name == "GC1"){
        //write the new voltage to the aux dac
        this->get_iface()->write_aux_dac(dboard_iface::UNIT_RX, dboard_iface::AUX_DAC_A, gain_to_gc1_rfvga_dac(gain));
    }
    else UHD_THROW_INVALID_CODE_PATH();
    _gains[name] = gain;
}

/***********************************************************************
 * Bandwidth Handling
 **********************************************************************/
void dbsrx2::set_bandwidth(double bandwidth){
    //clip the input
    bandwidth = std::clip<double>(bandwidth, 4e6, 40e6);

    _max2112_write_regs.lp = int((bandwidth/1e6 - 4)/0.29 + 12);
    _bandwidth = double(4 + (_max2112_write_regs.lp - 12) * 0.29)*1e6;

    if (dbsrx2_debug) std::cerr 
        << boost::format("DBSRX2 Bandwidth:\n")
        << boost::format("    %f MHz, lp: %f V") % (_bandwidth/1e6) % int(_max2112_write_regs.lp)
        << std::endl;

    this->send_reg(0x8, 0x8);
}

/***********************************************************************
 * RX Get and Set
 **********************************************************************/
void dbsrx2::rx_get(const wax::obj &key_, wax::obj &val){
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
        assert_has(_gains.keys(), key.name, "dbsrx2 gain name");
        val = _gains[key.name];
        return;

    case SUBDEV_PROP_GAIN_RANGE:
        assert_has(dbsrx2_gain_ranges.keys(), key.name, "dbsrx2 gain name");
        val = dbsrx2_gain_ranges[key.name];
        return;

    case SUBDEV_PROP_GAIN_NAMES:
        val = prop_names_t(dbsrx2_gain_ranges.keys());
        return;

    case SUBDEV_PROP_FREQ:
        val = _lo_freq;
        return;

    case SUBDEV_PROP_FREQ_RANGE:
        val = dbsrx2_freq_range;
        return;

    case SUBDEV_PROP_ANTENNA:
        val = std::string("J3");
        return;

    case SUBDEV_PROP_ANTENNA_NAMES:
        val = dbsrx2_antennas;
        return;

    case SUBDEV_PROP_CONNECTION:
        val = SUBDEV_CONN_COMPLEX_QI;
        return;

    case SUBDEV_PROP_ENABLED:
        val = true; //always enabled
        return;

    case SUBDEV_PROP_USE_LO_OFFSET:
        val = false;
        return;

    case SUBDEV_PROP_LO_LOCKED:
        val = this->get_locked();
        return;

    case SUBDEV_PROP_BANDWIDTH:
        val = _bandwidth;
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

void dbsrx2::rx_set(const wax::obj &key_, const wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);

    //handle the get request conditioned on the key
    switch(key.as<subdev_prop_t>()){

    case SUBDEV_PROP_FREQ:
        this->set_lo_freq(val.as<double>());
        return;

    case SUBDEV_PROP_GAIN:
        this->set_gain(val.as<double>(), key.name);
        return;

    case SUBDEV_PROP_ENABLED:
        return; //always enabled

    case SUBDEV_PROP_BANDWIDTH:
        this->set_bandwidth(val.as<double>());
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}

