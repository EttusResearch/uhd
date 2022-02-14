//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/rfnoc/noc_block_base.hpp>

namespace uhd { namespace rfnoc {

// doxygen tables need long long lines
// clang-format off
/*! Replay Block Control CLass
 *
 * \ingroup rfnoc_blocks
 *
 * The Replay block records data to memory and plays back data from memory.
 *
 * It is the responsibility of the user to manage the memory. Care must be taken to avoid
 * unintentional overlaps in the memory across blocks and ports. Each port of the Replay
 * block has access to the full memory space. This allows recording data on any input port
 * of any Replay block and playback on any output port of any Replay block. The section of
 * memory used by a record or play operation is controlled by setting the offset and size.
 *
 * For both record and playback, the offset and the size must be aligned to the memory's
 * word size. For playback, the size must also be aligned to the size of an item. An item
 * is a single unit of data as defined by the data type of a port. For record, the data
 * type is automatically determined by the connection to the upstream block by using the
 * "type" edge property for the corresponding input port. For playback, this is
 * automatically determined by the connection to the downstream block by using the "type"
 * edge property for the corresponding output port. These can be explicitly set by the
 * user, if necessary. It is the user's responsibility to manage the types of the
 * individual record and the playback ports. Methods to get the word size and item sizes
 * are provided to help calculate proper alignment.
 *
 * One key configuration of playback is the packet size. Larger packet sizes provide for
 * better throughput while smaller packet sizes provide for lower latency. By default,
 * the "mtu" edge property of the output port is used to define the maximum packet size to
 * achieve the best throughput to the downstream block without exceeding the supported
 * packet size. The maximum packet size can be explicitly set in terms of bytes or
 * number of items to allow users to tune the performance to suit their application.
 *
 * \section rfnoc_block_replay_properties Block Properties
 *
 * User Properties:
 *
 * | Key            | Description
 * |---------------:|---------------------------------------------------------
 * | record_offset  | The base address for recordings (data will be written here). Will be set by record().
 * | record_size    | Size of the record buffer. Will be set by record().
 * | play_offset    | Base address for playback (streaming will start from here). Set by play() or config_play().
 * | play_size      | Size of the playback buffer. Set by play() or config_play().
 * | packet_size    | Size of outgoing packets (in bytes). Gets set by set_max_items_per_packet().
 *
 * Edge properties:
 *
 * | Key   | Description
 * |------:|-------------------------------------------------------------------
 * | type  | Data type. On the input, it set by the upstream block (see get_record_type()). For output, it should be provided by set_play_type().
 *
 * \section rfnoc_block_replay_record Recording Data
 *
 * Before streaming data to the replay block, it must be configured to record
 * by calling record(), which will also configure the size of the record buffer
 * for the indicated input port.  After calling this method, the block will accept
 * incoming data until its record buffer is full (use the get_record_fullness()
 * method to query the fullness). Unlike the radio, the replay block does not
 * care about end-of-burst flags, and will not report underruns if subsequent
 * packets are not streamed to this block back-to-back.
 *
 * Once the record buffer is full, the block will no longer accept incoming data
 * and will back-pressure. If, for example, the radio block is connected to the replay
 * block, the radio could overrun in this scenario. By calling record() again,
 * the write-pointer of the block is reset, and it will accept new data. To
 * reset the write-pointer and buffer size to the same values as before, it is
 * sufficient to call record_restart().
 *
 * \section rfnoc_block_replay_playback Playing back Data
 *
 * To playback data from a given port, there are two options with basically the
 * same functionality: Either call config_play() to configure the playback
 * region on memory, and then call issue_stream_cmd() to start streaming.
 *
 * A shorthand version of this is to call play(), which is equivalent to
 * calling config_play() and issue_stream_cmd() together.
 *
 * In either case, the replay block will start streaming the requested number of
 * samples, or until it is stopped. If a specific number of samples is requested,
 * then it will tag the last outgoing packet with an end-of-burst flag. If more
 * samples were requested than stored in the playback buffer, it will wrap
 * around.
 *
 * To stop a continuous playback, either call stop(), or issue a stream command
 * with uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS.
 *
 * \section rfnoc_block_replay_actions Action Handling
 *
 * If this block receives TX or RX actions (uhd::rfnoc::tx_event_action_info o
 * uhd::rfnoc::rx_event_action_info), it will store them in a circular buffer.
 * The API calls get_record_async_metadata() and get_play_async_metadata() can
 * be used to read them back out asynchronously. To avoid the block controller
 * continously expanding in memory, the total number of messages that will be
 * stored is limited. If this block receives more event info objects than it can
 * store before get_record_async_metadata() or get_play_async_metadata() is
 * called, the oldest message will be dropped.
 */
// clang-format on
class UHD_API replay_block_control : public noc_block_base
{
public:
    RFNOC_DECLARE_BLOCK(replay_block_control)

