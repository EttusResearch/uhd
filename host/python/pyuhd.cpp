//
// Copyright 2017 Ettus Research LLC
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

#include <uhd/types/stream_cmd.hpp>
#include <uhd/types/tune_result.hpp>
#include <uhd/types/tune_request.hpp>
#include <uhd/usrp/multi_usrp.hpp>
#include <uhd/stream.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>


namespace bp = boost::python;


size_t Pyrecv(uhd::rx_streamer* rx_stream, bp::object& np_array, uhd::rx_metadata_t& rx_metadata){
    // Get a numpy array object from given python object no sanity checking
    PyObject* array_obj = PyArray_FROM_OF(np_array.ptr(),NPY_ARRAY_CARRAY);
    PyArrayObject* array_type_obj = reinterpret_cast<PyArrayObject*>(array_obj);

    // Get dimensions of the numpy array
    const size_t dims = PyArray_NDIM(array_type_obj);
    const npy_intp* shape = PyArray_SHAPE(array_type_obj);
    // How many bytes to jump to get to the next element of this stride
    const npy_intp* strides = PyArray_STRIDES(array_type_obj);
    const size_t channels = rx_stream->get_num_channels();


    // Check if numpy array sizes are ok
    if ((channels > 1) && (dims != 2)){
        return 0;
    }else if ((size_t)shape[0] < channels){
        return 0;
    }

    // Get a pointer to the storage
    std::vector<void*> channel_storage;
    char* data = PyArray_BYTES(array_type_obj);
    for (size_t i = 0; i<channels; ++i){
        channel_storage.push_back((void*)(data+i*strides[0]));
    }

    // Get data buffer and size of the array
    size_t nsamps_per_buff;
    if (dims > 1){
        nsamps_per_buff = (size_t)shape[1];
    }else{
        nsamps_per_buff = PyArray_SIZE(array_type_obj);
    }

    // Call the real recv()
    const size_t result =  rx_stream->recv(
        channel_storage,
        nsamps_per_buff,
        rx_metadata);
    return result;
};

size_t Pysend(uhd::tx_streamer* tx_stream, bp::object& np_array, uhd::tx_metadata_t& tx_metadata){
    // Get a numpy array object from given python object no sanity checking
    PyObject* array_obj = PyArray_FROM_OF(np_array.ptr(),NPY_ARRAY_CARRAY);
    PyArrayObject* array_type_obj = reinterpret_cast<PyArrayObject*>(array_obj);

    // Get dimensions of the numpy array
    const size_t dims = PyArray_NDIM(array_type_obj);
    const npy_intp* shape = PyArray_SHAPE(array_type_obj);
    // How many bytes to jump to get to the next element of the stride (next row)
    const npy_intp* strides = PyArray_STRIDES(array_type_obj);
    const size_t channels = tx_stream->get_num_channels();

    // Check if numpy array sizes are ok
    if ((channels > 1) && (dims != 2)){
        return 0;
    }else if ((size_t)shape[0] < channels){
        return 0;
    }

    // Get a pointer to the storage
    std::vector<void*> channel_storage;
    char* data = PyArray_BYTES(array_type_obj);
    for (size_t i = 0; i<channels; ++i){
        channel_storage.push_back((void*)(data+i*strides[0]));
    }

    // Get data buffer and size of the array
    size_t nsamps_per_buff;
    if (dims > 1){
        nsamps_per_buff = (size_t)shape[1];
    }else{
        nsamps_per_buff = PyArray_SIZE(array_type_obj);
    }

    // Call the real recv()
    const size_t result =  tx_stream->send(
        channel_storage,
        nsamps_per_buff,
        tx_metadata);
    return result;
};

// Manual wrapping beccause of non-standard overloading
void set_rx_gain_conv(uhd::usrp::multi_usrp* multi_usrp, double gain, size_t chan){
    multi_usrp->set_rx_gain(gain, chan);
}

void set_tx_gain_conv(uhd::usrp::multi_usrp* multi_usrp, double gain, size_t chan){
    multi_usrp->set_tx_gain(gain, chan);
}

// Converter for std::vector / std::list arguments from python iterables
struct iterable_converter
{
  template <typename Container>
  iterable_converter&
  from_python()
  {
    bp::converter::registry::push_back(
      &iterable_converter::convertible,
      &iterable_converter::construct<Container>,
      bp::type_id<Container>());
    return *this;
  }

  static void* convertible(PyObject* object)
  {
    return PyObject_GetIter(object) ? object : NULL;
  }

