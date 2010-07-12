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

#ifndef INCLUDED_USRP2_IMPL_HPP
#define INCLUDED_USRP2_IMPL_HPP

#include "usrp2_iface.hpp"
#include "clock_ctrl.hpp"
#include "codec_ctrl.hpp"
#include "serdes_ctrl.hpp"
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
#include <uhd/transport/udp_simple.hpp> //mtu
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/usrp/dboard_manager.hpp>

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

/*!
 * The io helper class encapculates the max packet sizes and otw types.
 * The otw types are read-only for now, this will be reimplemented
 * when it becomes possible to change the otw type in the usrp2.
 */
class usrp2_io_helper{
public:
    usrp2_io_helper(void){
        //setup rx otw type
        _rx_otw_type.width = 16;
        _rx_otw_type.shift = 0;
        _rx_otw_type.byteorder = uhd::otw_type_t::BO_BIG_ENDIAN;

        //setup tx otw type
        _tx_otw_type.width = 16;
        _tx_otw_type.shift = 0;
        _tx_otw_type.byteorder = uhd::otw_type_t::BO_BIG_ENDIAN;
    }

    inline size_t get_max_send_samps_per_packet(void) const{
        return _max_tx_bytes_per_packet/_tx_otw_type.get_sample_size();
    }

    inline size_t get_max_recv_samps_per_packet(void) const{
        return _max_rx_bytes_per_packet/_rx_otw_type.get_sample_size();
    }

    inline const uhd::otw_type_t &get_rx_otw_type(void) const{
        return _rx_otw_type;
    }

    inline const uhd::otw_type_t &get_tx_otw_type(void) const{
        return _tx_otw_type;
    }

private:
    uhd::otw_type_t _rx_otw_type, _tx_otw_type;
    static const size_t _max_rx_bytes_per_packet = uhd::transport::udp_simple::mtu
        - uhd::transport::vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        - sizeof(uhd::transport::vrt::if_packet_info_t().tlr) //forced to have trailer
        + sizeof(uhd::transport::vrt::if_packet_info_t().cid) //no class id ever used
    ;
    static const size_t _max_tx_bytes_per_packet = uhd::transport::udp_simple::mtu
        - uhd::transport::vrt::max_if_hdr_words32*sizeof(boost::uint32_t)
        + sizeof(uhd::transport::vrt::if_packet_info_t().cid) //no class id ever used
    ;
};

/*!
 * USRP2 mboard implementation guts:
 * The implementation details are encapsulated here.
 * Handles properties on the mboard, dboard, dsps...
 */
class usrp2_mboard_impl : public wax::obj{
public:
    typedef boost::shared_ptr<usrp2_mboard_impl> sptr;

    //structors
    usrp2_mboard_impl(size_t index, uhd::transport::udp_simple::sptr, const usrp2_io_helper &);
    ~usrp2_mboard_impl(void);

    inline double get_master_clock_freq(void){
        return _clock_ctrl->get_master_clock_rate();
    }

private:
    size_t _index;
    int _rev_hi, _rev_lo;
    const usrp2_io_helper &_io_helper;

    //properties for this mboard
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    //interfaces
    usrp2_iface::sptr _iface;
    usrp2_clock_ctrl::sptr _clock_ctrl;
    usrp2_codec_ctrl::sptr _codec_ctrl;
    usrp2_serdes_ctrl::sptr _serdes_ctrl;

    //rx and tx dboard methods and objects
    uhd::usrp::dboard_manager::sptr _dboard_manager;
    uhd::usrp::dboard_iface::sptr _dboard_iface;
    void dboard_init(void);

    //methods and shadows for clock configuration
    uhd::clock_config_t _clock_config;
    void init_clock_config(void);
    void update_clock_config(void);
    void set_time_spec(const uhd::time_spec_t &time_spec, bool now);

    //properties interface for rx dboard
    void rx_dboard_get(const wax::obj &, wax::obj &);
    void rx_dboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _rx_dboard_proxy;
    uhd::prop_names_t _rx_subdevs_in_use;
    uhd::usrp::dboard_eeprom_t _rx_db_eeprom;

    //properties interface for tx dboard
    void tx_dboard_get(const wax::obj &, wax::obj &);
    void tx_dboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _tx_dboard_proxy;
    uhd::prop_names_t _tx_subdevs_in_use;
    uhd::usrp::dboard_eeprom_t _tx_db_eeprom;

    //methods and shadows for the ddc dsp
    std::vector<size_t> _allowed_decim_and_interp_rates;
    size_t _ddc_decim;
    double _ddc_freq;
    void init_ddc_config(void);
    void issue_ddc_stream_cmd(const uhd::stream_cmd_t &stream_cmd);

    //methods and shadows for the duc dsp
    size_t _duc_interp;
    double _duc_freq;
    void init_duc_config(void);

    //properties interface for ddc
    void ddc_get(const wax::obj &, wax::obj &);
    void ddc_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _rx_dsp_proxy;

    //properties interface for duc
    void duc_get(const wax::obj &, wax::obj &);
    void duc_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _tx_dsp_proxy;

};

/*!
 * USRP2 implementation guts:
 * The implementation details are encapsulated here.
 * Handles device properties and streaming...
 */
class usrp2_impl : public uhd::device{
public:
    /*!
     * Create a new usrp2 impl base.
     * \param ctrl_transports the udp transports for control
     * \param data_transports the udp transports for data
     */
    usrp2_impl(
        std::vector<uhd::transport::udp_simple::sptr> ctrl_transports,
        std::vector<uhd::transport::udp_zero_copy::sptr> data_transports
    );

    ~usrp2_impl(void);

    //the io interface
    size_t get_max_send_samps_per_packet(void) const{
        return _io_helper.get_max_send_samps_per_packet();
    }
    size_t send(
        const std::vector<const void *> &, size_t,
        const uhd::tx_metadata_t &,
        const uhd::io_type_t &,
        uhd::device::send_mode_t
    );
    size_t get_max_recv_samps_per_packet(void) const{
        return _io_helper.get_max_recv_samps_per_packet();
    }
    size_t recv(
        const std::vector<void *> &, size_t,
        uhd::rx_metadata_t &,
        const uhd::io_type_t &,
        uhd::device::recv_mode_t
    );

private:
    //device properties interface
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    //pointers to mboards on this device (think mimo setup)
    std::vector<usrp2_mboard_impl::sptr> _mboards;
    uhd::dict<std::string, usrp2_mboard_impl::sptr> _mboard_dict;

    //io impl methods and members
    std::vector<uhd::transport::udp_zero_copy::sptr> _data_transports;
    const usrp2_io_helper _io_helper;
    UHD_PIMPL_DECL(io_impl) _io_impl;
    void io_init(void);
};

#endif /* INCLUDED_USRP2_IMPL_HPP */
