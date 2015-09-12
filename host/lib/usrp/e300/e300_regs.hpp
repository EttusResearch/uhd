//
// Copyright 2012-2014 Ettus Research LLC
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

#ifndef INCLUDED_E300_REGS_HPP
#define INCLUDED_E300_REGS_HPP

#include <stdint.h>
#include <uhd/config.hpp>

static const uint32_t VCRX_V2     = 15;
static const uint32_t VCRX_V1     = 14;
static const uint32_t VCTXRX_V2   = 13;
static const uint32_t VCTXRX_V1   = 12;
static const uint32_t TX_ENABLEB  = 11;
static const uint32_t TX_ENABLEA  = 10;
static const uint32_t RXC_BANDSEL = 8;
static const uint32_t RXB_BANDSEL = 6;
static const uint32_t RX_BANDSEL  = 3;
static const uint32_t TX_BANDSEL  = 0;

#endif /* INCLUDED_E300_REGS_HPP */
