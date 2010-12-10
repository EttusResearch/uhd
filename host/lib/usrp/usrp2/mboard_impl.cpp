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

#include "usrp2_impl.hpp"
#include "usrp2_regs.hpp"
#include <uhd/usrp/misc_utils.hpp>
#include <uhd/usrp/dsp_utils.hpp>
#include <uhd/usrp/mboard_props.hpp>
#include <uhd/utils/assert.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/algorithm.hpp>
#include <boost/bind.hpp>
#include <iostream>
#include <boost/date_time/posix_time/posix_time.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::posix_time;

/***********************************************************************
 * Structors
 **********************************************************************/
usrp2_mboard_impl::usrp2_mboard_impl(
    size_t index,
    transport::udp_simple::sptr ctrl_transport,
    transport::zero_copy_if::sptr data_transport,
    size_t recv_samps_per_packet,
    const device_addr_t &flow_control_hints
):
    _index(index),
    _iface(usrp2_iface::make(ctrl_transport))
{
    //Send a small data packet so the usrp2 knows the udp source port.
    //This setup must happen before further initialization occurs
    //or the async update packets will cause ICMP destination unreachable.
    transport::managed_send_buffer::sptr send_buff = data_transport->get_send_buff();
    static const boost::uint32_t data[2] = {
        uhd::htonx(boost::uint32_t(0 /* don't care seq num */)),
        uhd::htonx(boost::uint32_t(USRP2_INVALID_VRT_HEADER))
    };
    std::memcpy(send_buff->cast<void*>(), &data, sizeof(data));
    send_buff->commit(sizeof(data));

    //contruct the interfaces to mboard perifs
    _clock_ctrl = usrp2_clock_ctrl::make(_iface);
    _codec_ctrl = usrp2_codec_ctrl::make(_iface);
    //_gps_ctrl = usrp2_gps_ctrl::make(_iface);

    //if(_gps_ctrl->gps_detected()) std::cout << "GPS time: " << _gps_ctrl->get_time() << std::endl;

    //TODO move to dsp impl...
    //load the allowed decim/interp rates
    //_USRP2_RATES = range(4, 128+1, 1) + range(130, 256+1, 2) + range(260, 512+1, 4)
    _allowed_decim_and_interp_rates.clear();
    for (size_t i = 4; i <= 128; i+=1){
        _allowed_decim_and_interp_rates.push_back(i);
    }
    for (size_t i = 130; i <= 256; i+=2){
        _allowed_decim_and_interp_rates.push_back(i);
    }
    for (size_t i = 260; i <= 512; i+=4){
        _allowed_decim_and_interp_rates.push_back(i);
    }

    //setup the vrt rx registers
    _iface->poke32(_iface->regs.rx_ctrl_clear_overrun, 1); //reset
    _iface->poke32(_iface->regs.rx_ctrl_nsamps_per_pkt, recv_samps_per_packet);
    _iface->poke32(_iface->regs.rx_ctrl_nchannels, 1);
    _iface->poke32(_iface->regs.rx_ctrl_vrt_header, 0
        | (0x1 << 28) //if data with stream id
        | (0x1 << 26) //has trailer
        | (0x3 << 22) //integer time other
        | (0x1 << 20) //fractional time sample count
    );
    _iface->poke32(_iface->regs.rx_ctrl_vrt_stream_id, usrp2_impl::RECV_SID);
    _iface->poke32(_iface->regs.rx_ctrl_vrt_trailer, 0);
    _iface->poke32(_iface->regs.time64_tps, size_t(get_master_clock_freq()));

    //init the tx control registers
    _iface->poke32(_iface->regs.tx_ctrl_clear_state, 1); //reset
    _iface->poke32(_iface->regs.tx_ctrl_num_chan, 0);    //1 channel
    _iface->poke32(_iface->regs.tx_ctrl_report_sid, usrp2_impl::ASYNC_SID);
    _iface->poke32(_iface->regs.tx_ctrl_policy, U2_FLAG_TX_CTRL_POLICY_NEXT_PACKET);

    //setting the cycles per update (disabled by default)
    const double ups_per_sec = flow_control_hints.cast<double>("ups_per_sec", 0.0);
    if (ups_per_sec > 0.0){
        const size_t cycles_per_up = size_t(_clock_ctrl->get_master_clock_rate()/ups_per_sec);
        _iface->poke32(_iface->regs.tx_ctrl_cycles_per_up, U2_FLAG_TX_CTRL_UP_ENB | cycles_per_up);
    }

    //setting the packets per update (enabled by default)
    const double ups_per_fifo = flow_control_hints.cast<double>("ups_per_fifo", 8.0);
    if (ups_per_fifo > 0.0){
        const size_t packets_per_up = size_t(usrp2_impl::sram_bytes/ups_per_fifo/data_transport->get_send_frame_size());
        _iface->poke32(_iface->regs.tx_ctrl_packets_per_up, U2_FLAG_TX_CTRL_UP_ENB | packets_per_up);
    }

    //init the ddc
    init_ddc_config();

    //init the duc
    init_duc_config();

    //initialize the clock configuration
    init_clock_config();

    //init the codec before the dboard
    codec_init();

    //init the tx and rx dboards (do last)
    dboard_init();

    //set default subdev specs
    (*this)[MBOARD_PROP_RX_SUBDEV_SPEC] = subdev_spec_t();
    (*this)[MBOARD_PROP_TX_SUBDEV_SPEC] = subdev_spec_t();
}

