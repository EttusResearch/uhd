//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp/mboard/test.hpp>
#include <usrp_uhd/props.hpp>
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
        //extract the index if key is an indexed prop
        wax::type key = key_; size_t index = 0;
        if (key.type() == typeid(indexed_prop_t)){
            boost::tie(key, index) = wax::cast<indexed_prop_t>(key);
        }

        //handle the get request conditioned on the key
        switch(wax::cast<dboard_prop_t>(key)){
        case DBOARD_PROP_NAME:
            val = std::string("dboard test mboard");
            return;

        case DBOARD_PROP_SUBDEV:
            switch(_type){
            case TYPE_RX:
                val = _mgr->get_rx_subdev(index);
                return;

            case TYPE_TX:
                val = _mgr->get_tx_subdev(index);
                return;
            }

        case DBOARD_PROP_NUM_SUBDEVS:
            switch(_type){
            case TYPE_RX:
                val = _mgr->get_num_rx_subdevs();
                return;

            case TYPE_TX:
                val = _mgr->get_num_tx_subdevs();
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
        _dboard_managers.push_back(dboard::manager::sptr(
            new dboard::manager(0x0001, 0x0000, ifc)
        ));
    }
}

test::~test(void){
    /* NOP */
}

void test::get(const wax::type &key_, wax::type &val){
    //extract the index if key is an indexed prop
    wax::type key = key_; size_t index = 0;
    if (key.type() == typeid(indexed_prop_t)){
        boost::tie(key, index) = wax::cast<indexed_prop_t>(key);
    }

    //handle the get request conditioned on the key
    switch(wax::cast<mboard_prop_t>(key)){
    case MBOARD_PROP_NAME:
        val = std::string("usrp test mboard");
        return;

    case MBOARD_PROP_RX_DBOARD:
        val = wax::obj::sptr(
            new shell_dboard(_dboard_managers.at(index), shell_dboard::TYPE_RX)
        );
        return;

    case MBOARD_PROP_NUM_RX_DBOARDS:
        val = size_t(_dboard_managers.size());
        return;

    case MBOARD_PROP_TX_DBOARD:
        val = wax::obj::sptr(
            new shell_dboard(_dboard_managers.at(index), shell_dboard::TYPE_TX)
        );
        return;

    case MBOARD_PROP_NUM_TX_DBOARDS:
        val = size_t(_dboard_managers.size());
        return;

    case MBOARD_PROP_MTU:
    case MBOARD_PROP_CLOCK_RATE:
    case MBOARD_PROP_RX_DSP:
    case MBOARD_PROP_NUM_RX_DSPS:
    case MBOARD_PROP_TX_DSP:
    case MBOARD_PROP_NUM_TX_DSPS:
    case MBOARD_PROP_PPS_SOURCE:
    case MBOARD_PROP_PPS_POLARITY:
    case MBOARD_PROP_REF_SOURCE:
    case MBOARD_PROP_TIME_NOW:
    case MBOARD_PROP_TIME_NEXT_PPS:
        throw std::runtime_error("unhandled prop is usrp test mboard");
    }
}

void test::set(const wax::type &, const wax::type &){
    throw std::runtime_error("Cannot set on usrp test mboard");
}
