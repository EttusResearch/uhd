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

static size_t wrap_recv(uhd::rx_streamer* rx_stream,
    py::object& np_array,
    uhd::rx_metadata_t& metadata,
    const double timeout = 0.1)
{
    // Get a numpy array object from given python object
    // No sanity checking possible!
    PyObject* array_obj           = PyArray_FROM_OF(np_array.ptr(), NPY_ARRAY_CARRAY);
    PyArrayObject* array_type_obj = reinterpret_cast<PyArrayObject*>(array_obj);

    // Get dimensions of the numpy array
    const size_t dims     = PyArray_NDIM(array_type_obj);
    const npy_intp* shape = PyArray_SHAPE(array_type_obj);

    // How many bytes to jump to get to the next element of this stride
    // (next row)
    const npy_intp* strides = PyArray_STRIDES(array_type_obj);
    const size_t channels   = rx_stream->get_num_channels();

    // Check if numpy array sizes are okay
    if (((channels > 1) && (dims != 2)) or ((size_t)shape[0] < channels)) {
        // Manually decrement the ref count
        Py_DECREF(array_obj);
        // If we don't have a 2D NumPy array, assume we have a 1D array
        size_t input_channels = (dims != 2) ? 1 : shape[0];
        throw uhd::runtime_error(
            str(boost::format("Number of RX channels (%d) does not match the dimensions "
                              "of the data array (%d)")
                % channels % input_channels));
    }

    // Get a pointer to the storage
    std::vector<void*> channel_storage;
    char* data = PyArray_BYTES(array_type_obj);
    for (size_t i = 0; i < channels; ++i) {
        channel_storage.push_back((void*)(data + i * strides[0]));
    }

    // Get data buffer and size of the array
    size_t nsamps_per_buff;
    if (dims > 1) {
        nsamps_per_buff = (size_t)shape[1];
    } else {
        nsamps_per_buff = PyArray_SIZE(array_type_obj);
    }

    // Release the GIL only for the recv() call
    const size_t result = [&]() {
        py::gil_scoped_release release;
        // Call the real recv()
        return rx_stream->recv(channel_storage, nsamps_per_buff, metadata, timeout);
    }();

    // Manually decrement the ref count
    Py_DECREF(array_obj);
    return result;
}

static size_t wrap_send(uhd::tx_streamer* tx_stream,
    py::object& np_array,
    uhd::tx_metadata_t& metadata,
    const double timeout = 0.1)
{
    // Get a numpy array object from given python object
    // No sanity checking possible!
    // Note: this increases the ref count, which we'll need to manually decrease at the
    // end
    PyObject* array_obj           = PyArray_FROM_OF(np_array.ptr(), NPY_ARRAY_CARRAY);
    PyArrayObject* array_type_obj = reinterpret_cast<PyArrayObject*>(array_obj);

    // Get dimensions of the numpy array
    const size_t dims     = PyArray_NDIM(array_type_obj);
    const npy_intp* shape = PyArray_SHAPE(array_type_obj);

    // How many bytes to jump to get to the next element of the stride
    // (next row)
    const npy_intp* strides = PyArray_STRIDES(array_type_obj);
    const size_t channels   = tx_stream->get_num_channels();

    // Check if numpy array sizes are ok
    if (((channels > 1) && (dims != 2)) or ((size_t)shape[0] < channels)) {
        // Manually decrement the ref count
        Py_DECREF(array_obj);
        // If we don't have a 2D NumPy array, assume we have a 1D array
        size_t input_channels = (dims != 2) ? 1 : shape[0];
        throw uhd::runtime_error(
            str(boost::format("Number of TX channels (%d) does not match the dimensions "
                              "of the data array (%d)")
                % channels % input_channels));
    }

    // Get a pointer to the storage
    std::vector<void*> channel_storage;
    char* data = PyArray_BYTES(array_type_obj);
    for (size_t i = 0; i < channels; ++i) {
        channel_storage.push_back((void*)(data + i * strides[0]));
    }

    // Get data buffer and size of the array
    size_t nsamps_per_buff = (dims > 1) ? (size_t)shape[1] : PyArray_SIZE(array_type_obj);

    // Release the GIL only for the send() call
    const size_t result = [&]() {
        py::gil_scoped_release release;
        // Call the real send()
        return tx_stream->send(channel_storage, nsamps_per_buff, metadata, timeout);
    }();

    // Manually decrement the ref count
    Py_DECREF(array_obj);
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
