//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "rhodium_cpld_ctrl.hpp"
#include "rhodium_constants.hpp"
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <boost/format.hpp>
#include <chrono>

using namespace uhd;
using namespace uhd::math::fp_compare;

namespace {
//! Address of the CPLD scratch register
constexpr uint8_t CPLD_REGS_SCRATCH = 0x05;
//! Address of the CPLD gain table selection register
// constexpr uint8_t CPLD_REGS_GAIN_TBL_SEL = 0x06;

constexpr uint32_t MAX_GAIN_INDEX    = 60;
constexpr uint32_t MAX_LO_GAIN_INDEX = 30;

//! RX Demodulator Adjustment thresholds
constexpr double RX_DEMOD_ADJ_1500OHM_THRESHOLD = 3e9;
constexpr double RX_DEMOD_ADJ_200OHM_THRESHOLD  = 4.5e9;

/*
Unlike other CPLD fields, gain control doesn't use a register and instead
commands are directly sent over SPI. The format of these 24-bit commands is
as follows.
    23:22 Table select (1 = RX, 2 = TX)
    21:16 Gain index
    15:14 Reserved
    13    Write enable for DSA1
    12:7  Reserved
    6     Write enable for DSA2
    5:0   Reserved
The CPLD replies with the current attenuation settings as follows, but we
ignore this reply in our code.
    23:13 Reserved
    12:7  Current attenuator setting for DSA1
    6     Reserved
    5:0   Current attenuator setting for DSA2
*/
//! Gain loader constants
constexpr size_t GAIN_CTRL_TABLE_FIELD = 22;
constexpr size_t GAIN_CTRL_INDEX_FIELD = 16;
constexpr size_t GAIN_CTRL_DSA1_FIELD  = 13;
constexpr size_t GAIN_CTRL_DSA2_FIELD  = 6;

constexpr uint32_t GAIN_CTRL_TABLE_RX          = 1;
constexpr uint32_t GAIN_CTRL_TABLE_TX          = 2;
constexpr uint32_t GAIN_CTRL_DSA1_WRITE_ENABLE = 1;
constexpr uint32_t GAIN_CTRL_DSA2_WRITE_ENABLE = 1;

/*
Similar to gain control, LO control doesn't use a register and instead
commands are directly sent over SPI. The format of these 24-bit commands is
as follows:
    23:22 Table select (Always 3 = LO)
    21:16 Attenuator setting
    15:14 Reserved
    13    Write enable for RX LO DSA
    12:7  Reserved
    6     Write enable for TX LO DSA
    5:0   Reserved
The CPLD replies with the current attenuator settings as follows, but we
ignore this reply in our code.
    23:13 Reserved
    12:7  Current attenuator setting for RX LO DSA
    6     Reserved
    5:0   Current attenuator setting for TX LO DSA
*/
//! LO Gain loader constants
constexpr size_t LO_GAIN_CTRL_TABLE_FIELD = 22;
constexpr size_t LO_GAIN_CTRL_INDEX_FIELD = 16;
constexpr size_t LO_GAIN_CTRL_RX_LO_FIELD = 13;
constexpr size_t LO_GAIN_CTRL_TX_LO_FIELD = 6;

constexpr uint32_t LO_GAIN_CTRL_TABLE_LO            = 3;
constexpr uint32_t LO_GAIN_CTRL_RX_LO_WRITE_ENABLE  = 1;
constexpr uint32_t LO_GAIN_CTRL_RX_LO_WRITE_DISABLE = 0;
constexpr uint32_t LO_GAIN_CTRL_TX_LO_WRITE_ENABLE  = 1;
constexpr uint32_t LO_GAIN_CTRL_TX_LO_WRITE_DISABLE = 0;
} // namespace

