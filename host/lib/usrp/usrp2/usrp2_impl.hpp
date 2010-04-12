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

#include <uhd/usrp/usrp2.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/otw_type.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/clock_config.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <boost/assign/list_of.hpp>
#include <uhd/transport/vrt.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include "fw_common.h"

class usrp2_impl; //dummy class declaration

/*!
 * Make a usrp2 dboard interface.
 * \param impl a pointer to the usrp2 impl object
 * \return a sptr to a new dboard interface
 */
uhd::usrp::dboard_interface::sptr make_usrp2_dboard_interface(usrp2_impl *impl);

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

    ~wax_obj_proxy(void){
        /* NOP */
    }

private:
    get_t _get;
    set_t _set;

    wax_obj_proxy(const get_t &get, const set_t &set){
        _get = get;
        _set = set;
    };

    void get(const wax::obj &key, wax::obj &val){
        return _get(key, val);
    }

    void set(const wax::obj &key, const wax::obj &val){
        return _set(key, val);
    }
};

/*!
 * USRP2 implementation guts:
 * The implementation details are encapsulated here.
 * Handles properties on the mboard, dboard, dsps...
 */
class usrp2_impl : public uhd::device{
public:
    /*!
     * Create a new usrp2 impl base.
     * \param ctrl_transport the udp transport for control
     * \param data_transport the udp transport for data
     */
    usrp2_impl(
        uhd::transport::udp_simple::sptr ctrl_transport,
        uhd::transport::udp_zero_copy::sptr data_transport
    );

    ~usrp2_impl(void);

    //performs a control transaction
    usrp2_ctrl_data_t ctrl_send_and_recv(const usrp2_ctrl_data_t &);

    //peek and poke registers
    void poke32(boost::uint32_t addr, boost::uint32_t data);
    boost::uint32_t peek32(boost::uint32_t addr);

    void poke16(boost::uint32_t addr, boost::uint16_t data);
    boost::uint16_t peek16(boost::uint32_t addr);

    //spi read and write
    boost::uint32_t transact_spi(
        int which_slave,
        const uhd::usrp::spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits,
        bool readback
    );

    //misc access methods
    double get_master_clock_freq(void);

    //the io interface
    size_t send(const boost::asio::const_buffer &, const uhd::tx_metadata_t &, const uhd::io_type_t &);
    size_t recv(const boost::asio::mutable_buffer &, uhd::rx_metadata_t &, const uhd::io_type_t &);

private:
    //device properties interface
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    //the raw io interface (samples are in the usrp2 native format)
    void recv_raw(uhd::rx_metadata_t &);
    uhd::dict<boost::uint32_t, size_t> _tx_stream_id_to_packet_seq;
    uhd::dict<boost::uint32_t, size_t> _rx_stream_id_to_packet_seq;
    static const size_t _mtu = 1500; //FIXME we have no idea
    static const size_t _hdrs = (2 + 14 + 20 + 8); //size of headers (pad, eth, ip, udp)
    static const size_t _max_rx_samples_per_packet =
        (_mtu - _hdrs)/sizeof(boost::uint32_t) -
        USRP2_HOST_RX_VRT_HEADER_WORDS32 -
        USRP2_HOST_RX_VRT_TRAILER_WORDS32
    ;
    static const size_t _max_tx_samples_per_packet =
        (_mtu - _hdrs)/sizeof(boost::uint32_t) -
        uhd::transport::vrt::max_header_words32
    ;
    uhd::transport::managed_recv_buffer::sptr _rx_smart_buff;
    boost::asio::const_buffer _rx_copy_buff;
    size_t _fragment_offset_in_samps;
    uhd::otw_type_t _otw_type;
    void io_init(void);

    //udp transports for control and data
    uhd::transport::udp_simple::sptr _ctrl_transport;
    uhd::transport::udp_zero_copy::sptr _data_transport;

    //private vars for dealing with send/recv control
    boost::uint32_t _ctrl_seq_num;
    boost::mutex _ctrl_mutex;

    //methods and shadows for clock configuration
    uhd::clock_config_t _clock_config;
    void init_clock_config(void);
    void update_clock_config(void);
    void set_time_spec(const uhd::time_spec_t &time_spec, bool now);

    //rx and tx dboard methods and objects
    uhd::usrp::dboard_manager::sptr _dboard_manager;
    void dboard_init(void);

    //properties for the mboard
    void mboard_init(void);
    void mboard_get(const wax::obj &, wax::obj &);
    void mboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _mboard_proxy;

    //properties interface for rx dboard
    void rx_dboard_get(const wax::obj &, wax::obj &);
    void rx_dboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _rx_dboard_proxy;
    uhd::prop_names_t _rx_subdevs_in_use;

    //properties interface for tx dboard
    void tx_dboard_get(const wax::obj &, wax::obj &);
    void tx_dboard_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _tx_dboard_proxy;
    uhd::prop_names_t _tx_subdevs_in_use;
    void update_rx_mux_config(void);
    void update_tx_mux_config(void);

    //methods and shadows for the ddc dsp
    std::vector<size_t> _allowed_decim_and_interp_rates;
    size_t _ddc_decim;
    double _ddc_freq;
    void init_ddc_config(void);
    void update_ddc_config(void);
    void issue_ddc_stream_cmd(const uhd::stream_cmd_t &stream_cmd);

    //methods and shadows for the duc dsp
    size_t _duc_interp;
    double _duc_freq;
    void init_duc_config(void);
    void update_duc_config(void);

    //properties interface for ddc
    void ddc_get(const wax::obj &, wax::obj &);
    void ddc_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _rx_dsp_proxy;

    //properties interface for duc
    void duc_get(const wax::obj &, wax::obj &);
    void duc_set(const wax::obj &, const wax::obj &);
    wax_obj_proxy::sptr _tx_dsp_proxy;

};

#endif /* INCLUDED_USRP2_IMPL_HPP */