    static const uint16_t MINOR_COMPAT;
    static const uint16_t MAJOR_COMPAT;

    static const uint32_t REPLAY_ADDR_W;
    static const uint32_t REPLAY_BLOCK_OFFSET;

    static const uint32_t REG_COMPAT_ADDR;
    static const uint32_t REG_MEM_SIZE_ADDR;
    static const uint32_t REG_REC_RESTART_ADDR;
    static const uint32_t REG_REC_BASE_ADDR_LO_ADDR;
    static const uint32_t REG_REC_BASE_ADDR_HI_ADDR;
    static const uint32_t REG_REC_BUFFER_SIZE_LO_ADDR;
    static const uint32_t REG_REC_BUFFER_SIZE_HI_ADDR;
    static const uint32_t REG_REC_FULLNESS_LO_ADDR;
    static const uint32_t REG_REC_FULLNESS_HI_ADDR;
    static const uint32_t REG_PLAY_BASE_ADDR_LO_ADDR;
    static const uint32_t REG_PLAY_BASE_ADDR_HI_ADDR;
    static const uint32_t REG_PLAY_BUFFER_SIZE_LO_ADDR;
    static const uint32_t REG_PLAY_BUFFER_SIZE_HI_ADDR;
    static const uint32_t REG_PLAY_CMD_NUM_WORDS_LO_ADDR;
    static const uint32_t REG_PLAY_CMD_NUM_WORDS_HI_ADDR;
    static const uint32_t REG_PLAY_CMD_TIME_LO_ADDR;
    static const uint32_t REG_PLAY_CMD_TIME_HI_ADDR;
    static const uint32_t REG_PLAY_CMD_ADDR;
    static const uint32_t REG_PLAY_WORDS_PER_PKT_ADDR;
    static const uint32_t REG_PLAY_ITEM_SIZE_ADDR;
    static const uint32_t REG_REC_POS_LO_ADDR;
    static const uint32_t REG_REC_POS_HI_ADDR;
    static const uint32_t REG_PLAY_POS_LO_ADDR;
    static const uint32_t REG_PLAY_POS_HI_ADDR;
    static const uint32_t REG_PLAY_CMD_FIFO_SPACE_ADDR;

    static const uint32_t PLAY_CMD_STOP;
    static const uint32_t PLAY_CMD_FINITE;
    static const uint32_t PLAY_CMD_CONTINUOUS;

    /**************************************************************************
     * Replay Control API calls
     *************************************************************************/
    /*! Record
     *
     * Begin recording. The offset sets the starting location in memory and the size
     * limits the length of the recording. The flow of data is controlled by upstream
     * RFNoC blocks.
     *
     * \param offset Memory offset where to start recording the data. This value
     * must be aligned to the memory word size. Use get_word_size() to get the
     * size of the memory word.
     * \param size Size limit, in bytes. This value must be aligned to the memory
     * word size and the item size. Use get_word_size() to get the size of the
     * memory word and get_item_size() to get the item size.  A value of 0 means
     * to use all available space.
     * \param port Which input port of the replay block to use
     * \throws uhd::value_error if offset+size exceeds the available memory.
     */
    virtual void record(
        const uint64_t offset, const uint64_t size, const size_t port = 0) = 0;

    /*! Restarts recording from the record offset
     *
     * This is a shortcut for calling record() again with the same arguments.
     *
     * \param port Which input port of the replay block to use
     */
    virtual void record_restart(const size_t port = 0) = 0;

