//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/clock_iface.hpp>
#include <uhdlib/rfnoc/ctrlport_endpoint.hpp>
#include <condition_variable>
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::rfnoc::chdr;

namespace {

// Mock clock interface for testing
class mock_clock_iface : public uhd::rfnoc::clock_iface
{
public:
    mock_clock_iface(const std::string& name = "mock_clock", double freq = 100e6)
        : uhd::rfnoc::clock_iface(name, freq, true)
    {
        set_running(true); // Start the clock
    }
};

// Test fixture class
class ctrlport_endpoint_fixture
{
public:
    ctrlport_endpoint_fixture(size_t buff_capacity = 512,
        size_t max_outstanding_async_msgs          = 8,
        double bus_clk_freq                        = 100e6,
        double tb_clk_freq                         = 200e6)
        : client_clk("client_clk", bus_clk_freq)
        , timebase_clk("timebase_clk", tb_clk_freq)
    {
        // Create send function that captures packets
        send_fn = [this](const ctrl_payload& pkt, double timeout) {
            std::lock_guard<std::mutex> lock(sent_packets_mutex);
            sent_packets.push_back(pkt);
            last_timeout = timeout;

            // Simulate peek
            if (pkt.op_code == OP_READ) {
                schedule_async_response(create_read_response(pkt));
            }

            // Simulate ACK response for write operations if auto_ack is enabled
            if (auto_ack_enabled && pkt.op_code == OP_WRITE) {
                schedule_async_response(create_write_ack(pkt));
            }
        }; // end of send_fn

        // Create the endpoint
        endpoint = ctrlport_endpoint::make(send_fn,
            MY_EPID,
            MY_LOCAL_PORT,
            buff_capacity,
            max_outstanding_async_msgs,
            client_clk,
            timebase_clk);
        endpoint->set_block_id("TEST");
    }

    ~ctrlport_endpoint_fixture()
    {
        // Wait for all response threads to complete
        wait_for_pending_responses();
    }

    static constexpr sep_id_t MY_EPID       = 0x1234;
    static constexpr uint16_t MY_LOCAL_PORT = 8;

    std::mutex sent_packets_mutex{};
    std::vector<ctrl_payload> sent_packets{};
    double last_timeout   = 0.0;
    bool auto_ack_enabled = false;

    // Thread management for async responses
    std::mutex response_threads_mutex{};
    std::vector<std::thread> response_threads{};
    std::atomic<size_t> pending_responses{0};

    ctrlport_endpoint::send_fn_t send_fn;
    mock_clock_iface client_clk;
    mock_clock_iface timebase_clk;
    ctrlport_endpoint::sptr endpoint;

    //! Helper function to get the last sent packet
    ctrl_payload get_last_sent_packet()
    {
        std::lock_guard<std::mutex> lock(sent_packets_mutex);
        BOOST_REQUIRE(!sent_packets.empty());
        return sent_packets.back();
    }

    //! Helper function to clear sent packets
    void clear_sent_packets()
    {
        std::lock_guard<std::mutex> lock(sent_packets_mutex);
        sent_packets.clear();
    }

    //! Helper function to get the number of sent packets
    size_t get_sent_packet_count()
    {
        std::lock_guard<std::mutex> lock(sent_packets_mutex);
        return sent_packets.size();
    }

    //! Enable/disable automatic ACK responses for testing
    void set_auto_ack(bool enable)
    {
        auto_ack_enabled = enable;
    }

    //! Send an ACK for a specific control request (public interface for tests)
    void send_ack(const ctrl_payload& request, ctrl_status_t status = CMD_OKAY)
    {
        schedule_async_response(create_write_ack(request, status));
    }

    //! Public helper function to wait until all pending responses have been executed
    void wait_for_all_responses(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(1000))
    {
        wait_for_pending_responses(timeout);
    }

