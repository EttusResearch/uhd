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

#include <uhd/utils.hpp>
#include <boost/format.hpp>
#include "usrp2_impl.hpp"
#include "dboard_interface.hpp"

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Helper Methods
 **********************************************************************/
void usrp2_impl::dboard_init(void){
    //grab the dboard ids over the control line
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_GIVE_ME_YOUR_DBOARD_IDS_BRO);
    usrp2_ctrl_data_t in_data = ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_THESE_ARE_MY_DBOARD_IDS_DUDE);
    std::cout << boost::format("rx id 0x%.2x, tx id 0x%.2x")
        % ntohs(in_data.data.dboard_ids.rx_id)
        % ntohs(in_data.data.dboard_ids.tx_id) << std::endl;

    //extract the dboard ids an convert them to enums
    dboard_id_t rx_dboard_id = static_cast<dboard_id_t>(
        ntohs(in_data.data.dboard_ids.rx_id)
    );
    dboard_id_t tx_dboard_id = static_cast<dboard_id_t>(
        ntohs(in_data.data.dboard_ids.tx_id)
    );

    //create a new dboard interface and manager
    dboard_interface::sptr _dboard_interface(
        new usrp2_dboard_interface(this)
    );
    dboard_manager::sptr _dboard_manager = dboard_manager::make(
        rx_dboard_id, tx_dboard_id, _dboard_interface
    );

    //load dboards
    _rx_dboards[""] = wax_obj_proxy(
        boost::bind(&usrp2_impl::rx_dboard_get, this, _1, _2),
        boost::bind(&usrp2_impl::rx_dboard_set, this, _1, _2)
    );
    _tx_dboards[""] = wax_obj_proxy(
        boost::bind(&usrp2_impl::tx_dboard_get, this, _1, _2),
        boost::bind(&usrp2_impl::tx_dboard_set, this, _1, _2)
    );
}

/***********************************************************************
 * RX DBoard Properties
 **********************************************************************/
void usrp2_impl::rx_dboard_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<dboard_prop_t>()){
    case DBOARD_PROP_NAME:
        val = std::string("usrp2 dboard (rx unit)");
        return;

    case DBOARD_PROP_SUBDEV:
        val = _dboard_manager->get_rx_subdev(name);
        return;

    case DBOARD_PROP_SUBDEV_NAMES:
        val = _dboard_manager->get_rx_subdev_names();
        return;

    //case DBOARD_PROP_CODEC:
    //    throw std::runtime_error("unhandled prop in usrp2 dboard");
    }
}

void usrp2_impl::rx_dboard_set(const wax::obj &, const wax::obj &){
    throw std::runtime_error("Cannot set on usrp2 dboard");
}

/***********************************************************************
 * TX DBoard Properties
 **********************************************************************/
void usrp2_impl::tx_dboard_get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(key.as<dboard_prop_t>()){
    case DBOARD_PROP_NAME:
        val = std::string("usrp2 dboard (tx unit)");
        return;

    case DBOARD_PROP_SUBDEV:
        val = _dboard_manager->get_tx_subdev(name);
        return;

    case DBOARD_PROP_SUBDEV_NAMES:
        val = _dboard_manager->get_tx_subdev_names();
        return;

    //case DBOARD_PROP_CODEC:
    //    throw std::runtime_error("unhandled prop in usrp2 dboard");
    }
}

void usrp2_impl::tx_dboard_set(const wax::obj &, const wax::obj &){
    throw std::runtime_error("Cannot set on usrp2 dboard");
}
