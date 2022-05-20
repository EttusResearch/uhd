//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/convert.hpp>
#include <uhd/exception.hpp>
#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/property.hpp>
#include <uhd/rfnoc/registry.hpp>
#include <uhd/rfnoc/replay_block_control.hpp>
#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/types/stream_cmd.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/utils/compat_check.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <string>

using namespace uhd::rfnoc;

// Block compatability version
const uint16_t replay_block_control::MINOR_COMPAT = 1;
const uint16_t replay_block_control::MAJOR_COMPAT = 1;

// NoC block address space
const uint32_t replay_block_control::REPLAY_ADDR_W = 8;
const uint32_t replay_block_control::REPLAY_BLOCK_OFFSET =
    1 << replay_block_control::REPLAY_ADDR_W; // 256 bytes

// Register offsets
const uint32_t replay_block_control::REG_COMPAT_ADDR                = 0x00;
const uint32_t replay_block_control::REG_MEM_SIZE_ADDR              = 0x04;
const uint32_t replay_block_control::REG_REC_RESTART_ADDR           = 0x08;
const uint32_t replay_block_control::REG_REC_BASE_ADDR_LO_ADDR      = 0x10;
const uint32_t replay_block_control::REG_REC_BASE_ADDR_HI_ADDR      = 0x14;
const uint32_t replay_block_control::REG_REC_BUFFER_SIZE_LO_ADDR    = 0x18;
const uint32_t replay_block_control::REG_REC_BUFFER_SIZE_HI_ADDR    = 0x1C;
const uint32_t replay_block_control::REG_REC_FULLNESS_LO_ADDR       = 0x20;
const uint32_t replay_block_control::REG_REC_FULLNESS_HI_ADDR       = 0x24;
const uint32_t replay_block_control::REG_PLAY_BASE_ADDR_LO_ADDR     = 0x28;
const uint32_t replay_block_control::REG_PLAY_BASE_ADDR_HI_ADDR     = 0x2C;
const uint32_t replay_block_control::REG_PLAY_BUFFER_SIZE_LO_ADDR   = 0x30;
const uint32_t replay_block_control::REG_PLAY_BUFFER_SIZE_HI_ADDR   = 0x34;
const uint32_t replay_block_control::REG_PLAY_CMD_NUM_WORDS_LO_ADDR = 0x38;
const uint32_t replay_block_control::REG_PLAY_CMD_NUM_WORDS_HI_ADDR = 0x3C;
const uint32_t replay_block_control::REG_PLAY_CMD_TIME_LO_ADDR      = 0x40;
const uint32_t replay_block_control::REG_PLAY_CMD_TIME_HI_ADDR      = 0x44;
const uint32_t replay_block_control::REG_PLAY_CMD_ADDR              = 0x48;
const uint32_t replay_block_control::REG_PLAY_WORDS_PER_PKT_ADDR    = 0x4C;
const uint32_t replay_block_control::REG_PLAY_ITEM_SIZE_ADDR        = 0x50;
const uint32_t replay_block_control::REG_REC_POS_LO_ADDR            = 0x54;
const uint32_t replay_block_control::REG_REC_POS_HI_ADDR            = 0x58;
const uint32_t replay_block_control::REG_PLAY_POS_LO_ADDR           = 0x5C;
const uint32_t replay_block_control::REG_PLAY_POS_HI_ADDR           = 0x60;
const uint32_t replay_block_control::REG_PLAY_CMD_FIFO_SPACE_ADDR   = 0x64;

// Stream commands
const uint32_t replay_block_control::PLAY_CMD_STOP       = 0;
const uint32_t replay_block_control::PLAY_CMD_FINITE     = 1;
const uint32_t replay_block_control::PLAY_CMD_CONTINUOUS = 2;

// Mask bits
constexpr uint32_t PLAY_COMMAND_TIMED_BIT  = 31;
constexpr uint32_t PLAY_COMMAND_TIMED_MASK = uint32_t(1) << PLAY_COMMAND_TIMED_BIT;
constexpr uint32_t PLAY_COMMAND_MASK       = 3;