usrp2_mboard_impl::~usrp2_mboard_impl(void){
    _iface->poke32(_iface->regs.tx_ctrl_cycles_per_up, 0);
    _iface->poke32(_iface->regs.tx_ctrl_packets_per_up, 0);
}

/***********************************************************************
 * Helper Methods
 **********************************************************************/
void usrp2_mboard_impl::init_clock_config(void){
    //setup the clock configuration settings
    _clock_config.ref_source = clock_config_t::REF_INT;
    _clock_config.pps_source = clock_config_t::PPS_SMA;
    _clock_config.pps_polarity = clock_config_t::PPS_NEG;

    //update the clock config (sends a control packet)
    update_clock_config();
}

void usrp2_mboard_impl::update_clock_config(void){
    boost::uint32_t pps_flags = 0;

    //translate pps source enums
    switch(_clock_config.pps_source){
    case clock_config_t::PPS_SMA:  pps_flags |= U2_FLAG_TIME64_PPS_SMA;  break;
    default: throw std::runtime_error("unhandled clock configuration pps source");
    }

    //translate pps polarity enums
    switch(_clock_config.pps_polarity){
    case clock_config_t::PPS_POS: pps_flags |= U2_FLAG_TIME64_PPS_POSEDGE; break;
    case clock_config_t::PPS_NEG: pps_flags |= U2_FLAG_TIME64_PPS_NEGEDGE; break;
    default: throw std::runtime_error("unhandled clock configuration pps polarity");
    }

    //set the pps flags
    _iface->poke32(_iface->regs.time64_flags, pps_flags);

    //clock source ref 10mhz
    switch(_iface->get_rev()){
    case usrp2_iface::USRP_N200:
    case usrp2_iface::USRP_N210:
        switch(_clock_config.ref_source){
        case clock_config_t::REF_INT : _iface->poke32(_iface->regs.misc_ctrl_clock, 0x12); break;
        case clock_config_t::REF_SMA : _iface->poke32(_iface->regs.misc_ctrl_clock, 0x1C); break;
        default: throw std::runtime_error("unhandled clock configuration reference source");
        }
        _clock_ctrl->enable_external_ref(true); //USRP2P has an internal 10MHz TCXO
        break;

    case usrp2_iface::USRP2_REV3:
    case usrp2_iface::USRP2_REV4:
        switch(_clock_config.ref_source){
        case clock_config_t::REF_INT : _iface->poke32(_iface->regs.misc_ctrl_clock, 0x10); break;
        case clock_config_t::REF_SMA : _iface->poke32(_iface->regs.misc_ctrl_clock, 0x1C); break;
        default: throw std::runtime_error("unhandled clock configuration reference source");
        }
        _clock_ctrl->enable_external_ref(_clock_config.ref_source != clock_config_t::REF_INT);
        break;

    case usrp2_iface::USRP_NXXX: break;
    }

    //Handle the serdes clocking based on master/slave mode:
    //   - Masters always drive the clock over serdes.
    //   - Slaves always lock to this serdes clock.
    //   - Slaves lock their time over the serdes.
    const bool master_mode = bool(_iface->peek32(_iface->regs.status) & (1 << 8));
    _clock_ctrl->enable_mimo_clock_out(master_mode);
    if (master_mode){
        switch(_iface->get_rev()){
        case usrp2_iface::USRP_N200:
        case usrp2_iface::USRP_N210:
            _clock_ctrl->set_mimo_clock_delay(0/*TODO*/);
            break;

        case usrp2_iface::USRP2_REV4:
            _clock_ctrl->set_mimo_clock_delay(3.08e-9);
            break;

        default: break; //not handled
        }
    }
    else{
        _iface->poke32(_iface->regs.misc_ctrl_clock, 0x15);
        _clock_ctrl->enable_external_ref(true);
    }
    //TODO slaves lock time over the serdes...

}

