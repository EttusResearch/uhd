//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/chdr_packet_writer.hpp>
#include <uhdlib/rfnoc/ctrlport_endpoint.hpp>
#include <condition_variable>
#include <boost/format.hpp>
#include <deque>
#include <mutex>
#include <numeric>
#include <queue>


using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::rfnoc::chdr;

using namespace std::chrono;
using namespace std::chrono_literals;

namespace {
//! Max async msg (CTRL_WRITE) size in 32-bit words (2 hdr, 2 TS, 1 op-word, 1 data)
constexpr size_t ASYNC_MESSAGE_SIZE = 6;
//! Default completion timeout for transactions
constexpr double DEFAULT_TIMEOUT = 1.0;
//! Long timeout for when we wait on a timed command
constexpr double MASSIVE_TIMEOUT = 10.0;
//! Default value for whether ACKs are always required
constexpr bool DEFAULT_FORCE_ACKS = false;
} // namespace

ctrlport_endpoint::~ctrlport_endpoint() = default;

class ctrlport_endpoint_impl : public ctrlport_endpoint
{
public:
    ctrlport_endpoint_impl(const send_fn_t& send_fcn,
        sep_id_t my_epid,
        uint16_t local_port,
        size_t buff_capacity,
        size_t max_outstanding_async_msgs,
        const clock_iface& client_clk,
        const clock_iface& timebase_clk)
        : _handle_send(send_fcn)
        , _my_epid(my_epid)
        , _local_port(local_port)
        , _buff_capacity(buff_capacity)
        , _max_outstanding_async_msgs(max_outstanding_async_msgs)
        , _client_clk(client_clk)
        , _timebase_clk(timebase_clk)
    {
    }

    ~ctrlport_endpoint_impl() override = default;

    void poke32(uint32_t addr,
        uint32_t data,
        uhd::time_spec_t timestamp = uhd::time_spec_t::ASAP,
        bool ack                   = false) override
    {
        // Send request
        auto request = send_request_packet(OP_WRITE, addr, {data}, timestamp);
        // Optionally wait for an ACK
        if (ack || _policy.force_acks) {
            wait_for_ack(request);
        }
    }

    void multi_poke32(const std::vector<uint32_t> addrs,
        const std::vector<uint32_t> data,
        uhd::time_spec_t timestamp = uhd::time_spec_t::ASAP,
        bool ack                   = false) override
    {
        if (addrs.size() != data.size()) {
            throw uhd::value_error("addrs and data vectors must be of the same length");
        }
        for (size_t i = 0; i < data.size(); i++) {
            poke32(addrs[i],
                data[i],
                (i == 0) ? timestamp : uhd::time_spec_t::ASAP,
                (i == data.size() - 1) ? ack : false);
        }
    }

    void block_poke32(uint32_t first_addr,
        const std::vector<uint32_t> data,
        uhd::time_spec_t timestamp = uhd::time_spec_t::ASAP,
        bool ack                   = false) override
    {
        for (size_t i = 0; i < data.size(); i++) {
            poke32(first_addr + (i * sizeof(uint32_t)),
                data[i],
                (i == 0) ? timestamp : uhd::time_spec_t::ASAP,
                (i == data.size() - 1) ? ack : false);
        }

        /* TODO: Uncomment when the atomic block poke is implemented in the FPGA
        // Send request
        auto request = send_request_packet(OP_BLOCK_WRITE, first_addr, data, timestamp);
        // Optionally wait for an ACK
        if (ack || _policy.force_acks) {
            wait_for_ack(request);
        }
        */
    }

    uint32_t peek32(
        uint32_t addr, uhd::time_spec_t timestamp = uhd::time_spec_t::ASAP) override
    {
        // Send request
        auto request = send_request_packet(OP_READ, addr, {uint32_t(0)}, timestamp);

        // Wait for an ACK
        auto response = wait_for_ack(request);
        return response.data_vtr[0];
    }

    std::vector<uint32_t> block_peek32(uint32_t first_addr,
        size_t length,
        uhd::time_spec_t timestamp = uhd::time_spec_t::ASAP) override
    {
        std::vector<uint32_t> values;
        for (size_t i = 0; i < length; i++) {
            values.push_back(peek32(first_addr + (i * sizeof(uint32_t)),
                (i == 0) ? timestamp : uhd::time_spec_t::ASAP));
        }
        return values;

        /* TODO: Uncomment when the atomic block peek is implemented in the FPGA
        // Send request
        auto request = send_request_packet(OP_READ,
            first_addr,
            std::vector<uint32_t>(length, 0),
            timestamp);

        // Wait for an ACK
        auto response = wait_for_ack(request);
        return response.data_vtr;
        */
    }

