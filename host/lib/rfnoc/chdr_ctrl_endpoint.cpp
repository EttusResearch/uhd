//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/rfnoc/chdr_types.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/thread.hpp>
#include <uhdlib/rfnoc/chdr_ctrl_endpoint.hpp>
#include <uhdlib/rfnoc/chdr_packet_writer.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <atomic>
#include <mutex>
#include <thread>

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::rfnoc::chdr;


chdr_ctrl_endpoint::~chdr_ctrl_endpoint() = default;

class chdr_ctrl_endpoint_impl : public chdr_ctrl_endpoint
{
public:
    chdr_ctrl_endpoint_impl(chdr_ctrl_xport::sptr xport,
        const chdr::chdr_packet_factory& pkt_factory,
        sep_id_t my_epid)
        : _my_epid(my_epid)
        , _xport(xport)
        , _num_drops(0)
        , _send_pkt(pkt_factory.make_ctrl())
        , _recv_pkt(pkt_factory.make_ctrl())
        , _stop_recv_thread(false)
        , _recv_thread([this]() { recv_worker(); })
    {
        const std::string thread_name(str(boost::format("uhd_ctrl_ep%04x") % _my_epid));
        uhd::set_thread_name(&_recv_thread, thread_name);
        UHD_LOG_DEBUG("RFNOC",
            boost::format(
                "Started thread %s to process messages control messages on EPID %d")
                % thread_name % _my_epid);
    }

    ~chdr_ctrl_endpoint_impl() override
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
            while (true) {
                auto buff = _xport->get_recv_buff(100);
                if (buff) {
                    _xport->release_recv_buff(std::move(buff));
                } else {
                    break;
                }
            }
            // Release child endpoints
            _endpoint_map.clear(););
    }

    ctrlport_endpoint::sptr get_ctrlport_ep(sep_id_t dst_epid,
        uint16_t dst_port,
        size_t buff_capacity,
        size_t max_outstanding_async_msgs,
        const clock_iface& client_clk,
        const clock_iface& timebase_clk) override
    {
        std::lock_guard<std::mutex> lock(_mutex);

        ep_map_key_t key{dst_epid, dst_port};
        // Function to send a control payload
        auto send_fn = [this, dst_epid](const ctrl_payload& payload, double timeout) {
            // Build header
            chdr_header header;
            header.set_pkt_type(PKT_TYPE_CTRL);
            header.set_num_mdata(0);
            header.set_seq_num(_send_seqnum++);
            header.set_dst_epid(dst_epid);
            // Acquire send buffer and send the packet
            std::lock_guard<std::mutex> lock(_send_mutex);
            auto send_buff = _xport->get_send_buff(timeout * 1000);
            _send_pkt->refresh(send_buff->data(), header, payload);
            send_buff->set_packet_size(header.get_length());
            _xport->release_send_buff(std::move(send_buff));
        };

        if (_endpoint_map.find(key) == _endpoint_map.end()) {
            ctrlport_endpoint::sptr ctrlport_ep = ctrlport_endpoint::make(send_fn,
                _my_epid,
                dst_port,
                buff_capacity,
                max_outstanding_async_msgs,
                client_clk,
                timebase_clk);
            _endpoint_map.insert(std::make_pair(key, ctrlport_ep));
            UHD_LOG_DEBUG("RFNOC",
                boost::format("Created ctrlport endpoint for port %d on EPID %d")
                    % dst_port % _my_epid);
            return ctrlport_ep;
        } else {
            return _endpoint_map.at(key);
        }
    }

    size_t get_num_drops() const override
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
            // FIXME Move lock back once have threaded_io_service
            std::unique_lock<std::mutex> lock(_mutex);
            auto buff = _xport->get_recv_buff(0);
            if (buff) {
                // FIXME Move lock back to here once have threaded_io_service
                // std::lock_guard<std::mutex> lock(_mutex);
                try {
                    _recv_pkt->refresh(buff->data());
                    const ctrl_payload payload = _recv_pkt->get_payload();
                    ep_map_key_t key{payload.src_epid, payload.dst_port};
                    auto ep_iter = _endpoint_map.find(key);
                    if (ep_iter != _endpoint_map.end()) {
                        ep_iter->second->handle_recv(payload);
                    } else {
                        UHD_LOG_WARNING("RFNOC",
                            "chdr_ctrl_endpoint: Received async message for unknown "
                            "destination. Source EPID: "
                                << payload.src_epid
                                << " Destination Port: " << payload.dst_port);
                        _num_drops++;
                    }
                } catch (...) {
                    // Ignore all errors
                    UHD_LOG_DEBUG("RFNOC",
                        "chdr_ctrl_endpoint: Unidentified error in async message handler "
                        "loop.");
                    _num_drops++;
                }
                _xport->release_recv_buff(std::move(buff));
            } else {
                // FIXME Move lock back to lock_guard once have threaded_io_service
                lock.unlock();
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

    using ep_map_key_t = std::pair<sep_id_t, uint16_t>;

    // The endpoint ID of this software endpoint
    const sep_id_t _my_epid;
    // Send/recv transports
    chdr_ctrl_xport::sptr _xport;
    // The curent sequence number for a send packet
    size_t _send_seqnum = 0;
    // The number of packets dropped due to misclassification. See also
    // get_num_drops()
    std::atomic<size_t> _num_drops;
    // Packet containers
    chdr_ctrl_packet::uptr _send_pkt;
    chdr_ctrl_packet::cuptr _recv_pkt;
    // A collection of ctrlport endpoints (keyed by the port number)
    std::map<ep_map_key_t, ctrlport_endpoint::sptr> _endpoint_map;
    // Mutex that protects all state in this class except for _send_pkt
    std::mutex _mutex;
    // Mutex that protects _send_pkt and _xport.send
    std::mutex _send_mutex;
    // A thread that will handle all responses and async message requests
    // Must be declared after the mutexes, the thread starts at construction and
    // depends on the mutexes having been constructed.
    std::atomic_bool _stop_recv_thread;
    std::thread _recv_thread;
};

chdr_ctrl_endpoint::uptr chdr_ctrl_endpoint::make(chdr_ctrl_xport::sptr xport,
    const chdr::chdr_packet_factory& pkt_factory,
    sep_id_t my_epid)
{
    return std::make_unique<chdr_ctrl_endpoint_impl>(xport, pkt_factory, my_epid);
}
