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

#include "usrp_e100_iface.hpp"
#include "clock_ctrl.hpp"
#include "codec_ctrl.hpp"
#include <uhd/device.hpp>
#include <uhd/utils/pimpl.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/types/otw_type.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/transport/zero_copy.hpp>

#ifndef INCLUDED_USRP_E100_IMPL_HPP
#define INCLUDED_USRP_E100_IMPL_HPP

uhd::transport::zero_copy_if::sptr usrp_e100_make_mmap_zero_copy(usrp_e100_iface::sptr iface);

static const boost::uint16_t USRP_E_FPGA_COMPAT_NUM = 0x03;

//! load an fpga image from a bin file into the usrp-e fpga
extern void usrp_e100_load_fpga(const std::string &bin_file);

/*!
 * Make a usrp-e dboard interface.
 * \param iface the usrp-e interface object
 * \param clock the clock control interface
 * \param codec the codec control interface
 * \return a sptr to a new dboard interface
 */
uhd::usrp::dboard_iface::sptr make_usrp_e100_dboard_iface(
    usrp_e100_iface::sptr iface,
    usrp_e100_clock_ctrl::sptr clock,
    usrp_e100_codec_ctrl::sptr codec
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

/*!
 * USRP-E100 implementation guts:
 * The implementation details are encapsulated here.
 * Handles properties on the mboard, dboard, dsps...
 */
class usrp_e100_impl : public uhd::device{
public:
    //structors
    usrp_e100_impl(usrp_e100_iface::sptr, const uhd::device_addr_t &);
    ~usrp_e100_impl(void);

    //the io interface
    size_t send(const send_buffs_type &, size_t, const uhd::tx_metadata_t &, const uhd::io_type_t &, send_mode_t, double);
    size_t recv(const recv_buffs_type &, size_t, uhd::rx_metadata_t &, const uhd::io_type_t &, recv_mode_t, double);
    bool recv_async_msg(uhd::async_metadata_t &, double);
    size_t get_max_send_samps_per_packet(void) const;
    size_t get_max_recv_samps_per_packet(void) const;

private:
    //interface to ioctls and file descriptor
    usrp_e100_iface::sptr _iface;

    //handle io stuff
    uhd::transport::zero_copy_if::sptr _data_xport;
    UHD_PIMPL_DECL(io_impl) _io_impl;
    size_t _recv_frame_size, _send_frame_size;
    uhd::otw_type_t _send_otw_type, _recv_otw_type;
    void io_init(void);
    void issue_stream_cmd(const uhd::stream_cmd_t &stream_cmd);
    void handle_overrun(size_t);

    //configuration shadows
    uhd::clock_config_t _clock_config;

    //ad9522 clock control
    usrp_e100_clock_ctrl::sptr _clock_ctrl;

    //ad9862 codec control
    usrp_e100_codec_ctrl::sptr _codec_ctrl;

    //device functions and settings
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    //mboard functions and settings
    void mboard_init(void);
    void mboard_get(const wax::obj &, wax::obj &);
    void mboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _mboard_proxy;
    uhd::usrp::subdev_spec_t _rx_subdev_spec, _tx_subdev_spec;

    //xx dboard functions and settings
    void dboard_init(void);
    uhd::usrp::dboard_manager::sptr _dboard_manager;
    uhd::usrp::dboard_iface::sptr _dboard_iface;

    //rx dboard functions and settings
    uhd::usrp::dboard_eeprom_t _rx_db_eeprom;
    void rx_dboard_get(const wax::obj &, wax::obj &);
    void rx_dboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _rx_dboard_proxy;

    //tx dboard functions and settings
    uhd::usrp::dboard_eeprom_t _tx_db_eeprom, _gdb_eeprom;
    void tx_dboard_get(const wax::obj &, wax::obj &);
    void tx_dboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _tx_dboard_proxy;

    //rx ddc functions and settings
    void rx_ddc_init(void);
    void rx_ddc_get(const wax::obj &, wax::obj &);
    void rx_ddc_set(const wax::obj &, const wax::obj &);
    double _ddc_freq; size_t _ddc_decim;
    wax_obj_proxy::sptr _rx_ddc_proxy;

    //tx duc functions and settings
    void tx_duc_init(void);
    void tx_duc_get(const wax::obj &, wax::obj &);
    void tx_duc_set(const wax::obj &, const wax::obj &);
    double _duc_freq; size_t _duc_interp;
    wax_obj_proxy::sptr _tx_duc_proxy;

    //codec functions and settings
    void codec_init(void);
    void rx_codec_get(const wax::obj &, wax::obj &);
    void rx_codec_set(const wax::obj &, const wax::obj &);
    void tx_codec_get(const wax::obj &, wax::obj &);
    void tx_codec_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _rx_codec_proxy, _tx_codec_proxy;
    
    //clock control functions and settings
    void init_clock_config(void);
    void update_clock_config(void);
};

#endif /* INCLUDED_USRP_E100_IMPL_HPP */
