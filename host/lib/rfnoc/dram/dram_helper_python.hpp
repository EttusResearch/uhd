//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/dram/dram_helper.hpp>

#include <numpy/ndarrayobject.h>
#include <numpy/ndarraytypes.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

using namespace uhd::rfnoc;

static void wrap_upload(rfnoc_graph::sptr graph,
    const py::array& array,
    const std::string& cpu_fmt,
    const std::string& otw_fmt,
    replay_block_control::sptr replay,
    size_t port,
    size_t offset)
{
    // Using pybind11's numpy array wrapper provides easy
    // access instead of using numpy's C API macros and also keeps
    // track of the reference count for the numpy array.
    const char* data  = static_cast<const char*>(array.data());
    const size_t dims = array.ndim();

    if (dims != 1) {
        throw uhd::runtime_error("Only 1D numpy arrays are supported");
    }

    // determine how many samples we have to send
    size_t nsamps = array.size();

    uhd::rfnoc::dram::upload(graph, data, nsamps, cpu_fmt, otw_fmt, replay, port, offset);
}

static void wrap_download(rfnoc_graph::sptr graph,
    py::array& array,
    const std::string& cpu_fmt,
    const std::string& otw_fmt,
    replay_block_control::sptr replay,
    size_t port,
    size_t offset)
{
    // Using the pybind11's numpy array wrapper provides easy
    // access instead of using numpy's C API macros and also keeps
    // track of the reference count for the numpy array.
    if (!array.writeable()) {
        throw std::runtime_error("Array is not writable");
    }
    char* data        = static_cast<char*>(array.mutable_data());
    const size_t dims = array.ndim();

    if (dims != 1) {
        throw uhd::runtime_error("Only 1D numpy arrays are supported");
    }

    // determine how many samples we can receive
    size_t nsamps = array.size();

    uhd::rfnoc::dram::download(
        graph, data, nsamps, cpu_fmt, otw_fmt, replay, port, offset);
}


void export_dram_helper(py::module& m)
{
    m.def("upload",
        &wrap_upload,
        py::arg("graph"),
        py::arg("np_array"),
        py::arg("cpu_fmt"),
        py::arg("otw_fmt") = "sc16",
        py::arg("replay")  = nullptr,
        py::arg("port")    = 0,
        py::arg("offset")  = 0);
    m.def("download",
        &wrap_download,
        py::arg("graph"),
        py::arg("np_array"),
        py::arg("cpu_fmt"),
        py::arg("otw_fmt") = "sc16",
        py::arg("replay")  = nullptr,
        py::arg("port")    = 0,
        py::arg("offset")  = 0);
}
