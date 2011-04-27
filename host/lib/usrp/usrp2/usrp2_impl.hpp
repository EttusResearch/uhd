//
// Copyright 2010-2011 Ettus Research LLC
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

#ifndef INCLUDED_USRP2_IMPL_HPP
#define INCLUDED_USRP2_IMPL_HPP

#include "usrp2_iface.hpp"
#include "clock_ctrl.hpp"
#include "codec_ctrl.hpp"
#include <uhd/usrp/gps_ctrl.hpp>
#include <uhd/device.hpp>
#include <uhd/utils/pimpl.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/otw_type.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/usrp/subdev_spec.hpp>

/*!
 * Make a usrp2 dboard interface.
 * \param iface the usrp2 interface object
 * \param clk_ctrl the clock control object
 * \return a sptr to a new dboard interface
 */
uhd::usrp::dboard_iface::sptr make_usrp2_dboard_iface(
    usrp2_iface::sptr iface,
    usrp2_clock_ctrl::sptr clk_ctrl
);

/*!
 * Simple wax obj proxy class:
 * Provides a wax obj interface for a set and a get function.
 * This allows us to create nested properties structures
 * while maintaining flattened code within the implementation.
 */
class wax_obj_proxy : public wax::obj{
public:
    typedef boost::function<void(const wax::obj &, wax::obj &)>       get_t;
    typedef boost::function<void(const wax::obj &, const wax::obj &)> set_t;
    typedef boost::shared_ptr<wax_obj_proxy> sptr;

    static sptr make(const get_t &get, const set_t &set){
        return sptr(new wax_obj_proxy(get, set));
    }

private:
    get_t _get; set_t _set;
    wax_obj_proxy(const get_t &get, const set_t &set): _get(get), _set(set){};
    void get(const wax::obj &key, wax::obj &val){return _get(key, val);}
    void set(const wax::obj &key, const wax::obj &val){return _set(key, val);}
};

class usrp2_impl;

/*!
 * USRP2 mboard implementation guts:
 * The implementation details are encapsulated here.
 * Handles properties on the mboard, dboard, dsps...
 */
class usrp2_mboard_impl : public wax::obj{
public:
    typedef boost::shared_ptr<usrp2_mboard_impl> sptr;

    static const size_t NUM_RX_DSPS = 2;
    static const size_t NUM_TX_DSPS = 1;
    static const size_t MAX_NUM_DSPS = 2;

    //structors
    usrp2_mboard_impl(
        const uhd::device_addr_t &device_addr,
        size_t index, usrp2_impl &device
    );
    ~usrp2_mboard_impl(void);

    inline double get_master_clock_freq(void){
        return _clock_ctrl->get_master_clock_rate();
    }

    void handle_overflow(size_t);

private:
    size_t _index;
    usrp2_impl &_device;
    bool _mimo_clocking_mode_is_master;

    //interfaces
    usrp2_iface::sptr _iface;
    usrp2_clock_ctrl::sptr _clock_ctrl;
    usrp2_codec_ctrl::sptr _codec_ctrl;
    gps_ctrl::sptr _gps_ctrl;

    //properties for this mboard
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);
    uhd::usrp::subdev_spec_t _rx_subdev_spec, _tx_subdev_spec;

    //rx and tx dboard methods and objects
    uhd::usrp::dboard_manager::sptr _dboard_manager;
    uhd::usrp::dboard_iface::sptr _dboard_iface;
    void dboard_init(void);

    //methods and shadows for clock configuration
    uhd::clock_config_t _clock_config;
    void update_clock_config(void);
    void set_time_spec(const uhd::time_spec_t &time_spec, bool now);

    //properties interface for the codec
    void codec_init(void);
    void rx_codec_get(const wax::obj &, wax::obj &);
    void rx_codec_set(const wax::obj &, const wax::obj &);
    void tx_codec_get(const wax::obj &, wax::obj &);
    void tx_codec_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _rx_codec_proxy;
    wax_obj_proxy::sptr _tx_codec_proxy;

    void rx_codec_set_gain(double, const std::string &);
    uhd::dict<std::string, double> _codec_rx_gains;

    //properties interface for rx dboard
    void rx_dboard_get(const wax::obj &, wax::obj &);
    void rx_dboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _rx_dboard_proxy;
    uhd::usrp::dboard_eeprom_t _rx_db_eeprom;

    //properties interface for tx dboard
    void tx_dboard_get(const wax::obj &, wax::obj &);
    void tx_dboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _tx_dboard_proxy;
    uhd::usrp::dboard_eeprom_t _tx_db_eeprom, _gdb_eeprom;

    //methods and shadows for the dsps
    UHD_PIMPL_DECL(dsp_impl) _dsp_impl;
    void dsp_init(void);
    void issue_ddc_stream_cmd(const uhd::stream_cmd_t &, size_t);

    //properties interface for ddc
    void ddc_get(const wax::obj &, wax::obj &, size_t);
    void ddc_set(const wax::obj &, const wax::obj &, size_t);
    uhd::dict<std::string, wax_obj_proxy::sptr> _rx_dsp_proxies;

    //properties interface for duc
    void duc_get(const wax::obj &, wax::obj &, size_t);
    void duc_set(const wax::obj &, const wax::obj &, size_t);
    uhd::dict<std::string, wax_obj_proxy::sptr> _tx_dsp_proxies;

};

/*!
 * USRP2 implementation guts:
 * The implementation details are encapsulated here.
 * Handles device properties and streaming...
 */
class usrp2_impl : public uhd::device{
public:
    static const size_t sram_bytes = size_t(1 << 20);
    static const boost::uint32_t RECV_SID = 1;
    static const boost::uint32_t ASYNC_SID = 2;

    usrp2_impl(const uhd::device_addr_t &);

    ~usrp2_impl(void);

    //the io interface
    size_t send(
        const send_buffs_type &, size_t,
        const uhd::tx_metadata_t &, const uhd::io_type_t &,
        uhd::device::send_mode_t, double
    );
    size_t recv(
        const recv_buffs_type &, size_t,
        uhd::rx_metadata_t &, const uhd::io_type_t &,
        uhd::device::recv_mode_t, double
    );
    size_t get_max_send_samps_per_packet(void) const;
    size_t get_max_recv_samps_per_packet(void) const;
    bool recv_async_msg(uhd::async_metadata_t &, double);

    void update_xport_channel_mapping(void);

    //public frame sizes, set by mboard, used by io impl
    size_t recv_frame_size, send_frame_size;

    std::vector<uhd::transport::zero_copy_if::sptr> dsp_xports;
    std::vector<uhd::transport::zero_copy_if::sptr> err_xports;

private:
    //device properties interface
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    //pointers to mboards on this device (think mimo setup)
    std::vector<usrp2_mboard_impl::sptr> _mboards;
    uhd::dict<std::string, usrp2_mboard_impl::sptr> _mboard_dict;

    //io impl methods and members
    uhd::otw_type_t _rx_otw_type, _tx_otw_type;
    UHD_PIMPL_DECL(io_impl) _io_impl;
    void io_init(void);
    void handle_overflow(size_t);
};

#endif /* INCLUDED_USRP2_IMPL_HPP */