  template <typename Container>
  static void construct(
    PyObject* object,
    bp::converter::rvalue_from_python_stage1_data* data)
  {
    // Object is a borrowed reference, so create a handle indicting it is
    // borrowed for proper reference counting.
    bp::handle<> handle(bp::borrowed(object));

    // Obtain a handle to the memory block that the converter has allocated
    // for the C++ type.
    typedef bp::converter::rvalue_from_python_storage<Container>
                                                                storage_type;
    void* storage = reinterpret_cast<storage_type*>(data)->storage.bytes;

    typedef bp::stl_input_iterator<typename Container::value_type>
                                                                    iterator;

    // Allocate the C++ type into the converter's memory block, and assign
    // its handle to the converter's convertible variable.  The C++
    // container is populated by passing the begin and end iterators of
    // the python object to the container's constructor.
    new (storage) Container(
      iterator(bp::object(handle)), // begin
      iterator());                      // end
    data->convertible = storage;
  }
};


void export_multi_usrp(void)
{
    bp::object multi_usrp_module(bp::handle<>(bp::borrowed(PyImport_AddModule("pyuhd.multi_usrp"))));
    // make "from mypackage import IO" work
    bp::scope().attr("multi_usrp") = multi_usrp_module;
    // set the current scope to the new sub-module
    bp::scope io_scope = multi_usrp_module;

    bp::class_<uhd::usrp::multi_usrp, boost::shared_ptr<uhd::usrp::multi_usrp>, boost::noncopyable>("multi_usrp", bp::no_init)
        .def("make", &uhd::usrp::multi_usrp::make)
        .staticmethod("make")
        .def("get_rx_freq", &uhd::usrp::multi_usrp::get_rx_freq)
        .def("get_rx_num_channels", &uhd::usrp::multi_usrp::get_rx_num_channels)
        .def("get_rx_rate", &uhd::usrp::multi_usrp::get_rx_rate)
        .def("get_rx_stream", &uhd::usrp::multi_usrp::get_rx_stream)
        .def("set_rx_freq", &uhd::usrp::multi_usrp::set_rx_freq)
        .def("set_rx_gain", &set_rx_gain_conv)
        .def("set_rx_rate", &uhd::usrp::multi_usrp::set_rx_rate)
        .def("get_tx_freq", &uhd::usrp::multi_usrp::get_tx_freq)
        .def("get_tx_num_channels", &uhd::usrp::multi_usrp::get_tx_num_channels)
        .def("get_tx_rate", &uhd::usrp::multi_usrp::get_tx_rate)
        .def("get_tx_stream", &uhd::usrp::multi_usrp::get_tx_stream)
        .def("set_tx_freq", &uhd::usrp::multi_usrp::set_tx_freq)
        .def("set_tx_gain", &set_tx_gain_conv)
        .def("set_tx_rate", &uhd::usrp::multi_usrp::set_tx_rate)
        ;

    bp::class_<uhd::rx_streamer, boost::shared_ptr<uhd::rx_streamer>, boost::noncopyable>("rx_streamer", bp::no_init)
        .def("recv", &Pyrecv)
        .def("get_num_channels", &uhd::rx_streamer::get_num_channels)
        .def("get_max_num_samps", &uhd::rx_streamer::get_max_num_samps)
        .def("issue_stream_cmd", &uhd::rx_streamer::issue_stream_cmd)
        ;

    bp::class_<uhd::tx_streamer, boost::shared_ptr<uhd::tx_streamer>, boost::noncopyable>("tx_streamer", bp::no_init)
        .def("send", &Pysend)
        .def("get_num_channels", &uhd::tx_streamer::get_num_channels)
        .def("get_max_num_samps", &uhd::tx_streamer::get_max_num_samps)
        ;

}

