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

#include <boost/utility.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <uhd/transport/udp.hpp>
#include <uhd/dict.hpp>
#include "dboard_impl.hpp"
#include "fw_common.h"

#ifndef INCLUDED_IMPL_BASE_HPP
#define INCLUDED_IMPL_BASE_HPP

class impl_base : boost::noncopyable, public wax::obj{
public:
    typedef boost::shared_ptr<impl_base> sptr;

    /*!
     * Create a new usrp2 impl base.
     * \param ctrl_transport the udp transport for control
     * \param data_transport the udp transport for data
     */
    impl_base(
        uhd::transport::udp::sptr ctrl_transport,
        uhd::transport::udp::sptr data_transport
    );

    ~impl_base(void);

    //performs a control transaction
    usrp2_ctrl_data_t ctrl_send_and_recv(const usrp2_ctrl_data_t &);

    //properties access methods
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    //misc access methods
    double get_master_clock_freq(void);
    void update_clock_config(void);

private:
    //udp transports for control and data
    uhd::transport::udp::sptr _ctrl_transport;
    uhd::transport::udp::sptr _data_transport;

    //private vars for dealing with send/recv control
    uint32_t _ctrl_seq_num;
    boost::mutex _ctrl_mutex;

    //containers for the dboard objects
    uhd::dict<std::string, dboard_impl::sptr> _rx_dboards;
    uhd::dict<std::string, dboard_impl::sptr> _tx_dboards;

    //shadows for various settings
    std::string _pps_source, _pps_polarity, _ref_source;

    //mappings from clock config strings to over the wire enums
    uhd::dict<std::string, usrp2_pps_source_t>   _pps_source_dict;
    uhd::dict<std::string, usrp2_pps_polarity_t> _pps_polarity_dict;
    uhd::dict<std::string, usrp2_ref_source_t>   _ref_source_dict;
};

#endif /* INCLUDED_IMPL_BASE_HPP */
