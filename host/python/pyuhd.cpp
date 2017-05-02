//
// Copyright 2017-2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/shared_ptr.hpp>
#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#include <boost/python/suite/indexing/vector_indexing_suite.hpp>

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <numpy/arrayobject.h>

namespace bp = boost::python;

#include "stream_python.hpp"

#include "types/types_python.hpp"
#include "types/serial_python.hpp"
#include "types/time_spec_python.hpp"
#include "types/metadata_python.hpp"
#include "types/sensors_python.hpp"
#include "types/filters_python.hpp"
#include "types/tune_python.hpp"

#include "usrp/fe_connection_python.hpp"
#include "usrp/dboard_iface_python.hpp"
#include "usrp/subdev_spec_python.hpp"
#include "usrp/multi_usrp_python.hpp"

// Converter for std::vector / std::list arguments from python iterables
struct iterable_converter
{
    template <typename Container>
    iterable_converter& from_python()
    {
        bp::converter::registry::push_back(
            &iterable_converter::convertible,
            &iterable_converter::construct<Container>,
            bp::type_id<Container>()
        );
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

        // Obtain a handle to the memory block that the converter has
        // allocated for the C++ type.
        typedef bp::converter::rvalue_from_python_storage<Container> storage_type;

        void* storage = reinterpret_cast<storage_type*>(data)->storage.bytes;
        typedef bp::stl_input_iterator<typename Container::value_type> iterator;

        // Allocate the C++ type into the converter's memory block, and assign
        // its handle to the converter's convertible variable.  The C++
        // container is populated by passing the begin and end iterators of
        // the python object to the container's constructor.
        new (storage) Container(
          iterator(bp::object(handle)), // begin
          iterator()                    // end
        );

        data->convertible = storage;
    }
};

template<typename Dtype1, typename Dtype2>
struct uhd_to_python_dict
{
    static PyObject* convert(uhd::dict<Dtype1, Dtype2> const& input_dict)
    {
        bp::dict py_dict;
        for (const auto& key: input_dict.keys()){
            py_dict[key] = input_dict[key];
        }
        return bp::incref(py_dict.ptr());
    }

};

template<typename Container>
struct iterable_to_python_list
{
    static PyObject* convert(Container const& input)
    {
        bp::list py_list;
        for (const auto& element: input){
            py_list.append(element);
        }
        return bp::incref(py_list.ptr());
    }
};

// We need this hack because import_array() returns NULL
// for newer Python versions.
// This function is also necessary because it ensures access to the C API
// and removes a warning.
#if PY_MAJOR_VERSION >= 3
void* init_numpy()
{
    import_array();
    return NULL;
}
#else
void init_numpy()
{
    import_array();
}
#endif

BOOST_PYTHON_MODULE(libpyuhd)
{
    // Initialize the numpy C API
    // (otherwise we will see segmentation faults)
    init_numpy();

    bp::object package = bp::scope();
    package.attr("__path__") = "libpyuhd";

    // Declare converters
    iterable_converter()
        .from_python<std::vector<double> >()
        .from_python<std::vector<int> >()
        .from_python<std::vector<size_t> >()
        ;

    bp::to_python_converter<
        uhd::dict<std::string, std::string>,
        uhd_to_python_dict<std::string, std::string>, false >();
    bp::to_python_converter<
        std::vector<std::string>,
        iterable_to_python_list<std::vector<std::string> >, false >();

    // Register types submodule
    {
        bp::object types_module(
            bp::handle<>(bp::borrowed(PyImport_AddModule("libpyuhd.types")))
        );
        bp::scope().attr("types") = types_module;
        bp::scope io_scope = types_module;

        bp::implicitly_convertible<std::string, uhd::device_addr_t>();

        export_types();
        export_time_spec();
        export_spi_config();
        export_metadata();
        export_sensors();
        export_tune();
    }

    // Register usrp submodule
    {
        bp::object usrp_module(
            bp::handle<>(bp::borrowed(PyImport_AddModule("libpyuhd.usrp")))
        );
        bp::scope().attr("usrp") = usrp_module;
        bp::scope io_scope = usrp_module;

        export_multi_usrp();
        export_subdev_spec();
        export_dboard_iface();
        export_fe_connection();
        export_stream();
    }

    // Register filters submodule
    {
        bp::object filters_module(
            bp::handle<>(bp::borrowed(PyImport_AddModule("libpyuhd.filters")))
        );
        bp::scope().attr("filters") = filters_module;
        bp::scope io_scope = filters_module;

        export_filters();
    }
}

