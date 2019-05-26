//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/thread.hpp>
#include <uhdlib/rfnoc/chdr/chdr_packet.hpp>
#include <uhdlib/rfnoc/chdr/chdr_types.hpp>
#include <uhdlib/rfnoc/chdr_ctrl_endpoint.hpp>
#include <boost/format.hpp>
#include <mutex>
#include <thread>
#include <atomic>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::rfnoc::chdr;

using namespace std::placeholders;

chdr_ctrl_endpoint::~chdr_ctrl_endpoint() = default;

class chdr_ctrl_endpoint_impl : public chdr_ctrl_endpoint
{
public:
    chdr_ctrl_endpoint_impl(const both_xports_t& xports,
        const chdr::chdr_packet_factory& pkt_factory,
        sep_id_t dst_epid,
        sep_id_t my_epid)
        : _dst_epid(dst_epid)
        , _my_epid(my_epid)
        , _xports(xports)
        , _send_seqnum(0)
        , _send_pkt(pkt_factory.make_ctrl())
        , _recv_pkt(pkt_factory.make_ctrl())
        , _stop_recv_thread(false)
        , _recv_thread([this]() { recv_worker(); })
    {
        const std::string thread_name(str(boost::format("uhd_ctrl_ep%04x") % _my_epid));
        uhd::set_thread_name(&_recv_thread, thread_name);
        UHD_LOG_DEBUG("RFNOC",
            boost::format("Started thread %s to process messages for CHDR Ctrl EP for "
                          "EPID %d -> EPID %d")
                % thread_name % _my_epid % _dst_epid);
    }

    virtual ~chdr_ctrl_endpoint_impl()
    {
        UHD_SAFE_CALL(
            // Interrupt buffer updater loop
            _stop_recv_thread = true;
            // Wait for loop to finish
            // No timeout on join. The recv loop is guaranteed
            // to terminate in a reasonable amount of time because
            // there are no timed blocks on the underlying.
            _recv_thread.join();
            // Flush base transport
            while (_xports.recv->get_recv_buff(0.0001)) /*NOP*/;
            // Release child endpoints
            _endpoint_map.clear(););
    }

    virtual ctrlport_endpoint::sptr get_ctrlport_ep(uint16_t port,
        size_t buff_capacity,
        size_t max_outstanding_async_msgs,
        const clock_iface& client_clk,
        const clock_iface& timebase_clk)
    {
        std::lock_guard<std::mutex> lock(_mutex);

        // Function to send a control payload
        auto send_fn = [this](const ctrl_payload& payload, double timeout) {
            std::lock_guard<std::mutex> lock(_mutex);
            // Build header
            chdr_header header;
            header.set_pkt_type(PKT_TYPE_CTRL);
            header.set_num_mdata(0);
            header.set_seq_num(_send_seqnum++);
            header.set_dst_epid(_dst_epid);
            // Acquire send buffer and send the packet
            auto send_buff = _xports.send->get_send_buff(timeout);
            _send_pkt->refresh(send_buff->cast<void*>(), header, payload);
            send_buff->commit(header.get_length());
        };

        if (_endpoint_map.find(port) == _endpoint_map.end()) {
            ctrlport_endpoint::sptr ctrlport_ep = ctrlport_endpoint::make(send_fn,
                _my_epid,
                port,
                buff_capacity,
                max_outstanding_async_msgs,
                client_clk,
                timebase_clk);
            _endpoint_map.insert(std::make_pair(port, ctrlport_ep));
            UHD_LOG_DEBUG("RFNOC",
                boost::format("Created ctrlport endpoint for port %d on EPID %d") % port
                    % _my_epid);
            return ctrlport_ep;
        } else {
            return _endpoint_map.at(port);
        }
    }

    virtual size_t get_num_drops() const
    {
        return _num_drops;
    }

private:
    void recv_worker()
    {
        // Run forever:
        // - Pull packets from the base transport
        // - Route them based on the dst_port
        // - Pass them to the ctrlport_endpoint for additional processing
        while (not _stop_recv_thread) {
            auto buff = _xports.recv->get_recv_buff(0.0);
            if (buff) {
                std::lock_guard<std::mutex> lock(_mutex);
                try {
                    _recv_pkt->refresh(buff->cast<void*>());
                    const ctrl_payload payload = _recv_pkt->get_payload();
                    if (_endpoint_map.find(payload.dst_port) != _endpoint_map.end()) {
                        _endpoint_map.at(payload.dst_port)->handle_recv(payload);
                    }
                } catch (...) {
                    // Ignore all errors
                }
            } else {
                // Be a good citizen and yield if no packet is processed
                static const size_t MIN_DUR = 1;
                boost::this_thread::sleep_for(boost::chrono::nanoseconds(MIN_DUR));
                // We call sleep(MIN_DUR) above instead of yield() to ensure that we
                // relinquish the current scheduler time slot.
                // yield() is a hint to the scheduler to end the time
                // slice early and schedule in another thread that is ready to run.
                // However in most situations, there will be no other thread and
                // this thread will continue to run which will rail a CPU core.
                // We call sleep(MIN_DUR=1) instead which will sleep for a minimum
                // time. Ideally we would like to use boost::chrono::.*seconds::min()
                // but that is bound to 0, which causes the sleep_for call to be a
                // no-op and thus useless to actually force a sleep.
            }
        }
    }

    // The endpoint ID of the destination
    const sep_id_t _dst_epid;
    // The endpoint ID of this software endpoint
    const sep_id_t _my_epid;
    // Send/recv transports
    const uhd::both_xports_t _xports;
    // The curent sequence number for a send packet
    size_t _send_seqnum = 0;
    // The number of packets dropped
    size_t _num_drops = 0;
    // Packet containers
    chdr_ctrl_packet::uptr _send_pkt;
    chdr_ctrl_packet::cuptr _recv_pkt;
    // A collection of ctrlport endpoints (keyed by the port number)
    std::map<uint16_t, ctrlport_endpoint::sptr> _endpoint_map;
    // A thread that will handle all responses and async message requests
    std::atomic_bool _stop_recv_thread;
    std::thread _recv_thread;
    // Mutex that protects all state in this class
    std::mutex _mutex;
};

chdr_ctrl_endpoint::uptr chdr_ctrl_endpoint::make(const both_xports_t& xports,
    const chdr::chdr_packet_factory& pkt_factory,
    sep_id_t dst_epid,
    sep_id_t my_epid)
{
    return std::make_unique<chdr_ctrl_endpoint_impl>(
        xports, pkt_factory, dst_epid, my_epid);
}
