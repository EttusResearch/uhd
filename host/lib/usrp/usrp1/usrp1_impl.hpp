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

#include "usrp1_iface.hpp"
#include "usrp1_ctrl.hpp"
#include "clock_ctrl.hpp"
#include "codec_ctrl.hpp"
#include "soft_time_ctrl.hpp"
#include <uhd/device.hpp>
#include <uhd/utils/pimpl.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/otw_type.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/transport/usb_zero_copy.hpp>

#ifndef INCLUDED_USRP1_IMPL_HPP
#define INCLUDED_USRP1_IMPL_HPP

/*!
 * Simple wax obj proxy class:
 * Provides a wax obj interface for a set and a get function.
 * This allows us to create nested properties structures
 * while maintaining flattened code within the implementation.
 */
class wax_obj_proxy : public wax::obj {
public:
    typedef boost::function<void(const wax::obj &, wax::obj &)>       get_t;
    typedef boost::function<void(const wax::obj &, const wax::obj &)> set_t;
    typedef boost::shared_ptr<wax_obj_proxy> sptr;

    static sptr make(const get_t &get, const set_t &set){
        return sptr(new wax_obj_proxy(get, set));
    }

private:
    get_t _get; set_t _set;
    wax_obj_proxy(const get_t &get, const set_t &set): _get(get), _set(set) {};
    void get(const wax::obj &key, wax::obj &val) {return _get(key, val);}
    void set(const wax::obj &key, const wax::obj &val) {return _set(key, val);}
};

/*!
 * USRP1 implementation guts:
 * The implementation details are encapsulated here.
 * Handles properties on the mboard, dboard, dsps...
 */
class usrp1_impl : public uhd::device {
public:
    //! used everywhere to differentiate slots/sides...
    enum dboard_slot_t{
        DBOARD_SLOT_A = 'A',
        DBOARD_SLOT_B = 'B'
    };
    //and a way to enumerate through a list of the above...
    static const std::vector<dboard_slot_t> _dboard_slots;

    //structors
    usrp1_impl(uhd::transport::usb_zero_copy::sptr data_transport,
               usrp_ctrl::sptr ctrl_transport);

    ~usrp1_impl(void);

    //the io interface
    size_t send(const std::vector<const void *> &,
                size_t,
                const uhd::tx_metadata_t &,
                const uhd::io_type_t &,
                send_mode_t, double);

    size_t recv(const std::vector<void *> &,
                size_t, uhd::rx_metadata_t &,
                const uhd::io_type_t &,
                recv_mode_t, double);

    size_t get_max_send_samps_per_packet(void) const;

    size_t get_max_recv_samps_per_packet(void) const;

    bool recv_async_msg(uhd::async_metadata_t &, double);

private:
    /*!
     * Make a usrp1 dboard interface.
     * \param iface the usrp1 interface object
     * \param clock the clock control interface
     * \param codec the codec control interface
     * \param dboard_slot the slot identifier
     * \param rx_dboard_id the db id for the rx board (used for evil dbsrx purposes)
     * \return a sptr to a new dboard interface
     */
    static uhd::usrp::dboard_iface::sptr make_dboard_iface(
        usrp1_iface::sptr iface,
        usrp1_clock_ctrl::sptr clock,
        usrp1_codec_ctrl::sptr codec,
        dboard_slot_t dboard_slot,
        const uhd::usrp::dboard_id_t &rx_dboard_id
    );

    //soft time control emulation
    uhd::usrp::soft_time_ctrl::sptr _soft_time_ctrl;

    //interface to ioctls and file descriptor
    usrp1_iface::sptr _iface;

    //handle io stuff
    UHD_PIMPL_DECL(io_impl) _io_impl;
    void io_init(void);
    void rx_stream_on_off(bool);
    void handle_overrun(size_t);

    //underrun and overrun poll intervals
    size_t _rx_samps_per_poll_interval;
    size_t _tx_samps_per_poll_interval; 

    //otw types
    uhd::otw_type_t _rx_otw_type;
    uhd::otw_type_t _tx_otw_type;

    //configuration shadows
    uhd::clock_config_t _clock_config;
    uhd::usrp::subdev_spec_t _rx_subdev_spec, _tx_subdev_spec;

    //clock control
    usrp1_clock_ctrl::sptr _clock_ctrl;

    //ad9862 codec control interface
    uhd::dict<dboard_slot_t, usrp1_codec_ctrl::sptr> _codec_ctrls;

    //codec properties interfaces
    void codec_init(void);
    void rx_codec_get(const wax::obj &, wax::obj &, dboard_slot_t);
    void rx_codec_set(const wax::obj &, const wax::obj &, dboard_slot_t);
    void tx_codec_get(const wax::obj &, wax::obj &, dboard_slot_t);
    void tx_codec_set(const wax::obj &, const wax::obj &, dboard_slot_t);
    uhd::dict<dboard_slot_t, wax_obj_proxy::sptr> _rx_codec_proxies, _tx_codec_proxies;

    //device functions and settings
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    //mboard functions and settings
    void mboard_init(void);
    void mboard_get(const wax::obj &, wax::obj &);
    void mboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _mboard_proxy;

    //xx dboard functions and settings
    void dboard_init(void);
    uhd::dict<dboard_slot_t, uhd::usrp::dboard_manager::sptr> _dboard_managers;
    uhd::dict<dboard_slot_t, uhd::usrp::dboard_iface::sptr> _dboard_ifaces;

    //rx dboard functions and settings
    uhd::dict<dboard_slot_t, uhd::usrp::dboard_eeprom_t> _rx_db_eeproms;
    void rx_dboard_get(const wax::obj &, wax::obj &, dboard_slot_t);
    void rx_dboard_set(const wax::obj &, const wax::obj &, dboard_slot_t);
    uhd::dict<dboard_slot_t, wax_obj_proxy::sptr> _rx_dboard_proxies;

    //tx dboard functions and settings
    uhd::dict<dboard_slot_t, uhd::usrp::dboard_eeprom_t> _tx_db_eeproms;
    void tx_dboard_get(const wax::obj &, wax::obj &, dboard_slot_t);
    void tx_dboard_set(const wax::obj &, const wax::obj &, dboard_slot_t);
    uhd::dict<dboard_slot_t, wax_obj_proxy::sptr> _tx_dboard_proxies;

    //rx dsp functions and settings
    void rx_dsp_init(void);
    void rx_dsp_get(const wax::obj &, wax::obj &);
    void rx_dsp_set(const wax::obj &, const wax::obj &);
    uhd::dict<std::string, double> _rx_dsp_freqs;
    size_t _rx_dsp_decim;
    wax_obj_proxy::sptr _rx_dsp_proxy;

    //tx dsp functions and settings
    void tx_dsp_init(void);
    void tx_dsp_get(const wax::obj &, wax::obj &);
    void tx_dsp_set(const wax::obj &, const wax::obj &);
    uhd::dict<std::string, double> _tx_dsp_freqs;
    size_t _tx_dsp_interp;
    wax_obj_proxy::sptr _tx_dsp_proxy;

    //transports
    uhd::transport::usb_zero_copy::sptr _data_transport;
    usrp_ctrl::sptr _ctrl_transport;

    //capabilities
    size_t get_num_ducs(void);
    size_t get_num_ddcs(void);
    bool has_rx_halfband(void);
    bool has_tx_halfband(void);
};

#endif /* INCLUDED_USRP1_IMPL_HPP */
