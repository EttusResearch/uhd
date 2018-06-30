//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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

std::vector<int16_t> ad937x_fir::get_fir(int8_t &gain) const
{
    gain = _fir.gain_dB;
    return _fir_coefficients;
}
