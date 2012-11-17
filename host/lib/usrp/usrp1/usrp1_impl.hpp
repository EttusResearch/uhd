//
// Copyright 2010-2012 Ettus Research LLC
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
#include "codec_ctrl.hpp"
#include "soft_time_ctrl.hpp"
#include <uhd/device.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/utils/pimpl.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/otw_type.hpp>
#include <uhd/types/clock_config.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/transport/usb_zero_copy.hpp>
#include <boost/foreach.hpp>
#include <boost/weak_ptr.hpp>
#include <complex>

#ifndef INCLUDED_USRP1_IMPL_HPP
#define INCLUDED_USRP1_IMPL_HPP

static const std::string USRP1_EEPROM_MAP_KEY = "B000";

#define FR_RB_CAPS          3
#define FR_MODE             13
#define FR_DEBUG_EN         14
#define FR_DC_OFFSET_CL_EN  15
#define FR_ADC_OFFSET_0     16
#define FR_ADC_OFFSET_1     17
#define FR_ADC_OFFSET_2     18
#define FR_ADC_OFFSET_3     19

#define I2C_DEV_EEPROM      0x50
#define I2C_ADDR_BOOT       (I2C_DEV_EEPROM | 0x0)
#define I2C_ADDR_TX_A       (I2C_DEV_EEPROM | 0x4)
#define I2C_ADDR_RX_A       (I2C_DEV_EEPROM | 0x5)
#define I2C_ADDR_TX_B       (I2C_DEV_EEPROM | 0x6)
#define I2C_ADDR_RX_B       (I2C_DEV_EEPROM | 0x7)

#define SPI_ENABLE_CODEC_A  0x02
#define SPI_ENABLE_CODEC_B  0x04

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
    usrp1_impl(const uhd::device_addr_t &);
    ~usrp1_impl(void);

    //the io interface
    uhd::rx_streamer::sptr get_rx_stream(const uhd::stream_args_t &args);
    uhd::tx_streamer::sptr get_tx_stream(const uhd::stream_args_t &args);
    bool recv_async_msg(uhd::async_metadata_t &, double);

private:
    uhd::property_tree::sptr _tree;

    //device properties interface
    uhd::property_tree::sptr get_tree(void) const{
        return _tree;
    }

    //controllers
    uhd::usrp::fx2_ctrl::sptr _fx2_ctrl;
    usrp1_iface::sptr _iface;
    uhd::usrp::soft_time_ctrl::sptr _soft_time_ctrl;
    uhd::transport::usb_zero_copy::sptr _data_transport;
    struct db_container_type{
        usrp1_codec_ctrl::sptr codec;
        uhd::usrp::dboard_iface::sptr dboard_iface;
        uhd::usrp::dboard_manager::sptr dboard_manager;
    };
    uhd::dict<std::string, db_container_type> _dbc;

    double _master_clock_rate; //clock rate shadow

    //weak pointers to streamers for update purposes
    boost::weak_ptr<uhd::rx_streamer> _rx_streamer;
    boost::weak_ptr<uhd::tx_streamer> _tx_streamer;

    void set_mb_eeprom(const uhd::usrp::mboard_eeprom_t &);
    void set_db_eeprom(const std::string &, const std::string &, const uhd::usrp::dboard_eeprom_t &);
    double update_rx_codec_gain(const std::string &, const double); //sets A and B at once
    void update_rx_subdev_spec(const uhd::usrp::subdev_spec_t &);
    void update_tx_subdev_spec(const uhd::usrp::subdev_spec_t &);
    double update_rx_samp_rate(size_t dspno, const double);
    double update_tx_samp_rate(size_t dspno, const double);
    void update_rates(void);
    double update_rx_dsp_freq(const size_t, const double);
    double update_tx_dsp_freq(const size_t, const double);
    void update_tick_rate(const double rate);
    uhd::meta_range_t get_rx_dsp_freq_range(void);
    uhd::meta_range_t get_tx_dsp_freq_range(void);
    uhd::meta_range_t get_rx_dsp_host_rates(void);
    uhd::meta_range_t get_tx_dsp_host_rates(void);
    size_t _rx_dc_offset_shadow;
    void set_enb_rx_dc_offset(const std::string &db, const bool);
    std::complex<double> set_rx_dc_offset(const std::string &db, const std::complex<double> &);

    static uhd::usrp::dboard_iface::sptr make_dboard_iface(
        usrp1_iface::sptr,
        usrp1_codec_ctrl::sptr,
        dboard_slot_t,
        const double &,
        const uhd::usrp::dboard_id_t &
    );

    //handle io stuff
    UHD_PIMPL_DECL(io_impl) _io_impl;
    void io_init(void);
    void rx_stream_on_off(bool);
    void tx_stream_on_off(bool);
    void handle_overrun(size_t);

    //channel mapping shadows
    uhd::usrp::subdev_spec_t _rx_subdev_spec, _tx_subdev_spec;

    //capabilities
    size_t get_num_ducs(void);
    size_t get_num_ddcs(void);
    bool has_rx_halfband(void);
    bool has_tx_halfband(void);

    void vandal_conquest_loop(void);

    void set_reg(const std::pair<boost::uint8_t, boost::uint32_t> &reg);

    //handle the enables
    bool _rx_enabled, _tx_enabled;
    void enable_rx(bool enb){
        _rx_enabled = enb;
        _fx2_ctrl->usrp_rx_enable(enb);
    }
    void enable_tx(bool enb){
        _tx_enabled = enb;
        _fx2_ctrl->usrp_tx_enable(enb);
        BOOST_FOREACH(const std::string &key, _dbc.keys())
        {
            _dbc[key].codec->enable_tx_digital(enb);
        }
    }

    //conditionally disable and enable rx
    bool disable_rx(void){
        if (_rx_enabled){
            enable_rx(false);
            return true;
        }
        return false;
    }
    void restore_rx(bool last){
        if (last != _rx_enabled){
            enable_rx(last);
        }
    }

    //conditionally disable and enable tx
    bool disable_tx(void){
        if (_tx_enabled){
            enable_tx(false);
            return true;
        }
        return false;
    }
    void restore_tx(bool last){
        if (last != _tx_enabled){
            enable_tx(last);
        }
    }
};

#endif /* INCLUDED_USRP1_IMPL_HPP */
