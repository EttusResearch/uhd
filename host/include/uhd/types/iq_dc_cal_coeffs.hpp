//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <complex>
#include <sstream>
#include <vector>

namespace uhd {

struct iq_dc_cal_coeffs_t
{
    double scaling_coeff;
    std::vector<std::complex<double>> coeffs;
    double group_delay;
    std::complex<double> dc_offset;

    friend std::ostream& operator<<(std::ostream& os, const iq_dc_cal_coeffs_t& data);

    friend bool operator==(const iq_dc_cal_coeffs_t& lhs, const iq_dc_cal_coeffs_t& rhs);
};

inline std::ostream& operator<<(std::ostream& os, const iq_dc_cal_coeffs_t& data)
{
    os << "iq_dc_cal_coeffs_t(scaling_coeff=" << data.scaling_coeff << ", coeffs=[";
    for (size_t i = 0; i < data.coeffs.size(); i++) {
        os << data.coeffs[i];
        if (i != data.coeffs.size() - 1)
            os << ", ";
    }
    os << "], group_delay=" << data.group_delay << ", dc_offset=" << data.dc_offset
       << ")";
    return os;
}

inline bool operator==(const iq_dc_cal_coeffs_t& lhs, const iq_dc_cal_coeffs_t& rhs)
{
    return lhs.coeffs == rhs.coeffs && lhs.group_delay == rhs.group_delay
           && lhs.scaling_coeff == rhs.scaling_coeff && lhs.dc_offset == rhs.dc_offset;
}

} // namespace uhd
