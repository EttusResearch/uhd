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


#include "usrp2_impl.hpp"
#include "usrp2_regs.hpp"
#include <uhd/utils/assert.hpp>
#include <boost/format.hpp>
#include <cstddef>

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

    //extract the dboard ids an convert them
    dboard_id_t rx_dboard_id = ntohs(in_data.data.dboard_ids.rx_id);
    dboard_id_t tx_dboard_id = ntohs(in_data.data.dboard_ids.tx_id);

    //create a new dboard interface and manager
    dboard_interface::sptr _dboard_interface(
        make_usrp2_dboard_interface(this)
    );
    _dboard_manager = dboard_manager::make(
        rx_dboard_id, tx_dboard_id, _dboard_interface
    );

    //load dboards
    _rx_dboards[""] = wax_obj_proxy::make(
        boost::bind(&usrp2_impl::rx_dboard_get, this, _1, _2),
        boost::bind(&usrp2_impl::rx_dboard_set, this, _1, _2)
    );
    _tx_dboards[""] = wax_obj_proxy::make(
        boost::bind(&usrp2_impl::tx_dboard_get, this, _1, _2),
        boost::bind(&usrp2_impl::tx_dboard_set, this, _1, _2)
    );

    //init the subdevs in use (use the first subdevice)
    _rx_subdevs_in_use = prop_names_t(1, _dboard_manager->get_rx_subdev_names().at(0));
    update_rx_mux_config();

    _tx_subdevs_in_use = prop_names_t(1, _dboard_manager->get_tx_subdev_names().at(0));
    update_tx_mux_config();
}

void usrp2_impl::update_rx_mux_config(void){
    //calculate the rx mux
    boost::uint32_t rx_mux = 0;
    ASSERT_THROW(_rx_subdevs_in_use.size() == 1);
    wax::obj rx_subdev = _dboard_manager->get_rx_subdev(_rx_subdevs_in_use.at(0));
    std::cout << "Using: " << rx_subdev[SUBDEV_PROP_NAME].as<std::string>() << std::endl;
    if (rx_subdev[SUBDEV_PROP_QUADRATURE].as<bool>()){
        rx_mux = (0x01 << 2) | (0x00 << 0); //Q=ADC1, I=ADC0
    }else{
        rx_mux = 0x00; //ADC0
    }
    if (rx_subdev[SUBDEV_PROP_IQ_SWAPPED].as<bool>()){
        rx_mux = (((rx_mux >> 0) & 0x3) << 2) | (((rx_mux >> 2) & 0x3) << 0);
    }

    this->poke(offsetof(dsp_rx_regs_t, rx_mux) + DSP_RX_BASE, rx_mux);
}

void usrp2_impl::update_tx_mux_config(void){
    //calculate the tx mux
    boost::uint32_t tx_mux = 0x10;
    ASSERT_THROW(_tx_subdevs_in_use.size() == 1);
    wax::obj tx_subdev = _dboard_manager->get_tx_subdev(_tx_subdevs_in_use.at(0));
    std::cout << "Using: " << tx_subdev[SUBDEV_PROP_NAME].as<std::string>() << std::endl;
    if (tx_subdev[SUBDEV_PROP_IQ_SWAPPED].as<bool>()){
        tx_mux = (((tx_mux >> 0) & 0x1) << 1) | (((tx_mux >> 1) & 0x1) << 0);
    }

    this->poke(offsetof(dsp_tx_regs_t, tx_mux) + DSP_TX_BASE, tx_mux);
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

    case DBOARD_PROP_USED_SUBDEVS:
        val = _rx_subdevs_in_use;
        return;

    //case DBOARD_PROP_CODEC:
    //    throw std::runtime_error("unhandled prop in usrp2 dboard");
    }
}

void usrp2_impl::rx_dboard_set(const wax::obj &key, const wax::obj &val){
    if (key.as<dboard_prop_t>() == DBOARD_PROP_USED_SUBDEVS){
        _rx_subdevs_in_use = val.as<prop_names_t>();
        update_rx_mux_config(); //if the val is bad, this will throw
        return;
    }

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

    case DBOARD_PROP_USED_SUBDEVS:
        val = _tx_subdevs_in_use;
        return;

    //case DBOARD_PROP_CODEC:
    //    throw std::runtime_error("unhandled prop in usrp2 dboard");
    }
}

void usrp2_impl::tx_dboard_set(const wax::obj &key, const wax::obj &val){
    if (key.as<dboard_prop_t>() == DBOARD_PROP_USED_SUBDEVS){
        _tx_subdevs_in_use = val.as<prop_names_t>();
        update_tx_mux_config(); //if the val is bad, this will throw
        return;
    }

    throw std::runtime_error("Cannot set on usrp2 dboard");
}
