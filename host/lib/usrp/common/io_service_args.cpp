//
// Copyright 2019 Ettus Research, a National Instruments brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/common/io_service_args.hpp>
#include <uhdlib/usrp/constrained_device_args.hpp>
#include <string>

static const std::string LOG_ID            = "IO_SRV";
static const size_t MAX_NUM_XPORT_ADAPTERS = 2;

namespace uhd { namespace usrp {

namespace {

bool get_bool_arg(const device_addr_t& args, const std::string& key, const bool def)
{
    constrained_device_args_t::bool_arg arg(key, def);
    if (args.has_key(key)) {
        arg.parse(args[key]);
    }
    return arg.get();
}

io_service_args_t::wait_mode_t get_wait_mode_arg(const device_addr_t& args,
    const std::string& key,
    const io_service_args_t::wait_mode_t def)
{
    constrained_device_args_t::enum_arg<io_service_args_t::wait_mode_t> arg(key,
        def,
        {{"poll", io_service_args_t::POLL}, {"block", io_service_args_t::BLOCK}});

    if (args.has_key(key)) {
        arg.parse(args[key]);
    }
    return arg.get();
}

}; // namespace

io_service_args_t read_io_service_args(
    const device_addr_t& args, const io_service_args_t& defaults)
{
    io_service_args_t io_srv_args;
    std::string tmp_str, default_str;

    io_srv_args.recv_offload = get_bool_arg(args, "recv_offload", defaults.recv_offload);
    io_srv_args.send_offload = get_bool_arg(args, "send_offload", defaults.send_offload);

    io_srv_args.recv_offload_wait_mode = get_wait_mode_arg(
        args, "recv_offload_wait_mode", defaults.recv_offload_wait_mode);
    io_srv_args.send_offload_wait_mode = get_wait_mode_arg(
        args, "send_offload_wait_mode", defaults.send_offload_wait_mode);

    io_srv_args.num_poll_offload_threads =
        args.cast<size_t>("num_poll_offload_threads", defaults.num_poll_offload_threads);
    if (io_srv_args.num_poll_offload_threads == 0) {
        UHD_LOG_WARNING(LOG_ID,
            "Invalid value for num_poll_offload_threads. "
            "Value must be greater than 0.");
        io_srv_args.num_poll_offload_threads = 1;
    }

    auto create_key = [](const std::string& base, size_t index) {
        return base + "_" + std::to_string(index);
    };

    for (size_t i = 0; i < MAX_NUM_XPORT_ADAPTERS; i++) {
        std::string key = create_key("recv_offload_thread_cpu", i);
        if (args.has_key(key)) {
            io_srv_args.recv_offload_thread_cpu.push_back(args.cast<size_t>(key, 0));
        } else {
            io_srv_args.recv_offload_thread_cpu.push_back({});
        }
    }

    for (size_t i = 0; i < MAX_NUM_XPORT_ADAPTERS; i++) {
        std::string key = create_key("send_offload_thread_cpu", i);
        if (args.has_key(key)) {
            io_srv_args.send_offload_thread_cpu.push_back(args.cast<size_t>(key, 0));
        } else {
            io_srv_args.send_offload_thread_cpu.push_back({});
        }
    }

    for (size_t i = 0; i < io_srv_args.num_poll_offload_threads; i++) {
        std::string key = create_key("poll_offload_thread_cpu", i);
        if (args.has_key(key)) {
            io_srv_args.poll_offload_thread_cpu.push_back(args.cast<size_t>(key, 0));
        } else {
            io_srv_args.poll_offload_thread_cpu.push_back({});
        }
    }

    return io_srv_args;
}

}} // namespace uhd::usrp
