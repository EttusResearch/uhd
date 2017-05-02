//
// Copyright 2017 Ettus Research
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_UHD_STREAM_PYTHON_HPP
#define INCLUDED_UHD_STREAM_PYTHON_HPP

#include <uhd/stream.hpp>

static size_t wrap_recv(uhd::rx_streamer *rx_stream,
                        bp::object &np_array,
                        bp::object &metadata)
{
    // Extract the metadata
    bp::extract<uhd::rx_metadata_t&> get_metadata(metadata);
    if (not get_metadata.check())
    {
        return 0;
    }

    // Get a numpy array object from given python object
    // No sanity checking possible!
    PyObject* array_obj = PyArray_FROM_OF(np_array.ptr(), NPY_ARRAY_CARRAY);
    PyArrayObject* array_type_obj = reinterpret_cast<PyArrayObject*>(array_obj);

    // Get dimensions of the numpy array
    const size_t dims = PyArray_NDIM(array_type_obj);
    const npy_intp* shape = PyArray_SHAPE(array_type_obj);

    // How many bytes to jump to get to the next element of this stride
    // (next row)
    const npy_intp* strides = PyArray_STRIDES(array_type_obj);
    const size_t channels = rx_stream->get_num_channels();

    // Check if numpy array sizes are okay
    if ((channels > 1) && (dims != 2)) {
        return 0;
    } else if ((size_t) shape[0] < channels) {
        return 0;
    }

    // Get a pointer to the storage
    std::vector<void*> channel_storage;
    char* data = PyArray_BYTES(array_type_obj);
    for (size_t i = 0; i < channels; ++i)
    {
        channel_storage.push_back((void*)(data + i * strides[0]));
    }

    // Get data buffer and size of the array
    size_t nsamps_per_buff;
    if (dims > 1) {
        nsamps_per_buff = (size_t) shape[1];
    } else {
        nsamps_per_buff = PyArray_SIZE(array_type_obj);
    }

    // Call the real recv()
    const size_t result = rx_stream->recv(
        channel_storage,
        nsamps_per_buff,
        get_metadata()
    );

    return result;
}

static size_t wrap_send(uhd::tx_streamer *tx_stream,
                        bp::object &np_array,
                        bp::object &metadata)
{
    // Extract the metadata
    bp::extract<uhd::tx_metadata_t&> get_metadata(metadata);
    if (not get_metadata.check())
    {
        return 0;
    }

    // Get a numpy array object from given python object
    // No sanity checking possible!
    PyObject* array_obj = PyArray_FROM_OF(np_array.ptr(),NPY_ARRAY_CARRAY);
    PyArrayObject* array_type_obj = reinterpret_cast<PyArrayObject*>(array_obj);

    // Get dimensions of the numpy array
    const size_t dims = PyArray_NDIM(array_type_obj);
    const npy_intp* shape = PyArray_SHAPE(array_type_obj);

    // How many bytes to jump to get to the next element of the stride
    // (next row)
    const npy_intp* strides = PyArray_STRIDES(array_type_obj);
    const size_t channels = tx_stream->get_num_channels();

    // Check if numpy array sizes are ok
    if ((channels > 1) && (dims != 2)) {
        return 0;
    } else if ((size_t) shape[0] < channels) {
        return 0;
    }

    // Get a pointer to the storage
    std::vector<void*> channel_storage;
    char* data = PyArray_BYTES(array_type_obj);
    for (size_t i = 0; i < channels; ++i)
    {
        channel_storage.push_back((void*)(data + i * strides[0]));
    }

    // Get data buffer and size of the array
    size_t nsamps_per_buff;
    if (dims > 1) {
        nsamps_per_buff = (size_t) shape[1];
    } else {
        nsamps_per_buff = PyArray_SIZE(array_type_obj);
    }

    // Call the real recv()
    const size_t result = tx_stream->send(
        channel_storage,
        nsamps_per_buff,
        get_metadata()
    );
    return result;
}

void export_stream()
{
    using stream_args_t = uhd::stream_args_t;
    using rx_streamer   = uhd::rx_streamer;
    using tx_streamer   = uhd::tx_streamer;

    bp::class_<stream_args_t>
        ("stream_args", bp::init<const std::string&, const std::string&>())

        // Properties
        .def_readwrite("cpu_format", &stream_args_t::cpu_format)
        .def_readwrite("otw_format", &stream_args_t::otw_format)
        .def_readwrite("args"      , &stream_args_t::args      )
        .def_readwrite("channels"  , &stream_args_t::channels  )
        ;

    bp::class_<
        rx_streamer,
        boost::shared_ptr<rx_streamer>,
        boost::noncopyable>("rx_streamer", bp::no_init)

        // Methods
        .def("recv"             , &wrap_recv                          )
        .def("get_num_channels" , &uhd::rx_streamer::get_num_channels )
        .def("get_max_num_samps", &uhd::rx_streamer::get_max_num_samps)
        .def("issue_stream_cmd" , &uhd::rx_streamer::issue_stream_cmd )
        ;

    bp::class_<
        tx_streamer,
        boost::shared_ptr<tx_streamer>,
        boost::noncopyable>("tx_streamer", bp::no_init)

        // Methods
        .def("send"             , &wrap_send                     )
        .def("get_num_channels" , &tx_streamer::get_num_channels )
        .def("get_max_num_samps", &tx_streamer::get_max_num_samps)
        ;
}

#endif /* INCLUDED_UHD_STREAM_PYTHON_HPP */
