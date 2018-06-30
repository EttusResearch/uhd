//
// Copyright 2011-2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_TRANSPORT_SUPER_SEND_PACKET_HANDLER_HPP
#define INCLUDED_LIBUHD_TRANSPORT_SUPER_SEND_PACKET_HANDLER_HPP

#include <uhd/config.hpp>
#include <uhd/exception.hpp>
#include <uhd/convert.hpp>
#include <uhd/stream.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/tasks.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/thread.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/transport/vrt_if_packet.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhdlib/rfnoc/tx_stream_terminator.hpp>
#include <boost/function.hpp>
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>

#ifdef UHD_TXRX_DEBUG_PRINTS
// Included for debugging
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include "boost/date_time/posix_time/posix_time.hpp"
#include <map>
#include <fstream>
#endif

namespace uhd {
namespace transport {
namespace sph {

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
    typedef boost::function<bool(uhd::async_metadata_t &, const double)> async_receiver_type;
    typedef void(*vrt_packer_type)(uint32_t *, vrt::if_packet_info_t &);
    //typedef boost::function<void(uint32_t *, vrt::if_packet_info_t &)> vrt_packer_type;

    /*!
     * Make a new packet handler for send
     * \param size the number of transport channels
     */
    send_packet_handler(const size_t size = 1):
        _next_packet_seq(0), _cached_metadata(false)
    {
        this->set_enable_trailer(true);
        this->resize(size);
    }

    ~send_packet_handler(void){
        /* NOP */
    }

    //! Resize the number of transport channels
    void resize(const size_t size){
        if (this->size() == size) return;
        _props.resize(size);
        static const uint64_t zero = 0;
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

    //! Set the stream ID for a specific channel (or no SID)
    void set_xport_chan_sid(const size_t xport_chan, const bool has_sid, const uint32_t sid = 0){
        _props.at(xport_chan).has_sid = has_sid;
        _props.at(xport_chan).sid = sid;
    }

    ///////// RFNOC ///////////////////
    //! Get the stream ID for a specific channel (or zero if no SID)
    uint32_t get_xport_chan_sid(const size_t xport_chan) const {
        if (_props.at(xport_chan).has_sid) {
            return _props.at(xport_chan).sid;
        } else {
            return 0;
        }
    }

    void set_terminator(uhd::rfnoc::tx_stream_terminator::sptr terminator)
    {
        _terminator = terminator;
    }

    uhd::rfnoc::tx_stream_terminator::sptr get_terminator()
    {
        return _terminator;
    }
    ///////// RFNOC ///////////////////

    void set_enable_trailer(const bool enable)
    {
        _has_tlr = enable;
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
        _num_inputs = id.num_inputs;
        _converter = uhd::convert::get_converter(id)();
        this->set_scale_factor(32767.); //update after setting converter
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
        _converter->set_scalar(scale_factor);
    }

    //! Set the callback to get async messages
    void set_async_receiver(const async_receiver_type &async_receiver)
    {
        _async_receiver = async_receiver;
    }

    //! Overload call to get async metadata
    bool recv_async_msg(
        uhd::async_metadata_t &async_metadata, double timeout = 0.1
    ){
        if (_async_receiver) return _async_receiver(async_metadata, timeout);
        std::this_thread::sleep_for(std::chrono::microseconds(long(timeout*1e6)));
        return false;
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
        if_packet_info.packet_type = vrt::if_packet_info_t::PACKET_TYPE_DATA;
        //if_packet_info.has_sid = false; //set per channel
        if_packet_info.has_cid = false;
        if_packet_info.has_tlr = _has_tlr;
        if_packet_info.has_tsi = false;
        if_packet_info.has_tsf = metadata.has_time_spec;
        if_packet_info.tsf     = metadata.time_spec.to_ticks(_tick_rate);
        if_packet_info.sob     = metadata.start_of_burst;
        if_packet_info.eob     = metadata.end_of_burst;

        /*
         * Metadata is cached when we get a send requesting a start of burst with no samples.
         * It is applied here on the next call to send() that actually has samples to send.
         */
        if (_cached_metadata && nsamps_per_buff != 0)
        {
            // If the new metada has a time_spec, do not use the cached time_spec.
            if (!metadata.has_time_spec)
            {
                if_packet_info.has_tsf = _metadata_cache.has_time_spec;
                if_packet_info.tsf     = _metadata_cache.time_spec.to_ticks(_tick_rate);
            }
            if_packet_info.sob     = _metadata_cache.start_of_burst;
            if_packet_info.eob     = _metadata_cache.end_of_burst;
            _cached_metadata = false;
        }

        if (nsamps_per_buff <= _max_samples_per_packet){

            //TODO remove this code when sample counts of zero are supported by hardware
            #ifndef SSPH_DONT_PAD_TO_ONE
                static const uint64_t zero = 0;
                _zero_buffs.resize(buffs.size(), &zero);

                if (nsamps_per_buff == 0)
                {
                    // if this is a start of a burst and there are no samples
                    if (metadata.start_of_burst)
                    {
                        // cache metadata and apply on the next send()
                        _metadata_cache = metadata;
                        _cached_metadata = true;
                        return 0;
                    } else {
                        // send requests with no samples are handled here (such as end of burst)
                        return send_one_packet(_zero_buffs, 1, if_packet_info, timeout) & 0x0;
                    }
                }
            #endif

			size_t nsamps_sent = send_one_packet(buffs, nsamps_per_buff, if_packet_info, timeout);
#ifdef UHD_TXRX_DEBUG_PRINTS
			dbg_print_send(nsamps_per_buff, nsamps_sent, metadata, timeout);
#endif
			return nsamps_sent;        }
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
            const time_spec_t time_spec = metadata.time_spec + time_spec_t::from_ticks(total_num_samps_sent, _samp_rate);
            if_packet_info.tsf = time_spec.to_ticks(_tick_rate);
            if_packet_info.sob = false;

        }

        //send the final fragment with the helper function
        if_packet_info.eob = metadata.end_of_burst;
		size_t nsamps_sent = total_num_samps_sent
				+ send_one_packet(buffs, final_length, if_packet_info, timeout,
					total_num_samps_sent * _bytes_per_cpu_item);
#ifdef UHD_TXRX_DEBUG_PRINTS
		dbg_print_send(nsamps_per_buff, nsamps_sent, metadata, timeout);

#endif
		return nsamps_sent;
    }

private:

