//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/pwr_cal.hpp>
#include <uhd/cal/pwr_cal_generated.h>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/utils/interpolation.hpp>
#include <map>
#include <string>

using namespace uhd::usrp::cal;
using namespace uhd::math;

namespace {

//! We use the NIST normal temperature
constexpr int NORMAL_TEMPERATURE = 20;

constexpr size_t VERSION_MAJOR = 1;
constexpr size_t VERSION_MINOR = 0;

//! Return map with keys as values and vice versa
template <typename map_type>
map_type reverse_map(const map_type& map)
{
    map_type result;
    std::transform(map.cbegin(),
        map.cend(),
        std::inserter(result, result.end()),
        [](const typename map_type::value_type& entry) {
            return std::pair<typename map_type::mapped_type, typename map_type::key_type>(
                entry.second, entry.first);
        });
    return result;
}

} // namespace


class pwr_cal_impl : public pwr_cal
{
public:
    pwr_cal_impl(const std::string& name = "",
        const std::string& serial        = "",
        const uint64_t timestamp         = 0)
        : _name(name), _serial(serial), _timestamp(timestamp)
    {
    }

    /**************************************************************************
     * Container API (Basics)
     *************************************************************************/
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

    /**************************************************************************
     * Specific APIs
     *************************************************************************/
    void add_power_table(const std::map<double, double>& gain_power_map,
        const double min_power,
        const double max_power,
        const double freq,
        const boost::optional<int> temperature = boost::none) override
    {
        if (min_power > max_power) {
            throw uhd::runtime_error(
                std::string("Invalid min/max power levels: Min power must be smaller "
                            "than max power! (Is: "
                            + std::to_string(min_power) + " dBm, "
                            + std::to_string(max_power) + " dBm)"));
        }
        const int temp = bool(temperature) ? temperature.get() : _default_temp;
        _data[temp][static_cast<uint64_t>(freq)] = {
            gain_power_map, reverse_map(gain_power_map), min_power, max_power};
    }

    // Note: This is very similar to at_bilin_interp(), but we can't use that
    // because we mix types in the gain tables (we have uint64_t and double, and
    // a struct).
    double get_power(const double gain,
        const double freq,
        const boost::optional<int> temperature = boost::none) const override
    {
        UHD_ASSERT_THROW(!_data.empty());
        const uint64_t freqi = static_cast<uint64_t>(freq);
        const auto& table    = _get_table(temperature);

        const auto f_iters = get_bounding_iterators(table, freqi);
        const uint64_t f1i = f_iters.first->first;
        const uint64_t f2i = f_iters.second->first;
        // Frequency is out of bounds
        if (f1i == f2i) {
            return at_lin_interp(table.at(f1i).g2p, gain);
        }
        const double f1       = static_cast<double>(f1i);
        const double f2       = static_cast<double>(f2i);
        const auto gain_iters = get_bounding_iterators(table.at(f1).g2p, gain);
        const double gain1    = gain_iters.first->first;
        const double gain2    = gain_iters.second->first;
        // Gain is out of bounds
        if (gain1 == gain2) {
            return linear_interp(
                freq, f1, table.at(f1i).g2p.at(gain1), f2, table.at(f2i).g2p.at(gain1));
        }

        // Both gain and freq are within bounds: Bi-Linear interpolation
        // Find power values
        const auto power11 = table.at(f1i).g2p.at(gain1);
        const auto power12 = table.at(f1i).g2p.at(gain2);
        const auto power21 = table.at(f2i).g2p.at(gain1);
        const auto power22 = table.at(f2i).g2p.at(gain2);

        return bilinear_interp(
            freq, gain, f1, gain1, f2, gain2, power11, power12, power21, power22);
    }

    void clear() override
    {
        _data.clear();
    }

    void set_temperature(const int temperature) override
    {
        _default_temp = temperature;
    }

    int get_temperature() const override
    {
        return _default_temp;
    }

    void set_ref_gain(const double gain) override
    {
        _ref_gain = gain;
    }

    double get_ref_gain() const override
    {
        return _ref_gain;
    }

