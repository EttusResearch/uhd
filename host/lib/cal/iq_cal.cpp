//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/iq_cal.hpp>
#include <uhd/cal/iq_cal_generated.h>
#include <uhd/exception.hpp>
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
class iq_cal_impl : public iq_cal
{
public:
    iq_cal_impl(const std::string& name = "",
        const std::string& serial       = "",
        const uint64_t timestamp        = 0)
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

    //! Return the name of this calibration table
    std::string get_serial() const override
    {
        return _serial;
    }

    //! Timestamp of acquisition time
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

    std::complex<double> get_cal_coeff(const double freq) const override
    {
        UHD_ASSERT_THROW(!_coeffs.empty());
        // Find the first coefficient in the map that maps to a larger frequency
        // than freq (or equal)
        auto next_coeff = _coeffs.lower_bound(freq);
        if (next_coeff == _coeffs.end()) {
            // This means freq is larger than our biggest key, and thus we
            // can't interpolate. We return the coeffs of the largest key.
            return _coeffs.rbegin()->second;
        }
        if (next_coeff == _coeffs.begin()) {
            // This means freq is smaller than our smallest key, and thus we
            // can't interpolate. We return the coeffs of the smallest key.
            return _coeffs.begin()->second;
        }
        // Stash away freqs and coeffs for easier code
        const auto hi_freq  = next_coeff->first;
        const auto hi_coeff = next_coeff->second;
        next_coeff--;
        const auto lo_coeff = next_coeff->second;
        const auto lo_freq  = next_coeff->first; // lo == low, not LO
        // Now, we're guaranteed to be between two points
        if (_interp == interp_mode::NEAREST_NEIGHBOR) {
            return (hi_freq - freq) < (freq - lo_freq) ? hi_coeff : lo_coeff;
        }
        using uhd::math::linear_interp;
        return std::complex<double>(
            linear_interp<double>(
                freq, lo_freq, lo_coeff.real(), hi_freq, hi_coeff.real()),
            linear_interp<double>(
                freq, lo_freq, lo_coeff.imag(), hi_freq, hi_coeff.imag()));
    }

    void set_cal_coeff(const double freq,
        const std::complex<double> coeff,
        const double suppression_abs   = 0,
        const double suppression_delta = 0) override
    {
        _coeffs[freq] = coeff;
        _supp[freq]   = {suppression_abs, suppression_delta};
    }

    void clear() override
    {
        _coeffs.clear();
        _supp.clear();
    }

    /**************************************************************************
     * Container API (Serialization/Deserialization)
     *************************************************************************/
    std::vector<uint8_t> serialize() override
    {
        // This is a magic value to estimate the amount of space the builder will
        // have to reserve on top of the coeff data.
        // Worst case is we get this too low, and the builder will have to do a
        // single reallocation later.
        constexpr size_t RESERVE_HDR_BYTES = 20;
        const size_t initial_size_bytes =
            sizeof(IQCalCoeff) * _coeffs.size() + RESERVE_HDR_BYTES;
        flatbuffers::FlatBufferBuilder builder(initial_size_bytes);
        // Convert the coefficients to a vector of IQCalCoeff
        std::vector<IQCalCoeff> fb_coeffs;
        fb_coeffs.reserve(_coeffs.size());
        std::transform(_coeffs.cbegin(),
            _coeffs.cend(),
            std::back_inserter(fb_coeffs),
            [&](const coeffs_type::value_type& coeff) {
                const double freq     = coeff.first;
                const auto this_coeff = coeff.second;
                return IQCalCoeff(freq,
                    this_coeff.real(),
                    this_coeff.imag(),
                    _supp.count(freq) ? _supp.at(freq).first : 0.0,
                    _supp.count(freq) ? _supp.at(freq).second : 0.0);
            });
        // Now load it all into the FlatBuffer
        const auto metadata = CreateMetadataDirect(builder,
            _name.c_str(),
            _serial.c_str(),
            _timestamp,
            VERSION_MAJOR,
            VERSION_MINOR);
        auto cal_table      = CreateIQCalCoeffsDirect(builder, metadata, &fb_coeffs);
        FinishIQCalCoeffsBuffer(builder, cal_table);
        const size_t table_size = builder.GetSize();
        const uint8_t* table    = builder.GetBufferPointer();
        return std::vector<uint8_t>(table, table + table_size);
    }

    // This will amend the existing table. If that's not desired, then it is
    // necessary to call clear() ahead of time.
    void deserialize(const std::vector<uint8_t>& data) override
    {
        auto verifier = flatbuffers::Verifier(data.data(), data.size());
        if (!VerifyIQCalCoeffsBuffer(verifier)) {
            throw uhd::runtime_error("iq_cal: Invalid data provided!");
        }
        auto cal_table = GetIQCalCoeffs(static_cast<const void*>(data.data()));
        // TODO we can handle this more nicely
        UHD_ASSERT_THROW(cal_table->metadata()->version_major() == VERSION_MAJOR);
        _name       = std::string(cal_table->metadata()->name()->c_str());
        _serial     = std::string(cal_table->metadata()->serial()->c_str());
        _timestamp  = cal_table->metadata()->timestamp();
        auto coeffs = cal_table->coeffs();
        for (auto it = coeffs->begin(); it != coeffs->end(); ++it) {
            _coeffs[it->freq()] = {it->coeff_real(), it->coeff_imag()};
            // Suppression levels are really not necessary for runtime.
            // TODO: Come up with a way to skip this step when loading data for
            // runtime (and not future storage)
            _supp[it->freq()] = {it->suppression_abs(), it->suppression_delta()};
        }
    }


private:
    std::string _name;
    std::string _serial;
    uint64_t _timestamp;
    using coeffs_type = std::map<double, std::complex<double>>;
    coeffs_type _coeffs;
    // Abs suppression, delta suppression
    std::map<double, std::pair<double, double>> _supp;

    interp_mode _interp;
};


iq_cal::sptr iq_cal::make()
{
    return std::make_shared<iq_cal_impl>();
}

iq_cal::sptr iq_cal::make(
    const std::string& name, const std::string& serial, const uint64_t timestamp)
{
    return std::make_shared<iq_cal_impl>(name, serial, timestamp);
}
