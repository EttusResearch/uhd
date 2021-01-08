//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_devices.hpp"
#include "mpmd_impl.hpp"
#include <uhd/device.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/rfnoc/rfnoc_device.hpp>
// Need this import because pybind doesn't have an equivalent to Py_IsInitialized()
#include <Python.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>

using namespace uhd;
using namespace uhd::mpmd;
using namespace std::chrono_literals;
namespace py = pybind11;

constexpr auto SIMULATOR_EXIT_TIMEOUT    = 5s;
constexpr auto SIMULATOR_STARTUP_TIMEOUT = 5s;

// There can only be one python interpreter instantiated at a time
// The guard means we only destroy the interpreter if we created it
static std::unique_ptr<py::scoped_interpreter> interpreter_guard;

void ensure_python_interpreter()
{
    // This call is needed because the interpreter may already be running
    // i.e. UHD is being called from python through pyuhd
    if (not Py_IsInitialized()) {
        interpreter_guard = std::make_unique<py::scoped_interpreter>();
    }
}

py::object get_simulator_module()
{
    try {
        return py::module::import("usrp_mpm.process_manager");
    } catch (const py::error_already_set& ex) {
        std::string message("Simulator failed to import: ");
        message.append(ex.what());
        message.append("\nPYTHONPATH: ");
        auto pythonpath =
            py::str(py::module::import("sys").attr("path")).cast<std::string>();
        message.append(pythonpath);
        throw std::runtime_error(message);
    }
}

device_addrs_t mpmd_find_with_addr(
    const std::string& mgmt_addr, const device_addr_t& hint_);

void shutdown_process(py::object& process_manager)
{
    // TODO: Sometimes during a TX, the simulator gets shutdown before all of the packets
    // are sent
    py::object stop_fn = process_manager.attr("stop");
    const double timeout_floating =
        std::chrono::duration<double>(SIMULATOR_EXIT_TIMEOUT).count();
    const bool result = stop_fn(timeout_floating).cast<bool>();
    if (!result) {
        UHD_LOG_WARNING("SIM",
            "Simulator Subprocess did not exit, manual cleanup of subprocesses may "
            "be necessary.")
        process_manager.attr("terminate")();
    }
}

class sim_impl : public mpmd_impl
{
public:
    sim_impl(const uhd::device_addr_t& device_addr, py::object process_manager)
        : mpmd_impl(device_addr), _process_manager(std::move(process_manager))
    {
    }

    ~sim_impl() override
    {
        // Destroys the mb_ifaces, causing mpm to be unclaimed before shutting down the
        // simulator
        _deinit();
        shutdown_process(_process_manager);
    }

private:
    // This is an object of type ProcessManager
    // See mpm/python/usrp_mpm/process_manager.py
    py::object _process_manager;
};

device_addrs_t sim_find(const device_addr_t& hint_)
{
    device_addrs_t simulators;
    if (hint_.has_key("type") && hint_["type"] == "sim") {
        simulators.push_back(hint_);
        // Set addr to localhost
        simulators.back()["addr"]      = "127.0.0.1";
        simulators.back()["mgmt_addr"] = "127.0.0.1";
        // So discovery doesn't complain about hint mismatch
        simulators.back()["type"] = MPM_CATCHALL_DEVICE_TYPE;
    }
    return simulators;
}

/*! Ensure that the simulator is loaded by pinging the discovery port until it responds or
 *  the function times out
 */
bool check_simulator_status(
    const device_addr_t& device_addr, std::chrono::milliseconds timeout)
{
    const auto timeout_time = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < timeout_time) {
        const auto devices = mpmd_find_with_addr(device_addr["mgmt_addr"], device_addr);
        if (!devices.empty()) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return false;
}

device::sptr sim_make(const device_addr_t& device_args)
{
    // Ensure the interpreter is loaded
    ensure_python_interpreter();
    py::object manager_module = get_simulator_module();
    py::object manager_class  = manager_module.attr("ProcessManager");

    std::string config_arg("--default-args=config=");
    if (not device_args.has_key("config")) {
        throw std::runtime_error(
            "Please specify a config file using the args key 'config'");
    }
    config_arg.append(device_args["config"]);

    py::list process_args;
    process_args.append(py::str(config_arg));

    if (device_args.has_key("log_level")) {
        std::string level = device_args["log_level"];
        if (level == "trace") {
            process_args.append(py::str("-vv"));
        } else if (level == "debug") {
            process_args.append(py::str("-v"));
        } else if (level == "info") {
            // No-op
        } else if (level == "warning") {
            process_args.append(py::str("-q"));
        } else if (level == "error") {
            process_args.append(py::str("-qq"));
        }
    }

    py::object process_manager = manager_class(process_args);
    process_manager.attr("start")();

    const uint32_t pid = process_manager.attr("pid")().cast<uint32_t>();
    UHD_LOG_INFO("SIM", "Starting simulator as pid " << pid);
    if (not check_simulator_status(device_args, SIMULATOR_STARTUP_TIMEOUT)) {
        shutdown_process(process_manager);
        throw std::runtime_error("Simulator Startup timed out!");
    }
    return static_cast<device::sptr>(
        std::make_shared<sim_impl>(device_args, std::move(process_manager)));
}

UHD_STATIC_BLOCK(register_sim_device)
{
    device::register_device(&sim_find, &sim_make, device::USRP);
}
