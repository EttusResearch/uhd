//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//
#include <uhd/cal/dsa_cal.hpp>
#include <uhd/cal/dsa_cal_generated.h>
#include <uhd/exception.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <boost/range/adaptor/indexed.hpp>
#include <array>
#include <map>
#include <string>

using namespace uhd::usrp::cal;

namespace {
constexpr size_t VERSION_MAJOR = 2;
constexpr size_t VERSION_MINOR = 0;

/***********************************************************************
 * Helper routines
 **********************************************************************/
template <typename base, size_t num_gain_stages, int num_dsa>
class dsa_cal_impl : public base
{
public:
    dsa_cal_impl(const std::string& name = "",
        const std::string& serial        = "",
        const uint64_t timestamp         = 0)
        : _name(name), _serial(serial), _timestamp(timestamp)
    {
    }

    /**************************************************************************
     * Container API (Basics)
     *************************************************************************/
    std::string get_name() const
    {
        return _name;
    }

    std::string get_serial() const
    {
        return _serial;
    }

    uint64_t get_timestamp() const
    {
        return _timestamp;
    }

    /**************************************************************************
     * Specific APIs
     *************************************************************************/
    void add_frequency_band(const double max_freq,
        const std::string& name,
        const std::array<std::array<uint32_t, num_dsa>, num_gain_stages> dsa_values)
    {
        _data[max_freq] = name_dsas_pair(name, dsa_values);
    }

    const std::array<uint32_t, num_dsa> get_dsa_setting(
        double freq, size_t gain_index) const
    {
        if (_data.empty()) {
            throw uhd::runtime_error("Cannot get DSA settings from an empty container.");
        }
        // find the lowest band with band_freq_max <= freq
        const uint64_t freqi = static_cast<uint64_t>(freq);
        const auto freq_it   = _data.lower_bound(freqi);
        if (freq_it == _data.end()) {
            throw uhd::value_error("No DSA band found for freq " + std::to_string(freq));
        }
        // select DSA setting using gain_index
        if (gain_index >= num_gain_stages) {
            throw uhd::value_error(
                "gain index " + std::to_string(gain_index) + " out of bounds.");
        }
        return freq_it->second.second[gain_index];
    }

    bool is_same_band(double freq1, double freq2) const
    {
        const uint64_t freqi1 = static_cast<uint64_t>(freq1);
        const auto freq_it1   = _data.lower_bound(freqi1);
        const uint64_t freqi2 = static_cast<uint64_t>(freq2);
        const auto freq_it2   = _data.lower_bound(freqi2);
        return freq_it1 == freq_it2;
    }

    std::vector<uint32_t> get_band_settings(double freq, uint8_t dsa) const
    {
        std::vector<uint32_t> result;
        // find the lowest band with band_freq_max <= freq
        const uint64_t freqi = static_cast<uint64_t>(freq);
        const auto freq_it   = _data.lower_bound(freqi);
        if (freq_it == _data.end()) {
            throw uhd::value_error("No DSA band found for freq " + std::to_string(freq));
        }
        // select DSA setting using gain_index
        for (auto item : freq_it->second.second) {
            result.push_back(item[dsa]);
        }
        return result;
    }

    void clear()
    {
        _data.clear();
    }


    /**************************************************************************
     * Container API (Serialization/Deserialization)
     *************************************************************************/
    std::vector<uint8_t> serialize()
    {
        const size_t initial_size_bytes =
            2 * num_gain_stages * num_dsa * sizeof(uint32_t); // 2 band of DSA values
        flatbuffers::FlatBufferBuilder builder(initial_size_bytes);

        std::vector<flatbuffers::Offset<BandDsaMap>> band_dsas;
        band_dsas.reserve(_data.size());
        for (auto& band_dsas_pair : _data) {
            const uint64_t freq = band_dsas_pair.first;
            std::vector<flatbuffers::Offset<DsaStep>> dsa_steps;
            auto name_dsas = band_dsas_pair.second;
            for (auto const& values : name_dsas.second) // iterate over gain indizes
            {
                std::vector<uint32_t> steps(values.begin(), values.end());
                dsa_steps.push_back(CreateDsaStepDirect(builder, &steps));
            }

            band_dsas.push_back(CreateBandDsaMapDirect(
                builder, freq, &dsa_steps, name_dsas.first.c_str()));
        }

        // Now load it all into the FlatBuffer
        auto metadata     = CreateMetadataDirect(builder,
            _name.c_str(),
            _serial.c_str(),
            _timestamp,
            VERSION_MAJOR,
            VERSION_MINOR);
        auto gain_dsa_cal = CreateDsaCalDirect(builder, metadata, &band_dsas);
        FinishDsaCalBuffer(builder, gain_dsa_cal);
        const size_t table_size = builder.GetSize();
        const uint8_t* table    = builder.GetBufferPointer();
        return std::vector<uint8_t>(table, table + table_size);
    }

