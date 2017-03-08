#include "n310_periphs.hpp"
#include "../n310/periph_manager.hpp"
#include <boost/python.hpp>
#include <memory>

namespace bp = boost::python;

void export_n3xx(){
    //Register submodule types
    bp::object n3xx_module(bp::handle<>(bp::borrowed(PyImport_AddModule("libpyusrp_periphs.n3xx"))));
    bp::scope().attr("n3xx") = n3xx_module;
    bp::scope io_scope = n3xx_module;

    bp::class_<mpm::n3xx::n3xx_dboard_periph_manager, boost::noncopyable>("dboard_periph_manager", bp::no_init)
        .def("get_clock_gen()", &mpm::n3xx::n3xx_dboard_periph_manager::get_clock_gen)
        ;
    bp::class_<mpm::n3xx::periph_manager, boost::noncopyable, std::shared_ptr<mpm::n3xx::periph_manager> >("periph_manager", bp::init<std::string>())
        .def("get_dboard_A", &mpm::n3xx::periph_manager::get_dboard_A)
        ;
}

