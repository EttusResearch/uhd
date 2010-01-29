//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp/usrp.hpp>
#include <usrp_uhd/usrp/mboard/test.hpp>
#include <usrp_uhd/utils.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <stdexcept>

using namespace usrp_uhd::usrp;

/***********************************************************************
 * default callbacks for the send and recv
 * these should be replaced with callbacks from the mboard object
 **********************************************************************/
static void send_raw_default(const usrp_uhd::device::send_args_t &){
    throw std::runtime_error("No callback registered for send raw");
}

static void recv_raw_default(const usrp_uhd::device::recv_args_t &){
    throw std::runtime_error("No callback registered for recv raw");
}

/***********************************************************************
 * the usrp device wrapper
 **********************************************************************/
usrp::usrp(const device_addr_t & device_addr){
    //set the default callbacks, the code below should replace them
    _send_raw_cb = boost::bind(&send_raw_default, _1);
    _recv_raw_cb = boost::bind(&recv_raw_default, _1);

    //create mboard based on the device addr
    if (device_addr.type == DEVICE_ADDR_TYPE_VIRTUAL){
        _mboards[""] = mboard::base::sptr(new mboard::test(device_addr));
    }
}

usrp::~usrp(void){
    /* NOP */
}

void usrp::get(const wax::type &key_, wax::type &val){
    wax::type key; std::string name;
    tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(wax::cast<device_prop_t>(key)){
    case DEVICE_PROP_NAME:
        val = std::string("usrp device");
        return;

    case DEVICE_PROP_MBOARD:
        if (_mboards.count(name) == 0) throw std::invalid_argument(
            str(boost::format("Unknown mboard name %s") % name)
        );
        //turn the mboard sptr object into a wax::obj::sptr
        //this allows the properties access through the wax::proxy
        val = wax::obj::cast(_mboards[name]);
        return;

    case DEVICE_PROP_MBOARD_NAMES:
        val = prop_names_t(get_map_keys(_mboards));
        return;
    }
}

void usrp::set(const wax::type &, const wax::type &){
    throw std::runtime_error("Cannot set in usrp device");
}

void usrp::send_raw(const send_args_t &args){
    return _send_raw_cb(args);
}

void usrp::recv_raw(const recv_args_t &args){
    return _recv_raw_cb(args);
}
