//
// Copyright 2011-2012,2014 Ettus Research LLC
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

#include "rx_frontend_core_3000.hpp"
#include <boost/math/special_functions/round.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/bind.hpp>
#include <uhd/types/dict.hpp>

using namespace uhd;

#define REG_RX_FE_MAG_CORRECTION    _base + 0  //18 bits
#define REG_RX_FE_PHASE_CORRECTION  _base + 4  //18 bits
#define REG_RX_FE_OFFSET_I          _base + 8  //18 bits
#define REG_RX_FE_OFFSET_Q          _base + 12 //18 bits
#define REG_RX_FE_SWAP_IQ           _base + 16

#define FLAG_DSP_RX_MUX_SWAP_IQ     (1 << 0)
#define FLAG_DSP_RX_MUX_REAL_MODE   (1 << 1)
#define FLAG_DSP_RX_MUX_INVERT_Q    (1 << 2)
#define FLAG_DSP_RX_MUX_INVERT_I    (1 << 3)

#define OFFSET_FIXED (1ul << 31)
#define OFFSET_SET   (1ul << 30)
#define FLAG_MASK (OFFSET_FIXED | OFFSET_SET)

static boost::uint32_t fs_to_bits(const double num, const size_t bits){
    return boost::int32_t(boost::math::round(num * (1 << (bits-1))));
}

rx_frontend_core_3000::~rx_frontend_core_3000(void){
    /* NOP */
}

const std::complex<double> rx_frontend_core_3000::DEFAULT_DC_OFFSET_VALUE = std::complex<double>(0.0, 0.0);
const bool rx_frontend_core_3000::DEFAULT_DC_OFFSET_ENABLE = true;
const std::complex<double> rx_frontend_core_3000::DEFAULT_IQ_BALANCE_VALUE = std::complex<double>(0.0, 0.0);

class rx_frontend_core_3000_impl : public rx_frontend_core_3000{
public:
    rx_frontend_core_3000_impl(wb_iface::sptr iface, const size_t base):
        _i_dc_off(0), _q_dc_off(0), _iface(iface), _base(base)
    {
        //NOP
    }

    void set_mux(const std::string &mode, const bool inv_i = false, const bool inv_q = false) {
        static const uhd::dict<std::string, boost::uint32_t> mode_to_mux = boost::assign::map_list_of
            ("IQ", 0)
            ("QI", FLAG_DSP_RX_MUX_SWAP_IQ)
            ("I", FLAG_DSP_RX_MUX_REAL_MODE)
            ("Q", FLAG_DSP_RX_MUX_SWAP_IQ | FLAG_DSP_RX_MUX_REAL_MODE)
        ;
        _iface->poke32(REG_RX_FE_SWAP_IQ, mode_to_mux[mode]
            | (inv_i ? FLAG_DSP_RX_MUX_INVERT_I : 0)
            | (inv_q ? FLAG_DSP_RX_MUX_INVERT_Q : 0));
    }

    void set_dc_offset_auto(const bool enb) {
        _set_dc_offset(enb ? 0 : OFFSET_FIXED);
    }

    std::complex<double> set_dc_offset(const std::complex<double> &off) {
        static const double scaler = double(1ul << 29);
        _i_dc_off = boost::math::iround(off.real()*scaler);
        _q_dc_off = boost::math::iround(off.imag()*scaler);

        _set_dc_offset(OFFSET_SET | OFFSET_FIXED);

        return std::complex<double>(_i_dc_off/scaler, _q_dc_off/scaler);
    }

    void _set_dc_offset(const boost::uint32_t flags) {
        _iface->poke32(REG_RX_FE_OFFSET_I, flags | (_i_dc_off & ~FLAG_MASK));
        _iface->poke32(REG_RX_FE_OFFSET_Q, flags | (_q_dc_off & ~FLAG_MASK));
    }

    void set_iq_balance(const std::complex<double> &cor){
        _iface->poke32(REG_RX_FE_MAG_CORRECTION, fs_to_bits(cor.real(), 18));
        _iface->poke32(REG_RX_FE_PHASE_CORRECTION, fs_to_bits(cor.imag(), 18));
    }

    void populate_subtree(uhd::property_tree::sptr subtree)
    {
        subtree->create<std::complex<double> >("dc_offset/value")
            .set(DEFAULT_DC_OFFSET_VALUE)
            .set_coercer(boost::bind(&rx_frontend_core_3000::set_dc_offset, this, _1))
        ;
        subtree->create<bool>("dc_offset/enable")
            .set(DEFAULT_DC_OFFSET_ENABLE)
            .add_coerced_subscriber(boost::bind(&rx_frontend_core_3000::set_dc_offset_auto, this, _1))
        ;
        subtree->create<std::complex<double> >("iq_balance/value")
            .set(DEFAULT_IQ_BALANCE_VALUE)
            .add_coerced_subscriber(boost::bind(&rx_frontend_core_3000::set_iq_balance, this, _1))
        ;
    }

private:
    boost::int32_t _i_dc_off, _q_dc_off;
    wb_iface::sptr _iface;
    const size_t _base;
};

rx_frontend_core_3000::sptr rx_frontend_core_3000::make(wb_iface::sptr iface, const size_t base){
    return sptr(new rx_frontend_core_3000_impl(iface, base));
}
