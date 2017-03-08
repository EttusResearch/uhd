#include "lib_periphs.hpp"
#include "lmk04828.hpp"
#include <mpm/spi_iface.hpp>
#include <boost/python.hpp>

namespace bp = boost::python;

void export_lmk(){
    //Register submodule types
    bp::object lmk_module(bp::handle<>(bp::borrowed(PyImport_AddModule("libpyusrp_periphs.lmk"))));
    bp::scope().attr("lmk") = lmk_module;
    bp::scope io_scope = lmk_module;

    bp::class_<lmk04828_iface, boost::shared_ptr<lmk04828_iface>, boost::noncopyable >("lmk04828_iface", bp::no_init)
               .def("make", &lmk04828_iface::make)
               .def("verify_chip_id", &lmk04828_iface::verify_chip_id)
               .def("init", &lmk04828_iface::init)
               .def("send_sysref_pulse", &lmk04828_iface::send_sysref_pulse)
               ;
}

void export_spi(){
    //Register submodule types
    bp::object spi_module(bp::handle<>(bp::borrowed(PyImport_AddModule("libpyusrp_periphs.spi"))));
    bp::scope().attr("spi") = spi_module;
    bp::scope io_scope = spi_module;

    bp::class_<mpm::spi_iface, boost::noncopyable>("spi_iface", bp::no_init)
        .def("write_byte", &mpm::spi_iface::write_byte)
        .def("write_bytes", &mpm::spi_iface::write_bytes)
        .def("read_byte", &mpm::spi_iface::read_byte)
        .def("write_field", &mpm::spi_iface::write_field)
        .def("read_field", &mpm::spi_iface::read_field)
        .def("get_wire_mode", &mpm::spi_iface::get_wire_mode)
        .def("get_endianness", &mpm::spi_iface::get_endianness)
        .def("get_chip_select", &mpm::spi_iface::get_chip_select)
        ;

    bp::enum_<mpm::spi_iface::spi_endianness_t>("spi_endianness")
        .value("lsb_first", mpm::spi_iface::spi_endianness_t::LSB_FIRST)
        .value("msb_first", mpm::spi_iface::spi_endianness_t::MSB_FIRST)
        ;

    bp::enum_<mpm::spi_iface::spi_wire_mode_t>("spi_wire_mode")
        .value("three_wire_mode", mpm::spi_iface::spi_wire_mode_t::THREE_WIRE_MODE)
        .value("four_wire_mode", mpm::spi_iface::spi_wire_mode_t::FOUR_WIRE_MODE)
        ;
}

