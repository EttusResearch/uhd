//
// Copyright 2011-2012 Ettus Research LLC
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

#include "rx_frontend_core_200.hpp"
#include <boost/math/special_functions/round.hpp>

#define REG_RX_FE_SWAP_IQ             _base + 0 //lower bit
#define REG_RX_FE_MAG_CORRECTION      _base + 4 //18 bits
#define REG_RX_FE_PHASE_CORRECTION    _base + 8 //18 bits
#define REG_RX_FE_OFFSET_I            _base + 12 //18 bits
#define REG_RX_FE_OFFSET_Q            _base + 16 //18 bits

#define OFFSET_FIXED (1ul << 31)
#define OFFSET_SET   (1ul << 30)
#define FLAG_MASK (OFFSET_FIXED | OFFSET_SET)

static boost::uint32_t fs_to_bits(const double num, const size_t bits){
    return boost::int32_t(boost::math::round(num * (1 << (bits-1))));
}


class rx_frontend_core_200_impl : public rx_frontend_core_200{
public:
    rx_frontend_core_200_impl(wb_iface::sptr iface, const size_t base):
        _iface(iface), _base(base)
    {
        //NOP
    }

    void set_mux(const bool swap){
        _iface->poke32(REG_RX_FE_SWAP_IQ, swap? 1 : 0);
    }

    void set_dc_offset_auto(const bool enb){
        this->set_dc_offset(enb? 0 : OFFSET_FIXED);
    }

    std::complex<double> set_dc_offset(const std::complex<double> &off){
        static const double scaler = double(1ul << 29);
        _i_dc_off = boost::math::iround(off.real()*scaler);
        _q_dc_off = boost::math::iround(off.imag()*scaler);

        this->set_dc_offset(OFFSET_SET | OFFSET_FIXED);

        return std::complex<double>(_i_dc_off/scaler, _q_dc_off/scaler);
    }

    void set_dc_offset(const boost::uint32_t flags){
        _iface->poke32(REG_RX_FE_OFFSET_I, flags | (_i_dc_off & ~FLAG_MASK));
        _iface->poke32(REG_RX_FE_OFFSET_Q, flags | (_q_dc_off & ~FLAG_MASK));
    }

    void set_iq_balance(const std::complex<double> &cor){
        _iface->poke32(REG_RX_FE_MAG_CORRECTION, fs_to_bits(cor.real(), 18));
        _iface->poke32(REG_RX_FE_PHASE_CORRECTION, fs_to_bits(cor.imag(), 18));
    }

private:
    boost::int32_t _i_dc_off, _q_dc_off;
    wb_iface::sptr _iface;
    const size_t _base;
};

rx_frontend_core_200::sptr rx_frontend_core_200::make(wb_iface::sptr iface, const size_t base){
    return sptr(new rx_frontend_core_200_impl(iface, base));
}