    void poll32(uint32_t addr,
        uint32_t data,
        uint32_t mask,
        uhd::time_spec_t timeout,
        uhd::time_spec_t timestamp = uhd::time_spec_t::ASAP,
        bool ack                   = false) override
    {
        // TODO: Uncomment when this is implemented in the FPGA
        throw uhd::not_implemented_error("Control poll not implemented in the FPGA");

        // Send request
        auto request = send_request_packet(OP_POLL,
            addr,
            {data,
                mask,
                static_cast<uint32_t>(timeout.to_ticks(_timebase_clk.get_freq()))},
            timestamp);

        // Optionally wait for an ACK
        if (ack || _policy.force_acks) {
            wait_for_ack(request);
        }
    }

    void sleep(uhd::time_spec_t duration, bool ack = false) override
    {
        // Send request
        auto request = send_request_packet(OP_SLEEP,
            0,
            {static_cast<uint32_t>(duration.to_ticks(_timebase_clk.get_freq()))},
            uhd::time_spec_t::ASAP);

        // Optionally wait for an ACK
        if (ack || _policy.force_acks) {
            wait_for_ack(request);
        }
    }

    void register_async_msg_validator(async_msg_validator_t callback_f) override
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _validate_async_msg = callback_f;
    }

    void register_async_msg_handler(async_msg_callback_t callback_f) override
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _handle_async_msg = callback_f;
    }

    void set_policy(const std::string& name, const uhd::device_addr_t& args) override
    {
        std::unique_lock<std::mutex> lock(_mutex);
        if (name == "default") {
            _policy.timeout    = args.cast<double>("timeout", DEFAULT_TIMEOUT);
            _policy.force_acks = DEFAULT_FORCE_ACKS;
        } else {
            // TODO: Uncomment when custom policies are implemented
            throw uhd::not_implemented_error("Policy implemented in the FPGA");
        }
    }

    void handle_recv(const ctrl_payload& rx_ctrl) override
    {
        if (rx_ctrl.is_ack) {
            // Function to process a response with no sequence errors
            auto process_correct_response = [this, rx_ctrl]() {
                response_status_t resp_status = RESP_VALID;
                // Grant flow control credits
                _buff_occupied -= get_payload_size(_req_queue.front());
                _buff_free_cond.notify_one();
                if (get_payload_size(_req_queue.front()) != get_payload_size(rx_ctrl)) {
                    resp_status = RESP_SIZEERR;
                }
                // Pop the request from the queue
                _req_queue.pop_front();
                // Push the response into the response queue
                _resp_queue.push(std::make_tuple(rx_ctrl, resp_status));
                _resp_ready_cond.notify_one();
            };
            // Function to process a response with sequence errors
            auto process_incorrect_response = [this]() {
                // Grant flow control credits
                _buff_occupied -= get_payload_size(_req_queue.front());
                _buff_free_cond.notify_one();
                // Push a fabricated response into the response queue
                ctrl_payload resp(_req_queue.front());
                resp.is_ack = true;
                _resp_queue.push(std::make_tuple(resp, RESP_DROPPED));
                _resp_ready_cond.notify_one();
                // Pop the request from the queue
                _req_queue.pop_front();
            };

            // Peek at the request queue to check the expected sequence number
            std::unique_lock<std::mutex> lock(_mutex);
            int8_t seq_num_diff = int8_t(rx_ctrl.seq_num - _req_queue.front().seq_num);
            if (seq_num_diff == 0) { // No sequence error
                process_correct_response();
            } else if (seq_num_diff > 0) { // Packet(s) dropped
                // Tag all dropped packets
                for (int8_t i = 0; i < seq_num_diff; i++) {
                    process_incorrect_response();
                }
                // Process correct response
                process_correct_response();
            } else { // Reordered packet(s)
                // Requests are processed in order. If seq_num_diff is negative then we
                // have either already seen this response or we have dropped >128
                // responses. Either way ignore this packet.
            }
        } else {
            // Handle asynchronous message callback
            ctrl_status_t status = CMD_CMDERR;
            if (rx_ctrl.op_code != OP_WRITE && rx_ctrl.op_code != OP_BLOCK_WRITE) {
                UHD_LOG_ERROR(
                    "CTRLEP", "Malformed async message request: Invalid opcode");
            } else if (rx_ctrl.dst_port != _local_port) {
                UHD_LOG_ERROR("CTRLEP",
                    "Malformed async message request: Invalid port "
                        << rx_ctrl.dst_port << ", expected my local port "
                        << _local_port);
            } else if (rx_ctrl.data_vtr.empty()) {
                UHD_LOG_ERROR(
                    "CTRLEP", "Malformed async message request: Invalid num_data");
            } else {
                if (!_validate_async_msg(rx_ctrl.address, rx_ctrl.data_vtr)) {
                    UHD_LOG_ERROR("CTRLEP",
                        "Malformed async message request: Async message was not "
                        "validated by block controller!");
                } else {
                    status = CMD_OKAY;
                }
            }
            try {
                // Respond with an ACK packet
                // Flow control not needed because we have allocated special room in the
                // buffer for async message responses
                ctrl_payload tx_ctrl(rx_ctrl);
                tx_ctrl.is_ack     = true;
                tx_ctrl.src_epid   = _my_epid;
                tx_ctrl.status     = status;
                const auto timeout = [&]() {
                    std::unique_lock<std::mutex> lock(_mutex);
                    return _policy.timeout;
                }();
                _handle_send(tx_ctrl, timeout);
            } catch (...) {
                UHD_LOG_ERROR("CTRLEP",
                    "Encountered an error sending a response for an async message");
                return;
            }
            if (status == CMD_OKAY) {
                try {
                    _handle_async_msg(
                        rx_ctrl.address, rx_ctrl.data_vtr, rx_ctrl.timestamp);
                } catch (const std::exception& ex) {
                    UHD_LOG_ERROR("CTRLEP",
                        "Caught exception during async message handling: " << ex.what());
                } catch (...) {
                    UHD_LOG_ERROR("CTRLEP",
                        "Caught unknown exception during async message handling!");
                }
            }
        }
    }

    uint16_t get_src_epid() const override
    {
        // Is const, does not require a mutex
        return _my_epid;
    }

    uint16_t get_port_num() const override
    {
        // Is const, does not require a mutex
        return _local_port;
    }