    /*! Play back data.
     *
     * The offset and size define what data is played back on an output port.
     * It will stream out \p size bytes of data, starting at memory offset
     * \p offset.
     *
     * The data can be played once or repeated continuously until a stop
     * command is issued. If a time_spec is supplied, it will be placed in the
     * header of the first packet. Typically, this is used to tell a downstream
     * Radio block when to start transmitting the data.
     *
     * If the data type on the output port is not defined, this function will
     * throw an error.
     *
     * This is equivalent to calling config_play() with the same arguments for
     * \p offset, \p size, and \p port, and then calling issue_stream_cmd() with
     * the same time spec, and either continuous streaming (if \p repeat is true)
     * or STREAM_MODE_START_NUM_SAMPS_AND_DONE if it is not.
     *
     * \param offset Memory offset of the data to be played. This value must be
     *               aligned to the size of the word in memory. Use get_word_size()
     *               to get the memory word size.
     * \param size Size of data to play back. This value must be aligned to the
     *             size of the memory word and item size. Use get_word_size() to
     *             get the memory word size and get_output_item_size() to get
     *             the item size. This value will be used for the num_samps
     *             component of the underlying stream command.
     * \param port Which output port of the replay block to use
     * \param time_spec Set the time for the first item. Any non-zero value is
     *                  used to set the time in the header of the first packet.
     *                  Most commonly, this is used to set the start time of a
     *                  transmission. Note that this block will not wait for a
     *                  time to occur, rather, it will tag the first outgoing
     *                  packet with this time stamp.
     * \param repeat Determines whether the data should be played repeatedly or
     *               just once. If set to true, stop() must be called to stop
     *               the play back.
     * \throws uhd::value_error if offset+size exceeds the available memory.
     * \throws uhd::op_failed Too many play commands are queued.
     */
    virtual void play(const uint64_t offset,
        const uint64_t size,
        const size_t port                = 0,
        const uhd::time_spec_t time_spec = uhd::time_spec_t(0.0),
        const bool repeat                = false) = 0;

    /*! Stops playback
     *
     * Halts any currently executing play commands and cancels any other play commands
     * that are waiting to be executed for that output port.
     *
     * \param port Which output port of the replay block to use
     */
    virtual void stop(const size_t port = 0) = 0;

    /*! Get the size of the memory
     *
     * Returns the size of the shared memory space. Any record or playback buffers must
     * be configured in this memory space.
     *
     * \returns the size of the shared Replay memory
     */
    virtual uint64_t get_mem_size() const = 0;

    /*! Get the size of a memory word
     *
     * \returns the size of a memory word
     */
    virtual uint64_t get_word_size() const = 0;

    /**************************************************************************
     * Record State API calls
     *************************************************************************/
    /*! Get the starting offset of the current record buffer
     *
     * \param port Which input port of the replay block to use
     * \returns starting offset of the record buffer
     */
    virtual uint64_t get_record_offset(const size_t port = 0) const = 0;

    /*! Get the current size of the record space
     *
     * \param port Which input port of the replay block to use
     * \returns size of the record buffer
     */
    virtual uint64_t get_record_size(const size_t port = 0) const = 0;

    /*! Get the fullness of the current record buffer
     *
     * Returns the number of bytes that have been recorded in the record buffer. A record
     * restart will reset this number back to 0.
     *
     * \param port Which input port of the replay block to use
     * \returns fullness of the record buffer
     */
    virtual uint64_t get_record_fullness(const size_t port = 0) = 0;

    /*! Get the current record position
     *
     * \param port Which output port of the replay block to use
     * \returns the byte address of the current record position
     */
    virtual uint64_t get_record_position(const size_t port = 0) = 0;

    /*! Get the current record data type
     *
     * \param port Which input port of the replay block to use
     * \returns the current record data type
     */
    virtual io_type_t get_record_type(const size_t port = 0) const = 0;

    /*! Get the current record item size
     *
     * \param port Which input port of the replay block to use
     * \returns the size of an item in the current record buffer
     */
    virtual size_t get_record_item_size(const size_t port = 0) const = 0;

    /*! Return RX- (input-/record-) related metadata.
     *
     * The typical use case for this is when connecting Radio -> Replay for
     * recording, the radio may produce information like 'overrun occurred'.
     * When receiving to a host using a uhd::rx_streamer, this information is
     * returned as part of the uhd::rx_streamer::recv() call, but when the data
     * is streamed into the replay block, these metadata are stored inside the
     * replay block until queried by this method.
     *
     * \param metadata A metadata object to store the information in.
     * \param timeout A timeout (in seconds) to wait before returning.
     * \returns true if a message was available, and was popped into \p metadata.
     */
    virtual bool get_record_async_metadata(
        uhd::rx_metadata_t& metadata, const double timeout = 0.1) = 0;

    /**************************************************************************
     * Playback State API calls
     *************************************************************************/
    /*! Get the offset of the current playback buffer
     *
     * \param port Which output port of the replay block to use
     * \returns the offset of the current playback buffer
     */
    virtual uint64_t get_play_offset(const size_t port = 0) const = 0;

    /*! Get the current size of the playback space
     *
     * \param port Which output port of the replay block to use
     * \returns size of the playback buffer
     */
    virtual uint64_t get_play_size(const size_t port = 0) const = 0;

    /*! Get the current playback position
     *
     * \param port Which output port of the replay block to use
     * \returns the byte address of the current playback position
     */
    virtual uint64_t get_play_position(const size_t port = 0) = 0;

    /*! Get the maximum number of items in a packet
     *
     * \param port Which output port of the replay block to use
     * \returns the maximum number of items in a packet
     */
    virtual uint32_t get_max_items_per_packet(const size_t port = 0) const = 0;

