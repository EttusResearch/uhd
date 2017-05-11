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

#include <memory>
#include <string>

namespace mpm { namespace spi {

    /*! Implementation of a uhd::spi_iface that uses Linux' spidev underneath.
     */
    class spi_iface
    {
    public:
        using sptr = std::shared_ptr<spi_iface>;

        /*! Convenience function: SPI xfer is 24 bits write, 8 bits read.
         *
         * \param data The write data for this xfer
         *
         * \return 8 bits worth of the return xfer
         */
        virtual uint32_t transfer24_8(
            const uint32_t data
        ) = 0;

        /*!
         * \param device The path to the spidev used (e.g. "/dev/spidev0.0")
         * \param speed_hz Transaction speed in Hz
         */
        static sptr make_spidev(
            const std::string &device,
            const int speed_hz,
            const int spi_mode=3
        );
    };

}}; /* namespace mpm */

