//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp/usrp.hpp>
#include <usrp_uhd/usrp/mboard/test.hpp>
#include <stdexcept>

using namespace usrp_uhd::usrp;

usrp::usrp(const device_addr_t & device_addr){
    if (device_addr.type == DEVICE_ADDR_TYPE_VIRTUAL){
        _mboards.push_back(
            mboard::base::sptr(new mboard::test(device_addr))
        );
    }
}

usrp::~usrp(void){
    /* NOP */
}

void usrp::get(const wax::type &key_, wax::type &val){
    //extract the index if key is an indexed prop
    wax::type key = key_; size_t index = 0;
    if (key.type() == typeid(indexed_prop_t)){
        boost::tie(key, index) = wax::cast<indexed_prop_t>(key);
    }

    //handle the get request conditioned on the key
    switch(wax::cast<device_prop_t>(key)){
    case DEVICE_PROP_NAME:
        val = std::string("usrp device");
        return;

    case DEVICE_PROP_MBOARD:
        //turn the mboard sptr object into a wax::obj::sptr
        //this allows the properties access through the wax::proxy
        val = wax::obj::cast(_mboards.at(index));
        return;

    case DEVICE_PROP_NUM_MBOARDS:
        val = size_t(_mboards.size());
        return;
    }
}

void usrp::set(const wax::type &, const wax::type &){
    throw std::runtime_error("Cannot set in usrp device");
}

void usrp::send_raw(const send_args_t &){
    //TODO make the call on the mboard
}

void usrp::recv_raw(const recv_args_t &){
    //TODO make the call on the mboard
}