rhodium_cpld_ctrl::rhodium_cpld_ctrl(write_spi_t write_fn, read_spi_t read_fn)
{
    _write_reg_fn = [write_fn](const uint8_t addr, const uint32_t data) {
        UHD_LOG_TRACE("RH_CPLD",
            str(boost::format("Writing to CPLD: 0x%02X -> 0x%04X") % uint32_t(addr)
                % data));
        const uint32_t spi_transaction = 0 | ((addr & 0x7F) << 17) | data;
        UHD_LOG_TRACE(
            "RH_CPLD", str(boost::format("SPI Transaction: 0x%04X") % spi_transaction));
        write_fn(spi_transaction);
    };
    _write_raw_fn = [write_fn](const uint32_t data) {
        UHD_LOG_TRACE("RH_CPLD", str(boost::format("Writing to CPLD: 0x%06X") % data));
        UHD_LOG_TRACE("RH_CPLD", str(boost::format("SPI Transaction: 0x%06X") % data));
        write_fn(data);
    };
    _read_reg_fn = [read_fn](const uint8_t addr) {
        UHD_LOG_TRACE("RH_CPLD",
            str(boost::format("Reading from CPLD address 0x%02X") % uint32_t(addr)));
        const uint32_t spi_transaction = (1 << 16) | ((addr & 0x7F) << 17);
        UHD_LOG_TRACE(
            "RH_CPLD", str(boost::format("SPI Transaction: 0x%04X") % spi_transaction));
        return read_fn(spi_transaction);
    };

    reset();
    _loopback_test();
}

/******************************************************************************
 * API
 *****************************************************************************/
void rhodium_cpld_ctrl::reset()
{
    std::lock_guard<std::mutex> l(_set_mutex);
    UHD_LOG_TRACE("RH_CPLD", "Resetting CPLD to default state");
    // Set local register cache to default values and commit
    _regs = rhodium_cpld_regs_t();
    _gain_queue.clear();
    commit(true);
}

uint16_t rhodium_cpld_ctrl::get_reg(const uint8_t addr)
{
    std::lock_guard<std::mutex> l(_set_mutex);
    return _read_reg_fn(addr);
}

void rhodium_cpld_ctrl::set_scratch(const uint16_t val)
{
    std::lock_guard<std::mutex> l(_set_mutex);
    _regs.scratch = val;
    commit();
}

uint16_t rhodium_cpld_ctrl::get_scratch()
{
    return get_reg(CPLD_REGS_SCRATCH);
}

void rhodium_cpld_ctrl::set_tx_switches(const tx_sw2_t tx_sw2,
    const tx_sw3_sw4_t tx_sw3_sw4,
    const tx_sw5_t tx_sw5,
    const tx_hb_lb_sel_t tx_hb_lb_sel,
    const bool defer_commit)
{
    UHD_LOG_TRACE("RH_CPLD",
        "Configuring TX band selection switches."
        " sw2="
            << tx_sw2 << " sw3_sw4=" << tx_sw3_sw4 << " sw5=" << tx_sw5
            << " hb_lb_sel=" << tx_hb_lb_sel);

    std::lock_guard<std::mutex> l(_set_mutex);

    _regs.tx_sw2       = rhodium_cpld_regs_t::tx_sw2_t(tx_sw2);
    _regs.tx_sw3_sw4   = rhodium_cpld_regs_t::tx_sw3_sw4_t(tx_sw3_sw4);
    _regs.tx_sw5       = rhodium_cpld_regs_t::tx_sw5_t(tx_sw5);
    _regs.tx_hb_lb_sel = rhodium_cpld_regs_t::tx_hb_lb_sel_t(tx_hb_lb_sel);

    if (not defer_commit) {
        commit();
    }
}

