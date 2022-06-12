//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/cal/database.hpp>
#include <uhd/cal/pwr_cal.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/common/pwr_cal_mgr.hpp>
#include <unordered_map>
#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <algorithm>
#include <set>

using namespace uhd::usrp;

namespace {

// List of antenna names that are globally known to never have their own cal data
const std::set<std::string> INVALID_ANTENNAS{"CAL", "LOCAL"};

} // namespace

std::string pwr_cal_mgr::sanitize_antenna_name(std::string antenna_name)
{
    std::replace(antenna_name.begin(), antenna_name.end(), '/', '+');
    boost::to_lower(antenna_name);
    return antenna_name;
}

// Shorthand for filtering against INVALID_ANTENNAS
bool pwr_cal_mgr::is_valid_antenna(const std::string& antenna)
{
    return !INVALID_ANTENNAS.count(antenna);
}

class pwr_cal_mgr_impl : public pwr_cal_mgr
{
public:
    pwr_cal_mgr_impl(const std::string& serial,
        const std::string& log_id,
        get_double_type&& get_freq,
        get_str_type&& get_key,
        uhd::gain_group::sptr gain_group)
        : _log_id(log_id)
        , _get_freq(std::move(get_freq))
        , _get_key(std::move(get_key))
        , _gain_group(gain_group)
        , _hw_gain_name(gain_group->get_names().at(0))
    {
        set_serial(serial);
    }

    void set_gain_group(uhd::gain_group::sptr gain_group) override
    {
        _gain_group = gain_group;
    }

    bool has_power_data() override
    {
        const std::string key = _get_key();
        _load_cal_data(key);
        return _cal_data.count(key) && bool(_cal_data.at(key));
    }

    void populate_subtree(uhd::property_tree::sptr subtree) override
    {
        subtree->create<std::string>(uhd::fs_path("ref_power/key"))
            .set_coercer([](const std::string&) -> std::string {
                throw uhd::runtime_error("Cannot overwrite power cal key!");
            })
            .set_publisher([this]() { return _get_key(); });
        subtree->create<std::string>(uhd::fs_path("ref_power/serial"))
            .set_coercer([](const std::string&) -> std::string {
                throw uhd::runtime_error("Cannot overwrite cal serial!");
            })
            .set_publisher([this]() { return _serial; });
        if (!has_power_data()) {
            return;
        }
        subtree->create<double>(uhd::fs_path("ref_power/value"))
            .set_coercer(
                [this](const double power_dbm) { return this->set_power(power_dbm); })
            .set_publisher([this]() { return this->get_power(); });
        subtree->create<uhd::meta_range_t>(uhd::fs_path("ref_power/range"))
            .set_coercer([](const uhd::meta_range_t&) -> uhd::meta_range_t {
                throw uhd::runtime_error("Cannot overwrite power range!");
            })
            .set_publisher([this]() { return this->get_power_range(); });
    }

    double set_power(const double power_dbm) override
    {
        const std::string key = _get_key();
        _load_cal_data(key);
        UHD_ASSERT_THROW(_cal_data.count(key));
        _desired_power      = power_dbm;
        const uint64_t freq = static_cast<uint64_t>(std::round(_get_freq()));
        auto& cal_data = _cal_data.at(key);
        if (!cal_data) {
            const std::string err_msg = std::string("Attempting to set power for key ")
                                        + key + ", but no cal data available!";
            UHD_LOG_ERROR(_log_id, err_msg);
            throw uhd::runtime_error(err_msg);
        }

        const double desired_hw_gain = cal_data->get_gain(power_dbm, freq);
        // This sets all the gains
        _gain_group->set_value(desired_hw_gain);
        const double coerced_hw_gain    = _gain_group->get_value(_hw_gain_name);
        const double coerced_hw_power   = cal_data->get_power(coerced_hw_gain, freq);
        const double coerced_total_gain = _gain_group->get_value();
        const double coerced_total_power =
            coerced_hw_power + coerced_total_gain - coerced_hw_gain;
        UHD_LOG_TRACE(_log_id,
            "Desired power: " << power_dbm << " dBm -> desired gain: " << desired_hw_gain
                              << " dB; Actual HW power: " << coerced_hw_power
                              << " dBm -> actual HW gain: " << coerced_hw_gain
                              << " dB, Actual total power: " << coerced_total_power
                              << " dBm -> actual total gain: " << coerced_total_gain
                              << " dB");
        _mode = tracking_mode::TRACK_POWER;
        // We directly scale the power with the residual gain
        return coerced_total_power;
    }

