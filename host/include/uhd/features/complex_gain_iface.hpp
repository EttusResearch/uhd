//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/config.hpp>
#include <uhd/features/discoverable_feature.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/time_spec.hpp>
#include <complex>
#include <optional>

namespace uhd { namespace features {

template <uhd::direction_t trx>
class UHD_API complex_gain_iface : public discoverable_feature
{
public:
    using sptr = std::shared_ptr<complex_gain_iface>;

    static discoverable_feature::feature_id_t get_feature_id()
    {
        if constexpr (trx == uhd::direction_t::TX_DIRECTION) {
            return discoverable_feature::TX_COMPLEX_GAIN;
        } else {
            return discoverable_feature::RX_COMPLEX_GAIN;
        }
    }

    std::string get_feature_name() const
    {
        if constexpr (trx == uhd::direction_t::TX_DIRECTION) {
            return "TX Complex Gain";
        } else {
            return "RX Complex Gain";
        }
    }

    virtual ~complex_gain_iface() = default;

    /*! Apply complex gain at a specified time
     *
     * The specified complex gain coefficient will be applied at a
     * given timestamp. The default gain coefficient is 1.0 + 0.0i (unity gain).
     * Alternatively, if the user sets the command time for the noc block,
     * the timestamp is the current command time. If the timestamp has already passed or
     * no time is specified, the gain will be applied as soon as possible.
     *
     * Note to check if the complex gain feature is enabled in the device bitfile.
     * When the coefficient is queued, it will be immediately acknowledged by the USRP.
     * However, if the queue is full, the acknowledgment will be delayed until it can
     * be queued.
     *
     * gain_coeff The complex gain coefficient to apply (1.0 is unity)
     * chan The channel index
     * time Optional timestamp to specify when to apply the gain
     */
    virtual void set_gain_coeff(const std::complex<double> gain_coeff,
        const size_t chan,
        const std::optional<uhd::time_spec_t> time = {}) = 0;

    /*! Returns the currently active complex gain coefficient
     */
    virtual std::complex<double> get_gain_coeff(const size_t chan) = 0;
};

using tx_complex_gain_iface = complex_gain_iface<uhd::direction_t::TX_DIRECTION>;
using rx_complex_gain_iface = complex_gain_iface<uhd::direction_t::RX_DIRECTION>;

}} // namespace uhd::features
