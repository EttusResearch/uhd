//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
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