// User property names
const char* const PROP_KEY_RECORD_OFFSET = "record_offset";
const char* const PROP_KEY_RECORD_SIZE   = "record_size";
const char* const PROP_KEY_PLAY_OFFSET   = "play_offset";
const char* const PROP_KEY_PLAY_SIZE     = "play_size";
const char* const PROP_KEY_PKT_SIZE      = "packet_size";

// Depth of the async message queues
constexpr size_t ASYNC_MSG_QUEUE_SIZE = 128;

class replay_block_control_impl : public replay_block_control
{
public:
    RFNOC_BLOCK_CONSTRUCTOR(replay_block_control),
        _replay_reg_iface(*this, 0, REPLAY_BLOCK_OFFSET),
        _num_input_ports(get_num_input_ports()),
        _num_output_ports(get_num_output_ports()),
        _fpga_compat(_replay_reg_iface.peek32(REG_COMPAT_ADDR)),
        _word_size(
            uint16_t((_replay_reg_iface.peek32(REG_MEM_SIZE_ADDR) >> 16) & 0xFFFF) / 8),
        _mem_size(
            uint64_t(1ULL << (_replay_reg_iface.peek32(REG_MEM_SIZE_ADDR) & 0xFFFF))),
        _cmd_fifo_spaces(_num_output_ports)
    {
        if (get_num_input_ports() != get_num_output_ports()) {
            throw uhd::assertion_error(
                "Replay block has invalid hardware configuration! Number of input ports "
                "does not match number of output ports.");
        }
        uhd::assert_fpga_compat(MAJOR_COMPAT,
            MINOR_COMPAT,
            _fpga_compat,
            get_unique_id(),
            get_unique_id(),
            false /* Let it slide if minors mismatch */
        );
        RFNOC_LOG_TRACE(
            "Initializing replay block with num ports=" << get_num_input_ports());
        // Properties and actions can't propagate through this block, as we
        // treat source and sink of this block like the radio (they terminate
        // the graph).
        set_prop_forwarding_policy(forwarding_policy_t::DROP);
        set_action_forwarding_policy(forwarding_policy_t::DROP);
        // Same for MTU
        set_mtu_forwarding_policy(forwarding_policy_t::DROP);
        // Register action handler
        register_action_handler(ACTION_KEY_STREAM_CMD,
            [this](const res_source_info& src, action_info::sptr action) {
                stream_cmd_action_info::sptr stream_cmd_action =
                    std::dynamic_pointer_cast<stream_cmd_action_info>(action);
                if (!stream_cmd_action) {
                    RFNOC_LOG_WARNING("Received invalid stream command action!");
                    return;
                }
                RFNOC_LOG_TRACE("Received stream command: "
                                << stream_cmd_action->stream_cmd.stream_mode << " to "
                                << src.to_string());
                if (src.type != res_source_info::OUTPUT_EDGE) {
                    RFNOC_LOG_WARNING(
                        "Received stream command, but not to output port! Ignoring.");
                    return;
                }
                const size_t port = src.instance;
                if (port >= get_num_output_ports()) {
                    RFNOC_LOG_WARNING("Received stream command to invalid output port!");
                    return;
                }
                issue_stream_cmd(stream_cmd_action->stream_cmd, port);
            });
        register_action_handler(ACTION_KEY_RX_EVENT,
            [this](const res_source_info& src, action_info::sptr action) {
                rx_event_action_info::sptr rx_event_action =
                    std::dynamic_pointer_cast<rx_event_action_info>(action);
                if (!rx_event_action) {
                    RFNOC_LOG_WARNING("Received invalid RX event action!");
                    return;
                }
                _handle_rx_event_action(src, rx_event_action);
            });
        register_action_handler(ACTION_KEY_TX_EVENT,
            [this](const res_source_info& src, action_info::sptr action) {
                tx_event_action_info::sptr tx_event_action =
                    std::dynamic_pointer_cast<tx_event_action_info>(action);
                if (!tx_event_action) {
                    RFNOC_LOG_WARNING("Received invalid TX event action!");
                    return;
                }
                _handle_tx_event_action(src, tx_event_action);
            });

        // Initialize record properties
        _record_type.reserve(_num_input_ports);
        _record_offset.reserve(_num_input_ports);
        _record_size.reserve(_num_input_ports);
        _atomic_item_size_in.reserve(_num_input_ports);
        for (size_t port = 0; port < _num_input_ports; port++) {
            _register_input_props(port);
            _replay_reg_iface.poke64(
                REG_REC_BASE_ADDR_LO_ADDR, _record_offset.at(port).get(), port);
            _replay_reg_iface.poke64(
                REG_REC_BUFFER_SIZE_LO_ADDR, _record_size.at(port).get(), port);
        }


        // Initialize playback properties
        _play_type.reserve(_num_output_ports);
        _play_offset.reserve(_num_output_ports);
        _play_size.reserve(_num_output_ports);
        _packet_size.reserve(_num_output_ports);
        _atomic_item_size_out.reserve(_num_output_ports);
        for (size_t port = 0; port < _num_output_ports; port++) {
            _register_output_props(port);
            _replay_reg_iface.poke32(REG_PLAY_ITEM_SIZE_ADDR,
                uhd::convert::get_bytes_per_item(_play_type.at(port).get()),
                port);
            _replay_reg_iface.poke64(
                REG_PLAY_BASE_ADDR_LO_ADDR, _play_offset.at(port).get(), port);
            _replay_reg_iface.poke64(
                REG_PLAY_BUFFER_SIZE_LO_ADDR, _play_size.at(port).get(), port);
            _replay_reg_iface.poke32(REG_PLAY_WORDS_PER_PKT_ADDR,
                (_packet_size.at(port).get() - get_chdr_hdr_len()) / _word_size,
                port);
            // The register to get the command FIFO space was added in v1.1
            if (_fpga_compat >= 0x00010001) {
                _cmd_fifo_spaces[port] = _replay_reg_iface.peek32(
                    REG_PLAY_CMD_FIFO_SPACE_ADDR, port);
            }
        }
    }