    //! Get the number of pending async responses
    size_t get_pending_response_count() const
    {
        return pending_responses.load();
    }

private:
    //! Wait for all pending async responses to complete
    void wait_for_pending_responses(
        std::chrono::milliseconds timeout = std::chrono::milliseconds(100))
    {
        auto start_time = std::chrono::steady_clock::now();
        while (pending_responses.load() > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if (std::chrono::steady_clock::now() - start_time > timeout) {
                break; // Don't wait forever in case of test failures
            }
        }

        // Join any completed threads
        std::lock_guard<std::mutex> lock(response_threads_mutex);
        response_threads.erase(std::remove_if(response_threads.begin(),
                                   response_threads.end(),
                                   [](std::thread& t) {
                                       if (t.joinable()) {
                                           t.join();
                                           return true;
                                       }
                                       return false;
                                   }),
            response_threads.end());
    }

    //! Create a read response payload
    ctrl_payload create_read_response(const ctrl_payload& request)
    {
        ctrl_payload response;
        response.dst_port  = request.src_port;
        response.src_port  = request.dst_port;
        response.seq_num   = request.seq_num;
        response.timestamp = request.timestamp;
        response.is_ack    = true;
        response.src_epid  = request.src_epid;
        response.op_code   = request.op_code;
        response.address   = request.address;
        response.status    = CMD_OKAY;
        // Echo back the address as the data for predictable testing
        response.data_vtr = {static_cast<uint32_t>(request.address + 0xDEADBEEF)};
        return response;
    }

    //! Create a write ACK response payload
    ctrl_payload create_write_ack(
        const ctrl_payload& request, ctrl_status_t status = CMD_OKAY)
    {
        ctrl_payload response;
        response.dst_port  = request.src_port;
        response.src_port  = request.dst_port;
        response.seq_num   = request.seq_num;
        response.timestamp = request.timestamp;
        response.is_ack    = true;
        response.src_epid  = request.src_epid;
        response.op_code   = request.op_code;
        response.address   = request.address;
        response.status    = status;
        response.data_vtr  = request.data_vtr;
        return response;
    }

    //! Schedule an async response in a separate thread
    void schedule_async_response(const ctrl_payload& response, int64_t delay_ms = 1)
    {
        pending_responses++;

        std::lock_guard<std::mutex> lock(response_threads_mutex);
        response_threads.emplace_back([this, response, delay_ms]() {
            // Small delay to simulate hardware response time
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
            endpoint->handle_recv(response);
            pending_responses--;
        });
    }
};

//! Specialized fixture for testing buffer overflow scenarios
class small_buffer_fixture : public ctrlport_endpoint_fixture
{
public:
    small_buffer_fixture() : ctrlport_endpoint_fixture(16, 0) {}
};

} // namespace

BOOST_AUTO_TEST_CASE(test_ctrlport_endpoint_creation)
{
    mock_clock_iface client_clk("client_clk", 100e6);
    mock_clock_iface timebase_clk("timebase_clk", 200e6);

    client_clk.set_running(true);
    timebase_clk.set_running(true);

    bool send_called = false;
    auto send_fn     = [&send_called](const ctrl_payload& /*pkt*/, double /*timeout*/) {
        send_called = true;
    };

    auto endpoint = ctrlport_endpoint::make(send_fn,
        0x5678, // my_epid
        16, // local_port
        2048, // buff_capacity
        32, // max_outstanding_async_msgs
        client_clk,
        timebase_clk);

    BOOST_CHECK(endpoint != nullptr);
}

BOOST_FIXTURE_TEST_CASE(test_poke32_basic, ctrlport_endpoint_fixture)
{
    const uint32_t test_addr = 0x1000;
    const uint32_t test_data = 0xDEADBEEF;

    // Test basic poke32 without ACK
    endpoint->poke32(test_addr, test_data);

    // Verify one packet was sent
    BOOST_CHECK_EQUAL(get_sent_packet_count(), 1);

    auto packet = get_last_sent_packet();
    BOOST_CHECK_EQUAL(packet.op_code, OP_WRITE);
    BOOST_CHECK_EQUAL(packet.address, test_addr);
    BOOST_CHECK_EQUAL(packet.data_vtr.size(), 1);
    BOOST_CHECK_EQUAL(packet.data_vtr[0], test_data);

    UHD_LOG_INFO("TEST", "Testing ctrl_payload serialization: " << packet.to_string());
}