    uhd::meta_range_t get_power_limits(const double freq,
        const boost::optional<int> temperature = boost::none) const override
    {
        const auto table = at_nearest(_get_table(temperature), uint64_t(freq));
        return uhd::meta_range_t(table.min_power, table.max_power);
    }

    double get_gain(const double power_dbm,
        const double freq,
        const boost::optional<int> temperature = boost::none) const override
    {
        UHD_ASSERT_THROW(!_data.empty());
        const uint64_t freqi       = static_cast<uint64_t>(freq);
        const auto& table          = _get_table(temperature);
        const double power_coerced = get_power_limits(freq, temperature).clip(power_dbm);

        const auto f_iters = get_bounding_iterators(table, freqi);
        const uint64_t f1i = f_iters.first->first;
        const uint64_t f2i = f_iters.second->first;
        if (f1i == f2i) {
            // Frequency is out of bounds
            return at_lin_interp(table.at(f1i).p2g, power_coerced);
        }

        // NOTE: bilinear_interp() does not interpolate on an arbitrary tetragon,
        // but requires the coordinates to be on a rectangular grid. Due to the
        // frequency-dependent nature of power calibration, it is unlikely that
        // the bounding power values for f1 and f2 (respectively) are identical.
        // We therefore not only interpolate the final gain values, but we also
        // nearest-neighbor-interpolate the grid coordinates for the power.
        // This snap-to-grid adds another error, which can be counteracted by
        // good choice of frequency and gain points on which to sample.
        const auto f1pwr_iters = get_bounding_iterators(table.at(f1i).p2g, power_coerced);
        const double f1pwr1    = f1pwr_iters.first->first;
        const double f1pwr2    = f1pwr_iters.second->first;
        const auto f2pwr_iters = get_bounding_iterators(table.at(f2i).p2g, power_coerced);
        const double f2pwr1    = f2pwr_iters.first->first;
        const double f2pwr2    = f2pwr_iters.second->first;
        const double f1        = static_cast<double>(f1i);
        const double f2        = static_cast<double>(f2i);
        const double pwr1      = linear_interp(freq, f1, f1pwr1, f2, f2pwr1);
        const double pwr2      = linear_interp(freq, f1, f1pwr2, f2, f2pwr2);
        // Power is out of bounds (this shouldn't happen after coercing, but this
        // is just another good sanity check on our data)
        if (pwr1 == pwr2) {
            return linear_interp(freq,
                f1,
                at_nearest(table.at(f1i).p2g, pwr1),
                f2,
                at_nearest(table.at(f2i).p2g, pwr2));
        }
        // Both gain and freq are within bounds => Bi-Linear interpolation
        // Find gain values:
        const auto gain11 = table.at(f1i).p2g.at(f1pwr1);
        const auto gain12 = table.at(f1i).p2g.at(f1pwr2);
        const auto gain21 = table.at(f2i).p2g.at(f2pwr1);
        const auto gain22 = table.at(f2i).p2g.at(f2pwr2);
        return bilinear_interp(
            freq, power_coerced, f1, pwr1, f2, pwr2, gain11, gain12, gain21, gain22);
    }

    /**************************************************************************
     * Container API (Serialization/Deserialization)
     *************************************************************************/
    std::vector<uint8_t> serialize() override
    {
        const size_t initial_size_bytes = 1024 * 20; // 20 kiB as an initial guess
        flatbuffers::FlatBufferBuilder builder(initial_size_bytes);

        std::vector<flatbuffers::Offset<TempFreqMap>> temp_freq_map;
        temp_freq_map.reserve(_data.size());
        for (auto& temp_freq_pair : _data) {
            const int temperature = temp_freq_pair.first;
            std::vector<flatbuffers::Offset<FreqPowerMap>> freq_gain_map;
            for (auto& freq_gain_pair : temp_freq_pair.second) {
                const uint64_t freq    = freq_gain_pair.first;
                const double min_power = freq_gain_pair.second.min_power;
                const double max_power = freq_gain_pair.second.max_power;
                std::vector<PowerMap> gain_power_map;
                for (auto& gain_power_pair : freq_gain_pair.second.g2p) {
                    gain_power_map.push_back(
                        PowerMap(gain_power_pair.first, gain_power_pair.second));
                }

                freq_gain_map.push_back(CreateFreqPowerMapDirect(
                    builder, freq, &gain_power_map, min_power, max_power));
            }

            temp_freq_map.push_back(
                CreateTempFreqMapDirect(builder, temperature, &freq_gain_map));
        }

        // Now load it all into the FlatBuffer
        auto metadata = CreateMetadataDirect(builder,
            _name.c_str(),
            _serial.c_str(),
            _timestamp,
            VERSION_MAJOR,
            VERSION_MINOR);
        auto power_cal =
            CreatePowerCalDirect(builder, metadata, &temp_freq_map, get_ref_gain());
        FinishPowerCalBuffer(builder, power_cal);
        const size_t table_size = builder.GetSize();
        const uint8_t* table    = builder.GetBufferPointer();
        return std::vector<uint8_t>(table, table + table_size);
    }

