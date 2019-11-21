//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhdlib/transport/dpdk/common.hpp>
#include <uhdlib/transport/dpdk/service_queue.hpp>
#include <boost/program_options.hpp>
#include <iostream>
#include <memory>
#include <thread>

namespace po = boost::program_options;
namespace dpdk = uhd::transport::dpdk;

void requester(dpdk::service_queue* queue)
{
    uint32_t count = 0;
    std::chrono::seconds block(-1);
    auto req = dpdk::wait_req_alloc(
        dpdk::wait_type::WAIT_TYPE_COUNT, &count
    );
    std::cout << "Requesting count increment" << std::endl;
    queue->submit(req, block);
    if (count == 1) {
        std::cout << "PASS: Count=1" << std::endl;
    }
    wait_req_put(req);

    std::cout << "Requesting termination" << std::endl;
    req = dpdk::wait_req_alloc(
        dpdk::wait_type::WAIT_FLOW_CLOSE, NULL
    );
    queue->submit(req, block);
    wait_req_put(req);
}

void servicer(uhd::transport::dpdk::service_queue* queue)
{
    bool running = true;
    while (running) {
        auto req = queue->pop();
        if (!req) {
            std::this_thread::yield();
            continue;
        }
        switch (req->reason) {
        case dpdk::wait_type::WAIT_TYPE_COUNT:
            (*(uint32_t *) req->data)++;
            break;
        case dpdk::wait_type::WAIT_FLOW_CLOSE:
            running = false;
            break;
        case dpdk::wait_type::WAIT_SIMPLE:
            break;
        default:
            std::cout << "ERROR: Received unexpected service request type" << std::endl;
            throw uhd::runtime_error("Unexpected service request type");
        }
        if (queue->complete(req) == -ENOBUFS) {
            req->reason = dpdk::wait_type::WAIT_SIMPLE;
            while (queue->requeue(req) == -ENOBUFS);
        }
    }
}

int main(int argc, char **argv)
{
    po::options_description desc("Allowed options");
    int status = 0;
    std::string args;
    desc.add_options()
        ("help", "help message")
        ("args", po::value<std::string>(&args)->default_value(""), "UHD-DPDK args")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 0;
    }

    auto dpdk_args = uhd::device_addr_t(args);
    auto ctx = uhd::transport::dpdk::dpdk_ctx::get();
    ctx->init(args);

    uhd::transport::dpdk::dpdk_port* port = ctx->get_port(0);
    std::cout << "Port 0 MTU: " << port->get_mtu() << std::endl;
    status = ctx->get_port_link_status(0);
    std::cout << "Port 0 Link up: " << status << std::endl;

    std::cout << std::endl << "Now testing the service queue..." << std::endl;
    // Technically this isn't correct, since the lcore ID would need to be the
    // service thread's, but we didn't use a DPDK lcore for this...
    auto queue = new uhd::transport::dpdk::service_queue(8, rte_lcore_id());
    std::thread service_thread(servicer, queue);
    requester(queue);
    service_thread.join();
    std::cout << "PASS: Service thread terminated" << std::endl;
    delete queue;
    ctx.reset();
    return 0;
}
