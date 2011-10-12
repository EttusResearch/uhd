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

#ifndef INCLUDED_LIBUHD_TRANSPORT_SUPER_SEND_PACKET_HANDLER_HPP
#define INCLUDED_LIBUHD_TRANSPORT_SUPER_SEND_PACKET_HANDLER_HPP

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/convert.hpp>
#include <uhd/stream.hpp>
#include <uhd/utils/msg.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <boost/thread/thread_time.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <iostream>
#include <vector>

namespace uhd{ namespace transport{ namespace sph{

/***********************************************************************
 * Super send packet handler
 *
 * A send packet handler represents a group of channels.
 * The channel group shares a common sample rate.
 * All channels are sent in unison in send().
 **********************************************************************/
class send_packet_handler{
public:
    typedef boost::function<managed_send_buffer::sptr(double)> get_buff_type;
    typedef void(*vrt_packer_type)(boost::uint32_t *, vrt::if_packet_info_t &);
    //typedef boost::function<void(boost::uint32_t *, vrt::if_packet_info_t &)> vrt_packer_type;

    /*!
     * Make a new packet handler for send
     * \param size the number of transport channels
     */
    send_packet_handler(const size_t size = 1):
        _next_packet_seq(0)
    {
        this->resize(size);
        this->set_scale_factor(32767.);
    }

    //! Resize the number of transport channels
    void resize(const size_t size){
        if (this->size() == size) return;
        _props.resize(size);
        static const boost::uint64_t zero = 0;
        _zero_buffs.resize(size, &zero);
    }

    //! Get the channel width of this handler
    size_t size(void) const{
        return _props.size();
    }

    //! Setup the vrt packer function and offset
    void set_vrt_packer(const vrt_packer_type &vrt_packer, const size_t header_offset_words32 = 0){
        _vrt_packer = vrt_packer;
        _header_offset_words32 = header_offset_words32;
    }

    //! Set the rate of ticks per second
    void set_tick_rate(const double rate){
        _tick_rate = rate;
    }

    //! Set the rate of samples per second
    void set_samp_rate(const double rate){
        _samp_rate = rate;
    }

    /*!
     * Set the function to get a managed buffer.
     * \param xport_chan which transport channel
     * \param get_buff the getter function
     */
    void set_xport_chan_get_buff(const size_t xport_chan, const get_buff_type &get_buff){
        _props.at(xport_chan).get_buff = get_buff;
    }

    //! Set the conversion routine for all channels
    void set_converter(const uhd::convert::id_type &id){
        _io_buffs.resize(id.num_inputs);
        _converter = uhd::convert::get_converter(id);
        _bytes_per_otw_item = uhd::convert::get_bytes_per_item(id.output_format);
        _bytes_per_cpu_item = uhd::convert::get_bytes_per_item(id.input_format);
    }

    /*!
     * Set the maximum number of samples per host packet.
     * Ex: A USRP1 in dual channel mode would be half.
     * \param num_samps the maximum samples in a packet
     */
    void set_max_samples_per_packet(const size_t num_samps){
        _max_samples_per_packet = num_samps;
    }

    //! Set the scale factor used in float conversion
    void set_scale_factor(const double scale_factor){
        _scale_factor = scale_factor;
    }

    /*******************************************************************
     * Send:
     * The entry point for the fast-path send calls.
     * Dispatch into combinations of single packet send calls.
     ******************************************************************/
    UHD_INLINE size_t send(
        const uhd::tx_streamer::buffs_type &buffs,
        const size_t nsamps_per_buff,
        const uhd::tx_metadata_t &metadata,
        const double timeout
    ){
        //translate the metadata to vrt if packet info
        vrt::if_packet_info_t if_packet_info;
        if_packet_info.has_sid = false;
        if_packet_info.has_cid = false;
        if_packet_info.has_tlr = false;
        if_packet_info.has_tsi = metadata.has_time_spec;
        if_packet_info.has_tsf = metadata.has_time_spec;
        if_packet_info.tsi     = boost::uint32_t(metadata.time_spec.get_full_secs());
        if_packet_info.tsf     = boost::uint64_t(metadata.time_spec.get_tick_count(_tick_rate));
        if_packet_info.sob     = metadata.start_of_burst;
        if_packet_info.eob     = metadata.end_of_burst;

        if (nsamps_per_buff <= _max_samples_per_packet){

            //TODO remove this code when sample counts of zero are supported by hardware
            #ifndef SSPH_DONT_PAD_TO_ONE
            if (nsamps_per_buff == 0) return send_one_packet(
                _zero_buffs, 1, if_packet_info, timeout
            ) & 0x0;
            #endif

            return send_one_packet(buffs, nsamps_per_buff, if_packet_info, timeout);
        }
        size_t total_num_samps_sent = 0;

        //false until final fragment
        if_packet_info.eob = false;

        const size_t num_fragments = (nsamps_per_buff-1)/_max_samples_per_packet;
        const size_t final_length = ((nsamps_per_buff-1)%_max_samples_per_packet)+1;

        //loop through the following fragment indexes
        for (size_t i = 0; i < num_fragments; i++){

            //send a fragment with the helper function
            const size_t num_samps_sent = send_one_packet(
                buffs, _max_samples_per_packet,
                if_packet_info, timeout,
                total_num_samps_sent*_bytes_per_cpu_item
            );
            total_num_samps_sent += num_samps_sent;
            if (num_samps_sent == 0) return total_num_samps_sent;

            //setup metadata for the next fragment
            const time_spec_t time_spec = metadata.time_spec + time_spec_t(0, total_num_samps_sent, _samp_rate);
            if_packet_info.tsi = boost::uint32_t(time_spec.get_full_secs());
            if_packet_info.tsf = boost::uint64_t(time_spec.get_tick_count(_tick_rate));
            if_packet_info.sob = false;

        }

        //send the final fragment with the helper function
        if_packet_info.eob = metadata.end_of_burst;
        return total_num_samps_sent + send_one_packet(
            buffs, final_length,
            if_packet_info, timeout,
            total_num_samps_sent*_bytes_per_cpu_item
        );
    }

private:

