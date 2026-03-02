//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/iq_dc_cal.hpp>
#include <uhd/cal/iq_dc_cal_generated.h>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/utils/interpolation.hpp>
#include <map>
#include <string>

using namespace uhd::usrp::cal;
using namespace uhd::math;

constexpr int VERSION_MAJOR = 1;
constexpr int VERSION_MINOR = 0;

/***********************************************************************
 * Helper routines
 **********************************************************************/
class iq_dc_cal_impl : public iq_dc_cal
{
public:
    iq_dc_cal_impl(const std::string& name = "",
        const std::string& serial          = "",
        const uint64_t timestamp           = 0)
        : _name(name)
        , _serial(serial)
        , _timestamp(timestamp)
        , _interp(interp_mode::LINEAR)
    {
    }

    std::string get_name() const override
    {
        return _name;
    }

    std::string get_serial() const override
    {
        return _serial;
    }

    uint64_t get_timestamp() const override
    {
        return _timestamp;
    }

    void set_interp_mode(const interp_mode interp) override
    {
        UHD_ASSERT_THROW(
            interp == interp_mode::LINEAR || interp == interp_mode::NEAREST_NEIGHBOR);
        _interp = interp;
    }

    uhd::iq_dc_cal_coeffs_t get_cal_coeff(const double freq) const override
    {
        auto coeffs = _get_interpolated_entry(freq);
        UHD_ASSERT_THROW(!_coeffs.empty());

        std::vector<std::complex<double>> cplx_coeffs;
        cplx_coeffs.reserve(coeffs.icross_coeffs.size());
        std::transform(coeffs.icross_coeffs.begin(),
            coeffs.icross_coeffs.end(),
            coeffs.qinline_coeffs.begin(),
            std::back_inserter(cplx_coeffs),
            [](double i, double q) { return std::complex<double>(i, q); });

        uhd::iq_dc_cal_coeffs_t result = {
            coeffs.scaling_factor, cplx_coeffs, coeffs.group_delay, coeffs.dc_offset};
        return result;
    }

    double get_group_delay(const double freq) override
    {
        auto coeffs = _get_interpolated_entry(freq);
        return coeffs.group_delay;
    }

    void set_cal_coeff(const double freq,
        const double scaling_factor,
        const std::vector<double> icross,
        const std::vector<double> qinline,
        const double delay          = 0,
        const double dc_offset_real = 0,
        const double dc_offset_imag = 0) override
    {
        _coeffs[freq] = {
            scaling_factor, icross, qinline, delay, {dc_offset_real, dc_offset_imag}};
    }

    void clear() override
    {
        _coeffs.clear();
    }

    /**************************************************************************
     * Container API (Serialization/Deserialization)
     *************************************************************************/
    std::vector<uint8_t> serialize() override
    {
        constexpr size_t RESERVE_HDR_BYTES = 20;
        const size_t initial_size_bytes =
            sizeof(IQDCCalCoeff) * _coeffs.size() + RESERVE_HDR_BYTES;
        flatbuffers::FlatBufferBuilder builder(initial_size_bytes);
        std::vector<flatbuffers::Offset<IQDCCalCoeff>> fb_coeffs;
        fb_coeffs.reserve(_coeffs.size());
        std::transform(_coeffs.cbegin(),
            _coeffs.cend(),
            std::back_inserter(fb_coeffs),
            [&](const coeffs_type::value_type& coeff) {
                const double freq     = coeff.first;
                const auto cal_values = coeff.second;
                return CreateIQDCCalCoeff(builder,
                    freq,
                    cal_values.scaling_factor,
                    builder.CreateVector(cal_values.icross_coeffs),
                    builder.CreateVector(cal_values.qinline_coeffs),
                    cal_values.group_delay,
                    cal_values.dc_offset.real(),
                    cal_values.dc_offset.imag());
            });
        // Now load it all into the FlatBuffer
        const auto metadata = CreateMetadataDirect(builder,
            _name.c_str(),
            _serial.c_str(),
            _timestamp,
            VERSION_MAJOR,
            VERSION_MINOR);
        auto cal_table      = CreateIQDCCalCoeffsDirect(builder, metadata, &fb_coeffs);
        FinishIQDCCalCoeffsBuffer(builder, cal_table);
        const size_t table_size = builder.GetSize();
        const uint8_t* table    = builder.GetBufferPointer();
        return std::vector<uint8_t>(table, table + table_size);
    }

