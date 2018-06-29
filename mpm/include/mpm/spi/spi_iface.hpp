//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <mpm/types/regs_iface.hpp>
#include <boost/noncopyable.hpp>
#include <memory>
#include <string>

namespace mpm { namespace spi {

    /*! Implementation of a uhd::spi_iface that uses Linux' spidev underneath.
     */
    class spi_iface : public boost::noncopyable
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

        /*! Convenience function: SPI xfer is 24 bits write, 16 bits read.
         *
         * \param data The write data for this xfer
         *
         * \return 16 bits worth of the return xfer
         */
        virtual uint32_t transfer24_16(
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

