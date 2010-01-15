//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp/dboard/manager.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/format.hpp>
#include <map>
#include "dboards.hpp"

using namespace usrp_uhd::usrp::dboard;

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
    //register the known dboards (dboard id, constructor, num subdevs)
    manager::register_tx_subdev(0x0000, &basic_tx::make, 1);
    manager::register_rx_subdev(0x0001, &basic_rx::make, 3);
}

/***********************************************************************
 * storage and registering for dboards
 **********************************************************************/
//hold a dboard constructor and the number of subdevs
typedef boost::tuple<manager::dboard_ctor_t, size_t> dboard_reg_val_t;

//map a dboard id to a registered value tuple
typedef std::map<manager::dboard_id_t, dboard_reg_val_t> dboard_reg_map_t;

//static instances of registered dboard ids
static dboard_reg_map_t dboard_rx_regs, dboard_tx_regs;

/*!
 * Helper function to register a dboard contructor to its id.
 * This should be called by the api calls to register subdevs.
 */
static void register_xx_subdev(
    manager::dboard_id_t dboard_id,
    manager::dboard_ctor_t dboard_ctor,
    size_t num_subdevs,
    dboard_reg_map_t &dboard_reg_map
){
    register_internal_dboards(); //always call first
    dboard_reg_map.insert(std::pair<manager::dboard_id_t, dboard_reg_val_t>(
        dboard_id, dboard_reg_val_t(dboard_ctor, num_subdevs)
    ));
}

void manager::register_rx_subdev(
    dboard_id_t dboard_id, dboard_ctor_t dboard_ctor, size_t num_subdevs
){
    register_xx_subdev(dboard_id, dboard_ctor, num_subdevs, dboard_rx_regs);
}

void manager::register_tx_subdev(
    dboard_id_t dboard_id, dboard_ctor_t dboard_ctor, size_t num_subdevs
){
    register_xx_subdev(dboard_id, dboard_ctor, num_subdevs, dboard_tx_regs);
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
    subdev_proxy(xcvr_base::sptr subdev, type_t type)
    : _subdev(subdev), _type(type){
        /* NOP */
    }

    ~subdev_proxy(void){
        /* NOP */
    }

private:
    xcvr_base::sptr   _subdev;
    type_t            _type;

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
static void create_subdevs(
    dboard_reg_map_t &dboard_reg_map,
    manager::dboard_id_t dboard_id,
    interface::sptr dboard_interface,
    std::vector<xcvr_base::sptr> &subdevs,
    std::string const& xx_type
){
    //verify that there is a registered constructor for this id
    if (dboard_reg_map.count(dboard_id) == 0){
        throw std::runtime_error(str(
            boost::format("Unknown %s dboard id: 0x%04x") % xx_type % dboard_id
        ));
    }
    //create new subdevices for the dboard ids
    for (size_t i = 0; i < dboard_reg_map[dboard_id].get<1>(); i++){
        subdevs.push_back(
            dboard_reg_map[dboard_id].get<0>()(
                xcvr_base::ctor_args_t(i, dboard_interface)
            )
        );
    }
}

manager::manager(
    dboard_id_t rx_dboard_id,
    dboard_id_t tx_dboard_id,
    interface::sptr dboard_interface
){
    register_internal_dboards(); //always call first
    create_subdevs(dboard_rx_regs, rx_dboard_id, dboard_interface, _rx_dboards, "rx");
    create_subdevs(dboard_tx_regs, tx_dboard_id, dboard_interface, _tx_dboards, "tx");
}

manager::~manager(void){
    /* NOP */
}

size_t manager::get_num_rx_subdevs(void){
    return _rx_dboards.size();
}

size_t manager::get_num_tx_subdevs(void){
    return _tx_dboards.size();
}

wax::obj::sptr manager::get_rx_subdev(size_t subdev_index){
    return wax::obj::sptr(new subdev_proxy(
        _rx_dboards.at(subdev_index), subdev_proxy::RX_TYPE)
    );
}

wax::obj::sptr manager::get_tx_subdev(size_t subdev_index){
    return wax::obj::sptr(new subdev_proxy(
        _tx_dboards.at(subdev_index), subdev_proxy::TX_TYPE)
    );
}