    /**************************************************************************
     * Replay Control API
     **************************************************************************/
    void record(const uint64_t offset, const uint64_t size, const size_t port) override
    {
        set_property<uint64_t>(
            PROP_KEY_RECORD_OFFSET, offset, {res_source_info::USER, port});
        set_property<uint64_t>(PROP_KEY_RECORD_SIZE, size, {res_source_info::USER, port});

        // The pointers to the new record buffer space must be set
        record_restart(port);
    }

    void record_restart(const size_t port) override
    {
        // Ensure that the buffer is properly configured before recording
        _validate_record_buffer(port);
        // Any value written to this register causes a record restart
        _replay_reg_iface.poke32(REG_REC_RESTART_ADDR, 0, port);
    }

    void play(const uint64_t offset,
        const uint64_t size,
        const size_t port,
        const uhd::time_spec_t time_spec,
        const bool repeat) override
    {
        config_play(offset, size, port);
        uhd::stream_cmd_t play_cmd =
            repeat ? uhd::stream_cmd_t(uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS)
                   : uhd::stream_cmd_t(uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE);
        play_cmd.num_samps  = size / get_play_item_size(port);
        play_cmd.time_spec  = time_spec;
        play_cmd.stream_now = (time_spec == 0.0);
        issue_stream_cmd(play_cmd, port);
    }

    void stop(const size_t port) override
    {
        uhd::stream_cmd_t stop_cmd =
            uhd::stream_cmd_t(uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS);
        issue_stream_cmd(stop_cmd, port);
    }

    uint64_t get_mem_size() const override
    {
        return _mem_size;
    }

    uint64_t get_word_size() const override
    {
        return _word_size;
    }

    /**************************************************************************
     * Record Buffer State API
     **************************************************************************/
    uint64_t get_record_offset(const size_t port) const override
    {
        return _record_offset.at(port).get();
    }

    uint64_t get_record_size(const size_t port) const override
    {
        return _record_size.at(port).get();
    }