    /*! Get the maximum size of a packet
     *
     * Returns the maximum size of a packet, inclusive of headers and payload.
     *
     * \param port Which output port of the replay block to use
     * \returns the maximum size of a packet
     */
    virtual uint32_t get_max_packet_size(const size_t port = 0) const = 0;

    /*! Get the current play data type
     *
     * \param port Which output port of the replay block to use
     * \returns the current play data type
     */
    virtual io_type_t get_play_type(const size_t port = 0) const = 0;

    /*! Get the current play item size
     *
     * \param port Which output port of the replay block to use
     * \returns the size of an item in the current play buffer
     */
    virtual size_t get_play_item_size(const size_t port = 0) const = 0;

    /*! Return TX- (output-/playback-) related metadata.
     *
     * The typical use case for this is when connecting Replay -> Radio for
     * playback, the radio may produce information like 'underrun occurred'.
     * When transmitting from a host using a uhd::tx_streamer, this information
     * is returned as part of the uhd::tx_streamer::recv_async_msg() call, but
     * when the data is streamed into the replay block, these metadata are
     * stored inside the replay block until queried by this method.
     *
     * \param metadata A metadata object to store the information in.
     * \param timeout A timeout (in seconds) to wait before returning.
     * \returns true if a message was available, and was popped into \p metadata.
     */
    virtual bool get_play_async_metadata(
        uhd::async_metadata_t& metadata, const double timeout = 0.1) = 0;

    /**************************************************************************
     * Advanced Record Control API calls
     *************************************************************************/
    /*! Explicitly set the current record data type
     *
     * Sets the data type for items in the current record buffer for the given input port.
     *
     * \param type The data type
     * \param port Which input port of the replay block to use
     */
    virtual void set_record_type(const io_type_t type, const size_t port = 0) = 0;

    /**************************************************************************
     * Advanced Playback Control API calls
     *************************************************************************/
    /*! Configure the offsets and size of the playback buffer region
     *
     * Specifies a buffer area in the memory for playback. In order to begin
     * playback on this region, a stream command must be issued.
     *
     * \param offset Memory offset of the data to be played. This value must be
     * aligned to the size of the word in memory. Use get_word_size() to get the
     * memory word size.
     * \param size Size of data to play back. This value must be aligned to the
     * size of the memory word and item size. Use get_word_size() to get the
     * memory word size and get_output_item_size() to get the item size.
     * \param port Which output port of the replay block to use
     * \throws uhd::value_error if offset+size exceeds the available memory.
     */
    virtual void config_play(
        const uint64_t offset, const uint64_t size, const size_t port = 0) = 0;

    /*! Explicitly set the current play data type
     *
     * Sets the data type for items in the current play buffer for the given output port.
     *
     * \param type The data type
     * \param port Which output port of the replay block to use
     */
    virtual void set_play_type(const io_type_t type, const size_t port = 0) = 0;

    /*! Set the maximum number of items in a packet
     *
     * Sets the maximum number of items that can be in a packet's payload. An actual
     * packet may be smaller in order to coerce to mtu values, or to align with memory
     * word size.
     *
     * \param ipp Number of items per packet
     * \param port Which output port of the replay block to use
     */
    virtual void set_max_items_per_packet(const uint32_t ipp, const size_t port = 0) = 0;

    /*! Set the maximum size of a packet
     *
     * Sets the maximum packet size, inclusive of headers and payload. Cannot
     * exceed the MTU for this block.
     *
     * \param size The size of the packet
     * \param port Which output port of the replay block to use
     */
    virtual void set_max_packet_size(const uint32_t size, const size_t port = 0) = 0;

    /*! Issue a stream command to the replay block
     *
     * Issue stream commands to start or stop playback from the configured playback
     * buffer. Supports
     * STREAM_MODE_START_CONTINUOUS to start continuous repeating playback,
     * STREAM_MODE_NUM_SAMPS_AND_DONE to play the given number of samples once, and
     * STREAM_MODE_STOP_CONTINUOUS to stop all playback immediately.
     * If a time_spec is supplied, it is placed in the header of the first packet produced
     * for that command. Commands are queued and executed in order. A
     * STREAM_MODE_STOP_CONTINUOUS command will halt all playback and purge all commands
     * in the queue for a given output port.
     *
     * \param stream_cmd The command to execute
     * \param port Which output port of the replay block to use
     * \throws uhd::op_failed Too many commands are queued.
     */
    virtual void issue_stream_cmd(
        const uhd::stream_cmd_t& stream_cmd, const size_t port = 0) = 0;
};

}} /* namespace uhd::rfnoc */
