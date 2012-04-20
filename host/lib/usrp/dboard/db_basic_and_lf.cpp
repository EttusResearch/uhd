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

#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

//! provider function for the always zero freq
static double always_zero_freq(void){return 0.0;}

/***********************************************************************
 * Constants
 **********************************************************************/
static const uhd::dict<std::string, double> subdev_bandwidth_scalar = map_list_of
    ("A", 1.0)
    ("B", 1.0)
    ("AB", 2.0)
    ("BA", 2.0)
;

/***********************************************************************
 * The basic and lf boards:
 *   They share a common class because only the frequency bounds differ.
 **********************************************************************/
class basic_rx : public rx_dboard_base{
public:
    basic_rx(ctor_args_t args, double max_freq);
    ~basic_rx(void);

private:
    double _max_freq;
};

class basic_tx : public tx_dboard_base{
public:
    basic_tx(ctor_args_t args, double max_freq);
    ~basic_tx(void);

private:
    double _max_freq;
};

static const uhd::dict<std::string, std::string> sd_name_to_conn = map_list_of
    ("AB", "IQ")
    ("BA", "QI")
    ("A",  "I")
    ("B",  "Q")
;

/***********************************************************************
 * Register the basic and LF dboards
 **********************************************************************/
static dboard_base::sptr make_basic_rx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new basic_rx(args, 250e6));
}

static dboard_base::sptr make_basic_tx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new basic_tx(args, 250e6));
}

static dboard_base::sptr make_lf_rx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new basic_rx(args, 32e6));
}

static dboard_base::sptr make_lf_tx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new basic_tx(args, 32e6));
}

UHD_STATIC_BLOCK(reg_basic_and_lf_dboards){
    dboard_manager::register_dboard(0x0000, &make_basic_tx, "Basic TX", sd_name_to_conn.keys());
    dboard_manager::register_dboard(0x0001, &make_basic_rx, "Basic RX", sd_name_to_conn.keys());
    dboard_manager::register_dboard(0x000e, &make_lf_tx,    "LF TX",    sd_name_to_conn.keys());
    dboard_manager::register_dboard(0x000f, &make_lf_rx,    "LF RX",    sd_name_to_conn.keys());
}

/***********************************************************************
 * Basic and LF RX dboard
 **********************************************************************/
basic_rx::basic_rx(ctor_args_t args, double max_freq) : rx_dboard_base(args){
    _max_freq = max_freq;
    //this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, true);
    
    ////////////////////////////////////////////////////////////////////
    // Register properties
    ////////////////////////////////////////////////////////////////////
    if(get_rx_id() == 0x0001){
        this->get_rx_subtree()->create<std::string>("name").set(
            std::string(str(boost::format("BasicRX (%s)") % get_subdev_name()
        )));
    }
    else{
        this->get_rx_subtree()->create<std::string>("name").set(
            std::string(str(boost::format("LFRX (%s)") % get_subdev_name()
        )));
    }

    this->get_rx_subtree()->create<int>("gains"); //phony property so this dir exists
    this->get_rx_subtree()->create<double>("freq/value")
        .publish(&always_zero_freq);
    this->get_rx_subtree()->create<meta_range_t>("freq/range")
        .set(freq_range_t(-_max_freq, +_max_freq));
    this->get_rx_subtree()->create<std::string>("antenna/value")
        .set("");
    this->get_rx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(list_of(""));
    this->get_rx_subtree()->create<int>("sensors"); //phony property so this dir exists
    this->get_rx_subtree()->create<std::string>("connection")
        .set(sd_name_to_conn[get_subdev_name()]);
    this->get_rx_subtree()->create<bool>("enabled")
        .set(true); //always enabled
    this->get_rx_subtree()->create<bool>("use_lo_offset")
        .set(false);
    this->get_rx_subtree()->create<double>("bandwidth/value")
        .set(subdev_bandwidth_scalar[get_subdev_name()]*_max_freq);
    this->get_rx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(subdev_bandwidth_scalar[get_subdev_name()]*_max_freq, subdev_bandwidth_scalar[get_subdev_name()]*_max_freq));
    
    //disable RX dboard clock by default
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, false);

    //set GPIOs to output 0x0000 to decrease noise pickup
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, 0x0000);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, 0xFFFF);
    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, 0x0000);
}

basic_rx::~basic_rx(void){
    /* NOP */
}

/***********************************************************************
 * Basic and LF TX dboard
 **********************************************************************/
basic_tx::basic_tx(ctor_args_t args, double max_freq) : tx_dboard_base(args){
    _max_freq = max_freq;
    //this->get_iface()->set_clock_enabled(dboard_iface::UNIT_TX, true);

    ////////////////////////////////////////////////////////////////////
    // Register properties
    ////////////////////////////////////////////////////////////////////
    if(get_tx_id() == 0x0000){
        this->get_tx_subtree()->create<std::string>("name").set(
            std::string(str(boost::format("BasicTX (%s)") % get_subdev_name()
        )));
    }
    else{
        this->get_tx_subtree()->create<std::string>("name").set(
            std::string(str(boost::format("LFTX (%s)") % get_subdev_name()
        )));
    }

    this->get_tx_subtree()->create<int>("gains"); //phony property so this dir exists
    this->get_tx_subtree()->create<double>("freq/value")
        .publish(&always_zero_freq);
    this->get_tx_subtree()->create<meta_range_t>("freq/range")
        .set(freq_range_t(-_max_freq, +_max_freq));
    this->get_tx_subtree()->create<std::string>("antenna/value")
        .set("");
    this->get_tx_subtree()->create<std::vector<std::string> >("antenna/options")
        .set(list_of(""));
    this->get_tx_subtree()->create<int>("sensors"); //phony property so this dir exists
    this->get_tx_subtree()->create<std::string>("connection")
        .set(sd_name_to_conn[get_subdev_name()]);
    this->get_tx_subtree()->create<bool>("enabled")
        .set(true); //always enabled
    this->get_tx_subtree()->create<bool>("use_lo_offset")
        .set(false);
    this->get_tx_subtree()->create<double>("bandwidth/value")
        .set(subdev_bandwidth_scalar[get_subdev_name()]*_max_freq);
    this->get_tx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(subdev_bandwidth_scalar[get_subdev_name()]*_max_freq, subdev_bandwidth_scalar[get_subdev_name()]*_max_freq));
    
    //disable TX dboard clock by default
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_TX, false);

    //set GPIOs to output 0x0000 to decrease noise pickup
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, 0x0000);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, 0xFFFF);
    this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, 0x0000);
}

basic_tx::~basic_tx(void){
    /* NOP */
}