void rhodium_cpld_ctrl::set_rx_switches(const rx_sw2_sw7_t rx_sw2_sw7,
    const rx_sw3_t rx_sw3,
    const rx_sw4_sw5_t rx_sw4_sw5,
    const rx_sw6_t rx_sw6,
    const rx_hb_lb_sel_t rx_hb_lb_sel,
    const bool defer_commit)
{
    UHD_LOG_TRACE("RH_CPLD",
        "Configuring RX band selection switches."
        " sw2_sw7="
            << rx_sw2_sw7 << " sw3=" << rx_sw3 << " sw4_sw5=" << rx_sw4_sw5
            << " sw6=" << rx_sw6 << " hb_lb_sel=" << rx_hb_lb_sel);

    std::lock_guard<std::mutex> l(_set_mutex);

    _regs.rx_sw2_sw7   = rhodium_cpld_regs_t::rx_sw2_sw7_t(rx_sw2_sw7);
    _regs.rx_sw3       = rhodium_cpld_regs_t::rx_sw3_t(rx_sw3);
    _regs.rx_sw4_sw5   = rhodium_cpld_regs_t::rx_sw4_sw5_t(rx_sw4_sw5);
    _regs.rx_sw6       = rhodium_cpld_regs_t::rx_sw6_t(rx_sw6);
    _regs.rx_hb_lb_sel = rhodium_cpld_regs_t::rx_hb_lb_sel_t(rx_hb_lb_sel);

    if (not defer_commit) {
        commit();
    }
}

void rhodium_cpld_ctrl::set_rx_input_switches(
    const rx_sw1_t rx_sw1, const cal_iso_sw_t cal_iso_sw, const bool defer_commit)
{
    UHD_LOG_TRACE("RH_CPLD",
        "Configuring RX input selection switches."
        " sw1="
            << rx_sw1 << " cal_iso=" << cal_iso_sw);

    std::lock_guard<std::mutex> l(_set_mutex);

    _regs.rx_sw1     = rhodium_cpld_regs_t::rx_sw1_t(rx_sw1);
    _regs.cal_iso_sw = rhodium_cpld_regs_t::cal_iso_sw_t(cal_iso_sw);

    if (not defer_commit) {
        commit();
    }
}

void rhodium_cpld_ctrl::set_tx_output_switches(
    const tx_sw1_t tx_sw1, const bool defer_commit)
{
    UHD_LOG_TRACE("RH_CPLD",
        "Configuring TX output selection switches."
        " sw1="
            << tx_sw1);

    std::lock_guard<std::mutex> l(_set_mutex);

    _regs.tx_sw1 = rhodium_cpld_regs_t::tx_sw1_t(tx_sw1);

    if (not defer_commit) {
        commit();
    }
}

void rhodium_cpld_ctrl::set_rx_lo_source(
    const rx_lo_input_sel_t rx_lo_input_sel, const bool defer_commit)
{
    UHD_LOG_TRACE("RH_CPLD", "Setting RX LO source to " << rx_lo_input_sel);
    std::lock_guard<std::mutex> l(_set_mutex);

    _regs.rx_lo_input_sel = rhodium_cpld_regs_t::rx_lo_input_sel_t(rx_lo_input_sel);

    if (not defer_commit) {
        commit();
    }
}

void rhodium_cpld_ctrl::set_tx_lo_source(
    const tx_lo_input_sel_t tx_lo_input_sel, const bool defer_commit)
{
    UHD_LOG_TRACE("RH_CPLD", "Setting TX LO source to " << tx_lo_input_sel);
    std::lock_guard<std::mutex> l(_set_mutex);

    _regs.tx_lo_input_sel = rhodium_cpld_regs_t::tx_lo_input_sel_t(tx_lo_input_sel);

    if (not defer_commit) {
        commit();
    }
}

