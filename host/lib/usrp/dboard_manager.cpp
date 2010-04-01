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

#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/utils/gain_handler.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/types/dict.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * storage and registering for dboards
 **********************************************************************/
//dboard registry tuple: dboard constructor, canonical name, subdev names
typedef boost::tuple<dboard_manager::dboard_ctor_t, std::string, prop_names_t> args_t;

//map a dboard id to a dboard constructor
typedef uhd::dict<dboard_id_t, args_t> id_to_args_map_t;
UHD_SINGLETON_FCN(id_to_args_map_t, get_id_to_args_map)

void dboard_manager::register_dboard(
    dboard_id_t dboard_id,
    dboard_ctor_t dboard_ctor,
    const std::string &name,
    const prop_names_t &subdev_names
){
    //std::cout << "registering: " << name << std::endl;
    if (get_id_to_args_map().has_key(dboard_id)){
        throw std::runtime_error(str(boost::format(
            "The dboard id 0x%04x is already registered to %s."
        ) % dboard_id % dboard_id::to_string(dboard_id)));
    }
    get_id_to_args_map()[dboard_id] = args_t(dboard_ctor, name, subdev_names);
}

std::string dboard_id::to_string(const dboard_id_t &id){
    std::string name = (get_id_to_args_map().has_key(id))? get_id_to_args_map()[id].get<1>() : "unknown";
    return str(boost::format("%s (0x%04x)") % name % id);
}

/***********************************************************************
 * internal helper classe
 **********************************************************************/
/*!
 * A special wax proxy object that forwards calls to a subdev.
 * A sptr to an instance will be used in the properties structure. 
 */
class subdev_proxy : boost::noncopyable, public wax::obj{
public:
    typedef boost::shared_ptr<subdev_proxy> sptr;
    enum type_t{RX_TYPE, TX_TYPE};

    //structors
    subdev_proxy(dboard_base::sptr subdev, type_t type)
    : _subdev(subdev), _type(type){
        //initialize gain props struct
        gain_handler::props_t gain_props;
        gain_props.value = SUBDEV_PROP_GAIN;
        gain_props.range = SUBDEV_PROP_GAIN_RANGE;
        gain_props.names = SUBDEV_PROP_GAIN_NAMES;

        //make a new gain handler
        _gain_handler = gain_handler::make(
            this->get_link(), gain_props,
            boost::bind(&gain_handler::is_equal<subdev_prop_t>, _1, _2)
        );
    }

    ~subdev_proxy(void){
        /* NOP */
    }

private:
    gain_handler::sptr   _gain_handler;
    dboard_base::sptr   _subdev;
    type_t              _type;

    //forward the get calls to the rx or tx
    void get(const wax::obj &key, wax::obj &val){
        if (_gain_handler->intercept_get(key, val)) return;
        switch(_type){
        case RX_TYPE: return _subdev->rx_get(key, val);
        case TX_TYPE: return _subdev->tx_get(key, val);
        }
    }

    //forward the set calls to the rx or tx
    void set(const wax::obj &key, const wax::obj &val){
        if (_gain_handler->intercept_set(key, val)) return;
        switch(_type){
        case RX_TYPE: return _subdev->rx_set(key, val);
        case TX_TYPE: return _subdev->tx_set(key, val);
        }
    }
};

/***********************************************************************
 * dboard manager implementation class
 **********************************************************************/
class dboard_manager_impl : public dboard_manager{

public:
    dboard_manager_impl(
        dboard_id_t rx_dboard_id,
        dboard_id_t tx_dboard_id,
        dboard_interface::sptr interface
    );
    ~dboard_manager_impl(void);

    //dboard_interface
    prop_names_t get_rx_subdev_names(void);
    prop_names_t get_tx_subdev_names(void);
    wax::obj get_rx_subdev(const std::string &subdev_name);
    wax::obj get_tx_subdev(const std::string &subdev_name);

private:
    //list of rx and tx dboards in this dboard_manager
    //each dboard here is actually a subdevice proxy
    //the subdevice proxy is internal to the cpp file
    uhd::dict<std::string, subdev_proxy::sptr> _rx_dboards;
    uhd::dict<std::string, subdev_proxy::sptr> _tx_dboards;
    dboard_interface::sptr _interface;
    void set_nice_gpio_pins(void);
};

/***********************************************************************
 * make routine for dboard manager
 **********************************************************************/
dboard_manager::sptr dboard_manager::make(
    dboard_id_t rx_dboard_id,
    dboard_id_t tx_dboard_id,
    dboard_interface::sptr interface
){
    return dboard_manager::sptr(
        new dboard_manager_impl(rx_dboard_id, tx_dboard_id, interface)
    );
}

/***********************************************************************
 * implementation class methods
 **********************************************************************/
