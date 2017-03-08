#pragma once

#include <vector>
#include "../adi/t_mykonos.h"

// wraps FIR allocation
class ad937x_fir
{
    // these should be const, but can't be because of C API
    // 
    mykonosFir_t _fir;
    std::vector<int16_t> fir_coefficients;

    void ad937x_fir::_set_gain(int8_t gain);
public:
    ad937x_fir(int8_t gain, const std::vector<int16_t>& coefficients);
    ad937x_fir(int8_t gain, std::vector<int16_t>&& coefficients);

    mykonosFir_t* getFir();
};


