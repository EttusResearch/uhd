//
// Copyright 2017 Ettus Research (National Instruments)
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