    uint64_t get_record_fullness(const size_t port) override
    {
        return _replay_reg_iface.peek64(REG_REC_FULLNESS_LO_ADDR, port);
    }

    uint64_t get_record_position(const size_t port) override
    {
        if (_fpga_compat < 0x00010001) {
            throw uhd::not_implemented_error("Replay block version 1.1 or "
                "greater required to get record position.  "
                "Update the FPGA image to get this feature.");
        }
        return _replay_reg_iface.peek64(REG_REC_POS_LO_ADDR, port);
    }

    io_type_t get_record_type(const size_t port) const override
    {
        return _record_type.at(port).get();
    }

    size_t get_record_item_size(const size_t port) const override
    {
        return uhd::convert::get_bytes_per_item(get_record_type(port));
    }

    bool get_record_async_metadata(
        uhd::rx_metadata_t& metadata, const double timeout = 0.0)
    {
        return _record_msg_queue.pop_with_timed_wait(metadata, timeout);
    }

    /**************************************************************************
     * Playback State API
     **************************************************************************/
    uint64_t get_play_offset(const size_t port) const override
    {
        return _play_offset.at(port).get();
    }

    uint64_t get_play_size(const size_t port) const override
    {
        return _play_size.at(port).get();
    }

    uint64_t get_play_position(const size_t port) override
    {
        if (_fpga_compat < 0x00010001) {
            throw uhd::not_implemented_error(
                "Replay block version 1.1 or greater required to get play position.  "
                "Update the FPGA image to get this feature.");
        }
        return _replay_reg_iface.peek64(REG_PLAY_POS_LO_ADDR, port);
    }

    uint32_t get_max_items_per_packet(const size_t port) const override
    {
        return (_packet_size.at(port).get() - get_chdr_hdr_len())
               / get_play_item_size(port);
    }

    uint32_t get_max_packet_size(const size_t port) const override
    {
        return _packet_size.at(port).get();
    }

    io_type_t get_play_type(const size_t port) const override
    {
        return _play_type.at(port).get();
    }

    size_t get_play_item_size(const size_t port) const override
    {
        return uhd::convert::get_bytes_per_item(get_play_type(port));
    }

    bool get_play_async_metadata(uhd::async_metadata_t& metadata, const double timeout)
    {
        return _playback_msg_queue.pop_with_timed_wait(metadata, timeout);
    }

    /**************************************************************************
     * Advanced Record Control API calls
     *************************************************************************/
    void set_record_type(const io_type_t type, const size_t port) override
    {
        set_property<std::string>(
            PROP_KEY_TYPE, type, {res_source_info::INPUT_EDGE, port});
    }

    /**************************************************************************
     * Advanced Playback Control API
     **************************************************************************/
    void config_play(
        const uint64_t offset, const uint64_t size, const size_t port) override
    {
        set_property<uint64_t>(
            PROP_KEY_PLAY_OFFSET, offset, {res_source_info::USER, port});
        set_property<uint64_t>(PROP_KEY_PLAY_SIZE, size, {res_source_info::USER, port});
        _validate_play_buffer(port);
    }

    void set_play_type(const io_type_t type, const size_t port) override
    {
        set_property<std::string>(
            PROP_KEY_TYPE, type, {res_source_info::OUTPUT_EDGE, port});
    }

    void set_max_items_per_packet(const uint32_t ipp, const size_t port) override
    {
        set_max_packet_size(get_chdr_hdr_len() + ipp * get_play_item_size(port), port);
    }

    void set_max_packet_size(const uint32_t size, const size_t port) override
    {
        set_property<uint32_t>(PROP_KEY_PKT_SIZE, size, {res_source_info::USER, port});
    }

