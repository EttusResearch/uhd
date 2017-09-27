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

#include "../adi/t_mykonos.h"
#include <vector>

// wraps FIR allocation
// provides a fixed mykonosFir_t pointer for API consumption
class ad937x_fir
{
    std::vector<int16_t> _fir_coefficients;
    mykonosFir_t _fir;
public:
    mykonosFir_t* const fir = &_fir;
    ad937x_fir();
    ad937x_fir(int8_t gain, const std::vector<int16_t>& coefficients);

    void set_fir(int8_t gain, const std::vector<int16_t>& coefficients);
    std::vector<int16_t> get_fir(int8_t &gain) const;
};