void rhodium_cpld_ctrl::set_rx_lo_path(const double freq, const bool defer_commit)
{
    UHD_LOG_TRACE("RH_CPLD", "Configuring RX LO filter and settings. freq=" << freq);
    std::lock_guard<std::mutex> l(_set_mutex);

    auto freq_compare = fp_compare_epsilon<double>(freq, RHODIUM_FREQ_COMPARE_EPSILON);

    if (freq_compare < RX_DEMOD_ADJ_1500OHM_THRESHOLD) {
        _regs.rx_demod_adj =
            rhodium_cpld_regs_t::rx_demod_adj_t::RX_DEMOD_ADJ_RES_1500_OHM;
    } else if (freq_compare < RX_DEMOD_ADJ_200OHM_THRESHOLD) {
        _regs.rx_demod_adj =
            rhodium_cpld_regs_t::rx_demod_adj_t::RX_DEMOD_ADJ_RES_200_OHM;
    } else {
        _regs.rx_demod_adj = rhodium_cpld_regs_t::rx_demod_adj_t::RX_DEMOD_ADJ_RES_OPEN;
    }

    if (freq_compare < RHODIUM_LO_0_9_GHZ_LPF_THRESHOLD_FREQ) {
        _regs.rx_lo_filter_sel =
            rhodium_cpld_regs_t::rx_lo_filter_sel_t::RX_LO_FILTER_SEL_0_9GHZ_LPF;
    } else if (freq_compare < RHODIUM_LO_2_25_GHZ_LPF_THRESHOLD_FREQ) {
        _regs.rx_lo_filter_sel =
            rhodium_cpld_regs_t::rx_lo_filter_sel_t::RX_LO_FILTER_SEL_2_25GHZ_LPF;
    } else {
        _regs.rx_lo_filter_sel =
            rhodium_cpld_regs_t::rx_lo_filter_sel_t::RX_LO_FILTER_SEL_5_85GHZ_LPF;
    }

    if (not defer_commit) {
        commit();
    }
}

void rhodium_cpld_ctrl::set_tx_lo_path(const double freq, const bool defer_commit)
{
    UHD_LOG_TRACE("RH_CPLD", "Configuring TX LO filter and settings. freq=" << freq);
    std::lock_guard<std::mutex> l(_set_mutex);

    auto freq_compare = fp_compare_epsilon<double>(freq, RHODIUM_FREQ_COMPARE_EPSILON);

    if (freq_compare < RHODIUM_LO_0_9_GHZ_LPF_THRESHOLD_FREQ) {
        _regs.tx_lo_filter_sel =
            rhodium_cpld_regs_t::tx_lo_filter_sel_t::TX_LO_FILTER_SEL_0_9GHZ_LPF;
    } else if (freq_compare < RHODIUM_LO_2_25_GHZ_LPF_THRESHOLD_FREQ) {
        _regs.tx_lo_filter_sel =
            rhodium_cpld_regs_t::tx_lo_filter_sel_t::TX_LO_FILTER_SEL_2_25GHZ_LPF;
    } else {
        _regs.tx_lo_filter_sel =
            rhodium_cpld_regs_t::tx_lo_filter_sel_t::TX_LO_FILTER_SEL_5_85GHZ_LPF;
    }

    if (not defer_commit) {
        commit();
    }
}

void rhodium_cpld_ctrl::set_gain_index(const uint32_t index,
    const gain_band_t band,
    const uhd::direction_t dir,
    const bool defer_commit)
{
    UHD_ASSERT_THROW(index <= MAX_GAIN_INDEX);
    UHD_ASSERT_THROW(dir == RX_DIRECTION or dir == TX_DIRECTION);

    if (band == HIGH) {
        if (dir == RX_DIRECTION) {
            _regs.rx_gain_tbl_sel = rhodium_cpld_regs_t::RX_GAIN_TBL_SEL_HIGHBAND;
        } else {
            _regs.tx_gain_tbl_sel = rhodium_cpld_regs_t::TX_GAIN_TBL_SEL_HIGHBAND;
        }
    } else {
        if (dir == RX_DIRECTION) {
            _regs.rx_gain_tbl_sel = rhodium_cpld_regs_t::RX_GAIN_TBL_SEL_LOWBAND;
        } else {
            _regs.tx_gain_tbl_sel = rhodium_cpld_regs_t::TX_GAIN_TBL_SEL_LOWBAND;
        }
    }

    const uint8_t table_id = (dir == RX_DIRECTION) ? GAIN_CTRL_TABLE_RX
                                                   : GAIN_CTRL_TABLE_TX;

    const uint32_t cmd = (table_id << GAIN_CTRL_TABLE_FIELD)
                         | (index << GAIN_CTRL_INDEX_FIELD)
                         | (GAIN_CTRL_DSA1_WRITE_ENABLE << GAIN_CTRL_DSA1_FIELD)
                         | (GAIN_CTRL_DSA2_WRITE_ENABLE << GAIN_CTRL_DSA2_FIELD);

    std::lock_guard<std::mutex> l(_set_mutex);
    _gain_queue.emplace_back(cmd);

    if (not defer_commit) {
        commit();
    }
}

