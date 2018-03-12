//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/noncopyable.hpp>
#include <memory>

namespace mpm { namespace types {

    /*! Interface to a register reader/writer interface
     */
    class regs_iface : public boost::noncopyable
    {
    public:
        using sptr = std::shared_ptr<regs_iface>;

        /*! Return an 8-bit value from a given address
         */
        virtual uint8_t peek8(
            const uint32_t addr
        ) = 0;

        /*! Write an 8-bit value to a given address
         */
        virtual void poke8(
            const uint32_t addr,
            const uint8_t data
        ) = 0;

        /*! Return a 16-bit value from a given address
         */
        virtual uint16_t peek16(
            const uint32_t addr
        ) = 0;

        /*! Write a 16-bit value to a given address
         */
        virtual void poke16(
            const uint32_t addr,
            const uint16_t data
        ) = 0;
    };

}}; /* namespace mpm::regs */

