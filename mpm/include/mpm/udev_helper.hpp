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

#include <libudev.h>
#include <string>
#include <vector>

namespace mpm {
    /*!
     * The udev_helper class:
     *
     * talks to libudev and holds a udev context. Device enumeration is done
     * once during initialization.
     * On destruction the udev context is unreferenced again.
     */
    class udev_helper{
    public:
        udev_helper();
        ~udev_helper();
        /*!
         * Return the nvmem device associated with the parent address
         * \param address of the parent platform driver
         * \return a string containing the name of file of the device in /sys
         */
        std::string get_eeprom(const std::string &address);
        /*!
         * Find spidevices associated with the spi_master
         * \param address of the parent platform driver
         * \return a vector of string containing the device paths is /dev
         */
        std::vector<std::string> get_spidev_nodes(const std::string &spi_master);

    private:
        udev *_udev;
        udev_enumerate *_enumerate;
    };
}

#ifdef LIBMPM_PYTHON
void export_udev_helper(){
    LIBMPM_BOOST_PREAMBLE("udev")
    bp::class_<mpm::udev_helper>("udev_helper", bp::init<>())
        .def("get_eeprom", &mpm::udev_helper::get_eeprom)
        .def("get_spidev_nodes", &mpm::udev_helper::get_spidev_nodes)
    ;
}
#endif

