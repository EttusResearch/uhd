//
// Copyright 2011 Ettus Research LLC
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

#include "b100_iface.hpp"
#include "b100_ctrl.hpp"
#include "clock_ctrl.hpp"
#include "codec_ctrl.hpp"
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

#ifndef INCLUDED_B100_IMPL_HPP
#define INCLUDED_B100_IMPL_HPP

static const std::string     B100_FW_FILE_NAME = "usrp_b100_fw.ihx";
static const std::string     B100_FPGA_FILE_NAME = "usrp_b100_fpga.bin";
static const boost::uint16_t B100_FW_COMPAT_NUM = 0x02;
static const boost::uint16_t B100_FPGA_COMPAT_NUM = 0x05;
static const size_t          B100_NUM_RX_DSPS = 2;
static const size_t          B100_NUM_TX_DSPS = 1;
static const boost::uint32_t B100_DSP_SID_BASE = 2; //leave room for other dsp (increments by 1)
static const boost::uint32_t B100_ASYNC_SID = 1;

/*!
 * Make a b100 dboard interface.
 * \param iface the b100 interface object
 * \param clock the clock control interface
 * \param codec the codec control interface
 * \return a sptr to a new dboard interface
 */
uhd::usrp::dboard_iface::sptr make_b100_dboard_iface(
    b100_iface::sptr iface,
    b100_clock_ctrl::sptr clock,
    b100_codec_ctrl::sptr codec
);

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
class b100_impl : public uhd::device {
public:
    //structors
    b100_impl(uhd::transport::usb_zero_copy::sptr data_transport,
                uhd::transport::usb_zero_copy::sptr ctrl_transport,
                uhd::usrp::fx2_ctrl::sptr fx2_ctrl,
                double master_clock_rate);

    ~b100_impl(void);

    //the io interface
    size_t send(const send_buffs_type &,
                size_t,
                const uhd::tx_metadata_t &,
                const uhd::io_type_t &,
                send_mode_t, double);

    size_t recv(const recv_buffs_type &,
                size_t, uhd::rx_metadata_t &,
                const uhd::io_type_t &,
                recv_mode_t, double);

    size_t get_max_send_samps_per_packet(void) const;

    size_t get_max_recv_samps_per_packet(void) const;

    bool recv_async_msg(uhd::async_metadata_t &, double);

private:
    //clock control
    b100_clock_ctrl::sptr _clock_ctrl;

    //interface to ioctls and file descriptor
    b100_iface::sptr _iface;

    //handle io stuff
    uhd::transport::zero_copy_if::sptr _data_transport;
    UHD_PIMPL_DECL(io_impl) _io_impl;
    void update_xport_channel_mapping(void);
    void io_init(void);
    void handle_overrun(size_t);

    //otw types
    uhd::otw_type_t _recv_otw_type;
    uhd::otw_type_t _send_otw_type;

    //configuration shadows
    uhd::clock_config_t _clock_config;
    uhd::usrp::subdev_spec_t _rx_subdev_spec, _tx_subdev_spec;

    //ad9862 codec control interface
    b100_codec_ctrl::sptr _codec_ctrl;

    //codec properties interfaces
    void codec_init(void);
    void rx_codec_get(const wax::obj &, wax::obj &);
    void rx_codec_set(const wax::obj &, const wax::obj &);
    void tx_codec_get(const wax::obj &, wax::obj &);
    void tx_codec_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _rx_codec_proxy, _tx_codec_proxy;

    //device functions and settings
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    //mboard functions and settings
    void mboard_init(void);
    void mboard_get(const wax::obj &, wax::obj &);
    void mboard_set(const wax::obj &, const wax::obj &);
    void update_clock_config(void);
    wax_obj_proxy::sptr _mboard_proxy;
    
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
        b100_iface::sptr iface,
        b100_clock_ctrl::sptr clock,
        b100_codec_ctrl::sptr codec,
        const uhd::usrp::dboard_id_t &rx_dboard_id
    );

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

    //transports
    b100_ctrl::sptr _fpga_ctrl;
    uhd::usrp::fx2_ctrl::sptr _fx2_ctrl;

};

#endif /* INCLUDED_b100_IMPL_HPP */