void usrp2_mboard_impl::set_time_spec(const time_spec_t &time_spec, bool now){
    //set the ticks
    _iface->poke32(_iface->regs.time64_ticks, time_spec.get_tick_count(get_master_clock_freq()));

    //set the flags register
    boost::uint32_t imm_flags = (now)? U2_FLAG_TIME64_LATCH_NOW : U2_FLAG_TIME64_LATCH_NEXT_PPS;
    _iface->poke32(_iface->regs.time64_imm, imm_flags);

    //set the seconds (latches in all 3 registers)
    _iface->poke32(_iface->regs.time64_secs, boost::uint32_t(time_spec.get_full_secs()));
}

void usrp2_mboard_impl::handle_overflow(void){
    if (_continuous_streaming){ //re-issue the stream command if already continuous
        this->issue_ddc_stream_cmd(stream_cmd_t::STREAM_MODE_START_CONTINUOUS);
    }
}

void usrp2_mboard_impl::issue_ddc_stream_cmd(const stream_cmd_t &stream_cmd){
    _continuous_streaming = stream_cmd.stream_mode == stream_cmd_t::STREAM_MODE_START_CONTINUOUS;
    _iface->poke32(_iface->regs.rx_ctrl_stream_cmd, dsp_type1::calc_stream_cmd_word(stream_cmd));
    _iface->poke32(_iface->regs.rx_ctrl_time_secs,  boost::uint32_t(stream_cmd.time_spec.get_full_secs()));
    _iface->poke32(_iface->regs.rx_ctrl_time_ticks, stream_cmd.time_spec.get_tick_count(get_master_clock_freq()));
}

/***********************************************************************
 * MBoard Get Properties
 **********************************************************************/
static const std::string dboard_name = "0";