    // This will amend the existing table. If that's not desired, then it is
    // necessary to call clear() ahead of time.
    void deserialize(const std::vector<uint8_t>& data) override
    {
        auto verifier = flatbuffers::Verifier(data.data(), data.size());
        if (!VerifyIQDCCalCoeffsBuffer(verifier)) {
            throw uhd::runtime_error("iq_dc_cal: Invalid data provided!");
        }
        auto cal_table = GetIQDCCalCoeffs(static_cast<const void*>(data.data()));
        UHD_ASSERT_THROW(cal_table->metadata()->version_major() == VERSION_MAJOR);
        _name       = std::string(cal_table->metadata()->name()->c_str());
        _serial     = std::string(cal_table->metadata()->serial()->c_str());
        _timestamp  = cal_table->metadata()->timestamp();
        auto coeffs = cal_table->coeffs();
        for (auto it = coeffs->begin(); it != coeffs->end(); ++it) {
            std::vector<double> coeff_real(
                it->coeff_real()->begin(), it->coeff_real()->end());
            std::vector<double> coeff_imag(
                it->coeff_imag()->begin(), it->coeff_imag()->end());
            _coeffs[it->freq()] = {it->scaling_factor(),
                coeff_real,
                coeff_imag,
                it->delay(),
                {it->dc_offset_real(), it->dc_offset_imag()}};
        }
    }


private:
    std::string _name;
    std::string _serial;
    uint64_t _timestamp;
    struct iq_dc_cal_entry
    {
        double scaling_factor;
        std::vector<double> icross_coeffs;
        std::vector<double> qinline_coeffs;
        double group_delay;
        std::complex<double> dc_offset;
    };
    using coeffs_type = std::map<double, iq_dc_cal_entry>;
    coeffs_type _coeffs;

    interp_mode _interp;

    std::complex<double> _interpolate_cplx(const double freq,
        const double low_freq,
        const double hi_freq,
        const std::complex<double> low_offset,
        const std::complex<double> hi_offset) const
    {
        return {
            linear_interp(freq, low_freq, low_offset.real(), hi_freq, hi_offset.real()),
            linear_interp(freq, low_freq, low_offset.imag(), hi_freq, hi_offset.imag())};
    }

    iq_dc_cal_entry _get_interpolated_entry(const double freq) const
    {
        UHD_ASSERT_THROW(!_coeffs.empty());
        auto next_coeff = _coeffs.lower_bound(freq);
        if (next_coeff == _coeffs.end()) {
            UHD_LOG_DEBUG("IQ_DC_CAL",
                "Frequency " << freq
                             << " Hz is above range of available calibration data. "
                                "Returning largest key ("
                             << _coeffs.rbegin()->first << " Hz).");
            return _coeffs.rbegin()->second;
        }
        if (next_coeff == _coeffs.begin()) {
            UHD_LOG_DEBUG("IQ_DC_CAL",
                "Frequency " << freq
                             << " Hz is below range of available calibration data. "
                                "Returning smallest key ("
                             << _coeffs.begin()->first << " Hz).");
            return _coeffs.begin()->second;
        }

        const auto hi_freq  = next_coeff->first;
        const auto hi_coeff = next_coeff->second;
        next_coeff--;
        const auto low_coeff = next_coeff->second;
        const auto low_freq  = next_coeff->first;

        if (_interp == interp_mode::NEAREST_NEIGHBOR) {
            UHD_LOG_TRACE("IQ_DC_CAL",
                "Nearest neighbor: Choosing data from "
                    << std::fixed << std::setprecision(1)
                    << ((hi_freq - freq) < (freq - low_freq) ? hi_freq : low_freq)
                    << " Hz for frequency: " << freq << " Hz");
            auto retval = (hi_freq - freq) < (freq - low_freq) ? hi_coeff : low_coeff;
            // For DC offset it always makes sense to take the interpolated value:
            retval.dc_offset = _interpolate_cplx(
                freq, low_freq, hi_freq, low_coeff.dc_offset, hi_coeff.dc_offset);
            return retval;
        }
        UHD_LOG_TRACE("IQ_DC_CAL",
            "Interpolating between " << std::fixed << std::setprecision(1) << low_freq
                                     << " Hz and " << hi_freq
                                     << " Hz for frequency: " << freq << " Hz");
        using uhd::math::linear_interp;
        auto interpolate_coeffs = [&](const std::vector<double>& v1,
                                      const std::vector<double>& v2) {
            std::vector<double> coeffs(v1.size());
            std::transform(v1.begin(),
                v1.end(),
                v2.begin(),
                coeffs.begin(),
                [freq, low_freq, hi_freq](const double i, const double j) {
                    return linear_interp(freq, low_freq, i, hi_freq, j);
                });
            return coeffs;
        };
        auto scaling_factor = linear_interp(
            freq, low_freq, low_coeff.scaling_factor, hi_freq, hi_coeff.scaling_factor);
        auto icross_coeffs =
            interpolate_coeffs(low_coeff.icross_coeffs, hi_coeff.icross_coeffs);
        auto qinline_coeffs =
            interpolate_coeffs(low_coeff.qinline_coeffs, hi_coeff.qinline_coeffs);
        auto group_delay = linear_interp(
            freq, low_freq, low_coeff.group_delay, hi_freq, hi_coeff.group_delay);
        std::complex<double> dc_offset = _interpolate_cplx(
            freq, low_freq, hi_freq, low_coeff.dc_offset, hi_coeff.dc_offset);
        return {scaling_factor, icross_coeffs, qinline_coeffs, group_delay, dc_offset};
    }
};

iq_dc_cal::sptr iq_dc_cal::make()
{
    return std::make_shared<iq_dc_cal_impl>();
}

iq_dc_cal::sptr iq_dc_cal::make(
    const std::string& name, const std::string& serial, const uint64_t timestamp)
{
    return std::make_shared<iq_dc_cal_impl>(name, serial, timestamp);
}
