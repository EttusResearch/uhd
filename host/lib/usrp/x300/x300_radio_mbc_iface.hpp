//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_X300_MBC_IFACE_HPP
#define INCLUDED_LIBUHD_X300_MBC_IFACE_HPP

#include <uhd/types/time_spec.hpp>
#include <cstddef>
#include <string>

namespace uhd { namespace usrp { namespace x300 {

class x300_radio_mbc_iface
{
public:
    virtual ~x300_radio_mbc_iface() {}

    //! Return the current output of the ADC via a register
    virtual uint32_t get_adc_rx_word() = 0;

    //! Set ADC test word (see x300_adc_ctrl::set_test_word())
    virtual void set_adc_test_word(
        const std::string& patterna, const std::string& patternb) = 0;

    //! Enable or disable the ADC checker
    virtual void set_adc_checker_enabled(const bool enb) = 0;

    //! Query ADC checker lock bit
    virtual bool get_adc_checker_locked(const bool I) = 0;

    //! Return current ADC error status
    virtual uint32_t get_adc_checker_error_code(const bool I) = 0;

    /*! Runs some ADC self tests
     *
     * - First, the ADC gets set to produce a constant value and we see if it
     *   reaches the FPGA
     * - Then, the ADC is put into ramp mode, and we see if we read the ramp
     *   with no errors
     *
     * \param ramp_time_ms The duration of the ramp test. Increasing the test
     *                     will increase the probability of a bit error.
     * \throws uhd::runtime_error if one or more bit errors occurred
     */
    virtual void self_test_adc(const uint32_t ramp_time_ms) = 0;

    //! Call sync() on the DAC object
    virtual void sync_dac() = 0;

    //! Set the FRAMEP/N sync pulse. If time is not zero, it will do so at the
    // given time.
    virtual void set_dac_sync(
        const bool enb, const uhd::time_spec_t& time = uhd::time_spec_t(0.0)) = 0;

    //! Call verify_sync() on the DAC object
    virtual void dac_verify_sync() = 0;
};

}}} // namespace uhd::usrp::x300

#endif /* INCLUDED_LIBUHD_X300_MBC_IFACE_HPP */