void export_types(void)
{

    bp::object types_module(bp::handle<>(bp::borrowed(PyImport_AddModule("pyuhd.types"))));
    // make "from mypackage import IO" work
    bp::scope().attr("types") = types_module;
    // set the current scope to the new sub-module
    bp::scope io_scope = types_module;

    bp::implicitly_convertible<std::string, uhd::device_addr_t>();

    bp::enum_<uhd::stream_cmd_t::stream_mode_t>("stream_mode")
        .value("start_cont", uhd::stream_cmd_t::STREAM_MODE_START_CONTINUOUS)
        .value("stop_cont", uhd::stream_cmd_t::STREAM_MODE_STOP_CONTINUOUS)
        .value("num_done", uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_DONE)
        .value("num_more", uhd::stream_cmd_t::STREAM_MODE_NUM_SAMPS_AND_MORE)
        ;

    bp::enum_<uhd::rx_metadata_t::error_code_t>("rx_metadata_error_code")
        .value("none", uhd::rx_metadata_t::error_code_t::ERROR_CODE_NONE)
        .value("timeout", uhd::rx_metadata_t::error_code_t::ERROR_CODE_TIMEOUT)
        .value("late", uhd::rx_metadata_t::error_code_t::ERROR_CODE_LATE_COMMAND)
        .value("broken_chain", uhd::rx_metadata_t::error_code_t::ERROR_CODE_BROKEN_CHAIN)
        .value("overflow", uhd::rx_metadata_t::error_code_t::ERROR_CODE_OVERFLOW)
        .value("alignment", uhd::rx_metadata_t::error_code_t::ERROR_CODE_ALIGNMENT)
        .value("bad_packet", uhd::rx_metadata_t::error_code_t::ERROR_CODE_BAD_PACKET)
        ;

    bp::enum_<uhd::tune_request_t::policy_t>("tune_request_policy")
        .value("none", uhd::tune_request_t::POLICY_NONE)
        .value("auto", uhd::tune_request_t::POLICY_AUTO)
        .value("manual", uhd::tune_request_t::POLICY_MANUAL)
        ;


    bp::class_<uhd::stream_args_t>("stream_args", bp::init<std::string,std::string>())
        .def_readwrite("cpu_format", &uhd::stream_args_t::cpu_format)
        .def_readwrite("otw_format", &uhd::stream_args_t::otw_format)
        .def_readwrite("args", &uhd::stream_args_t::args)
        .def_readwrite("channels", &uhd::stream_args_t::channels)
        ;

    bp::class_<uhd::stream_cmd_t>("stream_cmd", bp::init<uhd::stream_cmd_t::stream_mode_t>())
        .def_readwrite("num_samps", &uhd::stream_cmd_t::num_samps)
        .def_readwrite("time_spec", &uhd::stream_cmd_t::time_spec)
        .def_readwrite("stream_now", &uhd::stream_cmd_t::stream_now)
        ;

    bp::class_<uhd::rx_metadata_t>("rx_metadata", bp::init<>())
        .def("reset", &uhd::rx_metadata_t::reset)
        .def("to_pp_string", &uhd::rx_metadata_t::to_pp_string)
        .def("strerror", &uhd::rx_metadata_t::strerror)
        .def("__str__", &uhd::rx_metadata_t::to_pp_string, bp::args("compact")=false)
        .def_readonly("has_time_spec", &uhd::rx_metadata_t::has_time_spec)
        .def_readonly("time_spec", &uhd::rx_metadata_t::time_spec)
        .def_readonly("more_fragments", &uhd::rx_metadata_t::more_fragments)
        .def_readonly("start_of_burst", &uhd::rx_metadata_t::start_of_burst)
        .def_readonly("end_of_burst", &uhd::rx_metadata_t::end_of_burst)
        .def_readonly("error_code", &uhd::rx_metadata_t::error_code)
        .def_readonly("out_of_sequence", &uhd::rx_metadata_t::out_of_sequence)
        ;

    bp::class_<uhd::tx_metadata_t>("tx_metadata", bp::init<>())
        .def_readwrite("has_time_spec", &uhd::tx_metadata_t::has_time_spec)
        .def_readwrite("time_spec", &uhd::tx_metadata_t::time_spec)
        .def_readwrite("start_of_burst", &uhd::tx_metadata_t::start_of_burst)
        .def_readwrite("end_of_burst", &uhd::tx_metadata_t::end_of_burst)
        ;


    bp::class_<uhd::tune_request_t>("tune_request", bp::init<double>())
        .def_readwrite("target_freq", &uhd::tune_request_t::target_freq)
        .def_readwrite("rf_freq_policy", &uhd::tune_request_t::rf_freq_policy)
        .def_readwrite("dsp_freq_policy", &uhd::tune_request_t::dsp_freq_policy)
        .def_readwrite("rf_freq", &uhd::tune_request_t::rf_freq)
        .def_readwrite("dsp_freq", &uhd::tune_request_t::dsp_freq)
        .def_readwrite("args", &uhd::tune_request_t::args)
        ;

    bp::class_<uhd::tune_result_t>("tune_result", bp::init<>())
        ;

}


BOOST_PYTHON_MODULE(libpyuhd)
{
    bp::object package = bp::scope();
    package.attr("__path__") = "pyuhd";
    iterable_converter()
        .from_python<std::vector<double> >()
        .from_python<std::vector<int> >()
        .from_python<std::vector<size_t> >()
        ;
    export_multi_usrp();
    export_types();
    import_array();
}

