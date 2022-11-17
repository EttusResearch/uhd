//
// Copyright 2011,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhdlib/usrp/cores/tx_frontend_core_200.hpp>
#include <boost/assign/list_of.hpp>
#include <cmath>
#include <functional>

using namespace uhd;

#define REG_TX_FE_DC_OFFSET_I      _base + 0 * _offset // 24 bits
#define REG_TX_FE_DC_OFFSET_Q      _base + 1 * _offset // 24 bits
#define REG_TX_FE_MAG_CORRECTION   _base + 2 * _offset // 18 bits
#define REG_TX_FE_PHASE_CORRECTION _base + 3 * _offset // 18 bits
#define REG_TX_FE_MUX              _base + 4 * _offset // 8 bits (std output = 0x10, reversed = 0x01)

const std::complex<double> tx_frontend_core_200::DEFAULT_DC_OFFSET_VALUE =
    std::complex<double>(0.0, 0.0);
const std::complex<double> tx_frontend_core_200::DEFAULT_IQ_BALANCE_VALUE =
    std::complex<double>(0.0, 0.0);

namespace {
static const double DC_OFFSET_MIN = -1.0;
static const double DC_OFFSET_MAX = 1.0;
} // namespace

static uint32_t fs_to_bits(const double num, const size_t bits)
{
    return int32_t(std::lround(num * (1 << (bits - 1))));
}

tx_frontend_core_200::~tx_frontend_core_200(void)
{
    /* NOP */
}

class tx_frontend_core_200_impl : public tx_frontend_core_200
{
public:
    tx_frontend_core_200_impl(
        wb_iface::sptr iface, const size_t base, const size_t offset)
        : _iface(iface), _base(base), _offset(offset)
    {
        // NOP
    }

    void set_mux(const std::string& mode) override
    {
        static const uhd::dict<std::string, uint32_t> mode_to_mux =
            boost::assign::map_list_of(
                "IQ", (0x1 << 4) | (0x0 << 0)) // DAC0Q=DUC0Q, DAC0I=DUC0I
            ("QI", (0x0 << 4) | (0x1 << 0)) // DAC0Q=DUC0I, DAC0I=DUC0Q
            ("I", (0xf << 4) | (0x0 << 0)) // DAC0Q=ZERO,  DAC0I=DUC0I
            ("Q", (0x0 << 4) | (0xf << 0)) // DAC0Q=DUC0I, DAC0I=ZERO
            ;
        _iface->poke32(REG_TX_FE_MUX, mode_to_mux[mode]);
    }

    std::complex<double> set_dc_offset(const std::complex<double>& off) override
    {
        static const double scaler = double(1ul << 23);
        const int32_t i_dc_off     = static_cast<int>(std::lround(off.real() * scaler));
        const int32_t q_dc_off     = static_cast<int>(std::lround(off.imag() * scaler));

        _iface->poke32(REG_TX_FE_DC_OFFSET_I, i_dc_off);
        _iface->poke32(REG_TX_FE_DC_OFFSET_Q, q_dc_off);

        return std::complex<double>(i_dc_off / scaler, q_dc_off / scaler);
    }

    void set_iq_balance(const std::complex<double>& cor) override
    {
        _iface->poke32(REG_TX_FE_MAG_CORRECTION, fs_to_bits(cor.real(), 18));
        _iface->poke32(REG_TX_FE_PHASE_CORRECTION, fs_to_bits(cor.imag(), 18));
    }

    void populate_subtree(uhd::property_tree::sptr subtree) override
    {
        subtree->create<uhd::meta_range_t>("dc_offset/range")
            .set(meta_range_t(DC_OFFSET_MIN, DC_OFFSET_MAX));
        subtree->create<std::complex<double>>("dc_offset/value")
            .set(DEFAULT_DC_OFFSET_VALUE)
            .set_coercer(std::bind(
                &tx_frontend_core_200::set_dc_offset, this, std::placeholders::_1));
        subtree->create<std::complex<double>>("iq_balance/value")
            .set(DEFAULT_IQ_BALANCE_VALUE)
            .add_coerced_subscriber(std::bind(
                &tx_frontend_core_200::set_iq_balance, this, std::placeholders::_1));
    }

private:
    wb_iface::sptr _iface;
    const size_t _base;
    const size_t _offset;
};

tx_frontend_core_200::sptr tx_frontend_core_200::make(
    wb_iface::sptr iface, const size_t base, const size_t offset)
{
    return sptr(new tx_frontend_core_200_impl(iface, base, offset));
}