    // This will amend the existing table. If that's not desired, then it is
    // necessary to call clear() ahead of time.
    void deserialize(const std::vector<uint8_t>& data) override
    {
        auto verifier = flatbuffers::Verifier(data.data(), data.size());
        if (!VerifyPowerCalBuffer(verifier)) {
            throw uhd::runtime_error("pwr_cal: Invalid data provided!");
        }
        auto cal_table = GetPowerCal(static_cast<const void*>(data.data()));
        if (cal_table->metadata()->version_major() != VERSION_MAJOR) {
            throw uhd::runtime_error("pwr_cal: Compat number mismatch!");
        }
        if (cal_table->metadata()->version_minor() != VERSION_MINOR) {
            UHD_LOG_WARNING("CAL",
                "pwr_cal: Expected compat number "
                    << VERSION_MAJOR << "." << VERSION_MINOR << ", got "
                    << cal_table->metadata()->version_major() << "."
                    << cal_table->metadata()->version_minor());
        }
        _name      = std::string(cal_table->metadata()->name()->c_str());
        _serial    = std::string(cal_table->metadata()->serial()->c_str());
        _timestamp = cal_table->metadata()->timestamp();
        if (cal_table->ref_gain() >= 0.0) {
            _ref_gain = cal_table->ref_gain();
        }

        auto temp_freq_map = cal_table->temp_freq_map();
        for (auto it = temp_freq_map->begin(); it != temp_freq_map->end(); ++it) {
            const int temperature = it->temperature();
            auto freq_gain_map    = it->freqs();
            for (auto f_it = freq_gain_map->begin(); f_it != freq_gain_map->end();
                 ++f_it) {
                std::map<double, double> power;
                auto power_map = f_it->powers();
                for (auto g_it = power_map->begin(); g_it != power_map->end(); ++g_it) {
                    power.insert({g_it->gain(), g_it->power_dbm()});
                }
                add_power_table(power,
                    f_it->min_power(),
                    f_it->max_power(),
                    f_it->freq(),
                    temperature);
            }
        }
    }


private:
    // We map the gain to power, and power to gain, in different data structures.
    // This is suboptimal w.r.t. memory usage (it duplicates the keys/values),
    // but helps us with the algorithms above.
    // This could also be solved with a Boost.Bimap, but it doesn't seem worth
    // the additional dependency.
    struct pwr_cal_table
    {
        std::map<double, double> g2p; //!< Maps gain to power
        std::map<double, double> p2g; //!< Maps power to gain
        double min_power;
        double max_power;
    };

    using freq_table_map = std::map<uint64_t /* freq */, pwr_cal_table>;

    freq_table_map _get_table(const boost::optional<int> temperature) const
    {
        const int temp = bool(temperature) ? temperature.get() : _default_temp;
        return at_nearest(_data, temp);
    }

    std::string _name;
    std::string _serial;
    uint64_t _timestamp;

    //! The actual gain table
    std::map<int /* temp */, freq_table_map> _data;
    double _ref_gain  = 0.0;
    int _default_temp = NORMAL_TEMPERATURE;
};


pwr_cal::sptr pwr_cal::make()
{
    return std::make_shared<pwr_cal_impl>();
}

pwr_cal::sptr pwr_cal::make(
    const std::string& name, const std::string& serial, const uint64_t timestamp)
{
    return std::make_shared<pwr_cal_impl>(name, serial, timestamp);
}
