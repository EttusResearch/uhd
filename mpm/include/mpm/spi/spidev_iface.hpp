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

#include "uhd/types/serial.hpp"
#include <boost/shared_ptr.hpp>

namespace mpm { namespace spi {

    /*! Implementation of a uhd::spi_iface that uses Linux' spidev underneath.
     */
    class spidev_iface : public uhd::spi_iface
    {
    public:
        typedef boost::shared_ptr<spidev_iface> sptr;
        virtual uint32_t read_spi(
            int which_slave,
            const uhd::spi_config_t &config,
            uint32_t data,
            size_t num_bits
            ) = 0;

        virtual void write_spi(
            int which_slave,
            const uhd::spi_config_t &config,
            uint32_t data,
            size_t num_bits
            ) = 0;

        virtual uint32_t transact_spi(
            int /* which_slave */,
            const uhd::spi_config_t & /* config */,
            uint32_t data,
            size_t num_bits,
            bool readback
            ) = 0;
        /*!
         * \param device The path to the spidev used.
         */
        static sptr make(const std::string &device);
    };

}}; /* namespace mpm */

//void export_spi(){
    //// Register submodule spi
    //bp::object spi_module(bp::handle<>(bp::borrowed(PyImport_AddModule("libpyusrp_periphs.spi"))));
    //bp::scope().attr("spi") = spi_module;
    //bp::scope io_scope = spi_module;

    //bp::class_<spi_lock, boost::noncopyable, boost::shared_ptr<spi_lock> >("spi_lock", bp::no_init)
        //.def("make", &spi_lock::make)
        //.def("get_spidev", &spi_lock::get_spidev)
        //;

    //bp::class_<mpm::spi_iface, boost::noncopyable>("spi_iface", bp::no_init)
        //.def("write_byte", &mpm::spi_iface::write_byte)
        //.def("write_bytes", &mpm::spi_iface::write_bytes)
        //.def("read_byte", &mpm::spi_iface::read_byte)
        //.def("write_field", &mpm::spi_iface::write_field)
        //.def("read_field", &mpm::spi_iface::read_field)
        //.def("get_wire_mode", &mpm::spi_iface::get_wire_mode)
        //.def("get_endianness", &mpm::spi_iface::get_endianness)
        //.def("get_chip_select", &mpm::spi_iface::get_chip_select)
        //;

    //bp::enum_<mpm::spi_iface::spi_endianness_t>("spi_endianness")
        //.value("lsb_first", mpm::spi_iface::spi_endianness_t::LSB_FIRST)
        //.value("msb_first", mpm::spi_iface::spi_endianness_t::MSB_FIRST)
        //;

    //bp::enum_<mpm::spi_iface::spi_wire_mode_t>("spi_wire_mode")
        //.value("three_wire_mode", mpm::spi_iface::spi_wire_mode_t::THREE_WIRE_MODE)
        //.value("four_wire_mode", mpm::spi_iface::spi_wire_mode_t::FOUR_WIRE_MODE)
        //;
//}