    double get_power() override
    {
        const std::string key = _get_key();
        _load_cal_data(key);
        UHD_ASSERT_THROW(_cal_data.count(key));
        auto& cal_data         = _cal_data.at(key);
        if (!cal_data) {
            const std::string err_msg = std::string("Attempting to get power for key ")
                                        + key + ", but no cal data available!";
            UHD_LOG_ERROR(_log_id, err_msg);
            throw uhd::runtime_error(err_msg);
        }

        const uint64_t freq    = static_cast<uint64_t>(std::round(_get_freq()));
        const double hw_gain = _gain_group->get_value(_hw_gain_name);
        const double hw_power = cal_data->get_power(hw_gain, freq);
        // We directly scale the power with the residual gain
        return hw_power + (_gain_group->get_value() - hw_gain);
    }

    void update_power() override
    {
        if (_mode == tracking_mode::TRACK_POWER) {
            set_power(_desired_power);
        }
    }

    uhd::meta_range_t get_power_range() override
    {
        const std::string key = _get_key();
        _load_cal_data(key);
        UHD_ASSERT_THROW(_cal_data.count(key));
        auto& cal_data = _cal_data.at(key);
        if (!cal_data) {
            const std::string err_msg = std::string("Attempting to get power range for key ")
                                        + key + ", but no cal data available!";
            UHD_LOG_ERROR(_log_id, err_msg);
            throw uhd::runtime_error(err_msg);
        }
        const uint64_t freq = static_cast<uint64_t>(std::round(_get_freq()));
        return cal_data->get_power_limits(freq);
    }

    void set_temperature(const int temp_C) override
    {
        for (auto& cal_data : _cal_data) {
            if (cal_data.second) {
                cal_data.second->set_temperature(temp_C);
            }
        }
    }

    void set_tracking_mode(const tracking_mode mode) override
    {
        _mode = mode;
    }

    tracking_mode get_tracking_mode() override
    {
        return _mode;
    }

    void set_serial(const std::string& serial) override
    {
        if (serial == _serial || serial.empty()) {
            return;
        }
        _serial = serial;
        _cal_data.clear();
    }

    void _load_cal_data(const std::string& key)
    {
        if (_cal_data.count(key)) {
            return;
        }
        cal::pwr_cal::sptr cal_data(nullptr);
        UHD_LOG_TRACE(
            _log_id, "Looking for power cal data for " << key << ", serial " << _serial);
        bool cal_data_found = false;
        if (cal::database::has_cal_data(key, _serial)) {
            try {
                cal_data = cal::container::make<cal::pwr_cal>(
                    cal::database::read_cal_data(key, _serial));
                cal_data_found = true;
            } catch (const uhd::exception& ex) {
                UHD_LOG_WARNING(_log_id, "Error loading cal data: " << ex.what());
            }
        }
        _cal_data.insert({key, cal_data});
        UHD_LOG_TRACE(_log_id,
            (bool(cal_data) ? "" : "No ") << "power cal data found for key " << key
                                          << ", key " << key << ", serial " << _serial);
        if (cal_data_found) {
            // If we found cal data, check that all the other antennas/keys also
            // have cal data
            if (std::any_of(_cal_data.cbegin(),
                    _cal_data.cend(),
                    [](const cal_data_map_type::value_type& data) {
                        return !bool(data.second);
                    })) {
                UHD_LOG_WARNING(_log_id,
                    "Some ports for " << _serial
                                      << " have power cal data, others do not. This "
                                         "will cause inconsistent behaviour across "
                                         "antenna ports when setting power levels.");
            }
        }
    }

    std::string get_serial() const override
    {
        return _serial;
    }

    std::string get_key() override
    {
        return _get_key();
    }

private:
    const std::string _log_id;
    std::string _serial;

    get_double_type _get_freq;
    get_str_type _get_key;
    uhd::gain_group::sptr _gain_group;
    const std::string _hw_gain_name;

    //! Store the cal data for every cal key
    using cal_data_map_type =
        std::unordered_map<std::string /* key */, uhd::usrp::cal::pwr_cal::sptr>;
    cal_data_map_type _cal_data;

    double _desired_power = 0;
    tracking_mode _mode   = tracking_mode::TRACK_GAIN;
};

pwr_cal_mgr::sptr pwr_cal_mgr::make(const std::string& serial,
    const std::string& log_id,
    pwr_cal_mgr::get_double_type&& get_freq,
    pwr_cal_mgr::get_str_type&& get_key,
    uhd::gain_group::sptr gain_group)
{
    return std::make_shared<pwr_cal_mgr_impl>(serial,
        log_id,
        std::move(get_freq),
        std::move(get_key),
        gain_group);
}