BOOST_FIXTURE_TEST_CASE(test_poke32_with_ack, ctrlport_endpoint_fixture)
{
    const uint32_t test_addr = 0x2000;
    const uint32_t test_data = 0xCAFE1234;

    // Automatically provide ACK responses
    set_auto_ack(true);

    // Test poke32 with ACK - this should not timeout
    endpoint->poke32(test_addr, test_data, uhd::time_spec_t::ASAP, true);

    // Verify one packet was sent
    BOOST_CHECK_EQUAL(get_sent_packet_count(), 1);

    auto packet = get_last_sent_packet();
    BOOST_CHECK_EQUAL(packet.op_code, OP_WRITE);
    BOOST_CHECK_EQUAL(packet.address, test_addr);
    BOOST_CHECK_EQUAL(packet.data_vtr.size(), 1);
    BOOST_CHECK_EQUAL(packet.data_vtr[0], test_data);
}

BOOST_FIXTURE_TEST_CASE(test_poke32_with_timestamp, ctrlport_endpoint_fixture)
{
    const uint32_t test_addr = 0x3000;
    const uint32_t test_data = 0x12345678;
    const uhd::time_spec_t test_time(1.5); // 1.5 seconds

    // Test poke32 with timestamp
    endpoint->poke32(test_addr, test_data, test_time);

    // Verify one packet was sent
    BOOST_CHECK_EQUAL(get_sent_packet_count(), 1);

    auto packet = get_last_sent_packet();
    BOOST_CHECK_EQUAL(packet.op_code, OP_WRITE);
    BOOST_CHECK_EQUAL(packet.address, test_addr);
    BOOST_CHECK_EQUAL(packet.data_vtr.size(), 1);
    BOOST_CHECK_EQUAL(packet.data_vtr[0], test_data);
    BOOST_CHECK(packet.has_timestamp());
    BOOST_CHECK_EQUAL(
        packet.timestamp.value(), test_time.to_ticks(timebase_clk.get_freq()));
}

BOOST_FIXTURE_TEST_CASE(test_peek32_basic, ctrlport_endpoint_fixture)
{
    const uint32_t test_addr = 0x4000;

    // Test basic peek32
    uint32_t result = endpoint->peek32(test_addr);

    // Verify one packet was sent
    BOOST_CHECK_EQUAL(get_sent_packet_count(), 1);

    auto packet = get_last_sent_packet();
    BOOST_CHECK_EQUAL(packet.op_code, OP_READ);
    BOOST_CHECK_EQUAL(packet.address, test_addr);
    BOOST_CHECK_EQUAL(packet.data_vtr.size(), 1);
    BOOST_CHECK_EQUAL(packet.data_vtr[0], 0); // Read operations send dummy data

    // Verify the mock response value (address + 0xDEADBEEF)
    BOOST_CHECK_EQUAL(result, test_addr + 0xDEADBEEF);
}

BOOST_FIXTURE_TEST_CASE(test_peek32_with_timestamp, ctrlport_endpoint_fixture)
{
    const uint32_t test_addr = 0x5000;
    const uhd::time_spec_t test_time(2.0); // 2.0 seconds

    // Test peek32 with timestamp
    uint32_t result = endpoint->peek32(test_addr, test_time);

    // Verify one packet was sent
    BOOST_CHECK_EQUAL(get_sent_packet_count(), 1);

    auto packet = get_last_sent_packet();
    BOOST_CHECK_EQUAL(packet.op_code, OP_READ);
    BOOST_CHECK_EQUAL(packet.address, test_addr);
    BOOST_CHECK_EQUAL(packet.data_vtr.size(), 1);
    BOOST_CHECK_EQUAL(packet.data_vtr[0], 0); // Read operations send dummy data
    BOOST_CHECK(packet.has_timestamp());

    // Verify the mock response value
    BOOST_CHECK_EQUAL(result, test_addr + 0xDEADBEEF);
}

