//
// Copyright 2011 Ettus Research LLC
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

#include "tx_frontend_core_200.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/exception.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/math/special_functions/round.hpp>

#define REG_TX_FE_DC_OFFSET_I         _base + 0 //24 bits
#define REG_TX_FE_DC_OFFSET_Q         _base + 1 //24 bits
#define REG_TX_FE_MAG_CORRECTION      _base + 2 //18 bits
#define REG_TX_FE_PHASE_CORRECTION    _base + 3 //18 bits
#define REG_TX_FE_MUX                 _base + 4 //8 bits (std output = 0x10, reversed = 0x01)

static boost::uint32_t fs_to_bits(const double num, const size_t bits){
    return boost::int32_t(boost::math::round(num * (1 << (bits-1))));
}


class tx_frontend_core_200_impl : public tx_frontend_core_200{
public:
    tx_frontend_core_200_impl(wb_iface::sptr iface, const size_t base):
        _iface(iface), _base(base)
    {
        //NOP
    }

    void set_mux(const std::string &mode){
        static const uhd::dict<std::string, boost::uint32_t> mode_to_mux = boost::assign::map_list_of
            ("IQ", (0x1 << 4) | (0x0 << 0)) //DAC0Q=DUC0Q, DAC0I=DUC0I
            ("QI", (0x0 << 4) | (0x1 << 0)) //DAC0Q=DUC0I, DAC0I=DUC0Q
            ("I",  (0xf << 4) | (0x0 << 0)) //DAC0Q=ZERO,  DAC0I=DUC0I
            ("Q",  (0x0 << 4) | (0xf << 0)) //DAC0Q=DUC0I, DAC0I=ZERO
        ;
        _iface->poke32(REG_TX_FE_MUX, mode_to_mux[mode]);
    }

    void set_dc_offset(const std::complex<double> &off){
        _iface->poke32(REG_TX_FE_DC_OFFSET_I, fs_to_bits(off.real(), 24));
        _iface->poke32(REG_TX_FE_DC_OFFSET_Q, fs_to_bits(off.imag(), 24));
    }

    void set_correction(const std::complex<double> &cor){
        _iface->poke32(REG_TX_FE_MAG_CORRECTION, fs_to_bits(std::abs(cor), 18));
        _iface->poke32(REG_TX_FE_PHASE_CORRECTION, fs_to_bits(std::atan2(cor.real(), cor.imag()), 18));
    }

private:
    wb_iface::sptr _iface;
    const size_t _base;
};

tx_frontend_core_200::sptr tx_frontend_core_200::make(wb_iface::sptr iface, const size_t base){
    return sptr(new tx_frontend_core_200_impl(iface, base));
}
