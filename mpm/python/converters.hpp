//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/python.hpp>
#include <boost/python/stl_iterator.hpp>
#include <vector>
#include <string>

namespace bp = boost::python;

template<typename MapType>
struct map_to_python_dict
{
    static PyObject* convert(MapType const& input_map)
    {
        bp::dict py_dict;
        for (const auto& element: input_map){
            py_dict[element.first] = element.second;
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

void export_converter(){
    // LIBMPM_BOOST_PREAMBLE("helper")
    bp::to_python_converter<std::vector< std::string >, iterable_to_python_list<std::vector< std::string > >, false>();
}
