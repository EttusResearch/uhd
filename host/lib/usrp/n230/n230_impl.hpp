//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_N230_IMPL_HPP
#define INCLUDED_N230_IMPL_HPP

#include <uhd/property_tree.hpp>
#include <uhd/device.hpp>
#include <uhd/usrp/subdev_spec.hpp>

#include "n230_device_args.hpp"
#include "n230_eeprom_manager.hpp"
#include "n230_resource_manager.hpp"
#include "n230_stream_manager.hpp"
#include "recv_packet_demuxer_3000.hpp"

namespace uhd { namespace usrp { namespace n230 {

class n230_impl : public uhd::device
{
public: //Functions
    // ctor and dtor
    n230_impl(const uhd::device_addr_t& device_addr);
    virtual ~n230_impl(void);

    //---------------------------------------------------------------------
    // uhd::device interface
    //
    static sptr make(const uhd::device_addr_t &hint, size_t which = 0);

    //! Make a new receive streamer from the streamer arguments
    virtual uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &args);

    //! Make a new transmit streamer from the streamer arguments
    virtual uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &args);

    //!Receive and asynchronous message from the device.
    virtual bool recv_async_msg(uhd::async_metadata_t &async_metadata, double timeout = 0.1);

    //!Registration methods the discovery and factory system.
    //[static void register_device(const find_t &find, const make_t &make)]
    static uhd::device_addrs_t n230_find(const uhd::device_addr_t &hint);
    static uhd::device::sptr n230_make(const uhd::device_addr_t &device_addr);
    //
    //---------------------------------------------------------------------

    typedef uhd::transport::bounded_buffer<uhd::async_metadata_t> async_md_type;

private:    //Functions
    void _initialize_property_tree(const fs_path& mb_path);
    void _initialize_radio_properties(const fs_path& mb_path, size_t instance);

    void _update_rx_subdev_spec(const uhd::usrp::subdev_spec_t &);
    void _update_tx_subdev_spec(const uhd::usrp::subdev_spec_t &);
    void _check_time_source(std::string);
    void _check_clock_source(std::string);

private:    //Classes and Members
    n230_device_args_t                        _dev_args;
    boost::shared_ptr<n230_resource_manager>  _resource_mgr;
    boost::shared_ptr<n230_eeprom_manager>    _eeprom_mgr;
    boost::shared_ptr<n230_stream_manager>    _stream_mgr;
};

}}} //namespace

#endif /* INCLUDED_N230_IMPL_HPP */