    void issue_stream_cmd(const uhd::stream_cmd_t& stream_cmd, const size_t port) override
    {
        // Ensure that the buffer is properly configured before issuing a stream command
        _validate_play_buffer(port);
        RFNOC_LOG_TRACE("replay_block_control_impl::issue_stream_cmd(port="
                        << port << ", mode=" << char(stream_cmd.stream_mode) << ")");

        // Setup the mode to instruction flags
        const uint8_t play_cmd = [stream_cmd]() -> uint8_t {
            switch (stream_cmd.stream_mode) {
                case uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS:
                    return PLAY_CMD_STOP; // Stop playing back data
                case uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE:
                case uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE:
                    return PLAY_CMD_FINITE; // Play NUM_SAMPS then stop
                case uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS:
                    return PLAY_CMD_CONTINUOUS; // Playback continuously over the play
                                                // buffer until stopped
                default:
                    throw uhd::value_error("Requested invalid stream command.");
            }
        }();

        // The register to get the command FIFO space was added in v1.1
        if (_fpga_compat >= 0x00010001) {
            // Make sure the command queue has space.  Allow stop commands to pass.
            if (not play_cmd == PLAY_CMD_STOP and _cmd_fifo_spaces[port] == 0) {
                _cmd_fifo_spaces[port] = _replay_reg_iface.peek32(
                    REG_PLAY_CMD_FIFO_SPACE_ADDR, port);
                if (_cmd_fifo_spaces[port] == 0) {
                    throw uhd::op_failed("[Replay] Play command queue is full");
                }
            }
        }

        // Calculate the number of words to transfer in NUM_SAMPS mode
        if (play_cmd == PLAY_CMD_FINITE) {
            uint64_t num_words =
                stream_cmd.num_samps * get_play_item_size(port) / get_word_size();
            _replay_reg_iface.poke64(REG_PLAY_CMD_NUM_WORDS_LO_ADDR, num_words, port);
        }

        // Set the time for the command
        const uint32_t timed_flag = (stream_cmd.stream_now) ? 0 : PLAY_COMMAND_TIMED_MASK;
        if (!stream_cmd.stream_now) {
            const double tick_rate = get_tick_rate();
            UHD_LOG_DEBUG("REPLAY",
                "Using tick rate " << (tick_rate / 1e6) << " MHz to set stream command.");
            const uint64_t ticks = stream_cmd.time_spec.to_ticks(tick_rate);
            _replay_reg_iface.poke64(REG_PLAY_CMD_TIME_LO_ADDR, ticks, port);
        }

        // Issue the stream command
        uint32_t command_word = (play_cmd & PLAY_COMMAND_MASK) | timed_flag;
        _replay_reg_iface.poke32(REG_PLAY_CMD_ADDR, command_word, port);

        // The register to get the command FIFO space was added in v1.1
        if (_fpga_compat >= 0x00010001) {
            if (play_cmd == PLAY_CMD_STOP) {
                // The stop command will clear the FIFO, so reset the space value
                _cmd_fifo_spaces[port] = _replay_reg_iface.peek32(
                    REG_PLAY_CMD_FIFO_SPACE_ADDR, port);
            } else {
                _cmd_fifo_spaces[port]--;
            }
        }
    }

protected:
    // Block-specific register interface
    multichan_register_iface _replay_reg_iface;

private:
    void _register_input_props(const size_t port)
    {
        // Get default property values
        const io_type_t default_type = IO_TYPE_SC16;
        const uint64_t record_offset = 0;
        const uint64_t record_size   = _mem_size;

        // Initialize properties
        _record_type.emplace_back(property_t<std::string>(
            PROP_KEY_TYPE, default_type, {res_source_info::INPUT_EDGE, port}));
        _record_offset.push_back(property_t<uint64_t>(
            PROP_KEY_RECORD_OFFSET, record_offset, {res_source_info::USER, port}));
        _record_size.push_back(property_t<uint64_t>(
            PROP_KEY_RECORD_SIZE, record_size, {res_source_info::USER, port}));
        _atomic_item_size_in.push_back(property_t<size_t>(
            PROP_KEY_ATOMIC_ITEM_SIZE, _word_size, {res_source_info::INPUT_EDGE, port}));
        UHD_ASSERT_THROW(_record_type.size() == port + 1);
        UHD_ASSERT_THROW(_record_offset.size() == port + 1);
        UHD_ASSERT_THROW(_record_size.size() == port + 1);
        UHD_ASSERT_THROW(_atomic_item_size_in.size() == port + 1);

        // Register user properties
        register_property(&_record_type.at(port));
        register_property(&_record_offset.at(port));
        register_property(&_record_size.at(port));
        register_property(&_atomic_item_size_in.at(port));

        // Add property resolvers
        add_property_resolver({&_record_offset.at(port)}, {}, [this, port]() {
            _set_record_offset(_record_offset.at(port).get(), port);
        });
        add_property_resolver({&_record_size.at(port)},
            {&_record_size.at(port)},
            [this, port]() { _set_record_size(_record_size.at(port).get(), port); });
        add_property_resolver({&_atomic_item_size_in.back(),
                                  get_mtu_prop_ref({res_source_info::INPUT_EDGE, port})},
            {&_atomic_item_size_in.back()},
            [this, port, &ais_in = _atomic_item_size_in.back()]() {
                ais_in = uhd::math::lcm<size_t>(ais_in, get_word_size());
                ais_in = std::min<size_t>(
                    ais_in, get_mtu({res_source_info::INPUT_EDGE, port}));
                if (ais_in.get() % get_word_size() > 0) {
                    ais_in = ais_in - (ais_in.get() % get_word_size());
                }
                RFNOC_LOG_TRACE("Resolve atomic item size in to " << ais_in);
            });
    }

