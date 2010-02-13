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

#include <uhd/usrp/mboard/test.hpp>
#include <uhd/props.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <stdexcept>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::usrp::mboard;

/***********************************************************************
 * dummy interface for dboards
 **********************************************************************/
class dummy_interface : public uhd::usrp::dboard::interface{
public:
    dummy_interface(void){}
    ~dummy_interface(void){}
    void write_aux_dac(int, int){}
    int read_aux_adc(int){return 0;}
    void set_atr_reg(gpio_bank_t, uint16_t, uint16_t, uint16_t){}
    void set_gpio_ddr(gpio_bank_t, uint16_t, uint16_t){}
    void write_gpio(gpio_bank_t, uint16_t, uint16_t){}
    uint16_t read_gpio(gpio_bank_t){return 0;}
    void write_i2c (int, const std::string &){}
    std::string read_i2c (int, size_t){return "";}
    void write_spi (spi_dev_t, spi_push_t, const std::string &){}
    std::string read_spi (spi_dev_t, spi_latch_t, size_t){return "";}
    double get_rx_clock_rate(void){return 0.0;}
    double get_tx_clock_rate(void){return 0.0;}
};

/***********************************************************************
 * shell class to act as a dboard
 **********************************************************************/
class shell_dboard : public wax::obj{
public:
    enum type_t {TYPE_RX, TYPE_TX};
    shell_dboard(dboard::manager::sptr mgr, type_t type){
        _mgr = mgr;
        _type = type;
    }
    ~shell_dboard(void){}
private:
    void get(const wax::obj &key_, wax::obj &val){
        wax::obj key; std::string name;
        boost::tie(key, name) = extract_named_prop(key_);

        //handle the get request conditioned on the key
        switch(wax::cast<dboard_prop_t>(key)){
        case DBOARD_PROP_NAME:
            val = std::string("dboard test mboard");
            return;

        case DBOARD_PROP_SUBDEV:
            switch(_type){
            case TYPE_RX:
                val = _mgr->get_rx_subdev(name);
                return;

            case TYPE_TX:
                val = _mgr->get_tx_subdev(name);
                return;
            }

        case DBOARD_PROP_SUBDEV_NAMES:
            switch(_type){
            case TYPE_RX:
                val = _mgr->get_rx_subdev_names();
                return;

            case TYPE_TX:
                val = _mgr->get_tx_subdev_names();
                return;
            }

        case DBOARD_PROP_CODEC:
            val = NULL; //TODO
            return;
        }
    }

    void set(const wax::obj &, const wax::obj &){
        throw std::runtime_error("Cannot set on usrp test dboard");
    }

    type_t                _type;
    dboard::manager::sptr _mgr;
};

/***********************************************************************
 * test usrp mboard class
 **********************************************************************/
test::test(const device_addr_t &device_addr){
    //extract the number of dboards
    size_t num_dboards = boost::lexical_cast<size_t>(device_addr["num_dboards"]);
    //create a manager for each dboard
    for (size_t i = 0; i < num_dboards; i++){
        dboard::interface::sptr ifc(new dummy_interface());
        _dboard_managers[boost::lexical_cast<std::string>(i)] = dboard::manager::sptr(
            new dboard::manager(dboard::ID_BASIC_RX, dboard::ID_BASIC_TX, ifc)
        );
    }
}

test::~test(void){
    /* NOP */
}

void test::get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(wax::cast<mboard_prop_t>(key)){
    case MBOARD_PROP_NAME:
        val = std::string("usrp test mboard");
        return;

    case MBOARD_PROP_OTHERS:
        val = prop_names_t(); //empty other props
        return;

    case MBOARD_PROP_RX_DBOARD:
        if (not _dboard_managers.has_key(name)) throw std::invalid_argument(
            str(boost::format("Unknown rx dboard name %s") % name)
        );
        //FIXME store the shell dboard within the class
        //may not fix, plan to remove this test class when real usrps work
        //val = wax::obj::sptr(
        //    new shell_dboard(_dboard_managers[name], shell_dboard::TYPE_RX)
        //);
        return;

    case MBOARD_PROP_RX_DBOARD_NAMES:
        val = prop_names_t(_dboard_managers.get_keys());
        return;

    case MBOARD_PROP_TX_DBOARD:
        if (not _dboard_managers.has_key(name)) throw std::invalid_argument(
            str(boost::format("Unknown tx dboard name %s") % name)
        );
        //FIXME store the shell dboard within the class
        //may not fix, plan to remove this test class when real usrps work
        //val = wax::obj::sptr(
        //    new shell_dboard(_dboard_managers[name], shell_dboard::TYPE_TX)
        //);
        return;

    case MBOARD_PROP_TX_DBOARD_NAMES:
        val = prop_names_t(_dboard_managers.get_keys());
        return;

    case MBOARD_PROP_MTU:
    case MBOARD_PROP_CLOCK_RATE:
    case MBOARD_PROP_RX_DSP:
    case MBOARD_PROP_RX_DSP_NAMES:
    case MBOARD_PROP_TX_DSP:
    case MBOARD_PROP_TX_DSP_NAMES:
    case MBOARD_PROP_PPS_SOURCE:
    case MBOARD_PROP_PPS_SOURCE_NAMES:
    case MBOARD_PROP_PPS_POLARITY:
    case MBOARD_PROP_REF_SOURCE:
    case MBOARD_PROP_REF_SOURCE_NAMES:
    case MBOARD_PROP_TIME_NOW:
    case MBOARD_PROP_TIME_NEXT_PPS:
        throw std::runtime_error("unhandled prop is usrp test mboard");
    }
}

void test::set(const wax::obj &, const wax::obj &){
    throw std::runtime_error("Cannot set on usrp test mboard");
}
