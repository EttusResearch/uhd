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

#include <uhd/usrp/dboard/manager.hpp>
#include <boost/format.hpp>
#include <boost/bind.hpp>
#include <uhd/utils.hpp>
#include <iostream>
#include "usrp2_impl.hpp"

using namespace uhd;

/***********************************************************************
 * Structors
 **********************************************************************/
usrp2_impl::usrp2_impl(
    uhd::transport::udp::sptr ctrl_transport,
    uhd::transport::udp::sptr data_transport
){
    _ctrl_transport = ctrl_transport;
    _data_transport = data_transport;

    //load the allowed decim/interp rates
    //_USRP2_RATES = range(4, 128+1, 1) + range(130, 256+1, 2) + range(260, 512+1, 4)
    _allowed_decim_and_interp_rates.clear();
    for (size_t i = 4; i <= 128; i+=1){
        _allowed_decim_and_interp_rates.push_back(i);
    }
    for (size_t i = 130; i <= 256; i+=2){
        _allowed_decim_and_interp_rates.push_back(i);
    }
    for (size_t i = 260; i <= 512; i+=4){
        _allowed_decim_and_interp_rates.push_back(i);
    }

    //init the tx and rx dboards
    dboard_init();

    //init the ddc
    init_ddc_config();

    //init the duc
    init_duc_config();

    //initialize the clock configuration
    init_clock_config();
}

usrp2_impl::~usrp2_impl(void){
    /* NOP */
}

/***********************************************************************
 * Misc Access Methods
 **********************************************************************/
double usrp2_impl::get_master_clock_freq(void){
    return 100e6;
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
