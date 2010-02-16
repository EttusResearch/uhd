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

#include <boost/thread.hpp>
#include <boost/format.hpp>
#include <uhd/utils.hpp>
#include <iostream>
#include "usrp2_impl.hpp"
#include "usrp2_dboard_interface.hpp"

using namespace uhd::usrp;

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
    _dboard_interface = dboard::interface::sptr(
        new usrp2_dboard_interface(this)
    );
    _dboard_manager = dboard::manager::sptr(
        new dboard::manager(rx_dboard_id, tx_dboard_id, _dboard_interface)
    );
}

usrp2_impl::~usrp2_impl(void){
    /* NOP */
}

usrp2_ctrl_data_t usrp2_impl::ctrl_send_and_recv(const usrp2_ctrl_data_t &out_data){
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
