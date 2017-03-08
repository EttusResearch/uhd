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

