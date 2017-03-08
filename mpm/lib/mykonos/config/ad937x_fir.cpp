#include "ad937x_fir.h"
#include <limits>

ad937x_fir::ad937x_fir(int8_t gain, const std::vector<int16_t>& coefficients)
{
   
    _set_gain(gain);
}

ad937x_fir::ad937x_fir(int8_t gain, std::vector<int16_t>&& coefficients)
{
    set_coefficients(std::move(coefficients));
    _set_gain(gain);
}

mykonosFir_t* ad937x_fir::getFir()
{
    return &(_fir);
}

void ad937x_fir::set_coefficients(std::vector<int16_t>&& coefficients)
{
    if (coefficients.size() < std::numeric_limits<decltype(mykonosFir_t().numFirCoefs)>::max() ||
        coefficients.size() > 0)
    {
        // TODO: exception
    }
    fir_coefficients = std::move(coefficients);
    _fir.numFirCoefs = static_cast<uint8_t>(fir_coefficients.size());
    _fir.coefs = &fir_coefficients[0];
}

void ad937x_fir::_set_gain(int8_t gain)
{
    _fir.gain_dB = gain;
}

