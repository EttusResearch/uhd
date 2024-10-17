//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhdlib/rfnoc/rfnoc_tx_streamer_replay_buffered.hpp>
#include <numeric>

using namespace uhd;
using namespace uhd::rfnoc;

rfnoc_tx_streamer_replay_buffered::rfnoc_tx_streamer_replay_buffered(
    const size_t num_ports,
    const uhd::stream_args_t stream_args,
    std::function<void(const std::string&)> disconnect_cb,
    std::vector<replay_config_t> replay_configs)
    : rfnoc_tx_streamer(num_ports, stream_args, disconnect_cb)
    , _bytes_per_otw_item(uhd::convert::get_bytes_per_item(stream_args.otw_format))
{
    if (replay_configs.size() != num_ports) {
        throw uhd::value_error("[TX Streamer] Number of Replay configurations "
                               "does not match the number of channels");
    }

    for (auto config : replay_configs) {
        _replay_chans.push_back({config, 0, 0, 0});
        config.ctrl->set_play_type(stream_args.otw_format);
    }
}

rfnoc_tx_streamer_replay_buffered::~rfnoc_tx_streamer_replay_buffered()
{
    // Stop all playback
    for (auto chan : _replay_chans) {
        UHD_SAFE_CALL(chan.config.ctrl->stop(chan.config.port);)
    }
}

size_t rfnoc_tx_streamer_replay_buffered::send(const buffs_type& buffs,
    const size_t nsamps_per_buff,
    const tx_metadata_t& metadata,
    const double timeout)
{
    uint64_t record_size = nsamps_per_buff * _bytes_per_otw_item;

    // Make sure there is space in the buffer
    auto timeout_time = std::chrono::steady_clock::now()
                        + std::chrono::microseconds(long(timeout * 1000000));
    for (auto& chan : _replay_chans) {
        const auto& config   = chan.config;
        const auto& replay   = config.ctrl;
        auto& play_offset    = chan.play_offset;
        const auto& play_end = chan.play_end;

        // Make sure the send does not exceed the memory space
        if (record_size > config.mem_size) {
            throw uhd::runtime_error("[multi_usrp] Unable to buffer more than "
                                     + std::to_string(config.mem_size) + " bytes");
        }

        // Make sure nsamps_per_buff is properly aligned to the DRAM
        if (record_size % replay->get_word_size() != 0) {
            throw uhd::runtime_error(
                "[multi_usrp] Number of samples for send() call must be a "
                "multiple of "
                + std::to_string(std::lcm<uint64_t>(
                    replay->get_word_size(), uint64_t(_bytes_per_otw_item)))
                + " for DRAM alignment");
        }

        // The buffer can be full or empty when play_offset and play_end
        // are the same, so subtract one from the calculated room to make
        // sure the buffer is never absolutely full and it can be assumed
        // the buffer is empty when they are the same.
        auto room = (play_end == play_offset ? config.mem_size - 1
                     : play_end < play_offset
                         ? play_offset - play_end - 1
                         : std::max<uint64_t>(
                             config.mem_size - play_end - 1, play_offset - 1));
        while (room < record_size) {
            // Return 0 if timeout
            if (std::chrono::steady_clock::now() > timeout_time) {
                UHD_LOG_TRACE(
                    "MULTI_USRP", "send() timed out waiting for room in buffer");
                return 0;
            }
            try {
                play_offset =
                    replay->get_play_position(config.port) - config.start_address;
            } catch (uhd::op_timeout&) {
                // Internal timeout trying to read the register
                continue;
            }
            room = (play_end == play_offset ? config.mem_size - 1
                    : play_end < play_offset
                        ? play_offset - play_end - 1
                        : std::max<uint64_t>(
                            config.mem_size - play_end - 1, play_offset - 1));
        }
    }

    // Set up replay blocks to record
    for (auto& chan : _replay_chans) {
        const auto& config   = chan.config;
        const auto& replay   = config.ctrl;
        const auto& play_end = chan.play_end;
        auto& record_offset  = chan.record_offset;

        // Default to using space at end of last playback
        record_offset = play_end;
        // Change to beginning of memory block if not enough room at end
        if (config.mem_size - record_offset < record_size) {
            record_offset = 0;
        }
        while (1) {
            try {
                replay->record(
                    config.start_address + record_offset, record_size, config.port);
                break;
            } catch (uhd::op_timeout& e) {
                // Internal timeout trying to write the registers
                // Return 0 if timeout
                if (std::chrono::steady_clock::now() > timeout_time) {
                    UHD_LOG_TRACE("MULTI_USRP",
                        std::string("send() timed out while setting up to record: ")
                            + e.what());
                    return 0;
                }
            }
        }
    }

    // Send data to replay blocks
    auto num_samps = rfnoc_tx_streamer::send(buffs, nsamps_per_buff, metadata, timeout);

    if (num_samps) {
        // Play data
        for (auto& chan : _replay_chans) {
            const auto& config        = chan.config;
            const auto& replay        = config.ctrl;
            const auto& record_offset = chan.record_offset;
            const uint64_t play_start = config.start_address + record_offset;
            const uint64_t play_size  = num_samps * _bytes_per_otw_item;
            auto& play_end            = chan.play_end;

            while (1) {
                try {
                    replay->config_play(play_start, play_size, config.port);
                    uhd::stream_cmd_t play_cmd =
                        metadata.end_of_burst
                            ? uhd::stream_cmd_t(
                                uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE)
                            : uhd::stream_cmd_t(
                                uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE);
                    play_cmd.num_samps =
                        play_size / replay->get_play_item_size(config.port);
                    play_cmd.time_spec  = metadata.time_spec;
                    play_cmd.stream_now = not metadata.has_time_spec;
                    replay->issue_stream_cmd(play_cmd, config.port);
                    break;
                } catch (uhd::op_failed& e) {
                    // Too many play commands in queue
                    // Return 0 if timeout.
                    if (std::chrono::steady_clock::now() > timeout_time) {
                        UHD_LOG_TRACE("MULTI_USRP",
                            std::string("send() timed out issuing play command: ")
                                + e.what());
                        return 0;
                    }
                } catch (uhd::op_timeout& e) {
                    // Internal timeout trying to write the registers
                    // Return 0 if timeout
                    if (std::chrono::steady_clock::now() > timeout_time) {
                        UHD_LOG_TRACE("MULTI_USRP",
                            std::string("send() timed out issuing play command: ")
                                + e.what());
                        return 0;
                    }
                }
            }

            play_end = record_offset + play_size;
        }
    }
    return num_samps;
}
