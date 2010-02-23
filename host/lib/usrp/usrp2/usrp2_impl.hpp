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

#include <uhd/usrp/usrp2.hpp>
#include <uhd/dict.hpp>
#include <uhd/props.hpp>
#include <uhd/time_spec.hpp>
#include <boost/thread.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>
#include <uhd/transport/udp.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include "fw_common.h"

#ifndef INCLUDED_USRP2_IMPL_HPP
#define INCLUDED_USRP2_IMPL_HPP

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

    wax_obj_proxy(void){
        /* NOP */
    }

    wax_obj_proxy(const get_t &get, const set_t &set){
        _get = get;
        _set = set;
    };

    ~wax_obj_proxy(void){
        /* NOP */
    }

    void get(const wax::obj &key, wax::obj &val){
        return _get(key, val);
    }

    void set(const wax::obj &key, const wax::obj &val){
        return _set(key, val);
    }

private:
    get_t _get;
    set_t _set;
};

/*!
 * USRP2 implementation guts:
 * The implementation details are encapsulated here.
 * Handles properties on the mboard, dboard, dsps...
 */
class usrp2_impl : public uhd::device{
public:
    typedef boost::shared_ptr<usrp2_impl> sptr;

    /*!
     * Create a new usrp2 impl base.
     * \param ctrl_transport the udp transport for control
     * \param data_transport the udp transport for data
     */
    usrp2_impl(
        uhd::transport::udp::sptr ctrl_transport,
        uhd::transport::udp::sptr data_transport
    );

    ~usrp2_impl(void);

    //properties interface
    void get(const wax::obj &, wax::obj &);
    void set(const wax::obj &, const wax::obj &);

    //performs a control transaction
    usrp2_ctrl_data_t ctrl_send_and_recv(const usrp2_ctrl_data_t &);

    //misc access methods
    double get_master_clock_freq(void);

    //the io interface
    size_t send(const boost::asio::const_buffer &, const uhd::metadata_t &, const std::string &);
    size_t recv(const boost::asio::mutable_buffer &, uhd::metadata_t &, const std::string &);

private:
    //udp transports for control and data
    uhd::transport::udp::sptr _ctrl_transport;
    uhd::transport::udp::sptr _data_transport;

    //private vars for dealing with send/recv control
    uint32_t _ctrl_seq_num;
    boost::mutex _ctrl_mutex;

    //methods and shadows for clock configuration
    std::string _pps_source, _pps_polarity, _ref_source;
    void init_clock_config(void);
    void update_clock_config(void);

    //mappings from clock config strings to over the wire enums
    uhd::dict<std::string, usrp2_pps_source_t>   _pps_source_dict;
    uhd::dict<std::string, usrp2_pps_polarity_t> _pps_polarity_dict;
    uhd::dict<std::string, usrp2_ref_source_t>   _ref_source_dict;

    //rx and tx dboard methods and objects
    uhd::usrp::dboard_manager::sptr _dboard_manager;
    void dboard_init(void);

    //properties for the mboard
    void mboard_init(void);
    void mboard_get(const wax::obj &, wax::obj &);
    void mboard_set(const wax::obj &, const wax::obj &);
    uhd::dict<std::string, wax_obj_proxy> _mboards;

    //properties interface for rx dboard
    void rx_dboard_get(const wax::obj &, wax::obj &);
    void rx_dboard_set(const wax::obj &, const wax::obj &);
    uhd::dict<std::string, wax_obj_proxy> _rx_dboards;

    //properties interface for tx dboard
    void tx_dboard_get(const wax::obj &, wax::obj &);
    void tx_dboard_set(const wax::obj &, const wax::obj &);
    uhd::dict<std::string, wax_obj_proxy> _tx_dboards;

    //methods and shadows for the ddc dsp
    std::vector<size_t> _allowed_decim_and_interp_rates;
    size_t _ddc_decim;
    uhd::freq_t _ddc_freq;
    bool _ddc_enabled;
    uhd::time_spec_t _ddc_stream_at;
    void init_ddc_config(void);
    void update_ddc_config(void);
    void update_ddc_enabled(void);

    //methods and shadows for the duc dsp
    size_t _duc_interp;
    uhd::freq_t _duc_freq;
    void init_duc_config(void);
    void update_duc_config(void);

    //properties interface for ddc
    void ddc_get(const wax::obj &, wax::obj &);
    void ddc_set(const wax::obj &, const wax::obj &);
    uhd::dict<std::string, wax_obj_proxy> _rx_dsps;

    //properties interface for duc
    void duc_get(const wax::obj &, wax::obj &);
    void duc_set(const wax::obj &, const wax::obj &);
    uhd::dict<std::string, wax_obj_proxy> _tx_dsps;

};

#endif /* INCLUDED_USRP2_IMPL_HPP */