    void _register_output_props(const size_t port)
    {
        // Get default property values
        const io_type_t default_type = IO_TYPE_SC16;
        const uint64_t play_offset   = 0;
        const uint64_t play_size     = _mem_size;
        const uint32_t packet_size   = get_mtu({res_source_info::OUTPUT_EDGE, port});

        // Initialize properties
        _play_type.emplace_back(property_t<std::string>(
            PROP_KEY_TYPE, default_type, {res_source_info::OUTPUT_EDGE, port}));
        _play_offset.push_back(property_t<uint64_t>(
            PROP_KEY_PLAY_OFFSET, play_offset, {res_source_info::USER, port}));
        _play_size.push_back(property_t<uint64_t>(
            PROP_KEY_PLAY_SIZE, play_size, {res_source_info::USER, port}));
        _packet_size.push_back(property_t<uint32_t>(
            PROP_KEY_PKT_SIZE, packet_size, {res_source_info::USER, port}));
        _atomic_item_size_out.push_back(property_t<size_t>(
            PROP_KEY_ATOMIC_ITEM_SIZE, _word_size, {res_source_info::OUTPUT_EDGE, port}));
        UHD_ASSERT_THROW(_play_type.size() == port + 1);
        UHD_ASSERT_THROW(_play_offset.size() == port + 1);
        UHD_ASSERT_THROW(_play_size.size() == port + 1);
        UHD_ASSERT_THROW(_packet_size.size() == port + 1);
        UHD_ASSERT_THROW(_atomic_item_size_out.size() == port + 1);

        // Register user properties
        register_property(&_play_type.at(port));
        register_property(&_play_offset.at(port));
        register_property(&_play_size.at(port));
        register_property(&_packet_size.at(port));
        register_property(&_atomic_item_size_out.at(port));

        // Add property resolvers
        add_property_resolver({&_play_type.at(port)}, {}, [this, port]() {
            _set_play_type(_play_type.at(port).get(), port);
        });
        add_property_resolver({&_play_offset.at(port)}, {}, [this, port]() {
            _set_play_offset(_play_offset.at(port).get(), port);
        });
        add_property_resolver({&_play_size.at(port)},
            {&_play_size.at(port)},
            [this, port]() { _set_play_size(_play_size.at(port).get(), port); });
        add_property_resolver({&_packet_size.at(port),
                                  get_mtu_prop_ref({res_source_info::OUTPUT_EDGE, port})},
            {},
            [this, port]() { _set_packet_size(_packet_size.at(port).get(), port); });
        add_property_resolver({&_atomic_item_size_out.back(),
                                  get_mtu_prop_ref({res_source_info::OUTPUT_EDGE, port})},
            {&_atomic_item_size_out.back()},
            [this, port, &ais_out = _atomic_item_size_out.back()]() {
                ais_out = uhd::math::lcm<size_t>(ais_out, get_word_size());
                ais_out = std::min<size_t>(
                    ais_out, get_mtu({res_source_info::OUTPUT_EDGE, port}));
                if (ais_out.get() % get_word_size() > 0) {
                    ais_out = ais_out - (ais_out.get() % get_word_size());
                }
                RFNOC_LOG_TRACE("Resolve atomic item size out to " << ais_out);
            });
    }

