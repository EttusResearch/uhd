//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/rfnoc/replay_block_ctrl.hpp>
#include <mutex>

using namespace uhd;
using namespace uhd::rfnoc;

class replay_block_ctrl_impl : public replay_block_ctrl
{
public:
    static const uint32_t REPLAY_WORD_SIZE    = 8; // In bytes
    static const uint32_t SAMPLES_PER_WORD    = 2;
    static const uint32_t BYTES_PER_SAMPLE    = 4;
    static const uint32_t DEFAULT_BUFFER_SIZE = 32 * 1024 * 1024;
    static const uint32_t DEFAULT_WPP         = 182;
    static const uint32_t DEFAULT_SPP         = DEFAULT_WPP * SAMPLES_PER_WORD;


    UHD_RFNOC_BLOCK_CONSTRUCTOR(replay_block_ctrl)
    {
        _num_channels = get_input_ports().size();
        _params.resize(_num_channels);
        for (size_t chan = 0; chan < _params.size(); chan++) {
            _params[chan].words_per_packet = DEFAULT_WPP;
            sr_write("RX_CTRL_MAXLEN", DEFAULT_WPP, chan);

            // Configure replay channels to be adjacent DEFAULT_BUFFER_SIZE'd blocks
            _params[chan].rec_base_addr    = chan * DEFAULT_BUFFER_SIZE;
            _params[chan].play_base_addr   = chan * DEFAULT_BUFFER_SIZE;
            _params[chan].rec_buffer_size  = DEFAULT_BUFFER_SIZE;
            _params[chan].play_buffer_size = DEFAULT_BUFFER_SIZE;
            sr_write("REC_BASE_ADDR", _params[chan].rec_base_addr, chan);
            sr_write("REC_BUFFER_SIZE", _params[chan].rec_buffer_size, chan);
            sr_write("PLAY_BASE_ADDR", _params[chan].play_base_addr, chan);
            sr_write("PLAY_BUFFER_SIZE", _params[chan].play_buffer_size, chan);
        }
    }


    /**************************************************************************
     * API Calls
     **************************************************************************/

    void config_record(const uint32_t base_addr, const uint32_t size, const size_t chan)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _params[chan].rec_base_addr   = base_addr;
        _params[chan].rec_buffer_size = size;
        sr_write("REC_BASE_ADDR", base_addr, chan);
        sr_write("REC_BUFFER_SIZE", size, chan);
        sr_write("REC_RESTART", 0, chan);
    }

    void config_play(const uint32_t base_addr, const uint32_t size, const size_t chan)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _params[chan].play_base_addr   = base_addr;
        _params[chan].play_buffer_size = size;
        sr_write("PLAY_BASE_ADDR", base_addr, chan);
        sr_write("PLAY_BUFFER_SIZE", size, chan);
    }

    void record_restart(const size_t chan)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        sr_write("REC_RESTART", 0, chan);
    }

    uint32_t get_record_addr(const size_t chan)
    {
        return _params[chan].rec_base_addr;
    }

    uint32_t get_record_size(const size_t chan)
    {
        return _params[chan].rec_buffer_size;
    }

    uint32_t get_record_fullness(const size_t chan)
    {
        return user_reg_read32("REC_FULLNESS", chan);
    }

    uint32_t get_play_addr(const size_t chan)
    {
        return _params[chan].play_base_addr;
    }

    uint32_t get_play_size(const size_t chan)
    {
        return _params[chan].play_buffer_size;
    }

    void set_words_per_packet(const uint32_t num_words, const size_t chan)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _params[chan].words_per_packet = num_words;
        sr_write("RX_CTRL_MAXLEN", num_words, chan);
    }

    uint32_t get_words_per_packet(const size_t chan)
    {
        return _params[chan].words_per_packet;
    }

    void play_halt(const size_t chan)
    {
        sr_write("RX_CTRL_HALT", 1, chan);
    }


    /***************************************************************************
     * Radio-like Streamer
     **************************************************************************/

    void issue_stream_cmd(const uhd::stream_cmd_t& stream_cmd, const size_t chan)
    {
        std::lock_guard<std::mutex> lock(_mutex);
        UHD_RFNOC_BLOCK_TRACE() << "replay_block_ctrl_impl::issue_stream_cmd() " << chan
                                << " " << char(stream_cmd.stream_mode);

        if (not(_rx_streamer_active.count(chan) and _rx_streamer_active.at(chan))) {
            UHD_RFNOC_BLOCK_TRACE()
                << "replay_block_ctrl_impl::issue_stream_cmd() called on inactive "
                   "channel. Skipping.";
            return;
        }

        constexpr size_t max_num_samps = 0x0fffffff;
        if (stream_cmd.num_samps > max_num_samps) {
            UHD_LOG_ERROR("REPLAY",
                "Requesting too many samples in a single burst! "
                "Requested "
                    + std::to_string(stream_cmd.num_samps)
                    + ", maximum "
                      "is "
                    + std::to_string(max_num_samps) + ".");
            throw uhd::value_error("Requested too many samples in a single burst.");
        }

        // Setup the mode to instruction flags
        typedef std::tuple<bool, bool, bool, bool> inst_t;
        static const std::map<stream_cmd_t::stream_mode_t, inst_t> mode_to_inst{
            // reload, chain, samps, stop
            {stream_cmd_t::STREAM_MODE_START_CONTINUOUS,
                inst_t(true, true, false, false)},
            {stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS,
                inst_t(false, false, false, true)},
            {stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE,
                inst_t(false, false, true, false)},
            {stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE,
                inst_t(false, true, true, false)}};

        // Setup the instruction flag values
        bool inst_reload, inst_chain, inst_samps, inst_stop;
        std::tie(inst_reload, inst_chain, inst_samps, inst_stop) =
            mode_to_inst.at(stream_cmd.stream_mode);

        // Calculate how many words to transfer at a time in CONTINUOUS mode
        uint32_t cont_burst_size =
            (_params[chan].play_buffer_size > _params[chan].words_per_packet)
                ? _params[chan].words_per_packet
                : _params[chan].play_buffer_size;

        // Calculate the number of words to transfer in NUM_SAMPS mode
        uint32_t num_words = stream_cmd.num_samps / SAMPLES_PER_WORD;

        // Calculate the word from flags and length
        const uint32_t cmd_word =
            0 | (uint32_t(stream_cmd.stream_now ? 1 : 0) << 31)
            | (uint32_t(inst_chain ? 1 : 0) << 30) | (uint32_t(inst_reload ? 1 : 0) << 29)
            | (uint32_t(inst_stop ? 1 : 0) << 28)
            | (inst_samps ? num_words : (inst_stop ? 0 : cont_burst_size));

        // Issue the stream command
        sr_write("RX_CTRL_COMMAND", cmd_word, chan);
    }

private:
    struct replay_params_t
    {
        size_t words_per_packet;
        uint32_t rec_base_addr;
        uint32_t rec_buffer_size;
        uint32_t play_base_addr;
        uint32_t play_buffer_size;
    };

    size_t _num_channels;
    std::vector<replay_params_t> _params;

    std::mutex _mutex;
};

UHD_RFNOC_BLOCK_REGISTER(replay_block_ctrl, "Replay");