static args_t get_dboard_args(
    dboard_id_t dboard_id,
    std::string const& xx_type
){
    //special case, its rx and the none id (0xffff)
    if (xx_type == "rx" and dboard_id == dboard_id::NONE){
        return get_dboard_args(0x0001, xx_type);
    }

    //special case, its tx and the none id (0xffff)
    if (xx_type == "tx" and dboard_id == dboard_id::NONE){
        return get_dboard_args(0x0000, xx_type);
    }

    //verify that there is a registered constructor for this id
    if (not get_id_to_args_map().has_key(dboard_id)){
        throw std::runtime_error(str(
            boost::format("Unregistered %s dboard id: %s")
            % xx_type % dboard_id::to_string(dboard_id)
        ));
    }

    //return the dboard args for this id
    return get_id_to_args_map()[dboard_id];
}

dboard_manager_impl::dboard_manager_impl(
    dboard_id_t rx_dboard_id,
    dboard_id_t tx_dboard_id,
    dboard_interface::sptr interface
){
    _interface = interface;

    dboard_ctor_t rx_dboard_ctor; std::string rx_name; prop_names_t rx_subdevs;
    boost::tie(rx_dboard_ctor, rx_name, rx_subdevs) = get_dboard_args(rx_dboard_id, "rx");

    dboard_ctor_t tx_dboard_ctor; std::string tx_name; prop_names_t tx_subdevs;
    boost::tie(tx_dboard_ctor, tx_name, tx_subdevs) = get_dboard_args(tx_dboard_id, "tx");

    //initialize the gpio pins before creating subdevs
    set_nice_gpio_pins();

    //make xcvr subdevs (make one subdev for both rx and tx dboards)
    if (rx_dboard_ctor == tx_dboard_ctor){
        ASSERT_THROW(rx_subdevs == tx_subdevs);
        BOOST_FOREACH(const std::string &subdev, rx_subdevs){
            dboard_base::sptr xcvr_dboard = rx_dboard_ctor(
                dboard_base::ctor_args_t(subdev, interface, rx_dboard_id, tx_dboard_id)
            );
            //create a rx proxy for this xcvr board
            _rx_dboards[subdev] = subdev_proxy::sptr(
                new subdev_proxy(xcvr_dboard, subdev_proxy::RX_TYPE)
            );
            //create a tx proxy for this xcvr board
            _tx_dboards[subdev] = subdev_proxy::sptr(
                new subdev_proxy(xcvr_dboard, subdev_proxy::TX_TYPE)
            );
        }
    }

    //make tx and rx subdevs (separate subdevs for rx and tx dboards)
    else{
        //make the rx subdevs
        BOOST_FOREACH(const std::string &subdev, rx_subdevs){
            dboard_base::sptr rx_dboard = rx_dboard_ctor(
                dboard_base::ctor_args_t(subdev, interface, rx_dboard_id, dboard_id::NONE)
            );
            //create a rx proxy for this rx board
            _rx_dboards[subdev] = subdev_proxy::sptr(
                new subdev_proxy(rx_dboard, subdev_proxy::RX_TYPE)
            );
        }
        //make the tx subdevs
        BOOST_FOREACH(const std::string &subdev, tx_subdevs){
            dboard_base::sptr tx_dboard = tx_dboard_ctor(
                dboard_base::ctor_args_t(subdev, interface, dboard_id::NONE, tx_dboard_id)
            );
            //create a tx proxy for this tx board
            _tx_dboards[subdev] = subdev_proxy::sptr(
                new subdev_proxy(tx_dboard, subdev_proxy::TX_TYPE)
            );
        }
    }
}

dboard_manager_impl::~dboard_manager_impl(void){
    set_nice_gpio_pins();
}

prop_names_t dboard_manager_impl::get_rx_subdev_names(void){
    return _rx_dboards.get_keys();
}

prop_names_t dboard_manager_impl::get_tx_subdev_names(void){
    return _tx_dboards.get_keys();
}

wax::obj dboard_manager_impl::get_rx_subdev(const std::string &subdev_name){
    if (not _rx_dboards.has_key(subdev_name)) throw std::invalid_argument(
        str(boost::format("Unknown rx subdev name %s") % subdev_name)
    );
    //get a link to the rx subdev proxy
    return _rx_dboards[subdev_name]->get_link();
}

wax::obj dboard_manager_impl::get_tx_subdev(const std::string &subdev_name){
    if (not _tx_dboards.has_key(subdev_name)) throw std::invalid_argument(
        str(boost::format("Unknown tx subdev name %s") % subdev_name)
    );
    //get a link to the tx subdev proxy
    return _tx_dboards[subdev_name]->get_link();
}

void dboard_manager_impl::set_nice_gpio_pins(void){
    //std::cout << "Set nice GPIO pins" << std::endl;

    _interface->set_gpio_ddr(dboard_interface::GPIO_BANK_RX, 0x0000); //all inputs
    _interface->set_atr_reg(dboard_interface::GPIO_BANK_RX, dboard_interface::ATR_REG_IDLE, 0x0000); //all low

    _interface->set_gpio_ddr(dboard_interface::GPIO_BANK_TX, 0x0000); //all inputs
    _interface->set_atr_reg(dboard_interface::GPIO_BANK_TX, dboard_interface::ATR_REG_IDLE, 0x0000); //all low
}
