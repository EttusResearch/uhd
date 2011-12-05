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

//this file is included inside device class
//it supports the old send/recv functions
//this was replaced by the streamer API

#define _lazymin(x, y) (((x) > (y))? (y) : (x))

/*!
 * Send modes for the device send routine.
 */
enum send_mode_t{
    //! Tells the send routine to send the entire buffer
    SEND_MODE_FULL_BUFF = 0,
    //! Tells the send routine to return after one packet
    SEND_MODE_ONE_PACKET = 1
};

/*!
 * Recv modes for the device recv routine.
 */
enum recv_mode_t{
    //! Tells the recv routine to recv the entire buffer
    RECV_MODE_FULL_BUFF = 0,
    //! Tells the recv routine to return after one packet
    RECV_MODE_ONE_PACKET = 1
};

//! Typedef for a pointer to a single, or a collection of send buffers
typedef ref_vector<const void *> send_buffs_type;

//! Typedef for a pointer to a single, or a collection of recv buffers
typedef ref_vector<void *> recv_buffs_type;

/*!
 * Send buffers containing IF data described by the metadata.
 *
 * Send handles fragmentation as follows:
 * If the buffer has more samples than the maximum per packet,
 * the send method will fragment the samples across several packets.
 * Send will respect the burst flags when fragmenting to ensure
 * that start of burst can only be set on the first fragment and
 * that end of burst can only be set on the final fragment.
 * Fragmentation only applies in the full buffer send mode.
 *
 * This is a blocking call and will not return until the number
 * of samples returned have been read out of each buffer.
 * Under a timeout condition, the number of samples returned
 * may be less than the number of samples specified.
 *
 * \param buffs a vector of read-only memory containing IF data
 * \param nsamps_per_buff the number of samples to send, per buffer
 * \param metadata data describing the buffer's contents
 * \param io_type the type of data loaded in the buffer
 * \param send_mode tells send how to unload the buffer
 * \param timeout the timeout in seconds to wait on a packet
 * \return the number of samples sent
 */
size_t send(
    const send_buffs_type &buffs,
    size_t nsamps_per_buff,
    const tx_metadata_t &metadata,
    const io_type_t &io_type,
    send_mode_t send_mode,
    double timeout = 0.1
){
    if (_tx_streamer.get() == NULL or _tx_streamer->get_num_channels() != buffs.size() or _send_tid != io_type.tid){
        _send_tid = io_type.tid;
        _tx_streamer.reset(); //cleanup possible old one
        stream_args_t args;
        args.cpu_format = (_send_tid == io_type_t::COMPLEX_FLOAT32)? "fc32" : "sc16";
        args.otw_format = "sc16";
        args.args["noclear"] = "1";
        for (size_t ch = 0; ch < buffs.size(); ch++)
            args.channels.push_back(ch); //linear mapping
        _tx_streamer = get_tx_stream(args);
    }
    const size_t nsamps = (send_mode == SEND_MODE_ONE_PACKET)?
        _lazymin(nsamps_per_buff, get_max_send_samps_per_packet()) :
        nsamps_per_buff;
    return _tx_streamer->send(buffs, nsamps, metadata, timeout);
}

/*!
 * Receive buffers containing IF data described by the metadata.
 *
 * Receive handles fragmentation as follows:
 * If the buffer has insufficient space to hold all samples
 * that were received in a single packet over-the-wire,
 * then the buffer will be completely filled and the implementation
 * will hold a pointer into the remaining portion of the packet.
 * Subsequent calls will load from the remainder of the packet,
 * and will flag the metadata to show that this is a fragment.
 * The next call to receive, after the remainder becomes exahausted,
 * will perform an over-the-wire receive as usual.
 * See the rx metadata fragment flags and offset fields for details.
 *
 * This is a blocking call and will not return until the number
 * of samples returned have been written into each buffer.
 * Under a timeout condition, the number of samples returned
 * may be less than the number of samples specified.
 *
 * When using the full buffer recv mode, the metadata only applies
 * to the first packet received and written into the recv buffers.
 * Use the one packet recv mode to get per packet metadata.
 *
 * \param buffs a vector of writable memory to fill with IF data
 * \param nsamps_per_buff the size of each buffer in number of samples
 * \param metadata data to fill describing the buffer
 * \param io_type the type of data to fill into the buffer
 * \param recv_mode tells recv how to load the buffer
 * \param timeout the timeout in seconds to wait for a packet
 * \return the number of samples received or 0 on error
 */
size_t recv(
    const recv_buffs_type &buffs,
    size_t nsamps_per_buff,
    rx_metadata_t &metadata,
    const io_type_t &io_type,
    recv_mode_t recv_mode,
    double timeout = 0.1
){
    if (_rx_streamer.get() == NULL or _rx_streamer->get_num_channels() != buffs.size() or _recv_tid != io_type.tid){
        _recv_tid = io_type.tid;
        _rx_streamer.reset(); //cleanup possible old one
        stream_args_t args;
        args.cpu_format = (_recv_tid == io_type_t::COMPLEX_FLOAT32)? "fc32" : "sc16";
        args.otw_format = "sc16";
        args.args["noclear"] = "1";
        for (size_t ch = 0; ch < buffs.size(); ch++)
            args.channels.push_back(ch); //linear mapping
        _rx_streamer = get_rx_stream(args);
    }
    const size_t nsamps = (recv_mode == RECV_MODE_ONE_PACKET)?
        _lazymin(nsamps_per_buff, get_max_recv_samps_per_packet()) :
        nsamps_per_buff;
    return _rx_streamer->recv(buffs, nsamps, metadata, timeout);
}

/*!
 * Get the maximum number of samples per packet on send.
 * \return the number of samples
 */
size_t get_max_send_samps_per_packet(void){
    if (_tx_streamer.get() == NULL){
        stream_args_t args;
        args.cpu_format = "fc32";
        args.otw_format = "sc16";
        args.args["noclear"] = "1";
        _tx_streamer = get_tx_stream(args);
        _send_tid = io_type_t::COMPLEX_FLOAT32;
    }
    return _tx_streamer->get_max_num_samps();
}

/*!
 * Get the maximum number of samples per packet on recv.
 * \return the number of samples
 */
size_t get_max_recv_samps_per_packet(void){
    if (_rx_streamer.get() == NULL){
        stream_args_t args;
        args.cpu_format = "fc32";
        args.otw_format = "sc16";
        args.args["noclear"] = "1";
        _rx_streamer = get_rx_stream(args);
        _recv_tid = io_type_t::COMPLEX_FLOAT32;
    }
    return _rx_streamer->get_max_num_samps();
}

private:
    rx_streamer::sptr _rx_streamer;
    io_type_t::tid_t _recv_tid;
    tx_streamer::sptr _tx_streamer;
    io_type_t::tid_t _send_tid;
