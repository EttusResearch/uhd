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

#include <uhd/usrp/usrp.hpp>
#include <uhd/usrp/mboard/test.hpp>
#include <uhd/utils.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <stdexcept>

using namespace uhd::usrp;

/***********************************************************************
 * default callbacks for the send and recv
 * these should be replaced with callbacks from the mboard object
 **********************************************************************/
static void send_raw_default(const std::vector<boost::asio::const_buffer> &){
    throw std::runtime_error("No callback registered for send raw");
}

static const boost::asio::const_buffer recv_raw_default(void){
    throw std::runtime_error("No callback registered for recv raw");
}

/***********************************************************************
 * the usrp device wrapper
 **********************************************************************/
usrp::usrp(const device_addr_t & device_addr){
    //set the default callbacks, the code below should replace them
    _send_raw_cb = boost::bind(&send_raw_default, _1);
    _recv_raw_cb = boost::bind(&recv_raw_default);

    //create mboard based on the device addr
    if (device_addr.type == DEVICE_ADDR_TYPE_VIRTUAL){
        _mboards[""] = mboard::base::sptr(new mboard::test(device_addr));
    }
}

usrp::~usrp(void){
    /* NOP */
}

void usrp::get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

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
        val = _mboards[name]->get_link();
        return;

    case DEVICE_PROP_MBOARD_NAMES:
        val = prop_names_t(get_map_keys(_mboards));
        return;
    }
}

void usrp::set(const wax::obj &, const wax::obj &){
    throw std::runtime_error("Cannot set in usrp device");
}

void usrp::send_raw(const std::vector<boost::asio::const_buffer> &buffs){
    return _send_raw_cb(buffs);
}

const boost::asio::const_buffer usrp::recv_raw(void){
    return _recv_raw_cb();
}
