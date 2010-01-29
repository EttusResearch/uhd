//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp/mboard/test.hpp>
#include <usrp_uhd/utils.hpp>
#include <usrp_uhd/props.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <stdexcept>

using namespace usrp_uhd;
using namespace usrp_uhd::usrp;
using namespace usrp_uhd::usrp::mboard;

/***********************************************************************
 * dummy interface for dboards
 **********************************************************************/
class dummy_interface : public usrp_uhd::usrp::dboard::interface{
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
    void get(const wax::type &key_, wax::type &val){
        wax::type key; std::string name;
        tie(key, name) = extract_named_prop(key_);

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

    void set(const wax::type &, const wax::type &){
        throw std::runtime_error("Cannot set on usrp test dboard");
    }

    type_t                _type;
    dboard::manager::sptr _mgr;
};

/***********************************************************************
 * test usrp mboard class
 **********************************************************************/
test::test(const device_addr_t &device_addr){
    //create a manager for each dboard
    for (size_t i = 0; i < device_addr.virtual_args.num_dboards; i++){
        dboard::interface::sptr ifc(new dummy_interface());
        _dboard_managers[boost::lexical_cast<std::string>(i)] = dboard::manager::sptr(
            new dboard::manager(0x0001, 0x0000, ifc)
        );
    }
}

test::~test(void){
    /* NOP */
}

void test::get(const wax::type &key_, wax::type &val){
    wax::type key; std::string name;
    tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(wax::cast<mboard_prop_t>(key)){
    case MBOARD_PROP_NAME:
        val = std::string("usrp test mboard");
        return;

    case MBOARD_PROP_OTHERS:
        val = prop_names_t(); //empty other props
        return;

    case MBOARD_PROP_RX_DBOARD:
        if (_dboard_managers.count(name) == 0) throw std::invalid_argument(
            str(boost::format("Unknown rx dboard name %s") % name)
        );
        val = wax::obj::sptr(
            new shell_dboard(_dboard_managers[name], shell_dboard::TYPE_RX)
        );
        return;

    case MBOARD_PROP_RX_DBOARD_NAMES:
        val = prop_names_t(get_map_keys(_dboard_managers));
        return;

    case MBOARD_PROP_TX_DBOARD:
        if (_dboard_managers.count(name) == 0) throw std::invalid_argument(
            str(boost::format("Unknown tx dboard name %s") % name)
        );
        val = wax::obj::sptr(
            new shell_dboard(_dboard_managers[name], shell_dboard::TYPE_TX)
        );
        return;

    case MBOARD_PROP_TX_DBOARD_NAMES:
        val = prop_names_t(get_map_keys(_dboard_managers));
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

void test::set(const wax::type &, const wax::type &){
    throw std::runtime_error("Cannot set on usrp test mboard");
}