    // This will amend the existing table. If that's not desired, then it is
    // necessary to call clear() ahead of time.
    void deserialize(const std::vector<uint8_t>& data)
    {
        clear();
        auto verifier = flatbuffers::Verifier(data.data(), data.size());
        if (!VerifyDsaCalBuffer(verifier)) {
            throw uhd::runtime_error("dsa_cal: Invalid data provided! ");
        }
        auto cal_table = GetDsaCal(static_cast<const void*>(data.data()));
        if (cal_table->metadata()->version_major() != VERSION_MAJOR) {
            throw uhd::runtime_error("dsa_cal: Compat number mismatch!");
        }
        if (cal_table->metadata()->version_minor() != VERSION_MINOR) {
            UHD_LOG_WARNING("CAL",
                "gain_cal: Expected compat number "
                    << VERSION_MAJOR << "." << VERSION_MINOR << ", got "
                    << cal_table->metadata()->version_major() << "."
                    << cal_table->metadata()->version_minor());
        }
        _name      = std::string(cal_table->metadata()->name()->c_str());
        _serial    = std::string(cal_table->metadata()->serial()->c_str());
        _timestamp = cal_table->metadata()->timestamp();

        auto band_dsa_map = cal_table->band_dsa_map();
        for (auto it = band_dsa_map->begin(); it != band_dsa_map->end(); ++it) {
            const uint64_t max_freq = it->max_freq();
            const std::string name(it->name()->c_str());
            auto gains = it->gains();
            if (gains->size() != num_gain_stages) {
                UHD_LOG_ERROR("CAL",
                    "Invalid number of gain indizes. Got: "
                        << gains->size() << " expected: " << num_gain_stages);
                throw uhd::runtime_error("Invalid number of gain indizes");
            }
            std::array<std::array<uint32_t, num_dsa>, num_gain_stages> indizes;
            int i = 0;
            for (auto gain_it = gains->begin(); gain_it != gains->end(); ++gain_it) {
                if (gain_it->steps()->size() != num_dsa) {
                    UHD_LOG_ERROR("CAL",
                        "Invalid number of attenuator indizes. Got: "
                            << gain_it->steps()->size() << " expected: " << num_dsa);
                    throw uhd::runtime_error("Invalid number of attenuator indizes");
                }
                std::copy(gain_it->steps()->begin(),
                    gain_it->steps()->end(),
                    indizes[i++].begin());
            }
            add_frequency_band(max_freq, name, indizes);
        }
    }


private:
    std::string _name;
    std::string _serial;
    uint64_t _timestamp;

    using dsa_steps      = std::array<std::array<uint32_t, num_dsa>, num_gain_stages>;
    using name_dsas_pair = std::pair<std::string, dsa_steps>;

    std::map<uint64_t /* freq */, name_dsas_pair> _data;
};

} // namespace
std::shared_ptr<zbx_tx_dsa_cal> zbx_tx_dsa_cal::make()
{
    return std::make_shared<dsa_cal_impl<zbx_tx_dsa_cal,
        zbx_tx_dsa_cal::NUM_GAIN_STAGES,
        zbx_tx_dsa_cal::NUM_DSA>>();
}

std::shared_ptr<zbx_tx_dsa_cal> zbx_tx_dsa_cal::make(
    const std::string& name, const std::string& serial, const uint64_t timestamp)
{
    return std::make_shared<dsa_cal_impl<zbx_tx_dsa_cal,
        zbx_tx_dsa_cal::NUM_GAIN_STAGES,
        zbx_tx_dsa_cal::NUM_DSA>>(name, serial, timestamp);
}

std::shared_ptr<zbx_rx_dsa_cal> zbx_rx_dsa_cal::make()
{
    return std::make_shared<dsa_cal_impl<zbx_rx_dsa_cal,
        zbx_rx_dsa_cal::NUM_GAIN_STAGES,
        zbx_rx_dsa_cal::NUM_DSA>>();
}

std::shared_ptr<zbx_rx_dsa_cal> zbx_rx_dsa_cal::make(
    const std::string& name, const std::string& serial, const uint64_t timestamp)
{
    return std::make_shared<dsa_cal_impl<zbx_rx_dsa_cal,
        zbx_rx_dsa_cal::NUM_GAIN_STAGES,
        zbx_rx_dsa_cal::NUM_DSA>>(name, serial, timestamp);
}