    void _set_play_type(const io_type_t type, const size_t port)
    {
        uint32_t play_item_size = uhd::convert::get_bytes_per_item(type);
        _replay_reg_iface.poke32(REG_PLAY_ITEM_SIZE_ADDR, play_item_size, port);
    }

    void _set_record_offset(const uint64_t record_offset, const size_t port)
    {
        if ((record_offset % _word_size) != 0) {
            throw uhd::value_error("Record offset must be a multiple of word size.");
        }
        if (record_offset > _mem_size) {
            throw uhd::value_error("Record offset is out of bounds.");
        }
        _replay_reg_iface.poke64(REG_REC_BASE_ADDR_LO_ADDR, record_offset, port);
    }

    void _set_record_size(const uint64_t record_size, const size_t port)
    {
        if ((record_size % _word_size) != 0) {
            _record_size.at(port) = record_size - (record_size % _word_size);
            throw uhd::value_error("Record buffer size must be a multiple of word size.");
        }
        _replay_reg_iface.poke64(REG_REC_BUFFER_SIZE_LO_ADDR, record_size, port);
    }

    void _set_play_offset(const uint64_t play_offset, const size_t port)
    {
        if ((play_offset % _word_size) != 0) {
            throw uhd::value_error("Play offset must be a multiple of word size.");
        }
        if (play_offset > _mem_size) {
            throw uhd::value_error("Play offset is out of bounds.");
        }
        _replay_reg_iface.poke64(REG_PLAY_BASE_ADDR_LO_ADDR, play_offset, port);
    }

    void _set_play_size(const uint64_t play_size, const size_t port)
    {
        if ((play_size % _word_size) != 0) {
            _play_size.at(port) = play_size - (play_size % _word_size);
            throw uhd::value_error("Play buffer size must be a multiple of word size.");
        }
        if ((play_size % get_play_item_size(port)) != 0) {
            _play_size.at(port) = play_size - (play_size % get_play_item_size(port));
            throw uhd::value_error("Play buffer size must be a multiple of item size.");
        }
        _replay_reg_iface.poke64(REG_PLAY_BUFFER_SIZE_LO_ADDR, play_size, port);
    }

    void _set_packet_size(const uint32_t packet_size, const size_t port)
    {
        const size_t mtu               = get_mtu({res_source_info::OUTPUT_EDGE, port});
        uint32_t requested_packet_size = packet_size;
        if (requested_packet_size > mtu) {
            requested_packet_size = mtu;
            RFNOC_LOG_WARNING("Requested packet size exceeds MTU! Coercing to "
                              << requested_packet_size);
        }
        const size_t max_payload_bytes =
            get_max_payload_size({res_source_info::OUTPUT_EDGE, port});
        const uint32_t item_size = get_play_item_size(port);
        const uint32_t ipc       = _word_size / item_size; // items per cycle
        const uint32_t max_items = max_payload_bytes / item_size;
        const uint32_t max_ipp   = max_items - (max_items % ipc);
        const uint32_t requested_payload_size =
            requested_packet_size - (mtu - max_payload_bytes);
        uint32_t ipp = requested_payload_size / item_size;
        if (ipp > max_ipp) {
            RFNOC_LOG_DEBUG("ipp value " << ipp << " exceeds MTU of " << mtu
                                         << "! Coercing to " << max_ipp);
            ipp = max_ipp;
        }
        if ((ipp % ipc) != 0) {
            ipp = ipp - (ipp % ipc);
            RFNOC_LOG_WARNING(
                "ipp must be a multiple of the block bus width! Coercing to " << ipp);
        }
        if (ipp <= 0) {
            ipp = max_ipp;
            RFNOC_LOG_WARNING("ipp must be greater than zero! Coercing to " << ipp);
        }
        // Packet size must be a multiple of word size
        if ((packet_size % _word_size) != 0) {
            throw uhd::value_error("Packet size must be a multiple of word size.");
        }
        const uint16_t words_per_packet =
            uhd::narrow_cast<uint16_t>(ipp * item_size / _word_size);
        _replay_reg_iface.poke32(
            REG_PLAY_WORDS_PER_PKT_ADDR, uint32_t(words_per_packet), port);
    }