void usrp2_mboard_impl::get(const wax::obj &key_, wax::obj &val){
    named_prop_t key = named_prop_t::extract(key_);
    //handle the get request conditioned on the key
    switch(key.as<mboard_prop_t>()){
    case MBOARD_PROP_NAME:
        val = _iface->get_cname() + " mboard";
        return;

    case MBOARD_PROP_OTHERS:
        val = prop_names_t();
        return;

    case MBOARD_PROP_RX_DBOARD:
        UHD_ASSERT_THROW(key.name == dboard_name);
        val = _rx_dboard_proxy->get_link();
        return;

    case MBOARD_PROP_RX_DBOARD_NAMES:
        val = prop_names_t(1, dboard_name);
        return;

    case MBOARD_PROP_TX_DBOARD:
        UHD_ASSERT_THROW(key.name == dboard_name);
        val = _tx_dboard_proxy->get_link();
        return;

    case MBOARD_PROP_TX_DBOARD_NAMES:
        val = prop_names_t(1, dboard_name);
        return;

    case MBOARD_PROP_RX_DSP:
        UHD_ASSERT_THROW(key.name == "");
        val = _rx_dsp_proxy->get_link();
        return;

    case MBOARD_PROP_RX_DSP_NAMES:
        val = prop_names_t(1, "");
        return;

    case MBOARD_PROP_TX_DSP:
        UHD_ASSERT_THROW(key.name == "");
        val = _tx_dsp_proxy->get_link();
        return;

    case MBOARD_PROP_TX_DSP_NAMES:
        val = prop_names_t(1, "");
        return;

    case MBOARD_PROP_CLOCK_CONFIG:
        val = _clock_config;
        return;

    case MBOARD_PROP_TIME_NOW:{
            usrp2_iface::pair64 time64(
                _iface->peek64(_iface->regs.time64_secs_rb, _iface->regs.time64_ticks_rb)
            );
            val = time_spec_t(
                time64.first, time64.second, get_master_clock_freq()
            );
        }
        return;

    case MBOARD_PROP_RX_SUBDEV_SPEC:
        val = _rx_subdev_spec;
        return;

    case MBOARD_PROP_TX_SUBDEV_SPEC:
        val = _tx_subdev_spec;
        return;

    case MBOARD_PROP_EEPROM_MAP:
        val = _iface->mb_eeprom;
        return;

    default: UHD_THROW_PROP_GET_ERROR();
    }
}

/***********************************************************************
 * MBoard Set Properties
 **********************************************************************/
void usrp2_mboard_impl::set(const wax::obj &key, const wax::obj &val){
    //handle the set request conditioned on the key
    switch(key.as<mboard_prop_t>()){

    case MBOARD_PROP_CLOCK_CONFIG:
        _clock_config = val.as<clock_config_t>();
        update_clock_config();
        return;

    case MBOARD_PROP_TIME_NOW:
        set_time_spec(val.as<time_spec_t>(), true);
        return;

    case MBOARD_PROP_TIME_NEXT_PPS:
        set_time_spec(val.as<time_spec_t>(), false);
        return;

    case MBOARD_PROP_STREAM_CMD:
        issue_ddc_stream_cmd(val.as<stream_cmd_t>());
        return;

    case MBOARD_PROP_RX_SUBDEV_SPEC:
        _rx_subdev_spec = val.as<subdev_spec_t>();
        verify_rx_subdev_spec(_rx_subdev_spec, this->get_link());
        //sanity check
        UHD_ASSERT_THROW(_rx_subdev_spec.size() == 1);
        //set the mux
        _iface->poke32(_iface->regs.dsp_rx_mux, dsp_type1::calc_rx_mux_word(
            _dboard_manager->get_rx_subdev(_rx_subdev_spec.front().sd_name)[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()
        ));
        return;

    case MBOARD_PROP_TX_SUBDEV_SPEC:
        _tx_subdev_spec = val.as<subdev_spec_t>();
        verify_tx_subdev_spec(_tx_subdev_spec, this->get_link());
        //sanity check
        UHD_ASSERT_THROW(_tx_subdev_spec.size() == 1);
        //set the mux
        _iface->poke32(_iface->regs.dsp_tx_mux, dsp_type1::calc_tx_mux_word(
            _dboard_manager->get_tx_subdev(_tx_subdev_spec.front().sd_name)[SUBDEV_PROP_CONNECTION].as<subdev_conn_t>()
        ));
        return;

    case MBOARD_PROP_EEPROM_MAP:
        // Step1: commit the map, writing only those values set.
        // Step2: readback the entire eeprom map into the iface.
        val.as<mboard_eeprom_t>().commit(*_iface, mboard_eeprom_t::MAP_N100);
        _iface->mb_eeprom = mboard_eeprom_t(*_iface, mboard_eeprom_t::MAP_N100);
        return;

    default: UHD_THROW_PROP_SET_ERROR();
    }
}
