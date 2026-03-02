//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_lo_pd.hpp>

namespace uhd { namespace usrp { namespace hbx {

hbx_lo_pd::hbx_lo_pd(size_t start_address,
    hbx_cpld_ctrl::poke_fn_type&& poke_fn,
    hbx_cpld_ctrl::peek_fn_type&& peek_fn)
    : hbx_cpld_ctrl::spi_transactor(start_address, std::move(poke_fn), std::move(peek_fn))
{
    // Create sorted LUT: frequency (Hz) -> slope (V/dB)
    // Sort by frequency and convert GHz to Hz, mV/dB to V/dB
    std::vector<std::pair<double, double>> sorted_data;
    for (size_t i = 0; i < x_slope_freq.size(); ++i) {
        sorted_data.emplace_back(x_slope_freq[i], y_slope[i]);
    }
    std::sort(sorted_data.begin(), sorted_data.end());
    for (const auto& [freq, slope] : sorted_data) {
        slope_lut[freq * 1e9] = slope / 1000.0;
    }

    // Create sorted LUT: frequency (Hz) -> intercept (dBm)
    std::vector<std::pair<double, double>> sorted_log_data;
    for (size_t i = 0; i < x_log_intercept_freq.size(); ++i) {
        sorted_log_data.emplace_back(x_log_intercept_freq[i], y_log_intercept[i]);
    }
    std::sort(sorted_log_data.begin(), sorted_log_data.end());
    for (const auto& [freq, intercept] : sorted_log_data) {
        intercept_lut[freq * 1e9] = intercept;
    }
    UHD_LOG_TRACE(_log_id, "HBX LO Power Detector initialized...");
}

uint16_t hbx_lo_pd::read_data_raw()
{
    return this->spi_read(0x00);
}

double hbx_lo_pd::read_data_dbm(const double frequency)
{
    UHD_LOG_TRACE(_log_id, "Getting LO power detector for " << frequency / 1e6 << " MHz");
    const uint16_t raw_adc = read_data_raw();
    const auto calibration = get_calibration(frequency);
    return this->adc_to_dbm(raw_adc, calibration);
}

}}} // namespace uhd::usrp::hbx