    void _validate_record_buffer(const size_t port)
    {
        // The entire record buffer must be within the bounds of memory
        if ((get_record_offset(port) + get_record_size(port)) > get_mem_size()) {
            throw uhd::value_error("Record buffer goes out of bounds.");
        }
    }

    void _validate_play_buffer(const size_t port)
    {
        // Streaming requires that the buffer size is a multiple of item size
        if ((get_play_size(port) % get_play_item_size(port)) != 0) {
            throw uhd::value_error("Play size must be must be a multiple of item size.");
        }
        // The entire play buffer must be within the bounds of memory
        if ((get_play_offset(port) + get_play_size(port)) > get_mem_size()) {
            throw uhd::value_error("Play buffer goes out of bounds.");
        }
    }

    void _handle_rx_event_action(
        const res_source_info& src, rx_event_action_info::sptr rx_event_action)
    {
        UHD_ASSERT_THROW(src.type == res_source_info::INPUT_EDGE);
        uhd::rx_metadata_t rx_md{};
        rx_md.error_code = rx_event_action->error_code;
        RFNOC_LOG_DEBUG("Received RX error code on channel "
                        << src.instance << ", error code " << rx_md.strerror());
        _record_msg_queue.push_with_pop_on_full(rx_md);
    }

    void _handle_tx_event_action(
        const res_source_info& src, tx_event_action_info::sptr tx_event_action)
    {
        UHD_ASSERT_THROW(src.type == res_source_info::OUTPUT_EDGE);

        uhd::async_metadata_t md;
        md.event_code    = tx_event_action->event_code;
        md.channel       = src.instance;
        md.has_time_spec = tx_event_action->has_tsf;

        if (md.has_time_spec) {
            md.time_spec =
                uhd::time_spec_t::from_ticks(tx_event_action->tsf, get_tick_rate());
        }
        RFNOC_LOG_DEBUG("Received TX error code on channel "
                        << src.instance << ", error code "
                        << static_cast<int>(md.event_code));
        _playback_msg_queue.push_with_pop_on_full(md);
    }

    /**************************************************************************
     * Attributes
     *************************************************************************/
    const size_t _num_input_ports;
    const size_t _num_output_ports;

    // Block compat number
    const uint32_t _fpga_compat;

    // These size params are configurable in the FPGA
    const uint16_t _word_size;
    const uint64_t _mem_size;

    std::vector<property_t<std::string>> _record_type;
    std::vector<property_t<uint64_t>> _record_offset;
    std::vector<property_t<uint64_t>> _record_size;
    std::vector<property_t<std::string>> _play_type;
    std::vector<property_t<uint64_t>> _play_offset;
    std::vector<property_t<uint64_t>> _play_size;
    std::vector<property_t<uint32_t>> _packet_size;
    std::vector<property_t<size_t>> _atomic_item_size_in;
    std::vector<property_t<size_t>> _atomic_item_size_out;

    std::vector<size_t> _cmd_fifo_spaces;

    // Message queues for async data
    uhd::transport::bounded_buffer<uhd::async_metadata_t> _playback_msg_queue{
        ASYNC_MSG_QUEUE_SIZE};
    uhd::transport::bounded_buffer<uhd::rx_metadata_t> _record_msg_queue{
        ASYNC_MSG_QUEUE_SIZE};
};

UHD_RFNOC_BLOCK_REGISTER_DIRECT(
    replay_block_control, REPLAY_BLOCK, "Replay", CLOCK_KEY_GRAPH, "bus_clk")
