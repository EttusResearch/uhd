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