void rhodium_cpld_ctrl::set_lo_gain(
    const uint32_t index, const uhd::direction_t dir, const bool defer_commit)
{
    UHD_ASSERT_THROW(index <= MAX_LO_GAIN_INDEX);

    // The DSA has 0-30 dB of attenuation in 1 dB steps
    // This index directly controls the attenuation value of the LO DSA,
    // so reverse the gain value to write the value
    const uint32_t attenuation = MAX_LO_GAIN_INDEX - index;
    const uint8_t set_rx       = (dir == RX_DIRECTION or dir == DX_DIRECTION)
                               ? LO_GAIN_CTRL_RX_LO_WRITE_ENABLE
                               : LO_GAIN_CTRL_RX_LO_WRITE_DISABLE;
    const uint8_t set_tx = (dir == TX_DIRECTION or dir == DX_DIRECTION)
                               ? LO_GAIN_CTRL_TX_LO_WRITE_ENABLE
                               : LO_GAIN_CTRL_TX_LO_WRITE_DISABLE;

    const uint32_t cmd = (LO_GAIN_CTRL_TABLE_LO << LO_GAIN_CTRL_TABLE_FIELD)
                         | (attenuation << LO_GAIN_CTRL_INDEX_FIELD)
                         | (set_rx << LO_GAIN_CTRL_RX_LO_FIELD)
                         | (set_tx << LO_GAIN_CTRL_TX_LO_FIELD);

    std::lock_guard<std::mutex> l(_set_mutex);
    _gain_queue.emplace_back(cmd);

    if (not defer_commit) {
        commit();
    }
}


/******************************************************************************
 * Private methods
 *****************************************************************************/
void rhodium_cpld_ctrl::_loopback_test()
{
    UHD_LOG_TRACE("RH_CPLD", "Performing CPLD scratch loopback test...");
    using namespace std::chrono;
    const uint16_t random_number = // Derived from current time
        uint16_t(system_clock::to_time_t(system_clock::now()) & 0xFFFF);
    set_scratch(random_number);
    const uint16_t actual = get_scratch();
    if (actual != random_number) {
        UHD_LOGGER_ERROR("RH_CPLD")
            << "CPLD scratch loopback failed! "
            << boost::format("Expected: 0x%04X Got: 0x%04X") % random_number % actual;
        throw uhd::runtime_error("CPLD scratch loopback failed!");
    }
    UHD_LOG_TRACE("RH_CPLD", "CPLD scratch loopback test passed!");
}

void rhodium_cpld_ctrl::commit(const bool save_all)
{
    UHD_LOGGER_TRACE("RH_CPLD")
        << "Storing register cache " << (save_all ? "completely" : "selectively")
        << " to CPLD via SPI...";
    auto changed_addrs = save_all ? _regs.get_all_addrs()
                                  : _regs.get_changed_addrs<size_t>();
    for (const auto addr : changed_addrs) {
        _write_reg_fn(addr, _regs.get_reg(addr));
    }
    _regs.save_state();
    UHD_LOG_TRACE("RH_CPLD",
        "Storing cache complete: "
        "Updated "
            << changed_addrs.size() << " registers.");

    UHD_LOGGER_TRACE("RH_CPLD") << "Writing queued gain commands "
                                << "to CPLD via SPI...";
    for (const auto cmd : _gain_queue) {
        _write_raw_fn(cmd);
    }
    UHD_LOG_TRACE("RH_CPLD",
        "Writing queued gain commands complete: "
        "Wrote "
            << _gain_queue.size() << " commands.");
    _gain_queue.clear();
}
