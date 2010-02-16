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

#include <boost/format.hpp>
#include <uhd/utils.hpp>
#include <uhd/props.hpp>
#include <iostream>
#include "usrp2_impl.hpp"
#include "usrp2_dboard_interface.hpp"

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * USRP2 DBoard Wrapper
 **********************************************************************/
usrp2_dboard::usrp2_dboard(uhd::usrp::dboard::manager::sptr mgr, type_t type){
    _mgr = mgr;
    _type = type;
}

usrp2_dboard::~usrp2_dboard(void){
    /* NOP */
}

void usrp2_dboard::get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(wax::cast<dboard_prop_t>(key)){
    case DBOARD_PROP_NAME:
        val = std::string("usrp2 dboard");
        return;

    case DBOARD_PROP_SUBDEV:
        switch(_type){
        case TYPE_RX:
            val = _mgr->get_rx_subdev(name);
            return;

        case TYPE_TX:
            val = _mgr->get_tx_subdev(name);
            return;
        }

    case DBOARD_PROP_SUBDEV_NAMES:
        switch(_type){
        case TYPE_RX:
            val = _mgr->get_rx_subdev_names();
            return;

        case TYPE_TX:
            val = _mgr->get_tx_subdev_names();
            return;
        }

    case DBOARD_PROP_CODEC:
        throw std::runtime_error("unhandled prop in usrp2 dboard");
    }
}

void usrp2_dboard::set(const wax::obj &, const wax::obj &){
    throw std::runtime_error("Cannot set on usrp2 dboard");
}

/***********************************************************************
 * USRP2 Implementation
 **********************************************************************/
usrp2_impl::usrp2_impl(
    uhd::transport::udp::sptr ctrl_transport,
    uhd::transport::udp::sptr data_transport
){
    _ctrl_transport = ctrl_transport;
    _data_transport = data_transport;

    //grab the dboard ids over the control line
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_GIVE_ME_YOUR_DBOARD_IDS_BRO);
    usrp2_ctrl_data_t in_data = ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_THESE_ARE_MY_DBOARD_IDS_DUDE);
    std::cout << boost::format("rx id 0x%.2x, tx id 0x%.2x")
        % ntohs(in_data.data.dboard_ids.rx_id)
        % ntohs(in_data.data.dboard_ids.tx_id) << std::endl;

    //extract the dboard ids an convert them to enums
    dboard::dboard_id_t rx_dboard_id = static_cast<dboard::dboard_id_t>(
        ntohs(in_data.data.dboard_ids.rx_id)
    );
    dboard::dboard_id_t tx_dboard_id = static_cast<dboard::dboard_id_t>(
        ntohs(in_data.data.dboard_ids.tx_id)
    );

    //create a new dboard interface and manager
    dboard::interface::sptr dboard_interface(
        new usrp2_dboard_interface(this)
    );
    dboard::manager::sptr dboard_manager(
        new dboard::manager(rx_dboard_id, tx_dboard_id, dboard_interface)
    );

    //load dboards
    _rx_dboards[""] = usrp2_dboard::sptr(new usrp2_dboard(dboard_manager, usrp2_dboard::TYPE_RX));
    _tx_dboards[""] = usrp2_dboard::sptr(new usrp2_dboard(dboard_manager, usrp2_dboard::TYPE_TX));

    //TOD load dsps

}

usrp2_impl::~usrp2_impl(void){
    /* NOP */
}

/***********************************************************************
 * Control Send/Recv
 **********************************************************************/
usrp2_ctrl_data_t usrp2_impl::ctrl_send_and_recv(const usrp2_ctrl_data_t &out_data){
    boost::mutex::scoped_lock lock(_ctrl_mutex);

    //fill in the seq number and send
    usrp2_ctrl_data_t out_copy = out_data;
    out_copy.seq = htonl(++_ctrl_seq_num);
    _ctrl_transport->send(boost::asio::buffer(&out_copy, sizeof(usrp2_ctrl_data_t)));

    //loop and recieve until the time is up
    size_t num_timeouts = 0;
    while(true){
        uhd::shared_iovec iov = _ctrl_transport->recv();
        if (iov.len < sizeof(usrp2_ctrl_data_t)){
            //sleep a little so we dont burn cpu
            if (num_timeouts++ > 50) break;
            boost::this_thread::sleep(boost::posix_time::milliseconds(1));
        }else{
            //handle the received data
            usrp2_ctrl_data_t in_data = *reinterpret_cast<const usrp2_ctrl_data_t *>(iov.base);
            if (ntohl(in_data.seq) == _ctrl_seq_num){
                return in_data;
            }
            //didnt get seq, continue on...
        }
    }
    throw std::runtime_error("usrp2 no control response");
}

/***********************************************************************
 * Get Properties
 **********************************************************************/
void usrp2_impl::get(const wax::obj &key_, wax::obj &val){
    wax::obj key; std::string name;
    boost::tie(key, name) = extract_named_prop(key_);

    //handle the get request conditioned on the key
    switch(wax::cast<mboard_prop_t>(key)){
    case MBOARD_PROP_NAME:
        val = std::string("usrp2 mboard");
        return;

    case MBOARD_PROP_OTHERS:
        val = prop_names_t(); //empty other props
        return;

    case MBOARD_PROP_RX_DBOARD:
    case MBOARD_PROP_RX_DBOARD_NAMES:
    case MBOARD_PROP_TX_DBOARD:
    case MBOARD_PROP_TX_DBOARD_NAMES:
    case MBOARD_PROP_MTU:
    case MBOARD_PROP_CLOCK_RATE:
    case MBOARD_PROP_RX_DSP:
    case MBOARD_PROP_RX_DSP_NAMES:
    case MBOARD_PROP_TX_DSP:
    case MBOARD_PROP_TX_DSP_NAMES:
    case MBOARD_PROP_PPS_SOURCE:
    case MBOARD_PROP_PPS_SOURCE_NAMES:
    case MBOARD_PROP_PPS_POLARITY:
    case MBOARD_PROP_REF_SOURCE:
    case MBOARD_PROP_REF_SOURCE_NAMES:
    case MBOARD_PROP_TIME_NOW:
    case MBOARD_PROP_TIME_NEXT_PPS:
        throw std::runtime_error("unhandled prop in usrp2 mboard");
    }
}

/***********************************************************************
 * Set Properties
 **********************************************************************/
void usrp2_impl::set(const wax::obj &, const wax::obj &){
    throw std::runtime_error("Cannot set on usrp2 mboard");
}