    vrt_packer_type _vrt_packer;
    size_t _header_offset_words32;
    double _tick_rate, _samp_rate;
    struct xport_chan_props_type{
        xport_chan_props_type(void):has_sid(false),sid(0){}
        get_buff_type get_buff;
        bool has_sid;
        uint32_t sid;
        managed_send_buffer::sptr buff;
    };
    std::vector<xport_chan_props_type> _props;
    size_t _num_inputs;
    size_t _bytes_per_otw_item; //used in conversion
    size_t _bytes_per_cpu_item; //used in conversion
    uhd::convert::converter::sptr _converter; //used in conversion
    size_t _max_samples_per_packet;
    std::vector<const void *> _zero_buffs;
    size_t _next_packet_seq;
    bool _has_tlr;
    async_receiver_type _async_receiver;
    bool _cached_metadata;
    uhd::tx_metadata_t _metadata_cache;

    uhd::rfnoc::tx_stream_terminator::sptr _terminator;

#ifdef UHD_TXRX_DEBUG_PRINTS
    struct dbg_send_stat_t {
        dbg_send_stat_t(long wc, size_t nspb, size_t nss, uhd::tx_metadata_t md, double to, double rate):
            wallclock(wc), nsamps_per_buff(nspb), nsamps_sent(nss), metadata(md), timeout(to), samp_rate(rate)
        {}
        long wallclock;
        size_t nsamps_per_buff;
        size_t nsamps_sent;
        uhd::tx_metadata_t metadata;
        double timeout;
        double samp_rate;
        // Create a formatted print line for all the info gathered in this struct.
        std::string print_line() {
            boost::format fmt("send,%ld,%f,%i,%i,%s,%s,%s,%ld");
            fmt % wallclock;
            fmt % timeout % (int)nsamps_per_buff % (int) nsamps_sent;
            fmt % (metadata.start_of_burst ? "true":"false") % (metadata.end_of_burst ? "true":"false");
            fmt % (metadata.has_time_spec ? "true":"false") % metadata.time_spec.to_ticks(samp_rate);
            return fmt.str();
        }
    };

