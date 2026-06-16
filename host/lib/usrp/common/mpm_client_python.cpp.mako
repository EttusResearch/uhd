//
// Auto-generated pybind11 bindings for uhd::rpc_client
// Rendered at build time from mpm_server.proto by mpm_client_generate_cpp.py
//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
// clang-format off

#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include "usrp/mpm_client_python.hpp"
#include <mpm_client.hpp>

void export_rpc_client(py::module& m)
{
    using rpc_client    = uhd::rpc_client;
    using timeout_scope = uhd::rpc_client::timeout_scope;
    using dboard_iface  = uhd::rpc_client::dboard_iface;

    // -------------------------------------------------------------------------
    // Timeout scope handle
    // -------------------------------------------------------------------------
    py::class_<timeout_scope, rpc_client::timeout_scope_uptr>(m, "rpc_timeout_scope");

    // -------------------------------------------------------------------------
    // Daughterboard interface
    // -------------------------------------------------------------------------
    py::class_<dboard_iface, dboard_iface::sptr>(m, "rpc_dboard_iface")
% for method in dboard_methods:
<%
    param_names = [p.split()[-1] for p in method['parameters']]
    args_str = ', '.join('py::arg("' + n + '")' for n in param_names)
    suffix = (',\n             ' + args_str) if args_str else ''
%>        .def("${method['name']}", &dboard_iface::${method['name']}${suffix})
% endfor
    ;

    // -------------------------------------------------------------------------
    // Main RPC client
    // -------------------------------------------------------------------------
    py::class_<rpc_client, rpc_client::sptr>(m, "rpc_client")
        // Factory
        .def_static("make",              &rpc_client::make,
             py::arg("server_name"), py::arg("port"), py::arg("timeout_ms"))

        // Session management
        .def("set_token",                &rpc_client::set_token,        py::arg("token"))
        .def("set_timeout",              &rpc_client::set_timeout,      py::arg("timeout_ms"))
        .def("set_scope_timeout",        &rpc_client::set_scope_timeout,
             py::arg("timeout_ms"))

        // Generated RPC methods
% for method in main_methods:
<%
    param_names = [p.split()[-1] for p in method['parameters']]
    args_str = ', '.join('py::arg("' + n + '")' for n in param_names)
    suffix = (',\n             ' + args_str) if args_str else ''
%>        .def("${method['name']}", &rpc_client::${method['name']}${suffix})
% endfor

        // Daughterboard accessor — reference owned by rpc_client; keep parent alive
        .def("get_dboard",               &rpc_client::get_dboard,
             py::arg("db_idx"),
             py::return_value_policy::reference_internal)
    ;

}

// clang-format on