private:
    //! Returns the length of the control payload in 32-bit words
    inline static size_t get_payload_size(const ctrl_payload& payload)
    {
        return 2 + (payload.timestamp.is_initialized() ? 2 : 0) + payload.data_vtr.size();
    }

    //! Marks the start of a timeout for an operation and returns the expiration time
    inline const steady_clock::time_point start_timeout(double duration)
    {
        return steady_clock::now() + (static_cast<int>(std::ceil(duration / 1e-6)) * 1us);
    }

    //! Returns whether or not we have a timed command queued
    bool check_timed_in_queue() const
    {
        for (const auto& pyld : _req_queue) {
            if (pyld.has_timestamp()) {
                return true;
            }
        }
        return false;
    }

    //! Sends a request control packet to a remote device
    const ctrl_payload send_request_packet(ctrl_opcode_t op_code,
        uint32_t address,
        const std::vector<uint32_t>& data_vtr,
        const uhd::time_spec_t& time_spec)
    {
        if (!_client_clk.is_running()) {
            throw uhd::system_error("Ctrlport client clock is not running");
        }

        // Convert from uhd::time_spec to timestamp
        boost::optional<uint64_t> timestamp;
        if (time_spec != time_spec_t::ASAP) {
            if (!_timebase_clk.is_running()) {
                throw uhd::system_error("Timebase clock is not running");
            }
            timestamp = time_spec.to_ticks(_timebase_clk.get_freq());
        }

        std::unique_lock<std::mutex> lock(_mutex);

        // Assemble the control payload
        ctrl_payload tx_ctrl;
        tx_ctrl.dst_port    = _local_port;
        tx_ctrl.src_port    = _local_port;
        tx_ctrl.seq_num     = _tx_seq_num;
        tx_ctrl.timestamp   = timestamp;
        tx_ctrl.is_ack      = false;
        tx_ctrl.src_epid    = _my_epid;
        tx_ctrl.address     = address;
        tx_ctrl.data_vtr    = data_vtr;
        tx_ctrl.byte_enable = 0xF;
        tx_ctrl.op_code     = op_code;
        tx_ctrl.status      = CMD_OKAY;

        // Perform flow control
        // If there is no room in the downstream buffer, then wait until the timeout
        size_t pyld_size   = get_payload_size(tx_ctrl);
        auto buff_not_full = [this, pyld_size]() -> bool {
            // Allocate room in the queue for one async response packet
            // If we can fit the current request in the queue then we can proceed
            return (_buff_occupied + pyld_size)
                   <= (_buff_capacity
                          - (ASYNC_MESSAGE_SIZE * _max_outstanding_async_msgs));
        };
        if (!buff_not_full()) {
            // If there is a timed command in the queue, use the
            // MASSIVE_TIMEOUT instead
            auto timeout_time =
                start_timeout(check_timed_in_queue() ? MASSIVE_TIMEOUT : _policy.timeout);

            if (not _buff_free_cond.wait_until(lock, timeout_time, buff_not_full)) {
                throw uhd::op_timeout(
                    "Control operation timed out waiting for space in command buffer");
            }
        }
        _buff_occupied += pyld_size;
        _req_queue.push_back(tx_ctrl);

        // Send the payload as soon as there is room in the buffer
        _handle_send(tx_ctrl, _policy.timeout);
        _tx_seq_num = (_tx_seq_num + 1) % 64;

        return tx_ctrl;
    }

    //! Waits for and returns the ACK for the specified request
    const ctrl_payload wait_for_ack(const ctrl_payload& request)
    {
        auto resp_ready = [this]() -> bool { return !_resp_queue.empty(); };
        while (true) {
            std::unique_lock<std::mutex> lock(_mutex);
            // Wait until there is a response in the response queue
            if (!resp_ready()) {
                // If we're waiting for a timed command or if we have a
                // command in the queue, use the MASSIVE_TIMEOUT instead
                auto timeout_time = start_timeout(
                    check_timed_in_queue() ? MASSIVE_TIMEOUT : _policy.timeout);

                if (not _resp_ready_cond.wait_until(lock, timeout_time, resp_ready)) {
                    throw uhd::op_timeout("Control operation timed out waiting for ACK");
                }
            }
            // Extract the response packet
            ctrl_payload rx_ctrl;
            response_status_t resp_status;
            std::tie(rx_ctrl, resp_status) = _resp_queue.front();
            _resp_queue.pop();
            // Check if this is the response meant for the request
            // Filter by op_code, address and seq_num
            if (rx_ctrl.seq_num == request.seq_num && rx_ctrl.op_code == request.op_code
                && rx_ctrl.address == request.address) {
                // Validate transaction status
                if (rx_ctrl.status == CMD_CMDERR) {
                    throw uhd::op_failed("Control operation returned a failing status");
                } else if (rx_ctrl.status == CMD_TSERR) {
                    throw uhd::op_timerr("Control operation returned a timestamp error");
                }
                // Check data vector size
                if (rx_ctrl.data_vtr.empty()) {
                    throw uhd::op_failed(
                        "Control operation returned a malformed response");
                }
                // Validate response status
                if (resp_status == RESP_DROPPED) {
                    throw uhd::op_seqerr(
                        "Response for a control transaction was dropped");
                } else if (resp_status == RESP_RTERR) {
                    throw uhd::op_timerr("Control operation encountered a routing error");
                }
                return rx_ctrl;
            } else {
                // This response does not belong to the request we passed in. Move on.
                continue;
            }
        }
    }


    //! The parameters associated with the policy that governs this object
    struct policy_args
    {
        double timeout  = DEFAULT_TIMEOUT;
        bool force_acks = DEFAULT_FORCE_ACKS;
    };
    //! The software status (different from the transaction status) of the response
    enum response_status_t { RESP_VALID, RESP_DROPPED, RESP_RTERR, RESP_SIZEERR };

    //! Function to call to send a control packet
    const send_fn_t _handle_send;
    //! The endpoint ID of this software endpoint
    const sep_id_t _my_epid;
    //! The local port number on the control crossbar for this ctrlport endpoint
    const uint16_t _local_port;
    //! The downstream buffer capacity in 32-bit words (used for flow control)
    const size_t _buff_capacity;
    //! The max number of outstanding async messages that a block can have at any time
    const size_t _max_outstanding_async_msgs;
    //! The clock that drives the ctrlport endpoint
    const clock_iface& _client_clk;
    //! The clock that drives the timing logic for the ctrlport endpoint
    const clock_iface& _timebase_clk;

    //! The function to call to validate an async message (by default, all async
    // messages are considered valid)
    async_msg_validator_t _validate_async_msg =
        [](uint32_t, const std::vector<uint32_t>&) { return true; };
    //! The function to call to handle an async message
    async_msg_callback_t _handle_async_msg = async_msg_callback_t();
    //! The current control sequence number of outgoing packets
    uint8_t _tx_seq_num = 0;
    //! The number of occupied words in the downstream buffer
    ssize_t _buff_occupied = 0;
    //! The arguments for the policy that governs this register interface
    policy_args _policy;
    //! A condition variable that hold the "downstream buffer is free" condition
    std::condition_variable _buff_free_cond;
    //! A queue that holds all outstanding requests
    std::deque<ctrl_payload> _req_queue;
    //! A queue that holds all outstanding responses and their status
    std::queue<std::tuple<ctrl_payload, response_status_t>> _resp_queue;
    //! A condition variable that hold the "response is available" condition
    std::condition_variable _resp_ready_cond;
    //! A mutex to protect all state in this class
    std::mutex _mutex;
};

ctrlport_endpoint::sptr ctrlport_endpoint::make(const send_fn_t& handle_send,
    sep_id_t this_epid,
    uint16_t local_port,
    size_t buff_capacity,
    size_t max_outstanding_async_msgs,
    const clock_iface& client_clk,
    const clock_iface& timebase_clk)
{
    return std::make_shared<ctrlport_endpoint_impl>(handle_send,
        this_epid,
        local_port,
        buff_capacity,
        max_outstanding_async_msgs,
        client_clk,
        timebase_clk);
}
