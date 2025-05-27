//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_STREAM_PYTHON_HPP
#define INCLUDED_UHD_STREAM_PYTHON_HPP

#include <uhd/stream.hpp>
#include <uhd/types/metadata.hpp>
#include <boost/format.hpp>

#include <numpy/ndarrayobject.h>
#include <numpy/ndarraytypes.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

static size_t wrap_recv(uhd::rx_streamer* rx_stream,
    py::array& array,
    uhd::rx_metadata_t& metadata,
    const double timeout = 0.1)
{
    // Using the pybind11's numpy array wrapper provides easy
    // access instead of using numpy's C API macros and also keeps
    // track of the reference count for the numpy array.
    if (!array.writeable()) {
        throw std::runtime_error("Array is not writable");
    }
    char* data            = static_cast<char*>(array.mutable_data());
    const size_t dims     = array.ndim();
    const size_t channels = rx_stream->get_num_channels();

    // Check if numpy array sizes are okay
    if (((channels > 1) && (dims != 2)) or ((size_t)array.shape(0) < channels)) {
        // If we don't have a 2D NumPy array, assume we have a 1D array
        size_t input_channels = (dims != 2) ? 1 : array.shape(0);
        throw uhd::runtime_error(
            str(boost::format("Number of RX channels (%d) does not match the dimensions "
                              "of the data array (%d)")
                % channels % input_channels));
    }

    // Get a pointer to the storage
    std::vector<void*> channel_storage;
    for (size_t i = 0; i < channels; ++i) {
        channel_storage.push_back((void*)(data + i * array.strides(0)));
    }

    // determine how many samples we can receive
    size_t nsamps_per_buff = dims > 1 ? array.shape(1) : array.size();

    // Release the GIL only for the recv() call
    const size_t result = [&]() {
        py::gil_scoped_release release;
        // Call the real recv()
        return rx_stream->recv(channel_storage, nsamps_per_buff, metadata, timeout);
    }();

    return result;
}

static size_t wrap_send(uhd::tx_streamer* tx_stream,
    py::array& array,
    uhd::tx_metadata_t& metadata,
    const double timeout = 0.1)
{
    // Using pybind11's numpy array wrapper provides easy
    // access instead of using numpy's C API macros and also keeps
    // track of the reference count for the numpy array.
    const char* data      = static_cast<const char*>(array.data());
    const size_t dims     = array.ndim();
    const size_t channels = tx_stream->get_num_channels();

    // Check if numpy array sizes are ok
    if (((channels > 1) && (dims != 2)) or ((size_t)array.shape(0) < channels)) {
        // If we don't have a 2D NumPy array, assume we have a 1D array
        size_t input_channels = (dims != 2) ? 1 : array.shape(0);
        throw uhd::runtime_error(
            str(boost::format("Number of TX channels (%d) does not match the dimensions "
                              "of the data array (%d)")
                % channels % input_channels));
    }

    // Get a pointer to the storage
    std::vector<void*> channel_storage;
    for (size_t i = 0; i < channels; ++i) {
        channel_storage.push_back((void*)(data + i * array.strides(0)));
    }

    // determine how many samples we have to send
    size_t nsamps_per_buff = (dims > 1) ? (size_t)array.shape(1) : array.size();

    // Release the GIL only for the send() call
    const size_t result = [&]() {
        py::gil_scoped_release release;
        // Call the real send()
        return tx_stream->send(channel_storage, nsamps_per_buff, metadata, timeout);
    }();

    return result;
}

static bool wrap_recv_async_msg(uhd::tx_streamer* tx_stream,
    uhd::async_metadata_t& async_metadata,
    double timeout = 0.1)
{
    // Release the GIL
    py::gil_scoped_release release;
    return tx_stream->recv_async_msg(async_metadata, timeout);
}

void export_stream(py::module& m)
{
    using stream_args_t = uhd::stream_args_t;
    using rx_streamer   = uhd::rx_streamer;
    using tx_streamer   = uhd::tx_streamer;

    py::class_<stream_args_t>(m, "stream_args")
        .def(py::init<const std::string&, const std::string&>())
        // Properties
        .def_readwrite("cpu_format", &stream_args_t::cpu_format)
        .def_readwrite("otw_format", &stream_args_t::otw_format)
        .def_readwrite("args", &stream_args_t::args)
        .def_readwrite("channels", &stream_args_t::channels);

    py::class_<rx_streamer, rx_streamer::sptr>(m, "rx_streamer", "See: uhd::rx_streamer")
        // Methods
        .def("recv",
            &wrap_recv,
            py::arg("np_array"),
            py::arg("metadata"),
            py::arg("timeout") = 0.1)
        .def("get_num_channels", &uhd::rx_streamer::get_num_channels)
        .def("get_max_num_samps", &uhd::rx_streamer::get_max_num_samps)
        .def("issue_stream_cmd", &uhd::rx_streamer::issue_stream_cmd);

    py::class_<tx_streamer, tx_streamer::sptr>(m, "tx_streamer", "See: uhd::tx_streamer")
        // Methods
        .def("send",
            &wrap_send,
            py::arg("np_array"),
            py::arg("metadata"),
            py::arg("timeout") = 0.1)
        .def("get_num_channels", &tx_streamer::get_num_channels)
        .def("get_max_num_samps", &tx_streamer::get_max_num_samps)
        .def("recv_async_msg",
            &wrap_recv_async_msg,
            py::arg("async_metadata"),
            py::arg("timeout") = 0.1)
        .def(
            "recv_async_msg",
            [](tx_streamer& self, const double timeout) -> py::object {
                uhd::async_metadata_t md;
                if (self.recv_async_msg(md, timeout)) {
                    return py::cast(md);
                }
                return py::cast(nullptr);
            },
            py::arg("timeout") = 0.1);
}

#endif /* INCLUDED_UHD_STREAM_PYTHON_HPP */
