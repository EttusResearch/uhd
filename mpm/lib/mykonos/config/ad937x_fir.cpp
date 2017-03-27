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

#include "ad937x_fir.hpp"

ad937x_fir::ad937x_fir() :
    ad937x_fir(0, { 1, 0 })
{

}

ad937x_fir::ad937x_fir(int8_t gain, const std::vector<int16_t>& coefficients) :
    // These two constructors will be run in the order they are declared in the class definition
    // see C++ standard 12.6.2 section 13.3
    _fir_coefficients(coefficients),
    _fir({gain,
        static_cast<uint8_t>(_fir_coefficients.size()),
        _fir_coefficients.data()})
{

}

// ad937x_fir.fir should not be accessed during this operation
void ad937x_fir::set_fir(int8_t gain, const std::vector<int16_t>& coefficients)
{
    _fir.gain_dB = gain;

    _fir_coefficients = coefficients;
    _fir.coefs = _fir_coefficients.data();
    _fir.numFirCoefs = static_cast<uint8_t>(_fir_coefficients.size());
}

std::vector<int16_t> ad937x_fir::get_fir(int8_t &gain)
{
    gain = _fir.gain_dB;
    return _fir_coefficients;
}
