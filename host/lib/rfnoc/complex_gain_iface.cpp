//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/features/complex_gain_iface.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhdlib/usrp/cores/complex_gain_3000.hpp>
#include <complex>

using uhd::cores::complex_gain_3000;

namespace uhd { namespace features {

// An adapter class to expose the complex_gain_iface feature without having to
// expose the complex_gain_3000 core driver.
template <typename gain_iface_type>
class complex_gain_iface_impl : public gain_iface_type
{
public:
    complex_gain_iface_impl(complex_gain_3000::uptr cg) : _cg(std::move(cg)) {}

    void set_gain_coeff(const std::complex<double> gain_coeff,
        const size_t chan,
        const std::optional<uhd::time_spec_t> time) override
    {
        _cg->set_gain_coeff(gain_coeff, chan, time);
    }

    std::complex<double> get_gain_coeff(const size_t chan) override
    {
        return _cg->get_gain_coeff(chan);
    }

private:
    complex_gain_3000::uptr _cg;
};


// Factory functions
tx_complex_gain_iface::sptr make_tx_complex_gain_iface(
    uhd::rfnoc::multichan_register_iface& regs,
    const size_t base,
    const double tick_rate,
    const size_t nipc)
{
    return std::make_shared<complex_gain_iface_impl<tx_complex_gain_iface>>(
        std::make_unique<complex_gain_3000>(
            [&regs, base](
                uint32_t addr, uint32_t data, size_t chan, const uhd::time_spec_t& time) {
                regs.poke32(addr + base, data, chan, time);
            },
            [&regs, base](
                uint32_t addr, size_t chan) { return regs.peek32(addr + base, chan); },
            tick_rate,
            uhd::direction_t::TX_DIRECTION,
            nipc));
}

rx_complex_gain_iface::sptr make_rx_complex_gain_iface(
    uhd::rfnoc::multichan_register_iface& regs,
    const size_t base,
    const double tick_rate,
    const size_t nipc)
{
    return std::make_shared<complex_gain_iface_impl<rx_complex_gain_iface>>(
        std::make_unique<complex_gain_3000>(
            [&regs, base](
                uint32_t addr, uint32_t data, size_t chan, const uhd::time_spec_t& time) {
                regs.poke32(addr + base, data, chan, time);
            },
            [&regs, base](
                uint32_t addr, size_t chan) { return regs.peek32(addr + base, chan); },
            tick_rate,
            uhd::direction_t::RX_DIRECTION,
            nipc));
}

}} // namespace uhd::features
