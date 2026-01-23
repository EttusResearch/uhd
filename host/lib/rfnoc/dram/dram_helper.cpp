//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/dram/dram_helper.hpp>

#include <uhd/convert.hpp>
#include <uhd/types/metadata.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/utils/log.hpp>

#include <chrono>
#include <cstddef>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

namespace uhd::rfnoc::dram {

typedef std::pair<std::string, size_t> block_port_t;
typedef std::shared_ptr<block_port_t> block_port_ptr_t;

constexpr const char* DRAM_UTILS                  = "DRAM Utils";
constexpr const size_t MAX_CHUNK_DOWNLOAD_RETRIES = 10;

/*!
 * Helper to find a block connected to a given block/port combination.
 *
 * \returns a pair of (source, destination) blocks connected to the given block/port.
 */
std::pair<block_port_ptr_t, block_port_ptr_t> connected_blocks(
    rfnoc_graph::sptr graph, const std::string& block_id, size_t block_port)
{
    block_port_ptr_t src = nullptr;
    block_port_ptr_t dst = nullptr;
    for (auto& conn : graph->enumerate_active_connections()) {
        if (conn.dst_blockid == block_id and conn.dst_port == block_port) {
            src = std::make_shared<block_port_t>(
                block_port_t{conn.src_blockid, conn.src_port});
        }
        if (conn.src_blockid == block_id and conn.src_port == block_port) {
            dst = std::make_shared<block_port_t>(
                block_port_t{conn.dst_blockid, conn.dst_port});
        }
    }
    return {src, dst};
}

/*!
 * Helper to get a stable reading of the record fullness.
 */
size_t get_record_fullness(replay_block_control::sptr replay, size_t port)
{
    auto fullness = replay->get_record_fullness(port);
    while (true) {
        auto old_fullness = fullness;
        std::this_thread::sleep_for(1ms);
        fullness = replay->get_record_fullness(port);
        if (fullness == old_fullness) {
            return fullness;
        }
    }
}

/*!
 * Helper to get a replay block from the graph if none is given.
 *
 * \throws uhd::runtime_error if no replay block is found in the given graph.
 */
replay_block_control::sptr get_replay_block(
    rfnoc_graph::sptr graph, replay_block_control::sptr replay)
{
    if (!replay) {
        auto replay_blocks = graph->find_blocks<replay_block_control>("");
        if (replay_blocks.empty()) {
            throw uhd::runtime_error("Cannot find any replay block on the device");
        }
        replay = graph->get_block<replay_block_control>(replay_blocks.front());
    }
    UHD_LOG_TRACE(DRAM_UTILS, "Using " << replay->get_unique_id());
    return replay;
}

/*!
 * Helper function to coerce a byte size to the word size of the replay block.
 * \param replay The replay block to use for word size lookup.
 * \param num_bytes The number of bytes to coerce.
 * \returns The coerced number of bytes.
 */
size_t coerce_to_word_size(replay_block_control::sptr replay, size_t num_bytes)
{
    return num_bytes - (num_bytes % replay->get_word_size());
}

/*!
 * Helper to download a chunk of data from the replay block.
 *
 * Tries to download a chunk from replay. There is only a single attempt to get
 * the data. If this fails the streamer is stopped remaining data is flushed and false
 * is returned.
 *
 * \param replay The replay block to use.
 * \param port The output port of the replay block to use.
 * \param rx_streamer The RX streamer to use.
 * \param buff The buffer to write the samples to.
 *             This function adds offset to buffer so no need to adjust this beforehand.
 * \param offset_samples The sample offset into the buffer to start writing to.
 * \param num_samples The number of samples to download.
 * \param bytes_per_sample The number of bytes per sample.
 * \param offset The offset into the replay memory to start reading from (in samples).
 * \returns True on success, false on failure.
 */
bool download_chunk(replay_block_control::sptr replay,
    size_t port,
    uhd::rx_streamer::sptr rx_streamer,
    const void* buff,
    size_t offset_samples,
    size_t num_samples,
    size_t bytes_per_sample,
    size_t offset)
{
    replay->config_play((offset + offset_samples) * bytes_per_sample,
        num_samples * bytes_per_sample,
        port);

    uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
    stream_cmd.num_samps = num_samples;
    rx_streamer->issue_stream_cmd(stream_cmd);
    uhd::rx_metadata_t rx_md;
    size_t rx_samples = rx_streamer->recv(
        static_cast<const char*>(buff) + (offset_samples * bytes_per_sample),
        num_samples,
        rx_md,
        1.0);
    bool result = (rx_samples == num_samples)
                  && (rx_md.error_code == uhd::rx_metadata_t::ERROR_CODE_NONE);
    if (!result) {
        uhd::stream_cmd_t stream_cmd(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
        rx_streamer->issue_stream_cmd(stream_cmd);
        while (true) {
            // for flushing data we can reuse buff, because the data is not valid and
            // will be overwritten with the next attempt.
            rx_streamer->recv(
                static_cast<const char*>(buff) + (offset_samples * bytes_per_sample),
                num_samples,
                rx_md,
                1.0);
            if (rx_md.error_code == rx_metadata_t::ERROR_CODE_TIMEOUT) {
                // clear error code because this is expected behaviour
                // and shouldn't be logged as an error
                rx_md.error_code = rx_metadata_t::ERROR_CODE_NONE;
                break;
            }
        }
        if (rx_samples != num_samples) {
            UHD_LOG_DEBUG(DRAM_UTILS,
                "Missed samples received " << rx_samples << " vs expected "
                                           << num_samples);
        }
        if (rx_md.error_code != uhd::rx_metadata_t::ERROR_CODE_NONE) {
            UHD_LOG_DEBUG(DRAM_UTILS, "Received had metadata error " << rx_md.error_code);
        }
    }
    return result;
}

void upload(rfnoc_graph::sptr graph,
    const void* buff,
    const size_t nsamps,
    const std::string& cpu_fmt,
    const std::string& otw_fmt,
    replay_block_control::sptr replay,
    size_t port,
    size_t offset)
{
    UHD_LOG_DEBUG(DRAM_UTILS, "Send " << nsamps << " samples to replay block.");
    replay          = get_replay_block(graph, replay);
    auto [src, dst] = connected_blocks(graph, replay->get_unique_id(), port);

    if (src) {
        // if something is connected to the input port of the replay block
        // disconnect it temporarily
        UHD_LOG_TRACE(DRAM_UTILS,
            "Disconnect source block " << src->first << "#" << src->second
                                       << " for replay transfer.");
        graph->disconnect(src->first, src->second, replay->get_unique_id(), port);
    }

    const size_t bytes_per_sample = uhd::convert::get_bytes_per_item(cpu_fmt);
    const size_t num_bytes        = bytes_per_sample * nsamps;
    uhd::stream_args_t args(cpu_fmt, otw_fmt);
    args.args["transmit_policy"] =
        "stop_on_seq_error"; // stop on loss, to enable transmit restart
    auto tx_streamer = graph->create_tx_streamer(1, args);
    graph->connect(tx_streamer, 0, replay->get_unique_id(), port);
    graph->commit();

    replay->record(offset * bytes_per_sample, num_bytes, port);

    uhd::tx_metadata_t tx_md;
    tx_md.start_of_burst = true;
    tx_md.end_of_burst   = true;

    size_t UHD_UNUSED(sent_samples) = 0, received_samples = 0;

    while (bytes_per_sample * received_samples < replay->get_record_size(port)) {
        UHD_LOG_TRACE(DRAM_UTILS,
            "Send " << (nsamps - received_samples) << " samples to replay block");
        sent_samples += tx_streamer->send(
            static_cast<const char*>(buff) + received_samples * bytes_per_sample,
            nsamps - received_samples,
            tx_md,
            1.0);

        received_samples = get_record_fullness(replay, port) / bytes_per_sample;
    }

    UHD_LOG_TRACE(DRAM_UTILS, "Sent " << sent_samples << " samples to replay block");

    tx_streamer.reset();

    if (src) {
        UHD_LOG_TRACE(DRAM_UTILS,
            "Reconnect source block " << src->first << "#" << src->second
                                      << " after replay transfer.");
        graph->connect(src->first, src->second, replay->get_unique_id(), port);
    }
    graph->commit();
}

void download(rfnoc_graph::sptr graph,
    const void* buff,
    const size_t nsamps,
    const std::string& cpu_fmt,
    const std::string& otw_fmt,
    replay_block_control::sptr replay,
    size_t port,
    size_t offset)
{
    UHD_LOG_DEBUG(DRAM_UTILS, "Receive " << nsamps << " samples from replay block");
    replay = get_replay_block(graph, replay);

    auto [src, dst] = connected_blocks(graph, replay->get_unique_id(), port);
    if (dst) {
        UHD_LOG_TRACE(DRAM_UTILS,
            "Disconnect dest block " << dst->first << "#" << dst->second
                                     << " for replay transfer.");
        graph->disconnect(replay->get_unique_id(), port, dst->first, dst->second);
    }

    auto args        = stream_args_t(cpu_fmt, otw_fmt);
    auto rx_streamer = graph->create_rx_streamer(1, args);
    graph->connect(replay->get_unique_id(), port, rx_streamer, 0);
    graph->commit();

    const size_t bytes_per_sample = uhd::convert::get_bytes_per_item(cpu_fmt);
    const size_t chunk_in_bytes   = coerce_to_word_size(
        replay, 1000 * rx_streamer->get_max_num_samps() * bytes_per_sample);
    size_t samples_received = 0;

    UHD_LOG_INFO(DRAM_UTILS, "Download chunks of " << chunk_in_bytes << " bytes");

    size_t chunk_failures = 0;
    // download data in chunks
    while (samples_received < nsamps) {
        size_t chunk_nsamps =
            std::min(chunk_in_bytes / bytes_per_sample, nsamps - samples_received);
        UHD_LOG_TRACE(
            DRAM_UTILS, "Receive " << nsamps << " samples start at " << samples_received);

        bool success = download_chunk(replay,
            port,
            rx_streamer,
            buff,
            samples_received,
            chunk_nsamps,
            bytes_per_sample,
            offset);
        if (!success) {
            // we need some exit criteria to break the loop if receive is not working
            // MAX_CHUNK_DOWNLOAD_RETRIES is arbitrary, but should be enough to get
            // a few tries.
            if (chunk_failures > MAX_CHUNK_DOWNLOAD_RETRIES) {
                break;
            }
            chunk_failures++;
        } else {
            chunk_failures = 0;
            samples_received += chunk_nsamps;
        }
    }

    rx_streamer.reset();

    if (dst) {
        UHD_LOG_TRACE(DRAM_UTILS,
            "Reconnect dest block " << dst->first << "#" << dst->second
                                    << " after replay transfer.");
        graph->connect(replay->get_unique_id(), port, dst->first, dst->second);
    }
    graph->commit();
}

} // namespace uhd::rfnoc::dram
