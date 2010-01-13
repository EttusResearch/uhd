//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp/dboard/manager.hpp>

using namespace usrp_uhd::usrp::dboard;

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
//include dboard derived classes (local include)
#include "dboards.hpp"

#define MAKE_DBOARD_ID_WORD(rx_id, tx_id) \
    ((uint32_t(rx_id) << 16) | (uint32_t(tx_id) << 0))

manager::manager(
    uint16_t rx_dboard_id,
    uint16_t tx_dboard_id,
    interface::sptr dboard_interface
){
    //TODO build some boards based on ids
    //xcvrs will be added to both vectors
    switch(MAKE_DBOARD_ID_WORD(rx_dboard_id, tx_dboard_id)){
    default:
        _rx_dboards.push_back(xcvr_base::sptr(new basic_rx(0, dboard_interface)));
        _rx_dboards.push_back(xcvr_base::sptr(new basic_rx(1, dboard_interface)));
        _rx_dboards.push_back(xcvr_base::sptr(new basic_rx(2, dboard_interface)));
        _tx_dboards.push_back(xcvr_base::sptr(new basic_tx(0, dboard_interface)));
        break;
    }
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