    vrt_packer_type _vrt_packer;
    size_t _header_offset_words32;
    double _tick_rate, _samp_rate;
    struct xport_chan_props_type{
        get_buff_type get_buff;
    };
    std::vector<xport_chan_props_type> _props;
    std::vector<const void *> _io_buffs; //used in conversion
    size_t _bytes_per_otw_item; //used in conversion
    size_t _bytes_per_cpu_item; //used in conversion
    uhd::convert::function_type _converter; //used in conversion
    size_t _max_samples_per_packet;
    std::vector<const void *> _zero_buffs;
    size_t _next_packet_seq;
    double _scale_factor;

    /*******************************************************************
     * Send a single packet:
     ******************************************************************/
    UHD_INLINE size_t send_one_packet(
        const uhd::tx_streamer::buffs_type &buffs,
        const size_t nsamps_per_buff,
        vrt::if_packet_info_t &if_packet_info,
        const double timeout,
        const size_t buffer_offset_bytes = 0
    ){
        //load the rest of the if_packet_info in here
        if_packet_info.num_payload_words32 = (nsamps_per_buff*_io_buffs.size()*_bytes_per_otw_item)/sizeof(boost::uint32_t);
        if_packet_info.packet_count = _next_packet_seq;

        size_t buff_index = 0;
        BOOST_FOREACH(xport_chan_props_type &props, _props){
            managed_send_buffer::sptr buff = props.get_buff(timeout);
            if (buff.get() == NULL) return 0; //timeout

            //fill a vector with pointers to the io buffers
            BOOST_FOREACH(const void *&io_buff, _io_buffs){
                io_buff = reinterpret_cast<const char *>(buffs[buff_index++]) + buffer_offset_bytes;
            }
            boost::uint32_t *otw_mem = buff->cast<boost::uint32_t *>() + _header_offset_words32;

            //pack metadata into a vrt header
            _vrt_packer(otw_mem, if_packet_info);
            otw_mem += if_packet_info.num_header_words32;

            //copy-convert the samples into the send buffer
            _converter(_io_buffs, otw_mem, nsamps_per_buff, _scale_factor);

            //commit the samples to the zero-copy interface
            size_t num_bytes_total = (_header_offset_words32+if_packet_info.num_packet_words32)*sizeof(boost::uint32_t);
            buff->commit(num_bytes_total);

        }
        _next_packet_seq++; //increment sequence after commits
        return nsamps_per_buff;
    }
};

class send_packet_streamer : public send_packet_handler, public tx_streamer{
public:
    send_packet_streamer(const size_t max_num_samps){
        _max_num_samps = max_num_samps;
        this->set_max_samples_per_packet(_max_num_samps);
    }

    size_t get_num_channels(void) const{
        return this->size();
    }

    size_t get_max_num_samps(void) const{
        return _max_num_samps;
    }

    size_t send(
        const tx_streamer::buffs_type &buffs,
        const size_t nsamps_per_buff,
        const uhd::tx_metadata_t &metadata,
        const double timeout
    ){
        return send_packet_handler::send(buffs, nsamps_per_buff, metadata, timeout);
    }

private:
    size_t _max_num_samps;
};

}}} //namespace

#endif /* INCLUDED_LIBUHD_TRANSPORT_SUPER_SEND_PACKET_HANDLER_HPP */
