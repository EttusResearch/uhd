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
#include <uhd/utils.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include "dboard/dboards.hpp"

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

/***********************************************************************
 * register internal dboards
 *
 * Register internal/known dboards located in this build tree.
 * Each board should have entries below mapping an id to a constructor.
 * The xcvr type boards should register both rx and tx sides.
 *
 * This function will be called before new boards are registered.
 * This allows for internal boards to be externally overridden.
 * This function will also be called when creating a new dboard_manager
 * to ensure that the maps are filled with the entries below.
 **********************************************************************/
static void register_internal_dboards(void){
    //ensure that this function can only be called once per instance
    static bool called = false;
    if (called) return; called = true;
    //register the known dboards (dboard id, constructor, subdev names)
    dboard_manager::register_subdevs(ID_BASIC_TX, &basic_tx::make, list_of(""));
    dboard_manager::register_subdevs(ID_BASIC_RX, &basic_rx::make, list_of("a")("b")("ab"));
}

/***********************************************************************
 * storage and registering for dboards
 **********************************************************************/
typedef boost::tuple<dboard_manager::dboard_ctor_t, prop_names_t> args_t;

//map a dboard id to a dboard constructor
static uhd::dict<dboard_id_t, args_t> id_to_args_map;

void dboard_manager::register_subdevs(
    dboard_id_t dboard_id,
    dboard_ctor_t dboard_ctor,
    const prop_names_t &subdev_names
){
    register_internal_dboards(); //always call first
    id_to_args_map[dboard_id] = args_t(dboard_ctor, subdev_names);
}

/***********************************************************************
 * internal helper classes
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
        /* NOP */
    }

    ~subdev_proxy(void){
        /* NOP */
    }

private:
    dboard_base::sptr   _subdev;
    type_t              _type;

    //forward the get calls to the rx or tx
    void get(const wax::obj &key, wax::obj &val){
        switch(_type){
        case RX_TYPE: return _subdev->rx_get(key, val);
        case TX_TYPE: return _subdev->tx_get(key, val);
        }
    }

    //forward the set calls to the rx or tx
    void set(const wax::obj &key, const wax::obj &val){
        switch(_type){
        case RX_TYPE: return _subdev->rx_set(key, val);
        case TX_TYPE: return _subdev->tx_set(key, val);
        }
    }
};

/***********************************************************************
 * dboard dboard_manager methods
 **********************************************************************/
static args_t get_dboard_args(
    dboard_id_t dboard_id,
    std::string const& xx_type
){
    //special case, its rx and the none id (0xffff)
    if (xx_type == "rx" and dboard_id == ID_NONE){
        return args_t(&basic_rx::make, list_of("ab"));
    }

    //special case, its tx and the none id (0xffff)
    if (xx_type == "tx" and dboard_id == ID_NONE){
        return args_t(&basic_tx::make, list_of(""));
    }

    //verify that there is a registered constructor for this id
    if (not id_to_args_map.has_key(dboard_id)){
        throw std::runtime_error(str(
            boost::format("Unknown %s dboard id: 0x%04x") % xx_type % dboard_id
        ));
    }

    //return the dboard args for this id
    return id_to_args_map[dboard_id];
}

dboard_manager::dboard_manager(
    dboard_id_t rx_dboard_id,
    dboard_id_t tx_dboard_id,
    dboard_interface::sptr interface
){
    register_internal_dboards(); //always call first

    dboard_ctor_t rx_dboard_ctor; prop_names_t rx_subdevs;
    boost::tie(rx_dboard_ctor, rx_subdevs) = get_dboard_args(rx_dboard_id, "rx");

    dboard_ctor_t tx_dboard_ctor; prop_names_t tx_subdevs;
    boost::tie(tx_dboard_ctor, tx_subdevs) = get_dboard_args(tx_dboard_id, "tx");

    //initialize the gpio pins before creating subdevs
    interface->set_gpio_ddr(dboard_interface::GPIO_RX_BANK, 0x0000, 0xffff); //all inputs
    interface->set_gpio_ddr(dboard_interface::GPIO_TX_BANK, 0x0000, 0xffff);

    interface->write_gpio(dboard_interface::GPIO_RX_BANK, 0x0000, 0xffff); //all zeros
    interface->write_gpio(dboard_interface::GPIO_TX_BANK, 0x0000, 0xffff);

    interface->set_atr_reg(dboard_interface::GPIO_RX_BANK, 0x0000, 0x0000, 0x0000); //software controlled
    interface->set_atr_reg(dboard_interface::GPIO_TX_BANK, 0x0000, 0x0000, 0x0000);

    //make xcvr subdevs (make one subdev for both rx and tx dboards)
    if (rx_dboard_ctor == tx_dboard_ctor){
        ASSERT_THROW(rx_subdevs == tx_subdevs);
        BOOST_FOREACH(std::string name, rx_subdevs){
            dboard_base::sptr xcvr_dboard = rx_dboard_ctor(
                dboard_base::ctor_args_t(name, interface, rx_dboard_id, tx_dboard_id)
            );
            //create a rx proxy for this xcvr board
            _rx_dboards[name] = subdev_proxy::sptr(
                new subdev_proxy(xcvr_dboard, subdev_proxy::RX_TYPE)
            );
            //create a tx proxy for this xcvr board
            _tx_dboards[name] = subdev_proxy::sptr(
                new subdev_proxy(xcvr_dboard, subdev_proxy::TX_TYPE)
            );
        }
    }

    //make tx and rx subdevs (separate subdevs for rx and tx dboards)
    else{
        //make the rx subdevs
        BOOST_FOREACH(std::string name, rx_subdevs){
            dboard_base::sptr rx_dboard = rx_dboard_ctor(
                dboard_base::ctor_args_t(name, interface, rx_dboard_id, ID_NONE)
            );
            //create a rx proxy for this rx board
            _rx_dboards[name] = subdev_proxy::sptr(
                new subdev_proxy(rx_dboard, subdev_proxy::RX_TYPE)
            );
        }
        //make the tx subdevs
        BOOST_FOREACH(std::string name, tx_subdevs){
            dboard_base::sptr tx_dboard = tx_dboard_ctor(
                dboard_base::ctor_args_t(name, interface, ID_NONE, tx_dboard_id)
            );
            //create a tx proxy for this tx board
            _tx_dboards[name] = subdev_proxy::sptr(
                new subdev_proxy(tx_dboard, subdev_proxy::TX_TYPE)
            );
        }
    }
}

dboard_manager::~dboard_manager(void){
    /* NOP */
}

prop_names_t dboard_manager::get_rx_subdev_names(void){
    return _rx_dboards.get_keys();
}

prop_names_t dboard_manager::get_tx_subdev_names(void){
    return _tx_dboards.get_keys();
}

wax::obj dboard_manager::get_rx_subdev(const std::string &subdev_name){
    if (not _rx_dboards.has_key(subdev_name)) throw std::invalid_argument(
        str(boost::format("Unknown rx subdev name %s") % subdev_name)
    );
    //get a link to the rx subdev proxy
    return wax::cast<subdev_proxy::sptr>(_rx_dboards[subdev_name])->get_link();
}

wax::obj dboard_manager::get_tx_subdev(const std::string &subdev_name){
    if (not _tx_dboards.has_key(subdev_name)) throw std::invalid_argument(
        str(boost::format("Unknown tx subdev name %s") % subdev_name)
    );
    //get a link to the tx subdev proxy
    return wax::cast<subdev_proxy::sptr>(_tx_dboards[subdev_name])->get_link();
}
