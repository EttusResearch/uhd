//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <uhd/experts/expert_nodes.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/common/adrv9032_manager.hpp>
#include <cmath>
#include <cstdint>
#include <vector>

namespace uhd { namespace usrp { namespace b300 {

// The user guide guarantees finding an allowed frequency within 1 MHz.
inline constexpr double MAX_BLACKOUT_AVOIDANCE = 1e6;
// Taken from the ADRV9032 user guide: Tx QEC tracking calibration requires at least
// 1 MHz clearance from these blackout frequencies.
inline constexpr double MINIMUM_DISTANCE_FROM_BLACKOUT_FREQ = 1e6;
inline constexpr double BLACKOUT_SEARCH_STEP_SIZE           = 1e3;

inline bool rflo_frequency_allowed(double freq, double master_clock_rate)
{
    // There are certain frequencies that need to be avoided due to the limitations of the
    // Tx QEC tracking calibration. These formulas are taken directly from the ADRV9032
    // User Guide.

    // For all master clock rates, the Tx DAC sampling rate is 12 times the master clock
    // rate. Because we are initializing the ADRV9032 at 6.1GHz, we introduce a new
    // loopback ADC sampling rate. This is not documented in the user guide, but in
    // meeting with ADI we were advised that the sampling rate to use for the calculation
    // is actually the LBADC sampling rate, which is equal to the Tx DAC sampling rate
    // multiplied by 2/5.
    double sampling_rate              = (master_clock_rate * 12) * (2.0 / 5.0);
    std::vector<uint32_t> values_of_m = {2, 4, 6, 8, 10, 12};
    for (const uint32_t& m : values_of_m) {
        const uint32_t i =
            static_cast<uint32_t>(std::round(2 * m * (freq / sampling_rate)));
        double nearest_blackout_freq = i * (sampling_rate / (2 * m));
        if (uhd::math::fp_compare::fp_compare_epsilon<double>(
                std::abs(freq - nearest_blackout_freq))
            < MINIMUM_DISTANCE_FROM_BLACKOUT_FREQ) {
            return false;
        }
    }
    return true;
}

inline double avoid_blackout_frequencies(
    double freq, double master_clock_rate, const uhd::freq_range_t& lo_range)
{
    if (rflo_frequency_allowed(freq, master_clock_rate)) {
        return freq;
    } else {
        // Traverse both up and down from the desired frequency to find the nearest
        // allowable frequency.
        double max_adjust    = MAX_BLACKOUT_AVOIDANCE;
        double traverse_up   = freq;
        double traverse_down = freq;
        do {
            if (traverse_up < lo_range.stop()) {
                traverse_up += BLACKOUT_SEARCH_STEP_SIZE;
                if (rflo_frequency_allowed(traverse_up, master_clock_rate)) {
                    return traverse_up;
                }
            }
            if (traverse_down > lo_range.start()) {
                traverse_down -= BLACKOUT_SEARCH_STEP_SIZE;
                if (rflo_frequency_allowed(traverse_down, master_clock_rate)) {
                    return traverse_down;
                }
            }
        } while ((((traverse_up - freq) < max_adjust && traverse_up < lo_range.stop())
                  || ((freq - traverse_down) < max_adjust
                      && traverse_down > lo_range.start())));
        throw uhd::runtime_error(
            "Could not find a nearby RFLO frequency that avoids ADRV9032 "
            "blackout frequencies.");
    }
}

class b300_desired_channel_frequency_expert : public uhd::experts::worker_node_t
{
public:
    b300_desired_channel_frequency_expert(const uhd::experts::node_retriever_t& nr,
        const uhd::fs_path fe_path,
        double master_clock_rate)
        : uhd::experts::worker_node_t(fe_path / "b300_desired_channel_frequency_expert")
        , _master_clock_rate(master_clock_rate)
        , _freq_desired(nr, fe_path / "freq" / "desired")
        , _rflo_freq_desired(nr, fe_path / "RFLO" / "freq" / "desired")
        , _nco_freq_desired(nr, fe_path / "NCO" / "freq" / "desired")
    {
        bind_accessor(_freq_desired);
        bind_accessor(_rflo_freq_desired);
        bind_accessor(_nco_freq_desired);
    }

private:
    void resolve() override;
    double _master_clock_rate;
    // Inputs
    uhd::experts::data_reader_t<double> _freq_desired;
    //  Outputs
    uhd::experts::data_writer_t<double> _rflo_freq_desired;
    uhd::experts::data_writer_t<double> _nco_freq_desired;
};

class b300_rflo_channel_frequency_expert : public uhd::experts::worker_node_t
{
public:
    b300_rflo_channel_frequency_expert(const uhd::experts::node_retriever_t& nr,
        const uhd::fs_path fe_path,
        double master_clock_rate)
        : uhd::experts::worker_node_t(fe_path / "b300_rflo_channel_frequency_expert")
        , _master_clock_rate(master_clock_rate)
        , _rflo_source(nr, fe_path / "RFLO" / "source")
        , _rflo_freq_desired(nr, fe_path / "RFLO" / "freq" / "desired")
        , _rflo0_frequency(nr, "LO0_freq")
        , _rflo1_frequency(nr, "LO1_freq")
    {
        bind_accessor(_rflo_source);
        bind_accessor(_rflo_freq_desired);
        bind_accessor(_rflo0_frequency);
        bind_accessor(_rflo1_frequency);
    }

private:
    void resolve() override;
    double _master_clock_rate;
    // Inputs
    uhd::experts::data_reader_t<std::string> _rflo_source;
    uhd::experts::data_reader_t<double> _rflo_freq_desired;
    //  Outputs
    uhd::experts::data_writer_t<double> _rflo0_frequency;
    uhd::experts::data_writer_t<double> _rflo1_frequency;
};

class b300_nco_programming_expert : public uhd::experts::worker_node_t
{
public:
    b300_nco_programming_expert(const uhd::experts::node_retriever_t& nr,
        const uhd::fs_path fe_path,
        uhd::direction_t dir,
        std::function<double(const double)> set_nco_frequency)
        : uhd::experts::worker_node_t(fe_path / "b300_nco_programming_expert")
        , _dir(dir)
        , _set_nco_frequency(set_nco_frequency)
        , _nco_freq_desired(nr, fe_path / "NCO" / "freq" / "desired")
        , _nco_freq_coerced(nr, fe_path / "NCO" / "freq" / "coerced")
    {
        bind_accessor(_nco_freq_desired);
        bind_accessor(_nco_freq_coerced);
    }

private:
    void resolve() override;
    uhd::direction_t _dir;
    std::function<double(const double)> _set_nco_frequency;
    // Inputs
    uhd::experts::data_reader_t<double> _nco_freq_desired;
    //  Outputs
    uhd::experts::data_writer_t<double> _nco_freq_coerced;
};

class b300_rflo_programming_expert : public uhd::experts::worker_node_t
{
public:
    b300_rflo_programming_expert(const uhd::experts::node_retriever_t& nr,
        const std::string lo_name,
        std::function<double(const uhd::direction_t, const size_t, const double)>
            set_rflo_frequency)
        : uhd::experts::worker_node_t(lo_name / "b300_rflo_programming_expert")
        , _lo_name(lo_name)
        , _set_rflo_frequency(set_rflo_frequency)
        , _rflo_frequency(nr, lo_name + "_freq")
        , _rx0_rflo_source(nr, "rx_frontends/0/RFLO/source")
        , _rx1_rflo_source(nr, "rx_frontends/1/RFLO/source")
        , _tx0_rflo_source(nr, "tx_frontends/0/RFLO/source")
        , _tx1_rflo_source(nr, "tx_frontends/1/RFLO/source")
        , _rx0_rflo_freq_coerced(nr, "rx_frontends/0/RFLO/freq/coerced")
        , _rx1_rflo_freq_coerced(nr, "rx_frontends/1/RFLO/freq/coerced")
        , _tx0_rflo_freq_coerced(nr, "tx_frontends/0/RFLO/freq/coerced")
        , _tx1_rflo_freq_coerced(nr, "tx_frontends/1/RFLO/freq/coerced")
    {
        bind_accessor(_rflo_frequency);
        bind_accessor(_rx0_rflo_source);
        bind_accessor(_rx1_rflo_source);
        bind_accessor(_tx0_rflo_source);
        bind_accessor(_tx1_rflo_source);
        bind_accessor(_rx0_rflo_freq_coerced);
        bind_accessor(_rx1_rflo_freq_coerced);
        bind_accessor(_tx0_rflo_freq_coerced);
        bind_accessor(_tx1_rflo_freq_coerced);
    }

private:
    void resolve() override;
    std::string _lo_name;
    std::function<double(const uhd::direction_t, const size_t, const double)>
        _set_rflo_frequency;
    // Inputs
    uhd::experts::data_reader_t<double> _rflo_frequency;
    uhd::experts::data_reader_t<std::string> _rx0_rflo_source;
    uhd::experts::data_reader_t<std::string> _rx1_rflo_source;
    uhd::experts::data_reader_t<std::string> _tx0_rflo_source;
    uhd::experts::data_reader_t<std::string> _tx1_rflo_source;
    //  Outputs
    uhd::experts::data_writer_t<double> _rx0_rflo_freq_coerced;
    uhd::experts::data_writer_t<double> _rx1_rflo_freq_coerced;
    uhd::experts::data_writer_t<double> _tx0_rflo_freq_coerced;
    uhd::experts::data_writer_t<double> _tx1_rflo_freq_coerced;
};

class b300_coerced_channel_frequency_expert : public uhd::experts::worker_node_t
{
public:
    b300_coerced_channel_frequency_expert(
        const uhd::experts::node_retriever_t& nr, const uhd::fs_path fe_path)
        : uhd::experts::worker_node_t(fe_path / "b300_coerced_channel_frequency_expert")
        , _rflo_freq_coerced(nr, fe_path / "RFLO" / "freq" / "coerced")
        , _nco_freq_coerced(nr, fe_path / "NCO" / "freq" / "coerced")
        , _freq_coerced(nr, fe_path / "freq" / "coerced")
    {
        bind_accessor(_rflo_freq_coerced);
        bind_accessor(_nco_freq_coerced);
        bind_accessor(_freq_coerced);
    }

private:
    void resolve() override;
    // Inputs
    uhd::experts::data_reader_t<double> _rflo_freq_coerced;
    uhd::experts::data_reader_t<double> _nco_freq_coerced;
    //  Outputs
    uhd::experts::data_writer_t<double> _freq_coerced;
};

}}} // namespace uhd::usrp::b300