BOOST_FIXTURE_TEST_CASE(test_multi_poke32, ctrlport_endpoint_fixture)
{
    const std::vector<uint32_t> test_addrs = {0x1000, 0x2000, 0x3000};
    const std::vector<uint32_t> test_data  = {0xAABBCCDD, 0x11223344, 0xFFEEDDCC};

    // Test multi_poke32
    endpoint->multi_poke32(test_addrs, test_data);

    // Verify correct number of packets were sent
    BOOST_CHECK_EQUAL(get_sent_packet_count(), test_addrs.size());

    // Check each packet
    std::lock_guard<std::mutex> lock(sent_packets_mutex);
    for (size_t i = 0; i < test_addrs.size(); ++i) {
        auto& packet = sent_packets[i];
        BOOST_CHECK_EQUAL(packet.op_code, OP_WRITE);
        BOOST_CHECK_EQUAL(packet.address, test_addrs[i]);
        BOOST_CHECK_EQUAL(packet.data_vtr.size(), 1);
        BOOST_CHECK_EQUAL(packet.data_vtr[0], test_data[i]);
    }
}

BOOST_FIXTURE_TEST_CASE(test_multi_poke32_mismatched_sizes, ctrlport_endpoint_fixture)
{
    const std::vector<uint32_t> test_addrs = {0x1000, 0x2000};
    const std::vector<uint32_t> test_data  = {0xAABBCCDD}; // Different size

    // Test that mismatched sizes throw an exception
    UHD_LOG_INFO("TEST", "multi_poke32() error incoming:");
    BOOST_CHECK_THROW(endpoint->multi_poke32(test_addrs, test_data), uhd::value_error);
}

BOOST_FIXTURE_TEST_CASE(test_block_poke32, ctrlport_endpoint_fixture)
{
    const uint32_t base_addr              = 0x8000;
    const std::vector<uint32_t> test_data = {0x1111, 0x2222, 0x3333, 0x4444};

    // Test block_poke32
    endpoint->block_poke32(base_addr, test_data);

    // Verify correct number of packets were sent (currently implemented as individual
    // pokes)
    BOOST_CHECK_EQUAL(get_sent_packet_count(), test_data.size());

    // Check each packet has the correct address and data
    std::lock_guard<std::mutex> lock(sent_packets_mutex);
    for (size_t i = 0; i < test_data.size(); ++i) {
        auto& packet = sent_packets[i];
        BOOST_CHECK_EQUAL(packet.op_code, OP_WRITE);
        BOOST_CHECK_EQUAL(packet.address, base_addr + (i * sizeof(uint32_t)));
        BOOST_CHECK_EQUAL(packet.data_vtr.size(), 1);
        BOOST_CHECK_EQUAL(packet.data_vtr[0], test_data[i]);
    }
}

BOOST_FIXTURE_TEST_CASE(test_block_peek32, ctrlport_endpoint_fixture)
{
    const uint32_t base_addr = 0x9000;
    const size_t length      = 3;

    // Test block_peek32
    auto result = endpoint->block_peek32(base_addr, length);

    // Verify correct number of packets were sent
    BOOST_CHECK_EQUAL(get_sent_packet_count(), length);

    // Verify result vector has correct size
    BOOST_CHECK_EQUAL(result.size(), length);

    // Check each result value (should be address + 0xDEADBEEF from our mock)
    for (size_t i = 0; i < length; ++i) {
        uint32_t expected_addr = base_addr + (i * sizeof(uint32_t));
        BOOST_CHECK_EQUAL(result[i], expected_addr + 0xDEADBEEF);
    }
}

BOOST_FIXTURE_TEST_CASE(test_sleep, ctrlport_endpoint_fixture)
{
    const uhd::time_spec_t sleep_duration(0.001); // 1ms

    // Test sleep command
    endpoint->sleep(sleep_duration);

    // Verify one packet was sent
    BOOST_CHECK_EQUAL(get_sent_packet_count(), 1);

    auto packet = get_last_sent_packet();
    BOOST_CHECK_EQUAL(packet.op_code, OP_SLEEP);
    BOOST_CHECK_EQUAL(packet.address, 0);
    BOOST_CHECK_EQUAL(packet.data_vtr.size(), 1);
    // The sleep duration should be converted to ticks using timebase frequency
    uint32_t expected_ticks =
        static_cast<uint32_t>(sleep_duration.to_ticks(timebase_clk.get_freq()));
    BOOST_CHECK_EQUAL(packet.data_vtr[0], expected_ticks);
}

