//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhdlib/usrp/dboard/zbx/zbx_cpld_ctrl.hpp>
#include <chrono>
#include <map>
#include <thread>

namespace {
//! The time we need to wait after sending a SPI command
const uhd::time_spec_t SPI_THROTTLE_TIME = uhd::time_spec_t(2e-6);
} // namespace

namespace uhd { namespace usrp { namespace zbx {

// clang-format off
const std::unordered_map<size_t, std::unordered_map<zbx_cpld_ctrl::dsa_type, zbx_cpld_regs_t::zbx_cpld_field_t>>
    RX_DSA_CPLD_MAP
{
    {0, {
        {zbx_cpld_ctrl::dsa_type::DSA1,  zbx_cpld_regs_t::zbx_cpld_field_t::RX0_DSA1},
        {zbx_cpld_ctrl::dsa_type::DSA2,  zbx_cpld_regs_t::zbx_cpld_field_t::RX0_DSA2},
        {zbx_cpld_ctrl::dsa_type::DSA3A, zbx_cpld_regs_t::zbx_cpld_field_t::RX0_DSA3_A},
        {zbx_cpld_ctrl::dsa_type::DSA3B, zbx_cpld_regs_t::zbx_cpld_field_t::RX0_DSA3_B}
    }},
    {1, {
        {zbx_cpld_ctrl::dsa_type::DSA1,  zbx_cpld_regs_t::zbx_cpld_field_t::RX1_DSA1},
        {zbx_cpld_ctrl::dsa_type::DSA2,  zbx_cpld_regs_t::zbx_cpld_field_t::RX1_DSA2},
        {zbx_cpld_ctrl::dsa_type::DSA3A, zbx_cpld_regs_t::zbx_cpld_field_t::RX1_DSA3_A},
        {zbx_cpld_ctrl::dsa_type::DSA3B, zbx_cpld_regs_t::zbx_cpld_field_t::RX1_DSA3_B}
    }}
};

const std::unordered_map<size_t, std::unordered_map<zbx_cpld_ctrl::dsa_type, zbx_cpld_regs_t::zbx_cpld_field_t>>
    TX_DSA_CPLD_MAP
{
    {0, {
        {zbx_cpld_ctrl::dsa_type::DSA1, zbx_cpld_regs_t::zbx_cpld_field_t::TX0_DSA1},
        {zbx_cpld_ctrl::dsa_type::DSA2, zbx_cpld_regs_t::zbx_cpld_field_t::TX0_DSA2}
    }},
    {1, {
        {zbx_cpld_ctrl::dsa_type::DSA1, zbx_cpld_regs_t::zbx_cpld_field_t::TX1_DSA1},
        {zbx_cpld_ctrl::dsa_type::DSA2, zbx_cpld_regs_t::zbx_cpld_field_t::TX1_DSA2}
    }}
};
// clang-format on


const std::unordered_map<std::string, zbx_cpld_ctrl::dsa_type> zbx_cpld_ctrl::dsa_map{
    {ZBX_GAIN_STAGE_DSA1, zbx_cpld_ctrl::dsa_type::DSA1},
    {ZBX_GAIN_STAGE_DSA2, zbx_cpld_ctrl::dsa_type::DSA2},
    {ZBX_GAIN_STAGE_DSA3A, zbx_cpld_ctrl::dsa_type::DSA3A},
    {ZBX_GAIN_STAGE_DSA3B, zbx_cpld_ctrl::dsa_type::DSA3B}};

zbx_cpld_ctrl::zbx_cpld_ctrl(poke_fn_type&& poke_fn,
    peek_fn_type&& peek_fn,
    sleep_fn_type&& sleep_fn,
    const std::string& log_id)
    : _poke32(std::move(poke_fn))
    , _peek32(std::move(peek_fn))
    , _sleep(std::move(sleep_fn))
    , _lo_spi_offset(_regs.get_addr("SPI_READY"))
    , _log_id(log_id)
{
    UHD_LOG_TRACE(_log_id, "Entering CPLD ctor...");
    // Reset and stash the regs state. We can't assume the defaults in
    // gen_zbx_cpld_regs.py match what's on the hardware.
    commit(NO_CHAN, true);
    _regs.save_state();
}

void zbx_cpld_ctrl::set_scratch(const uint32_t value)
{
    _regs.SCRATCH = value;
    commit(NO_CHAN);
}

uint32_t zbx_cpld_ctrl::get_scratch()
{
    return _peek32(_regs.get_addr("SCRATCH"));
}

void zbx_cpld_ctrl::set_atr_mode(
    const size_t channel, const atr_mode_target target, const atr_mode mode)
{
    UHD_ASSERT_THROW(channel == 0 || channel == 1);
    if (target == atr_mode_target::DSA) {
        if (channel == 0) {
            _regs.RF0_DSA_OPTION = static_cast<zbx_cpld_regs_t::RF0_DSA_OPTION_t>(mode);
        } else {
            _regs.RF1_DSA_OPTION = static_cast<zbx_cpld_regs_t::RF1_DSA_OPTION_t>(mode);
        }
    } else {
        if (channel == 0) {
            _regs.RF0_OPTION = static_cast<zbx_cpld_regs_t::RF0_OPTION_t>(mode);
        } else {
            _regs.RF1_OPTION = static_cast<zbx_cpld_regs_t::RF1_OPTION_t>(mode);
        }
    }
    commit(channel == 0 ? CHAN0 : CHAN1);
}

void zbx_cpld_ctrl::set_sw_config(
    const size_t channel, const atr_mode_target target, const uint8_t rf_config)
{
    UHD_ASSERT_THROW(channel == 0 || channel == 1);
    // clang-format off
    static const std::map<std::pair<size_t, atr_mode_target>, zbx_cpld_regs_t::zbx_cpld_field_t>
        mode_map{
            {{0, atr_mode_target::PATH_LED}, zbx_cpld_regs_t::zbx_cpld_field_t::SW_RF0_CONFIG    },
            {{1, atr_mode_target::PATH_LED}, zbx_cpld_regs_t::zbx_cpld_field_t::SW_RF1_CONFIG    },
            {{0, atr_mode_target::DSA     }, zbx_cpld_regs_t::zbx_cpld_field_t::SW_RF0_DSA_CONFIG},
            {{1, atr_mode_target::DSA     }, zbx_cpld_regs_t::zbx_cpld_field_t::SW_RF1_DSA_CONFIG}
        };
    // clang-format on
    _regs.set_field(mode_map.at({channel, target}), rf_config);
    commit(channel == 0 ? CHAN0 : CHAN1);
}

uint8_t zbx_cpld_ctrl::get_current_config(
    const size_t channel, const atr_mode_target target)
{
    UHD_ASSERT_THROW(channel == 0 || channel == 1);
    const uint16_t addr       = _regs.get_addr("CURRENT_RF0_CONFIG");
    const uint32_t config_reg = _peek32(addr);
    _regs.set_reg(addr, config_reg);
    _regs.save_state();
    // clang-format off
    static const std::map<std::pair<size_t, atr_mode_target>, zbx_cpld_regs_t::zbx_cpld_field_t>
        mode_map{
            {{0, atr_mode_target::PATH_LED}, zbx_cpld_regs_t::zbx_cpld_field_t::CURRENT_RF0_CONFIG    },
            {{1, atr_mode_target::PATH_LED}, zbx_cpld_regs_t::zbx_cpld_field_t::CURRENT_RF1_CONFIG    },
            {{0, atr_mode_target::DSA},      zbx_cpld_regs_t::zbx_cpld_field_t::CURRENT_RF0_DSA_CONFIG},
            {{1, atr_mode_target::DSA},      zbx_cpld_regs_t::zbx_cpld_field_t::CURRENT_RF1_DSA_CONFIG}
        };
    // clang-format on
    return _regs.get_field(mode_map.at({channel, target}));
}

void zbx_cpld_ctrl::set_tx_gain_switches(
    const size_t channel, const uint8_t idx, const tx_dsa_type& dsa_steps)
{
    UHD_ASSERT_THROW(channel < ZBX_NUM_CHANS);

    UHD_LOG_TRACE(_log_id,
        "Set TX DSA for channel " << channel << ": DSA1=" << dsa_steps[0] << ", DSA2="
                                  << dsa_steps[1] << ", AMP=" << dsa_steps[2]);
    if (channel == 0) {
        _regs.TX0_DSA1[idx] = dsa_steps[0];
        _regs.TX0_DSA2[idx] = dsa_steps[1];
    } else if (channel == 1) {
        _regs.TX1_DSA1[idx] = dsa_steps[0];
        _regs.TX1_DSA2[idx] = dsa_steps[1];
    }
    // Correct amp path gets configured by switch_tx_antenna_switches()
    commit(channel == 0 ? CHAN0 : CHAN1);
}

void zbx_cpld_ctrl::set_rx_gain_switches(
    const size_t channel, const uint8_t idx, const rx_dsa_type& dsa_steps)
{
    UHD_LOG_TRACE(_log_id,
        "Setting RX DSA for channel "
            << channel << ": DSA1=" << dsa_steps[0] << ", DSA2=" << dsa_steps[1]
            << ", DSA3A=" << dsa_steps[2] << ", DSA3B=" << dsa_steps[3]);
    if (channel == 0) {
        _regs.RX0_DSA1[idx]   = dsa_steps[0];
        _regs.RX0_DSA2[idx]   = dsa_steps[1];
        _regs.RX0_DSA3_A[idx] = dsa_steps[2];
        _regs.RX0_DSA3_B[idx] = dsa_steps[3];
    } else if (channel == 1) {
        _regs.RX1_DSA1[idx]   = dsa_steps[0];
        _regs.RX1_DSA2[idx]   = dsa_steps[1];
        _regs.RX1_DSA3_A[idx] = dsa_steps[2];
        _regs.RX1_DSA3_B[idx] = dsa_steps[3];
    }
    commit(channel == 0 ? CHAN0 : CHAN1);
}

void zbx_cpld_ctrl::set_rx_gain_switches(
    const size_t channel, const uint8_t idx, const uint8_t table_idx)
{
    UHD_ASSERT_THROW(channel < ZBX_NUM_CHANS);
    UHD_LOG_TRACE(_log_id,
        "Setting RX DSA for channel " << channel << " from table index " << table_idx);
    if (channel == 0) {
        _regs.RX0_TABLE_SELECT[idx] = table_idx;
    } else {
        _regs.RX1_TABLE_SELECT[idx] = table_idx;
    }
    commit(channel == 0 ? CHAN0 : CHAN1);
}

void zbx_cpld_ctrl::set_tx_gain_switches(
    const size_t channel, const uint8_t idx, const uint8_t table_idx)
{
    UHD_ASSERT_THROW(channel < ZBX_NUM_CHANS);
    UHD_LOG_TRACE(_log_id,
        "Setting TX DSA for channel " << channel << " from table index " << table_idx);
    if (channel == 0) {
        _regs.TX0_TABLE_SELECT[idx] = table_idx;
    } else {
        _regs.TX1_TABLE_SELECT[idx] = table_idx;
    }
    commit(channel == 0 ? CHAN0 : CHAN1);
}

uint8_t zbx_cpld_ctrl::set_tx_dsa(
    const size_t channel, const uint8_t idx, const dsa_type tx_dsa, const uint8_t att)
{
    UHD_ASSERT_THROW(channel == 0 || channel == 1);
    UHD_ASSERT_THROW(tx_dsa == dsa_type::DSA1 || tx_dsa == dsa_type::DSA2);
    const uint8_t att_coerced = std::min(att, ZBX_TX_DSA_MAX_ATT);
    _regs.set_field(TX_DSA_CPLD_MAP.at(channel).at(tx_dsa), att_coerced, idx);
    commit(channel == 0 ? CHAN0 : CHAN1);
    return att_coerced;
}

uint8_t zbx_cpld_ctrl::set_rx_dsa(
    const size_t channel, const uint8_t idx, const dsa_type rx_dsa, const uint8_t att)
{
    UHD_ASSERT_THROW(channel == 0 || channel == 1);
    const uint8_t att_coerced = std::min(att, ZBX_RX_DSA_MAX_ATT);
    _regs.set_field(RX_DSA_CPLD_MAP.at(channel).at(rx_dsa), att_coerced, idx);
    commit(channel == 0 ? CHAN0 : CHAN1);
    return att_coerced;
}

uint8_t zbx_cpld_ctrl::get_tx_dsa(const size_t channel,
    const uint8_t idx,
    const dsa_type tx_dsa,
    const bool update_cache)
{
    UHD_ASSERT_THROW(channel == 0 || channel == 1);
    UHD_ASSERT_THROW(tx_dsa == dsa_type::DSA1 || tx_dsa == dsa_type::DSA2);
    if (update_cache) {
        update_field(TX_DSA_CPLD_MAP.at(channel).at(tx_dsa), idx);
    }
    return _regs.get_field(TX_DSA_CPLD_MAP.at(channel).at(tx_dsa), idx);
}

uint8_t zbx_cpld_ctrl::get_rx_dsa(const size_t channel,
    const uint8_t idx,
    const dsa_type rx_dsa,
    const bool update_cache)
{
    UHD_ASSERT_THROW(channel == 0 || channel == 1);
    if (update_cache) {
        update_field(RX_DSA_CPLD_MAP.at(channel).at(rx_dsa), idx);
    }
    return _regs.get_field(RX_DSA_CPLD_MAP.at(channel).at(rx_dsa), idx);
}

void zbx_cpld_ctrl::set_tx_antenna_switches(
    const size_t channel, const uint8_t idx, const std::string& antenna, const tx_amp amp)
{
    UHD_ASSERT_THROW(channel < ZBX_NUM_CHANS);
    UHD_ASSERT_THROW(
        amp == tx_amp::BYPASS || amp == tx_amp::LOWBAND || amp == tx_amp::HIGHBAND);

    // Antenna settings: TX/RX, CAL_LOOPBACK
    if (channel == 0) {
        if (antenna == ANTENNA_TXRX) {
            // clang-format off
            static const std::map<tx_amp,
                    std::pair<zbx_cpld_regs_t::TX0_ANT_11_t, zbx_cpld_regs_t::TX0_ANT_10_t>> amp_map{
                {tx_amp::BYPASS,   {zbx_cpld_regs_t::TX0_ANT_11_BYPASS_AMP,   zbx_cpld_regs_t::TX0_ANT_10_BYPASS_AMP}},
                {tx_amp::LOWBAND,  {zbx_cpld_regs_t::TX0_ANT_11_LOWBAND_AMP,  zbx_cpld_regs_t::TX0_ANT_10_LOWBAND_AMP}},
                {tx_amp::HIGHBAND, {zbx_cpld_regs_t::TX0_ANT_11_HIGHBAND_AMP, zbx_cpld_regs_t::TX0_ANT_10_HIGHBAND_AMP}}
            };
            // clang-format on
            if (idx == ATR_ADDR_TX || idx == ATR_ADDR_XX) {
                _regs.TX0_ANT_11[idx] = std::get<0>(amp_map.at(amp));
            }
            _regs.TX0_ANT_10[idx] = std::get<1>(amp_map.at(amp));
        } else if (antenna == ANTENNA_CAL_LOOPBACK) {
            _regs.TX0_ANT_10[idx] = zbx_cpld_regs_t::TX0_ANT_10_CAL_LOOPBACK;
            _regs.RX0_ANT_1[idx]  = zbx_cpld_regs_t::RX0_ANT_1_CAL_LOOPBACK;
            _regs.TX0_ANT_11[idx] = zbx_cpld_regs_t::TX0_ANT_11_BYPASS_AMP;
        } else {
            UHD_LOG_WARNING(_log_id,
                "ZBX Radio: TX Antenna setting not recognized: \"" << antenna.c_str()
                                                                   << "\"");
        }
    } else {
        // Antenna settings: TX/RX, CAL_LOOPBACK
        if (antenna == ANTENNA_TXRX) {
            // clang-format off
             static const std::map<tx_amp,
                     std::pair<zbx_cpld_regs_t::TX1_ANT_11_t, zbx_cpld_regs_t::TX1_ANT_10_t>> amp_map{
                {tx_amp::BYPASS,   {zbx_cpld_regs_t::TX1_ANT_11_BYPASS_AMP,   zbx_cpld_regs_t::TX1_ANT_10_BYPASS_AMP}},
                {tx_amp::LOWBAND,  {zbx_cpld_regs_t::TX1_ANT_11_LOWBAND_AMP,  zbx_cpld_regs_t::TX1_ANT_10_LOWBAND_AMP}},
                {tx_amp::HIGHBAND, {zbx_cpld_regs_t::TX1_ANT_11_HIGHBAND_AMP, zbx_cpld_regs_t::TX1_ANT_10_HIGHBAND_AMP}}
            };
            // clang-format on
            if (idx == ATR_ADDR_TX || idx == ATR_ADDR_XX) {
                _regs.TX1_ANT_11[idx] = std::get<0>(amp_map.at(amp));
            }
            _regs.TX1_ANT_10[idx] = std::get<1>(amp_map.at(amp));
        } else if (antenna == ANTENNA_CAL_LOOPBACK) {
            _regs.TX1_ANT_10[idx] = zbx_cpld_regs_t::TX1_ANT_10_CAL_LOOPBACK;
            _regs.RX1_ANT_1[idx]  = zbx_cpld_regs_t::RX1_ANT_1_CAL_LOOPBACK;
            _regs.TX1_ANT_11[idx] = zbx_cpld_regs_t::TX1_ANT_11_BYPASS_AMP;
        } else {
            UHD_LOG_WARNING(_log_id,
                "ZBX Radio: TX Antenna setting not recognized: \"" << antenna << "\"");
        }
    }
    commit(channel == 0 ? CHAN0 : CHAN1);
}

void zbx_cpld_ctrl::set_rx_antenna_switches(
    const size_t channel, const uint8_t idx, const std::string& antenna)
{
    UHD_ASSERT_THROW(channel < ZBX_NUM_CHANS);

    // Antenna settings: RX2, TX/RX, CAL_LOOPBACK, TERMINATION
    if (channel == 0) {
        if (antenna == ANTENNA_TXRX) {
            _regs.RX0_ANT_1[idx]  = zbx_cpld_regs_t::RX0_ANT_1_TX_RX;
            _regs.TX0_ANT_11[idx] = zbx_cpld_regs_t::TX0_ANT_11_TX_RX;
        } else if (antenna == ANTENNA_CAL_LOOPBACK) {
            _regs.RX0_ANT_1[idx]  = zbx_cpld_regs_t::RX0_ANT_1_CAL_LOOPBACK;
            _regs.TX0_ANT_10[idx] = zbx_cpld_regs_t::TX0_ANT_10_CAL_LOOPBACK;
            _regs.TX0_ANT_11[idx] = zbx_cpld_regs_t::TX0_ANT_11_BYPASS_AMP;
        } else if (antenna == ANTENNA_TERMINATION) {
            _regs.RX0_ANT_1[idx] = zbx_cpld_regs_t::RX0_ANT_1_TERMINATION;
        } else if (antenna == ANTENNA_RX) {
            _regs.RX0_ANT_1[idx] = zbx_cpld_regs_t::RX0_ANT_1_RX2;
        } else {
            UHD_LOG_WARNING(_log_id,
                "ZBX Radio: RX Antenna setting not recognized: \"" << antenna << "\"");
        }
    } else {
        if (antenna == ANTENNA_TXRX) {
            _regs.RX1_ANT_1[idx]  = zbx_cpld_regs_t::RX1_ANT_1_TX_RX;
            _regs.TX1_ANT_11[idx] = zbx_cpld_regs_t::TX1_ANT_11_TX_RX;
        } else if (antenna == ANTENNA_CAL_LOOPBACK) {
            _regs.RX1_ANT_1[idx]  = zbx_cpld_regs_t::RX1_ANT_1_CAL_LOOPBACK;
            _regs.TX1_ANT_10[idx] = zbx_cpld_regs_t::TX1_ANT_10_CAL_LOOPBACK;
            _regs.TX1_ANT_11[idx] = zbx_cpld_regs_t::TX1_ANT_11_BYPASS_AMP;
        } else if (antenna == ANTENNA_TERMINATION) {
            _regs.RX1_ANT_1[idx] = zbx_cpld_regs_t::RX1_ANT_1_TERMINATION;
        } else if (antenna == ANTENNA_RX) {
            _regs.RX1_ANT_1[idx] = zbx_cpld_regs_t::RX1_ANT_1_RX2;
        } else {
            UHD_LOG_WARNING(_log_id,
                "ZBX Radio: RX Antenna setting not recognized: \"" << antenna << "\"");
        }
    }
    commit(channel == 0 ? CHAN0 : CHAN1);
}

tx_amp zbx_cpld_ctrl::get_tx_amp_settings(
    const size_t channel, const uint8_t idx, const bool update_cache)
{
    if (channel == 0) {
        if (update_cache) {
            update_field(zbx_cpld_regs_t::zbx_cpld_field_t::TX0_ANT_10, idx);
            update_field(zbx_cpld_regs_t::zbx_cpld_field_t::TX0_ANT_11, idx);
        }
        if ((_regs.TX0_ANT_11[idx] == zbx_cpld_regs_t::TX0_ANT_11_BYPASS_AMP
                && _regs.TX0_ANT_10[idx] != zbx_cpld_regs_t::TX0_ANT_10_BYPASS_AMP)
            || (_regs.TX0_ANT_11[idx] == zbx_cpld_regs_t::TX0_ANT_11_HIGHBAND_AMP
                && _regs.TX0_ANT_10[idx] != zbx_cpld_regs_t::TX0_ANT_10_HIGHBAND_AMP)
            || (_regs.TX0_ANT_11[idx] == zbx_cpld_regs_t::TX0_ANT_11_LOWBAND_AMP
                && _regs.TX0_ANT_10[idx] != zbx_cpld_regs_t::TX0_ANT_10_LOWBAND_AMP)) {
            UHD_LOG_WARNING(
                _log_id, "Detected inconsistency in the TX amp switch settings.");
        }
        // clang-format off
        static const std::map<zbx_cpld_regs_t::TX0_ANT_10_t, tx_amp> amp_map{
            {zbx_cpld_regs_t::TX0_ANT_10_BYPASS_AMP  , tx_amp::BYPASS  },
            {zbx_cpld_regs_t::TX0_ANT_10_CAL_LOOPBACK, tx_amp::BYPASS  },
            {zbx_cpld_regs_t::TX0_ANT_10_LOWBAND_AMP , tx_amp::LOWBAND },
            {zbx_cpld_regs_t::TX0_ANT_10_HIGHBAND_AMP, tx_amp::HIGHBAND}
        };
        // clang-format on
        return amp_map.at(_regs.TX0_ANT_10[idx]);
    }
    if (channel == 1) {
        if (update_cache) {
            update_field(zbx_cpld_regs_t::zbx_cpld_field_t::TX0_ANT_10, idx);
            update_field(zbx_cpld_regs_t::zbx_cpld_field_t::TX0_ANT_11, idx);
        }
        if ((_regs.TX1_ANT_11[idx] == zbx_cpld_regs_t::TX1_ANT_11_BYPASS_AMP
                && _regs.TX1_ANT_10[idx] != zbx_cpld_regs_t::TX1_ANT_10_BYPASS_AMP)
            || (_regs.TX1_ANT_11[idx] == zbx_cpld_regs_t::TX1_ANT_11_HIGHBAND_AMP
                && _regs.TX1_ANT_10[idx] != zbx_cpld_regs_t::TX1_ANT_10_HIGHBAND_AMP)
            || (_regs.TX1_ANT_11[idx] == zbx_cpld_regs_t::TX1_ANT_11_LOWBAND_AMP
                && _regs.TX1_ANT_10[idx] != zbx_cpld_regs_t::TX1_ANT_10_LOWBAND_AMP)) {
            UHD_LOG_WARNING(
                _log_id, "Detected inconsistency in the TX amp switch settings.");
        }
        // clang-format off
        static const std::map<zbx_cpld_regs_t::TX1_ANT_10_t, tx_amp> amp_map{
            {zbx_cpld_regs_t::TX1_ANT_10_BYPASS_AMP  , tx_amp::BYPASS  },
            {zbx_cpld_regs_t::TX1_ANT_10_CAL_LOOPBACK, tx_amp::BYPASS  },
            {zbx_cpld_regs_t::TX1_ANT_10_LOWBAND_AMP , tx_amp::LOWBAND },
            {zbx_cpld_regs_t::TX1_ANT_10_HIGHBAND_AMP, tx_amp::HIGHBAND}
        };
        // clang-format on
        return amp_map.at(_regs.TX1_ANT_10[idx]);
    }
    UHD_THROW_INVALID_CODE_PATH();
}

void zbx_cpld_ctrl::set_rx_rf_filter(
    const size_t channel, const uint8_t idx, const uint8_t rf_fir)
{
    UHD_ASSERT_THROW(channel < ZBX_NUM_CHANS && rf_fir < 4);

    if (rf_fir == 0) {
        if (channel == 0) {
            _regs.RX0_4[idx] = zbx_cpld_regs_t::RX0_4_HIGHBAND;
            _regs.RX0_2[idx] = zbx_cpld_regs_t::RX0_2_HIGHBAND;
        } else {
            _regs.RX1_4[idx] = zbx_cpld_regs_t::RX1_4_HIGHBAND;
            _regs.RX1_2[idx] = zbx_cpld_regs_t::RX1_2_HIGHBAND;
        }
    } else {
        // Clang-format likes to "staircase" multiple tertiary statements, it's much
        // easier to read lined up
        // clang-format off
        if (channel == 0) {
            _regs.RX0_4[idx]     = zbx_cpld_regs_t::RX0_4_LOWBAND;
            _regs.RX0_2[idx]     = zbx_cpld_regs_t::RX0_2_LOWBAND;
            _regs.RX0_RF_11[idx] = rf_fir == 1 ? zbx_cpld_regs_t::RX0_RF_11_RF_1
                                 : rf_fir == 2 ? zbx_cpld_regs_t::RX0_RF_11_RF_2
                                               : zbx_cpld_regs_t::RX0_RF_11_RF_3;
            _regs.RX0_RF_3[idx] = rf_fir == 1 ? zbx_cpld_regs_t::RX0_RF_3_RF_1
                                : rf_fir == 2 ? zbx_cpld_regs_t::RX0_RF_3_RF_2
                                              : zbx_cpld_regs_t::RX0_RF_3_RF_3;
        } else {
            _regs.RX1_4[idx]     = zbx_cpld_regs_t::RX1_4_LOWBAND;
            _regs.RX1_2[idx]     = zbx_cpld_regs_t::RX1_2_LOWBAND;
            _regs.RX1_RF_11[idx] = rf_fir == 1 ? zbx_cpld_regs_t::RX1_RF_11_RF_1
                                 : rf_fir == 2 ? zbx_cpld_regs_t::RX1_RF_11_RF_2
                                               : zbx_cpld_regs_t::RX1_RF_11_RF_3;
            _regs.RX1_RF_3[idx] = rf_fir == 1 ? zbx_cpld_regs_t::RX1_RF_3_RF_1
                                : rf_fir == 2 ? zbx_cpld_regs_t::RX1_RF_3_RF_2
                                              : zbx_cpld_regs_t::RX1_RF_3_RF_3;
        }
        // clang-format on
    }
    commit(channel == 0 ? CHAN0 : CHAN1);
}

void zbx_cpld_ctrl::set_rx_if1_filter(
    const size_t channel, const uint8_t idx, const uint8_t if1_fir)
{
    UHD_ASSERT_THROW(channel < ZBX_NUM_CHANS && if1_fir != 0 && if1_fir < 5);

    // Clang-format likes to "staircase" multiple tertiary statements, it's much
    // easier to read lined up
    // clang-format off
    if (channel == 0) {
        _regs.RX0_IF1_5[idx] = if1_fir == 1 ? zbx_cpld_regs_t::RX0_IF1_5_FILTER_1
                        : if1_fir == 2 ? zbx_cpld_regs_t::RX0_IF1_5_FILTER_2
                        : if1_fir == 3 ? zbx_cpld_regs_t::RX0_IF1_5_FILTER_3
                                       : zbx_cpld_regs_t::RX0_IF1_5_FILTER_4;

        _regs.RX0_IF1_6[idx] = if1_fir == 1 ? zbx_cpld_regs_t::RX0_IF1_6_FILTER_1
                        : if1_fir == 2 ? zbx_cpld_regs_t::RX0_IF1_6_FILTER_2
                        : if1_fir == 3 ? zbx_cpld_regs_t::RX0_IF1_6_FILTER_3
                                       : zbx_cpld_regs_t::RX0_IF1_6_FILTER_4;
    } else {
        _regs.RX1_IF1_5[idx] = if1_fir == 1 ? zbx_cpld_regs_t::RX1_IF1_5_FILTER_1
                        : if1_fir == 2 ? zbx_cpld_regs_t::RX1_IF1_5_FILTER_2
                        : if1_fir == 3 ? zbx_cpld_regs_t::RX1_IF1_5_FILTER_3
                                       : zbx_cpld_regs_t::RX1_IF1_5_FILTER_4;

        _regs.RX1_IF1_6[idx] = if1_fir == 1 ? zbx_cpld_regs_t::RX1_IF1_6_FILTER_1
                        : if1_fir == 2 ? zbx_cpld_regs_t::RX1_IF1_6_FILTER_2
                        : if1_fir == 3 ? zbx_cpld_regs_t::RX1_IF1_6_FILTER_3
                                       : zbx_cpld_regs_t::RX1_IF1_6_FILTER_4;
    }
    // clang-format on
    commit(channel == 0 ? CHAN0 : CHAN1);
}

void zbx_cpld_ctrl::set_rx_if2_filter(
    const size_t channel, const uint8_t idx, const uint8_t if2_fir)
{
    UHD_ASSERT_THROW(channel < ZBX_NUM_CHANS && if2_fir != 0 && if2_fir < 3);

    if (channel == 0) {
        _regs.RX0_IF2_7_8[idx] = if2_fir == 1 ? zbx_cpld_regs_t::RX0_IF2_7_8_FILTER_1
                                              : zbx_cpld_regs_t::RX0_IF2_7_8_FILTER_2;
    } else {
        _regs.RX1_IF2_7_8[idx] = if2_fir == 1 ? zbx_cpld_regs_t::RX1_IF2_7_8_FILTER_1
                                              : zbx_cpld_regs_t::RX1_IF2_7_8_FILTER_2;
    }
    commit(channel == 0 ? CHAN0 : CHAN1);
}

void zbx_cpld_ctrl::set_tx_rf_filter(
    const size_t channel, const uint8_t idx, const uint8_t rf_fir)
{
    UHD_ASSERT_THROW(channel < ZBX_NUM_CHANS && rf_fir < 4);

    if (rf_fir == 0) {
        if (channel == 0) {
            _regs.TX0_RF_9[idx] = zbx_cpld_regs_t::TX0_RF_9_HIGHBAND;
            _regs.TX0_7[idx]    = zbx_cpld_regs_t::TX0_7_HIGHBAND;
        } else {
            _regs.TX1_RF_9[idx] = zbx_cpld_regs_t::TX1_RF_9_HIGHBAND;
            _regs.TX1_7[idx]    = zbx_cpld_regs_t::TX1_7_HIGHBAND;
        }
    } else {
        // Clang-format likes to "staircase" multiple tertiary statements, it's much
        // easier to read lined up
        // clang-format off
        if (channel == 0) {
            _regs.TX0_RF_9[idx] = rf_fir == 1 ? zbx_cpld_regs_t::TX0_RF_9_RF_1
                                : rf_fir == 2 ? zbx_cpld_regs_t::TX0_RF_9_RF_2
                                              : zbx_cpld_regs_t::TX0_RF_9_RF_3;

            _regs.TX0_RF_8[idx] = rf_fir == 1 ? zbx_cpld_regs_t::TX0_RF_8_RF_1
                                : rf_fir == 2 ? zbx_cpld_regs_t::TX0_RF_8_RF_2
                                              : zbx_cpld_regs_t::TX0_RF_8_RF_3;
            _regs.TX0_7[idx] = zbx_cpld_regs_t::TX0_7_LOWBAND;
        } else {
            _regs.TX1_RF_9[idx] = rf_fir == 1 ? zbx_cpld_regs_t::TX1_RF_9_RF_1
                                : rf_fir == 2 ? zbx_cpld_regs_t::TX1_RF_9_RF_2
                                              : zbx_cpld_regs_t::TX1_RF_9_RF_3;

            _regs.TX1_RF_8[idx] = rf_fir == 1 ? zbx_cpld_regs_t::TX1_RF_8_RF_1
                                : rf_fir == 2 ? zbx_cpld_regs_t::TX1_RF_8_RF_2
                                              : zbx_cpld_regs_t::TX1_RF_8_RF_3;
            _regs.TX1_7[idx] = zbx_cpld_regs_t::TX1_7_LOWBAND;
        }
        // clang-format on
    }
    commit(channel == 0 ? CHAN0 : CHAN1);
}

void zbx_cpld_ctrl::set_tx_if1_filter(
    const size_t channel, const uint8_t idx, const uint8_t if1_fir)
{
    UHD_ASSERT_THROW(channel < ZBX_NUM_CHANS && if1_fir != 0 && if1_fir < 7);

    if (if1_fir < 4) {
        // Clang-format likes to "staircase" multiple tertiary statements, it's much
        // easier to read lined up
        // clang-format off
        if (channel == 0) {
            _regs.TX0_IF1_6[idx] = zbx_cpld_regs_t::TX0_IF1_6_FILTER_0_3;
            _regs.TX0_IF1_3[idx] = zbx_cpld_regs_t::TX0_IF1_3_FILTER_0_3;
            _regs.TX0_IF1_4[idx] = if1_fir == 1 ? zbx_cpld_regs_t::TX0_IF1_4_FILTER_1
                                 : if1_fir == 2 ? zbx_cpld_regs_t::TX0_IF1_4_FILTER_2
                                                : zbx_cpld_regs_t::TX0_IF1_4_FILTER_3;

            _regs.TX0_IF1_5[idx] = if1_fir == 1 ? zbx_cpld_regs_t::TX0_IF1_5_FILTER_1
                                 : if1_fir == 2 ? zbx_cpld_regs_t::TX0_IF1_5_FILTER_2
                                                : zbx_cpld_regs_t::TX0_IF1_5_FILTER_3;
        } else {
            _regs.TX1_IF1_6[idx] = zbx_cpld_regs_t::TX1_IF1_6_FILTER_0_3;
            _regs.TX1_IF1_3[idx] = zbx_cpld_regs_t::TX1_IF1_3_FILTER_0_3;
            _regs.TX1_IF1_4[idx] = if1_fir == 1 ? zbx_cpld_regs_t::TX1_IF1_4_FILTER_1
                                 : if1_fir == 2 ? zbx_cpld_regs_t::TX1_IF1_4_FILTER_2
                                                : zbx_cpld_regs_t::TX1_IF1_4_FILTER_3;

            _regs.TX1_IF1_5[idx] = if1_fir == 1 ? zbx_cpld_regs_t::TX1_IF1_5_FILTER_1
                                 : if1_fir == 2 ? zbx_cpld_regs_t::TX1_IF1_5_FILTER_2
                                                : zbx_cpld_regs_t::TX1_IF1_5_FILTER_3;
        }
    } else {
        if (channel == 0) {
            _regs.TX0_IF1_4[idx] = zbx_cpld_regs_t::TX0_IF1_4_TERMINATION;
            _regs.TX0_IF1_5[idx] = zbx_cpld_regs_t::TX0_IF1_5_TERMINATION;
            _regs.TX0_IF1_3[idx] = if1_fir == 4 ? zbx_cpld_regs_t::TX0_IF1_3_FILTER_4
                                 : if1_fir == 5 ? zbx_cpld_regs_t::TX0_IF1_3_FILTER_5
                                                : zbx_cpld_regs_t::TX0_IF1_3_FILTER_6;

            _regs.TX0_IF1_6[idx] = if1_fir == 4 ? zbx_cpld_regs_t::TX0_IF1_6_FILTER_4
                                 : if1_fir == 5 ? zbx_cpld_regs_t::TX0_IF1_6_FILTER_5
                                                : zbx_cpld_regs_t::TX0_IF1_6_FILTER_6;
        } else {
            _regs.TX1_IF1_4[idx] = zbx_cpld_regs_t::TX1_IF1_4_TERMINATION;
            _regs.TX1_IF1_5[idx] = zbx_cpld_regs_t::TX1_IF1_5_TERMINATION;
            _regs.TX1_IF1_3[idx] = if1_fir == 4 ? zbx_cpld_regs_t::TX1_IF1_3_FILTER_4
                                 : if1_fir == 5 ? zbx_cpld_regs_t::TX1_IF1_3_FILTER_5
                                                : zbx_cpld_regs_t::TX1_IF1_3_FILTER_6;

            _regs.TX1_IF1_6[idx] = if1_fir == 4 ? zbx_cpld_regs_t::TX1_IF1_6_FILTER_4
                                 : if1_fir == 5 ? zbx_cpld_regs_t::TX1_IF1_6_FILTER_5
                                                : zbx_cpld_regs_t::TX1_IF1_6_FILTER_6;
        }
        // clang-format on
    }
    commit(channel == 0 ? CHAN0 : CHAN1);
}

void zbx_cpld_ctrl::set_tx_if2_filter(
    const size_t channel, const uint8_t idx, const uint8_t if2_fir)
{
    UHD_ASSERT_THROW(channel < ZBX_NUM_CHANS && if2_fir != 0 && if2_fir < 3);

    if (channel == 0) {
        _regs.TX0_IF2_1_2[idx] = if2_fir == 1 ? zbx_cpld_regs_t::TX0_IF2_1_2_FILTER_1
                                              : zbx_cpld_regs_t::TX0_IF2_1_2_FILTER_2;
    } else {
        _regs.TX1_IF2_1_2[idx] = if2_fir == 1 ? zbx_cpld_regs_t::TX1_IF2_1_2_FILTER_1
                                              : zbx_cpld_regs_t::TX1_IF2_1_2_FILTER_2;
    }
    commit(channel == 0 ? CHAN0 : CHAN1);
}

/******************************************************************************
 * LED control
 *****************************************************************************/
void zbx_cpld_ctrl::set_leds(const size_t channel,
    const uint8_t idx,
    const bool rx,
    const bool trx_rx,
    const bool trx_tx)
{
    UHD_ASSERT_THROW(channel < ZBX_NUM_CHANS);
    if (channel == 0) {
        _regs.RX0_RX_LED[idx] = rx ? zbx_cpld_regs_t::RX0_RX_LED_ENABLE
                                   : zbx_cpld_regs_t::RX0_RX_LED_DISABLE;
        _regs.RX0_TRX_LED[idx] = trx_rx ? zbx_cpld_regs_t::RX0_TRX_LED_ENABLE
                                        : zbx_cpld_regs_t::RX0_TRX_LED_DISABLE;
        _regs.TX0_TRX_LED[idx] = trx_tx ? zbx_cpld_regs_t::TX0_TRX_LED_ENABLE
                                        : zbx_cpld_regs_t::TX0_TRX_LED_DISABLE;
    } else {
        _regs.RX1_RX_LED[idx] = rx ? zbx_cpld_regs_t::RX1_RX_LED_ENABLE
                                   : zbx_cpld_regs_t::RX1_RX_LED_DISABLE;
        _regs.RX1_TRX_LED[idx] = trx_rx ? zbx_cpld_regs_t::RX1_TRX_LED_ENABLE
                                        : zbx_cpld_regs_t::RX1_TRX_LED_DISABLE;
        _regs.TX1_TRX_LED[idx] = trx_tx ? zbx_cpld_regs_t::TX1_TRX_LED_ENABLE
                                        : zbx_cpld_regs_t::TX1_TRX_LED_DISABLE;
    }
    commit(channel == 0 ? CHAN0 : CHAN1);
}

/******************************************************************************
 * LO control
 *****************************************************************************/
void zbx_cpld_ctrl::lo_poke16(const zbx_lo_t lo, const uint8_t addr, const uint16_t data)
{
    _lo_spi_transact(lo, addr, data, spi_xact_t::WRITE, true);
    // We always sleep here, in the assumption that the next poke to the CPLD is
    // also a
    // SPI transaction.
    // Note that this causes minor inefficiencies when stacking SPI writes with
    // other, non-SPI pokes (because the last SPI poke will still be followed by
    // a sleep, which isn't even necessary). If this becomes an issue, this
    // function can be changed to include a flag as an argument whether or not
    // to throttle.
}

uint16_t zbx_cpld_ctrl::lo_peek16(const zbx_lo_t lo, const uint8_t addr)
{
    _lo_spi_transact(lo, addr, 0, spi_xact_t::READ, true);
    // Now poll the LO_SPI_READY register until we have good return value
    const auto timeout = std::chrono::steady_clock::now()
                         + std::chrono::milliseconds(ZBX_LO_LOCK_TIMEOUT_MS);
    while (std::chrono::steady_clock::now() < timeout) {
        _regs.set_reg(_lo_spi_offset, _peek32(_lo_spi_offset));
        if (_regs.DATA_VALID) {
            break;
        }
    }

    // Mark this register clean again
    _regs.save_state();
    if (!_regs.DATA_VALID) {
        const std::string err_msg =
            "Unable to read back from LO SPI! Transaction timed out after "
            + std::to_string(ZBX_LO_LOCK_TIMEOUT_MS) + " ms.";
        UHD_LOG_ERROR(_log_id, err_msg);
        throw uhd::io_error(err_msg);
    }
    // The read worked. Now we run some sanity checks to make sure we got what
    // we expected
    UHD_ASSERT_THROW(_regs.ADDRESS == addr);
    UHD_ASSERT_THROW(_regs.LO_SELECT == zbx_cpld_regs_t::LO_SELECT_t(lo));
    // All good, return the read value
    return _regs.DATA;
}

bool zbx_cpld_ctrl::lo_spi_ready()
{
    return _peek32(_lo_spi_offset) & (1 << 30);
}

void zbx_cpld_ctrl::set_lo_source(
    const size_t idx, const zbx_lo_t lo, const zbx_lo_source_t lo_source)
{
    // LO source is either internal or external
    const bool internal = lo_source == zbx_lo_source_t::internal;
    switch (lo) {
        case zbx_lo_t::TX0_LO1:
            _regs.TX0_LO_14[idx] = internal ? zbx_cpld_regs_t::TX0_LO_14_INTERNAL
                                            : zbx_cpld_regs_t::TX0_LO_14_EXTERNAL;
            break;
        case zbx_lo_t::TX0_LO2:
            _regs.TX0_LO_13[idx] = internal ? zbx_cpld_regs_t::TX0_LO_13_INTERNAL
                                            : zbx_cpld_regs_t::TX0_LO_13_EXTERNAL;
            break;
        case zbx_lo_t::TX1_LO1:
            _regs.TX1_LO_14[idx] = internal ? zbx_cpld_regs_t::TX1_LO_14_INTERNAL
                                            : zbx_cpld_regs_t::TX1_LO_14_EXTERNAL;
            break;
        case zbx_lo_t::TX1_LO2:
            _regs.TX1_LO_13[idx] = internal ? zbx_cpld_regs_t::TX1_LO_13_INTERNAL
                                            : zbx_cpld_regs_t::TX1_LO_13_EXTERNAL;
            break;
        case zbx_lo_t::RX0_LO1:
            _regs.RX0_LO_9[idx] = internal ? zbx_cpld_regs_t::RX0_LO_9_INTERNAL
                                           : zbx_cpld_regs_t::RX0_LO_9_EXTERNAL;
            break;
        case zbx_lo_t::RX0_LO2:
            _regs.RX0_LO_10[idx] = internal ? zbx_cpld_regs_t::RX0_LO_10_INTERNAL
                                            : zbx_cpld_regs_t::RX0_LO_10_EXTERNAL;
            break;
        case zbx_lo_t::RX1_LO1:
            _regs.RX1_LO_9[idx] = internal ? zbx_cpld_regs_t::RX1_LO_9_INTERNAL
                                           : zbx_cpld_regs_t::RX1_LO_9_EXTERNAL;
            break;
        case zbx_lo_t::RX1_LO2:
            _regs.RX1_LO_10[idx] = internal ? zbx_cpld_regs_t::RX1_LO_10_INTERNAL
                                            : zbx_cpld_regs_t::RX1_LO_10_EXTERNAL;
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
    if (lo == zbx_lo_t::TX0_LO1 || lo == zbx_lo_t::TX0_LO2 || lo == zbx_lo_t::RX0_LO1
        || lo == zbx_lo_t::RX0_LO2) {
        commit(CHAN0);
    } else {
        commit(CHAN1);
    }
}

zbx_lo_source_t zbx_cpld_ctrl::get_lo_source(const size_t idx, zbx_lo_t lo)
{
    switch (lo) {
        case zbx_lo_t::TX0_LO1:
            return _regs.TX0_LO_14[idx] == zbx_cpld_regs_t::TX0_LO_14_INTERNAL
                       ? zbx_lo_source_t::internal
                       : zbx_lo_source_t::external;
        case zbx_lo_t::TX0_LO2:
            return _regs.TX0_LO_13[idx] == zbx_cpld_regs_t::TX0_LO_13_INTERNAL
                       ? zbx_lo_source_t::internal
                       : zbx_lo_source_t::external;
        case zbx_lo_t::TX1_LO1:
            return _regs.TX1_LO_14[idx] == zbx_cpld_regs_t::TX1_LO_14_INTERNAL
                       ? zbx_lo_source_t::internal
                       : zbx_lo_source_t::external;
        case zbx_lo_t::TX1_LO2:
            return _regs.TX1_LO_13[idx] == zbx_cpld_regs_t::TX1_LO_13_INTERNAL
                       ? zbx_lo_source_t::internal
                       : zbx_lo_source_t::external;
        case zbx_lo_t::RX0_LO1:
            return _regs.RX0_LO_9[idx] == zbx_cpld_regs_t::RX0_LO_9_INTERNAL
                       ? zbx_lo_source_t::internal
                       : zbx_lo_source_t::external;
        case zbx_lo_t::RX0_LO2:
            return _regs.RX0_LO_10[idx] == zbx_cpld_regs_t::RX0_LO_10_INTERNAL
                       ? zbx_lo_source_t::internal
                       : zbx_lo_source_t::external;
        case zbx_lo_t::RX1_LO1:
            return _regs.RX1_LO_9[idx] == zbx_cpld_regs_t::RX1_LO_9_INTERNAL
                       ? zbx_lo_source_t::internal
                       : zbx_lo_source_t::external;
        case zbx_lo_t::RX1_LO2:
            return _regs.RX1_LO_10[idx] == zbx_cpld_regs_t::RX1_LO_10_INTERNAL
                       ? zbx_lo_source_t::internal
                       : zbx_lo_source_t::external;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

void zbx_cpld_ctrl::pulse_lo_sync(const size_t ref_chan, const std::vector<zbx_lo_t>& los)
{
    if (_regs.BYPASS_SYNC_REGISTER == zbx_cpld_regs_t::BYPASS_SYNC_REGISTER_ENABLE) {
        const std::string err_msg = "Cannot pulse LO SYNC when bypass is enabled!";
        UHD_LOG_ERROR(_log_id, err_msg);
        throw uhd::runtime_error(_log_id + err_msg);
    }
    // Assert a 1 for all LOs to be sync'd
    static const std::unordered_map<zbx_lo_t, zbx_cpld_regs_t::zbx_cpld_field_t>
        lo_pulse_map{{
            {zbx_lo_t::TX0_LO1, zbx_cpld_regs_t::zbx_cpld_field_t::PULSE_TX0_LO1_SYNC},
            {zbx_lo_t::TX0_LO2, zbx_cpld_regs_t::zbx_cpld_field_t::PULSE_TX0_LO2_SYNC},
            {zbx_lo_t::TX1_LO1, zbx_cpld_regs_t::zbx_cpld_field_t::PULSE_TX1_LO1_SYNC},
            {zbx_lo_t::TX1_LO2, zbx_cpld_regs_t::zbx_cpld_field_t::PULSE_TX1_LO2_SYNC},
            {zbx_lo_t::RX0_LO1, zbx_cpld_regs_t::zbx_cpld_field_t::PULSE_RX0_LO1_SYNC},
            {zbx_lo_t::RX0_LO2, zbx_cpld_regs_t::zbx_cpld_field_t::PULSE_RX0_LO2_SYNC},
            {zbx_lo_t::RX1_LO1, zbx_cpld_regs_t::zbx_cpld_field_t::PULSE_RX1_LO1_SYNC},
            {zbx_lo_t::RX1_LO2, zbx_cpld_regs_t::zbx_cpld_field_t::PULSE_RX1_LO2_SYNC},
        }};
    for (const auto lo : los) {
        _regs.set_field(lo_pulse_map.at(lo), 1);
    }
    commit(ref_chan == 0 ? CHAN0 : CHAN1);
    // The bits are strobed, they self-clear. We reflect that here by resetting
    // them without another commit:
    for (const auto lo_it : lo_pulse_map) {
        _regs.set_field(lo_it.second, 0);
    }
    _regs.save_state();
}

void zbx_cpld_ctrl::set_lo_sync_bypass(const bool enable)
{
    _regs.BYPASS_SYNC_REGISTER = enable ? zbx_cpld_regs_t::BYPASS_SYNC_REGISTER_ENABLE
                                        : zbx_cpld_regs_t::BYPASS_SYNC_REGISTER_DISABLE;
    commit(NO_CHAN);
}

void zbx_cpld_ctrl::update_tx_dsa_settings(
    const std::vector<uint32_t>& dsa1_table, const std::vector<uint32_t>& dsa2_table)
{
    write_register_vector("TX0_TABLE_DSA1", dsa1_table);
    write_register_vector("TX0_TABLE_DSA2", dsa2_table);
    write_register_vector("TX1_TABLE_DSA1", dsa1_table);
    write_register_vector("TX1_TABLE_DSA2", dsa2_table);
    commit(NO_CHAN);
}

void zbx_cpld_ctrl::update_rx_dsa_settings(const std::vector<uint32_t>& dsa1_table,
    const std::vector<uint32_t>& dsa2_table,
    const std::vector<uint32_t>& dsa3a_table,
    const std::vector<uint32_t>& dsa3b_table)
{
    write_register_vector("RX0_TABLE_DSA1", dsa1_table);
    write_register_vector("RX0_TABLE_DSA2", dsa2_table);
    write_register_vector("RX0_TABLE_DSA3_A", dsa3a_table);
    write_register_vector("RX0_TABLE_DSA3_B", dsa3b_table);
    write_register_vector("RX1_TABLE_DSA1", dsa1_table);
    write_register_vector("RX1_TABLE_DSA2", dsa2_table);
    write_register_vector("RX1_TABLE_DSA3_A", dsa3a_table);
    write_register_vector("RX1_TABLE_DSA3_B", dsa3b_table);
    commit(NO_CHAN);
}

/******************************************************************************
 * Private methods
 *****************************************************************************/
void zbx_cpld_ctrl::_lo_spi_transact(const zbx_lo_t lo,
    const uint8_t addr,
    const uint16_t data,
    const spi_xact_t xact_type,
    const bool throttle)
{
    // Look up the channel based on the LO, so we can load the correct command
    // time for the poke
    const chan_t chan = (lo == zbx_lo_t::TX0_LO1 || lo == zbx_lo_t::TX0_LO2
                            || lo == zbx_lo_t::RX0_LO1 || lo == zbx_lo_t::RX0_LO2)
                            ? CHAN0
                            : CHAN1;
    // Note: For SPI transactions, we can't also be lugging around other
    // registers. This means that we assume that the state of _regs is clean.
    _regs.ADDRESS   = addr;
    _regs.DATA      = data;
    _regs.READ_FLAG = (xact_type == spi_xact_t::WRITE) ? zbx_cpld_regs_t::READ_FLAG_WRITE
                                                       : zbx_cpld_regs_t::READ_FLAG_READ;
    _regs.LO_SELECT         = zbx_cpld_regs_t::LO_SELECT_t(lo);
    _regs.START_TRANSACTION = zbx_cpld_regs_t::START_TRANSACTION_ENABLE;
    _poke32(_lo_spi_offset, _regs.get_reg(_lo_spi_offset), chan);
    _regs.START_TRANSACTION = zbx_cpld_regs_t::START_TRANSACTION_DISABLE;
    _regs.save_state();
    // Write complete. Now we need to send a sleep to throttle the SPI
    // transactions:
    if (throttle) {
        _sleep(SPI_THROTTLE_TIME);
    }
}

void zbx_cpld_ctrl::write_register_vector(
    const std::string& reg_addr_name, const std::vector<uint32_t>& values)
{
    UHD_LOG_DEBUG(
        _log_id, "Write " << values.size() << " values to register " << reg_addr_name);
    zbx_cpld_regs_t::zbx_cpld_field_t type = _regs.get_field_type(reg_addr_name);
    if (values.size() > _regs.get_array_size(type)) {
        const std::string err_msg = "Number of values passed for register vector("
                                    + std::to_string(values.size())
                                    + ") exceeds size of register ("
                                    + std::to_string(_regs.get_array_size(type)) + ")";
        UHD_LOG_ERROR(_log_id, err_msg);
        throw uhd::runtime_error(err_msg);
    }
    for (size_t i = 0; i < values.size(); i++) {
        _regs.set_field(type, values[i], i);
    }
}

void zbx_cpld_ctrl::commit(const chan_t chan, const bool save_all)
{
    UHD_LOG_TRACE(_log_id,
        "Storing register cache " << (save_all ? "completely" : "selectively")
                                  << " to CPLD...");
    const auto changed_addrs = save_all ? _regs.get_all_addrs<size_t>()
                                        : _regs.get_changed_addrs<size_t>();
    for (const auto addr : changed_addrs) {
        _poke32(addr, _regs.get_reg(addr), save_all ? NO_CHAN : chan);
    }
    _regs.save_state();
    UHD_LOG_TRACE(_log_id,
        "Storing cache complete: "
        "Updated "
            << changed_addrs.size() << " registers.");
}

void zbx_cpld_ctrl::update_field(
    const zbx_cpld_regs_t::zbx_cpld_field_t field, const size_t idx)
{
    const uint16_t addr     = _regs.get_addr(field) + 4 * idx;
    const uint32_t chip_val = _peek32(addr);
    _regs.set_reg(addr, chip_val);
    const auto changed_addrs = _regs.get_changed_addrs<size_t>();
    // If this is the only change in our register stack, then we call save_state()
    // because we don't want to write this value we just read from the CPLD back
    // to it. However, if there are other changes queued up, we'll have to wait
    // until the next commit() call. If this is not desired, we need to update
    // the regmap code to selectively save state.
    if (changed_addrs.empty()
        || (changed_addrs.size() == 1 && changed_addrs.count(addr))) {
        _regs.save_state();
    } else {
        UHD_LOG_DEBUG(_log_id,
            "Not saving register state after calling update_field(). This may "
            "cause unnecessary writes in the future.");
    }
}

}}} // namespace uhd::usrp::zbx