    void dbg_print_send(size_t nsamps_per_buff, size_t nsamps_sent,
            const uhd::tx_metadata_t &metadata, const double timeout,
            bool dbg_print_directly = true)
    {
        dbg_send_stat_t data(boost::get_system_time().time_of_day().total_microseconds(),
            nsamps_per_buff,
            nsamps_sent,
            metadata,
            timeout,
            _samp_rate
        );
        if(dbg_print_directly){
            dbg_print_err(data.print_line());
        }
    }
    void dbg_print_err(std::string msg) {
        msg = "super_send_packet_handler," + msg;
        fprintf(stderr, "%s\n", msg.c_str());
    }


#endif

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
        if_packet_info.num_payload_bytes = nsamps_per_buff*_num_inputs*_bytes_per_otw_item;
        if_packet_info.num_payload_words32 = (if_packet_info.num_payload_bytes + 3/*round up*/)/sizeof(uint32_t);
        if_packet_info.packet_count = _next_packet_seq;

        //get a buffer for each channel or timeout
        BOOST_FOREACH(xport_chan_props_type &props, _props){
            if (not props.buff) props.buff = props.get_buff(timeout);
            if (not props.buff) return 0; //timeout
        }

        //setup the data to share with converter threads
        _convert_nsamps = nsamps_per_buff;
        _convert_buffs = &buffs;
        _convert_buffer_offset_bytes = buffer_offset_bytes;
        _convert_if_packet_info = &if_packet_info;

        //perform N channels of conversion
        for (size_t i = 0; i < this->size(); i++) {
            convert_to_in_buff(i);
        }

        _next_packet_seq++; //increment sequence after commits
        return nsamps_per_buff;
    }

    /*! Run the conversion from the internal buffers to the user's input
     *  buffer.
     *
     * - Calls the converter
     * - Releases internal data buffers
     * - Updates read/write pointers
     */
    UHD_INLINE void convert_to_in_buff(const size_t index)
    {
        //shortcut references to local data structures
        managed_send_buffer::sptr &buff = _props[index].buff;
        vrt::if_packet_info_t if_packet_info = *_convert_if_packet_info;
        const tx_streamer::buffs_type &buffs = *_convert_buffs;

        //fill IO buffs with pointers into the output buffer
        const void *io_buffs[4/*max interleave*/];
        for (size_t i = 0; i < _num_inputs; i++){
            const char *b = reinterpret_cast<const char *>(buffs[index*_num_inputs + i]);
            io_buffs[i] = b + _convert_buffer_offset_bytes;
        }
        const ref_vector<const void *> in_buffs(io_buffs, _num_inputs);

        //pack metadata into a vrt header
        uint32_t *otw_mem = buff->cast<uint32_t *>() + _header_offset_words32;
        if_packet_info.has_sid = _props[index].has_sid;
        if_packet_info.sid = _props[index].sid;
        _vrt_packer(otw_mem, if_packet_info);
        otw_mem += if_packet_info.num_header_words32;

        //perform the conversion operation
        _converter->conv(in_buffs, otw_mem, _convert_nsamps);

        //commit the samples to the zero-copy interface
        const size_t num_vita_words32 = _header_offset_words32+if_packet_info.num_packet_words32;
        buff->commit(num_vita_words32*sizeof(uint32_t));
        buff.reset(); //effectively a release
    }

    //! Shared variables for the worker threads
    size_t _convert_nsamps;
    const tx_streamer::buffs_type *_convert_buffs;
    size_t _convert_buffer_offset_bytes;
    vrt::if_packet_info_t *_convert_if_packet_info;

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

    bool recv_async_msg(
        uhd::async_metadata_t &async_metadata, double timeout = 0.1
    ){
        return send_packet_handler::recv_async_msg(async_metadata, timeout);
    }

private:
    size_t _max_num_samps;
};

} // namespace sph
} // namespace transport
} // namespace uhd

#endif /* INCLUDED_LIBUHD_TRANSPORT_SUPER_SEND_PACKET_HANDLER_HPP */
