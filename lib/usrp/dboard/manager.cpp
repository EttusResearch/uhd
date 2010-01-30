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

#include <usrp_uhd/usrp/dboard/manager.hpp>
#include <usrp_uhd/utils.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/foreach.hpp>
#include "dboards.hpp"

using namespace usrp_uhd;
using namespace usrp_uhd::usrp::dboard;
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
 * This function will also be called when creating a new manager
 * to ensure that the maps are filled with the entries below.
 **********************************************************************/
static void register_internal_dboards(void){
    //ensure that this function can only be called once per instance
    static bool called = false;
    if (called) return; called = true;
    //register the known dboards (dboard id, constructor, subdev names)
    manager::register_subdevs(ID_BASIC_TX, &basic_tx::make, list_of(""));
    manager::register_subdevs(ID_BASIC_RX, &basic_rx::make, list_of("a")("b")("ab"));
}

/***********************************************************************
 * storage and registering for dboards
 **********************************************************************/
//map a dboard id to a dboard constructor
static std::map<dboard_id_t, manager::dboard_ctor_t> id_to_ctor_map;

//map a dboard constructor to subdevice names
static std::map<manager::dboard_ctor_t, prop_names_t> ctor_to_names_map;

void manager::register_subdevs(
    dboard_id_t dboard_id,
    dboard_ctor_t dboard_ctor,
    const prop_names_t &subdev_names
){
    register_internal_dboards(); //always call first
    id_to_ctor_map[dboard_id] = dboard_ctor;
    ctor_to_names_map[dboard_ctor] = subdev_names;
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
    subdev_proxy(base::sptr subdev, type_t type)
    : _subdev(subdev), _type(type){
        /* NOP */
    }

    ~subdev_proxy(void){
        /* NOP */
    }

private:
    base::sptr   _subdev;
    type_t       _type;

    //forward the get calls to the rx or tx
    void get(const wax::type &key, wax::type &val){
        switch(_type){
        case RX_TYPE: return _subdev->rx_get(key, val);
        case TX_TYPE: return _subdev->tx_get(key, val);
        }
    }

    //forward the set calls to the rx or tx
    void set(const wax::type &key, const wax::type &val){
        switch(_type){
        case RX_TYPE: return _subdev->rx_set(key, val);
        case TX_TYPE: return _subdev->tx_set(key, val);
        }
    }
};

/***********************************************************************
 * dboard manager methods
 **********************************************************************/
static manager::dboard_ctor_t const& get_dboard_ctor(
    dboard_id_t dboard_id,
    std::string const& xx_type
){
    //verify that there is a registered constructor for this id
    if (id_to_ctor_map.count(dboard_id) == 0){
        throw std::runtime_error(str(
            boost::format("Unknown %s dboard id: 0x%04x") % xx_type % dboard_id
        ));
    }
    //return the dboard constructor for this id
    return id_to_ctor_map[dboard_id];
}

manager::manager(
    dboard_id_t rx_dboard_id,
    dboard_id_t tx_dboard_id,
    interface::sptr dboard_interface
){
    register_internal_dboards(); //always call first
    const dboard_ctor_t rx_dboard_ctor = get_dboard_ctor(rx_dboard_id, "rx");
    const dboard_ctor_t tx_dboard_ctor = get_dboard_ctor(tx_dboard_id, "tx");
    //make xcvr subdevs (make one subdev for both rx and tx dboards)
    if (rx_dboard_ctor == tx_dboard_ctor){
        BOOST_FOREACH(std::string name, ctor_to_names_map[rx_dboard_ctor]){
            base::sptr xcvr_dboard = rx_dboard_ctor(
                base::ctor_args_t(name, dboard_interface)
            );
            _rx_dboards[name] = xcvr_dboard;
            _tx_dboards[name] = xcvr_dboard;
        }
    }
    //make tx and rx subdevs (separate subdevs for rx and tx dboards)
    else{
        //make the rx subdevs
        BOOST_FOREACH(std::string name, ctor_to_names_map[rx_dboard_ctor]){
            _rx_dboards[name] = rx_dboard_ctor(
                base::ctor_args_t(name, dboard_interface)
            );
        }
        //make the tx subdevs
        BOOST_FOREACH(std::string name, ctor_to_names_map[tx_dboard_ctor]){
            _tx_dboards[name] = tx_dboard_ctor(
                base::ctor_args_t(name, dboard_interface)
            );
        }
    }
}

manager::~manager(void){
    /* NOP */
}

prop_names_t manager::get_rx_subdev_names(void){
    return get_map_keys(_rx_dboards);
}

prop_names_t manager::get_tx_subdev_names(void){
    return get_map_keys(_tx_dboards);
}

wax::obj::sptr manager::get_rx_subdev(const std::string &subdev_name){
    if (_rx_dboards.count(subdev_name) == 0) throw std::invalid_argument(
        str(boost::format("Unknown rx subdev name %s") % subdev_name)
    );
    return wax::obj::sptr(new subdev_proxy(
        _rx_dboards[subdev_name], subdev_proxy::RX_TYPE)
    );
}

wax::obj::sptr manager::get_tx_subdev(const std::string &subdev_name){
    if (_tx_dboards.count(subdev_name) == 0) throw std::invalid_argument(
        str(boost::format("Unknown tx subdev name %s") % subdev_name)
    );
    return wax::obj::sptr(new subdev_proxy(
        _tx_dboards[subdev_name], subdev_proxy::TX_TYPE)
    );
}