BOOST_FIXTURE_TEST_CASE(test_async_message_handlers, ctrlport_endpoint_fixture)
{
    bool validator_called  = false;
    bool handler_called    = false;
    uint32_t received_addr = 0;
    std::vector<uint32_t> received_data;

    // Register async message validator
    endpoint->register_async_msg_validator(
        [&validator_called, &received_addr, &received_data](
            uint32_t addr, const std::vector<uint32_t>& data) {
            validator_called = true;
            received_addr    = addr;
            received_data    = data;
            return true; // Accept all messages
        });

    // Register async message handler
    endpoint->register_async_msg_handler(
        [&handler_called](uint32_t /*addr*/,
            const std::vector<uint32_t>& /*data*/,
            boost::optional<uint64_t> /*timestamp*/) { handler_called = true; });

    // Simulate receiving an async message with correct port
    ctrl_payload async_msg;
    async_msg.op_code  = OP_WRITE;
    async_msg.address  = 0x7000;
    async_msg.data_vtr = {0xABCD1234};
    async_msg.is_ack   = false; // Async messages are not ACKs
    async_msg.dst_port = MY_LOCAL_PORT;
    async_msg.src_port = 0; // From some other port
    async_msg.src_epid = 0x5678; // From some other endpoint
    async_msg.seq_num  = 123;

    endpoint->handle_recv(async_msg);

    // Verify the handlers were called
    BOOST_CHECK(validator_called);
    BOOST_CHECK(handler_called);
    BOOST_CHECK_EQUAL(received_addr, 0x7000);
    BOOST_CHECK_EQUAL(received_data.size(), 1);
    if (!received_data.empty()) {
        BOOST_CHECK_EQUAL(received_data[0], 0xABCD1234);
    }
}

BOOST_FIXTURE_TEST_CASE(test_command_fifo_overflow, small_buffer_fixture)
{
    // Set a small timeout or this test will take forever
    endpoint->set_policy("default", "timeout=0.01");

    // Fill up the buffer
    const size_t num_commands = 4; // These will fit within the buffer capacity of 16
                                   // words (4 commands * 4 words each)
    for (size_t i = 0; i < num_commands; ++i) {
        uint32_t addr = 0x1000 + (i * 4);
        uint32_t data = 0xDEADBEEF + i;
        endpoint->poke32(addr, data);
    }

    // This should time out due to overflow
    UHD_LOG_INFO("TEST", "Timeout error incoming:");
    BOOST_REQUIRE_THROW(endpoint->poke32(0x2000, 0xFA11), uhd::op_timeout);

    // Now ACK the first packet we sent out
    send_ack(sent_packets.front());

    // This should fit now
    endpoint->poke32(0x2000, 0x600D);

    // But this should fail again
    UHD_LOG_INFO("TEST", "Timeout error incoming:");
    BOOST_REQUIRE_THROW(endpoint->poke32(0x2004, 0xFA11), uhd::op_timeout);
}

BOOST_FIXTURE_TEST_CASE(test_dropped_ack_handling, small_buffer_fixture)
{
    set_auto_ack(false);

    const uint32_t test_addr = 0x3000;
    const uint32_t test_data = 0xF00DF00D;

    // Send three pokes without acks
    for (int i = 0; i < 3; ++i) {
        endpoint->poke32(test_addr + (i * 4), test_data + i);
    }

    auto stats = endpoint->get_stats();
    BOOST_CHECK_EQUAL(stats.ctrl_packets_sent, 3);
    BOOST_CHECK_EQUAL(stats.ack_packets_received, 0);

    // Now send ACKs for two of them, but drop the second one
    send_ack(sent_packets[0]);
    wait_for_all_responses();
    send_ack(sent_packets[2]);
    wait_for_all_responses();

    stats = endpoint->get_stats();
    BOOST_CHECK_EQUAL(stats.ctrl_packets_sent, 3);
    BOOST_CHECK_EQUAL(stats.ack_packets_received, 2);
    BOOST_CHECK_EQUAL(stats.buffer_fullness, 0);
}
