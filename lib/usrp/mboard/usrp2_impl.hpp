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
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/transport/udp.hpp>
#include "usrp2_fw_common.h"

#ifndef INCLUDED_USRP2_IMPL_HPP
#define INCLUDED_USRP2_IMPL_HPP

class usrp2_impl : boost::noncopyable{
public:
    typedef boost::shared_ptr<usrp2_impl> sptr;

    usrp2_impl(
        uhd::transport::udp::sptr ctrl_transport,
        uhd::transport::udp::sptr data_transport
    );

    ~usrp2_impl(void);

    usrp2_ctrl_data_t ctrl_send_and_recv(const usrp2_ctrl_data_t &);

private:
    uhd::transport::udp::sptr _ctrl_transport;
    uhd::transport::udp::sptr _data_transport;

    uint32_t _ctrl_seq_num;

    uhd::usrp::dboard::manager::sptr   _dboard_manager;
    uhd::usrp::dboard::interface::sptr _dboard_interface;
};

#endif /* INCLUDED_USRP2_IMPL_HPP */
