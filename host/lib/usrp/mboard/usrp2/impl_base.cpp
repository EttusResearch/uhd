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
#include "impl_base.hpp"
#include "dboard_interface.hpp"

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Structors
 **********************************************************************/
impl_base::impl_base(
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
    dboard::interface::sptr _dboard_interface(
        new dboard_interface(this)
    );
    dboard::manager::sptr dboard_manager(
        new dboard::manager(rx_dboard_id, tx_dboard_id, _dboard_interface)
    );

    //load dboards
    _rx_dboards[""] = dboard_impl::sptr(new dboard_impl(dboard_manager, dboard_impl::TYPE_RX));
    _tx_dboards[""] = dboard_impl::sptr(new dboard_impl(dboard_manager, dboard_impl::TYPE_TX));

    //TOD load dsps

    //init the pps source clock config
    _pps_source_dict["sma"]  = USRP2_PPS_SOURCE_SMA;
    _pps_source_dict["mimo"] = USRP2_PPS_SOURCE_MIMO;
    _pps_source = "sma";

    //init the pps polarity clock config
    _pps_polarity_dict["pos"] = USRP2_PPS_POLARITY_POS;
    _pps_polarity_dict["neg"] = USRP2_PPS_POLARITY_NEG;
    _pps_polarity = "neg";

    //init the ref source clock config
    _ref_source_dict["int"]  = USRP2_REF_SOURCE_INT;
    _ref_source_dict["sma"]  = USRP2_REF_SOURCE_SMA;
    _ref_source_dict["mimo"] = USRP2_REF_SOURCE_MIMO;
    _ref_source = "int";

    //update the clock config (sends a control packet)
    update_clock_config();
}

impl_base::~impl_base(void){
    /* NOP */
}

/***********************************************************************
 * Misc Access Methods
 **********************************************************************/
double impl_base::get_master_clock_freq(void){
    return 100e6;
}

void impl_base::update_clock_config(void){
    //setup the out data
    usrp2_ctrl_data_t out_data;
    out_data.id = htonl(USRP2_CTRL_ID_HERES_A_NEW_CLOCK_CONFIG_BRO);
    out_data.data.clock_config.pps_source   = _pps_source_dict  [_pps_source];
    out_data.data.clock_config.pps_polarity = _pps_polarity_dict[_pps_polarity];
    out_data.data.clock_config.ref_source   = _ref_source_dict  [_ref_source];

    //send and recv
    usrp2_ctrl_data_t in_data = ctrl_send_and_recv(out_data);
    ASSERT_THROW(htonl(in_data.id) == USRP2_CTRL_ID_GOT_THE_NEW_CLOCK_CONFIG_DUDE);
}

/***********************************************************************
 * Control Send/Recv
 **********************************************************************/
usrp2_ctrl_data_t impl_base::ctrl_send_and_recv(const usrp2_ctrl_data_t &out_data){
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
void impl_base::get(const wax::obj &key_, wax::obj &val){
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
        val = _rx_dboards[name]->get_link();
        return;

    case MBOARD_PROP_RX_DBOARD_NAMES:
        val = prop_names_t(_rx_dboards.get_keys());
        return;

    case MBOARD_PROP_TX_DBOARD:
        val = _tx_dboards[name]->get_link();
        return;

    case MBOARD_PROP_TX_DBOARD_NAMES:
        val = prop_names_t(_tx_dboards.get_keys());
        return;

    case MBOARD_PROP_MTU:
        // FIXME we dont know the real MTU...
        // give them something to fragment about
        val = size_t(1500);
        return;

    case MBOARD_PROP_CLOCK_RATE:
        val = freq_t(get_master_clock_freq());
        return;

    case MBOARD_PROP_RX_DSP:
        throw std::runtime_error("unhandled prop in usrp2 mboard");

    case MBOARD_PROP_RX_DSP_NAMES:
        throw std::runtime_error("unhandled prop in usrp2 mboard");

    case MBOARD_PROP_TX_DSP:
        throw std::runtime_error("unhandled prop in usrp2 mboard");

    case MBOARD_PROP_TX_DSP_NAMES:
        throw std::runtime_error("unhandled prop in usrp2 mboard");

    case MBOARD_PROP_PPS_SOURCE:
        val = _pps_source;
        return;

    case MBOARD_PROP_PPS_SOURCE_NAMES:
        val = prop_names_t(_pps_source_dict.get_keys());
        return;

    case MBOARD_PROP_PPS_POLARITY:
        val = _pps_polarity;
        return;

    case MBOARD_PROP_REF_SOURCE:
        val = _ref_source;
        return;

    case MBOARD_PROP_REF_SOURCE_NAMES:
        val = prop_names_t(_ref_source_dict.get_keys());
        return;

    case MBOARD_PROP_TIME_NOW:
    case MBOARD_PROP_TIME_NEXT_PPS:
        throw std::runtime_error("Error: trying to get write-only property on usrp2 mboard");

    }
}

/***********************************************************************
 * Set Properties
 **********************************************************************/
void impl_base::set(const wax::obj &key, const wax::obj &val){
    //handle the get request conditioned on the key
    switch(wax::cast<mboard_prop_t>(key)){

    case MBOARD_PROP_PPS_SOURCE:{
            std::string name = wax::cast<std::string>(val);
            ASSERT_THROW(_pps_source_dict.has_key(name));
            _pps_source = name; //shadow
            update_clock_config();
        }
        return;

    case MBOARD_PROP_PPS_POLARITY:{
            std::string name = wax::cast<std::string>(val);
            ASSERT_THROW(_pps_polarity_dict.has_key(name));
            _pps_polarity = name; //shadow
            update_clock_config();
        }
        return;

    case MBOARD_PROP_REF_SOURCE:{
            std::string name = wax::cast<std::string>(val);
            ASSERT_THROW(_ref_source_dict.has_key(name));
            _ref_source = name; //shadow
            update_clock_config();
        }
        return;

    case MBOARD_PROP_NAME:
    case MBOARD_PROP_OTHERS:
    case MBOARD_PROP_MTU:
    case MBOARD_PROP_CLOCK_RATE:
    case MBOARD_PROP_RX_DSP:
    case MBOARD_PROP_RX_DSP_NAMES:
    case MBOARD_PROP_TX_DSP:
    case MBOARD_PROP_TX_DSP_NAMES:
    case MBOARD_PROP_RX_DBOARD:
    case MBOARD_PROP_RX_DBOARD_NAMES:
    case MBOARD_PROP_TX_DBOARD:
    case MBOARD_PROP_TX_DBOARD_NAMES:
    case MBOARD_PROP_PPS_SOURCE_NAMES:
    case MBOARD_PROP_REF_SOURCE_NAMES:
    case MBOARD_PROP_TIME_NOW:
    case MBOARD_PROP_TIME_NEXT_PPS:
        throw std::runtime_error("Error: trying to set read-only property on usrp2 mboard");

    }
}
