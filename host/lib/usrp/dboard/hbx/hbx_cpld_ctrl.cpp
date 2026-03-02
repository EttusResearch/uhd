//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/log.hpp>
#include <uhd/utils/scope_exit.hpp>
#include <uhdlib/usrp/dboard/hbx/hbx_cpld_ctrl.hpp>
#include <chrono>
#include <map>
#include <thread>

namespace uhd { namespace usrp { namespace hbx {

hbx_cpld_ctrl::hbx_cpld_ctrl(poke_fn_type&& poke_fn,
    peek_fn_type&& peek_fn,
    sleep_fn_type&& sleep_fn,
    const std::string& log_id)
    : _poke32(std::move(poke_fn))
    , _peek32(std::move(peek_fn))
    , _sleep(std::move(sleep_fn))
    , _log_id(log_id)
{
    UHD_LOG_TRACE(_log_id, "Entering hbx_cpld_ctrl ctor...");
    // First check if we can talk to the CPLD at all by poking and peeking the scratch
    // register.
    const uint32_t random_value = static_cast<uint32_t>(time(NULL));
    _poke32(_regs.get_addr("SCRATCH_REG"), random_value, NO_CHAN);
    if (_peek32(_regs.get_addr("SCRATCH_REG")) != random_value)
        UHD_LOG_THROW(uhd::runtime_error,
            _log_id,
            "Failed to communicate with daughterboard CPLD controller! This may indicate "
            "a hardware issue.");
    UHD_LOG_TRACE(_log_id, "CPLD communication successful.");

    // We don't use the power detector feature, so disable it.
    // These are latching registers, so we need to write them directly.
    _poke32(_regs.get_addr("RX_RF_PD_CONTROL_DISABLE"), 0x2, NO_CHAN);
    // Then reset just in case it has triggered previously.
    _poke32(_regs.get_addr("RX_RF_PD_OVERLOAD_RESET"), 0x4, NO_CHAN);

    // Now that we know that the communication works and the power detector is off, we can
    // put the CPLD into a known state by writing the default values from our software
    // regmap.
    commit(true);
    _regs.save_state();

    // Set RX to termination in IDLE state on startup
    set_rx_antenna_switches(ATR_ADDR_0X, rx_ant_t::TERM, false);

    // Set ATR mode to classic on startup
    set_atr_mode(hbx_cpld_ctrl::atr_t::CLASSIC_ATR);
}

void hbx_cpld_ctrl::set_atr_mode(hbx_cpld_ctrl::atr_t mode)
{
    if (mode == hbx_cpld_ctrl::atr_t::CLASSIC_ATR) {
        _regs.RF0_OPTION = _regs.RF0_OPTION_CLASSIC_ATR;
    } else if (mode == hbx_cpld_ctrl::atr_t::SW_DEFINED) {
        _regs.RF0_OPTION = _regs.RF0_OPTION_SW_DEFINED;
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
    commit();
}

void hbx_cpld_ctrl::set_atr_state(const uint8_t idx)
{
    _regs.SW_RF0_CONFIG = idx;
    commit();
}

uint16_t hbx_cpld_ctrl::get_addr(const std::string& field)
{
    return _regs.get_addr(field);
}

void hbx_cpld_ctrl::mixer_init_callback(const init_t step, const direction_t trx)
{
    if (trx == TX_DIRECTION) {
        if (step == FINALIZE) {
            _regs.TX_ADMV_EN = 1;
            commit();
        } else {
            _regs.TX_ADMV_EN = 0;
            commit();
            _regs.TX_ADMV_RST = 1;
            commit();
            _regs.TX_ADMV_RST = 0;
            commit();
        }
    } else {
        if (step == FINALIZE) {
            _regs.RX_ADMV_EN = 1;
            commit();
        } else {
            _regs.RX_ADMV_EN = 0;
            commit();
            _regs.RX_ADMV_RST = 1;
            commit();
            _regs.RX_ADMV_RST = 0;
            commit();
        }
    }
}

void hbx_cpld_ctrl::set_tx_antenna_switches(
    const uint8_t idx, const bool loopback, const bool tx)
{
    _regs.TX_RF_SW_LOOP[idx] = loopback ? _regs.TX_RF_SW_LOOP_LOOPBACK
                                        : _regs.TX_RF_SW_LOOP_ANTENNA;
    _regs.TX_RF_SW_TRX[idx]  = tx ? _regs.TX_RF_SW_TRX_TX : _regs.TX_RF_SW_TRX_RX;

    commit();
}

void hbx_cpld_ctrl::set_rx_antenna_switches(
    const uint8_t idx, const rx_ant_t setting, const bool sync_inj)
{
    switch (setting) {
        case rx_ant_t::LOOPBACK:
            _regs.RX_SW_TRX[idx] = _regs.RX_SW_TRX_LOOPBACK;
            break;
        case rx_ant_t::TRX:
            _regs.RX_SW_TRX[idx] = _regs.RX_SW_TRX_TRX;
            break;
        case rx_ant_t::RX:
            _regs.RX_SW_TRX[idx] = _regs.RX_SW_TRX_RX;
            break;
        case rx_ant_t::TERM:
            _regs.RX_SW_TRX[idx] = _regs.RX_SW_TRX_TERMINATION;
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();

            break;
    }
    _regs.RX_DIRECT_SW1[idx] = sync_inj ? _regs.RX_DIRECT_SW1_SYNC_INJ
                                        : _regs.RX_DIRECT_SW1_RF;

    commit();
}

void hbx_cpld_ctrl::set_lo_switches_and_leds(
    const bool lo_export, const bool lo_import, const direction_t trx)
{
    if (trx == RX_DIRECTION) {
        // Switches for RX LO
        for (auto idx : ATR_ADDRS) {
            _regs.RX_LO_IN_SW[idx] = lo_import ? _regs.RX_LO_IN_SW_EXTERNAL
                                               : _regs.RX_LO_IN_SW_INTERNAL;
            // LEDs for RX LO
            _regs.RX_LO_LED[idx]   = lo_export ? 1 : 0;
            _regs.RX_LO_LED_G[idx] = lo_import ? 1 : 0;
        }
    } else if (trx == TX_DIRECTION) {
        // Switches for TX LO
        for (auto idx : ATR_ADDRS) {
            _regs.TX_LO_IN_SW[idx] = lo_import ? _regs.TX_LO_IN_SW_EXTERNAL
                                               : _regs.TX_LO_IN_SW_INTERNAL;
            // LEDs for TX LO
            _regs.TX_LO_LED[idx]   = lo_export ? 1 : 0;
            _regs.TX_LO_LED_G[idx] = lo_import ? 1 : 0;
        }
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
    commit();
}

void hbx_cpld_ctrl::set_leds(
    const uint8_t idx, const bool rx, const bool trx_rx, const bool trx_tx)
{
    _regs.RX1_LED_G[idx]  = rx;
    _regs.TXRX_LED[idx]   = trx_tx;
    _regs.TXRX_LED_G[idx] = trx_rx;
    commit();
}

void hbx_cpld_ctrl::set_tx_rf_path_switches(const hbx_tune_map_item_t& tune_map_item)
{
    std::lock_guard<std::mutex> lock(_tx_lo_sw_mutex);
    for (const auto& idx : ATR_ADDRS) {
        set_tx_path_switches(idx, tune_map_item.rf_band, tune_map_item.rf_filter_branch);
        set_tx_lo_switches(idx, tune_map_item.rf_band, tune_map_item.lo_filter_branch);
    }

    // Commit the changes to the CPLD registers
    commit();
}

void hbx_cpld_ctrl::set_tx_path_switches(
    const uint8_t idx, const uint8_t tx_band, const uint8_t tx_filter_branch)
{
    // Set the TX_BB_SW1 and other registers based on the tx_band
    switch (tx_band) {
        case 0:
            // For tx_band == 0, set TX_LF_SW1 to BAND_0 and TX_RF_SW_LF to BAND_0_1
            _regs.TX_LF_SW1[idx]   = hbx_cpld_regs_t::TX_LF_SW1_BAND_0;
            _regs.TX_RF_SW_LF[idx] = hbx_cpld_regs_t::TX_RF_SW_LF_BAND_0_1;
            break;

        case 1:
            // For tx_band == 1, set TX_LF_SW1 to BAND_1, TX_RF_SW_LF to BAND_0_1, and
            // TX_BB_SW1 to BAND_1
            _regs.TX_LF_SW1[idx]   = hbx_cpld_regs_t::TX_LF_SW1_BAND_1;
            _regs.TX_RF_SW_LF[idx] = hbx_cpld_regs_t::TX_RF_SW_LF_BAND_0_1;
            _regs.TX_BB_SW1[idx]   = hbx_cpld_regs_t::TX_BB_SW1_BAND_1;
            set_tx_filter_branch(idx, tx_band, tx_filter_branch);
            break;

        case 2:
            // For tx_band == 2, set required registers
            _regs.TX_RF_SW_LF[idx] = hbx_cpld_regs_t::TX_RF_SW_LF_BAND_2_3;
            _regs.TX_BB_SW1[idx]   = hbx_cpld_regs_t::TX_BB_SW1_BAND_2_3;
            _regs.TX_BB_SW2[idx]   = hbx_cpld_regs_t::TX_BB_SW2_BAND_2;
            set_tx_filter_branch(
                idx, tx_band, tx_filter_branch); // ADMV1320 control wrapper
            break;

        case 3:
            // For tx_band == 3, set required registers
            _regs.TX_RF_SW_LF[idx] = hbx_cpld_regs_t::TX_RF_SW_LF_BAND_2_3;
            _regs.TX_BB_SW1[idx]   = hbx_cpld_regs_t::TX_BB_SW1_BAND_2_3;
            _regs.TX_BB_SW2[idx]   = hbx_cpld_regs_t::TX_BB_SW2_BAND_3;
            set_tx_filter_branch(
                idx, tx_band, tx_filter_branch); // ADMV1320 control wrapper
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

void hbx_cpld_ctrl::set_tx_filter_branch(
    const uint8_t idx, const uint8_t tx_band, const uint8_t tx_filter_branch)
{
    switch (tx_band) {
        case 1:
            switch (tx_filter_branch) {
                case 1:
                    _regs.TX_B1_SW1[idx] = hbx_cpld_regs_t::TX_B1_SW1_FILTER_1;
                    _regs.TX_B1_SW2[idx] = hbx_cpld_regs_t::TX_B1_SW2_FILTER_1;
                    break;
                case 2:
                    _regs.TX_B1_SW1[idx] = hbx_cpld_regs_t::TX_B1_SW1_FILTER_2;
                    _regs.TX_B1_SW2[idx] = hbx_cpld_regs_t::TX_B1_SW2_FILTER_2;
                    break;
                case 3:
                    _regs.TX_B1_SW1[idx] = hbx_cpld_regs_t::TX_B1_SW1_FILTER_3;
                    _regs.TX_B1_SW2[idx] = hbx_cpld_regs_t::TX_B1_SW2_FILTER_3;
                    break;
                case 4:
                    _regs.TX_B1_SW1[idx] = hbx_cpld_regs_t::TX_B1_SW1_FILTER_4;
                    _regs.TX_B1_SW2[idx] = hbx_cpld_regs_t::TX_B1_SW2_FILTER_4;
                    break;
                case 5:
                    _regs.TX_B1_SW1[idx] = hbx_cpld_regs_t::TX_B1_SW1_FILTER_5;
                    _regs.TX_B1_SW2[idx] = hbx_cpld_regs_t::TX_B1_SW2_FILTER_5;
                    break;
                case 6:
                    _regs.TX_B1_SW1[idx] = hbx_cpld_regs_t::TX_B1_SW1_FILTER_6;
                    _regs.TX_B1_SW2[idx] = hbx_cpld_regs_t::TX_B1_SW2_FILTER_6;
                    break;
                case 7:
                    _regs.TX_B1_SW1[idx] = hbx_cpld_regs_t::TX_B1_SW1_FILTER_7;
                    _regs.TX_B1_SW2[idx] = hbx_cpld_regs_t::TX_B1_SW2_FILTER_7;
                    break;
                case 8:
                    _regs.TX_B1_SW1[idx] = hbx_cpld_regs_t::TX_B1_SW1_FILTER_8;
                    _regs.TX_B1_SW2[idx] = hbx_cpld_regs_t::TX_B1_SW2_FILTER_8;
                    break;
                default:
                    UHD_THROW_INVALID_CODE_PATH();
            }
            break;
        case 2:
            // ADMV 1320 driver control takes care of band 2.
            break;
        case 3:
            // ADMV 1320 driver control takes care of band 3.
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

void hbx_cpld_ctrl::set_tx_lo_switches(const uint8_t idx,
    const uint8_t tx_band,
    const std::vector<uint8_t>& lo_filter_branch)
{
    switch (tx_band) {
        case 0:
            // For tx_band 0, LO is not used
            break;

        case 1:
            // For tx_band == 1, set TX_LO_SW1 and TX_LO_B1_FB
            _regs.TX_LO_SW1[idx] = hbx_cpld_regs_t::TX_LO_SW1_BAND_1;
            set_tx_lo_filter_branch(idx, tx_band, lo_filter_branch);
            break;

        case 2:
            // For tx_band == 2, set TX_LO_B23_SW2, TX_LO_B23_SW1, TX_LO_SW1, and
            // TX_LO_SBHM_FB
            _regs.TX_LO_B23_SW2[idx] = hbx_cpld_regs_t::TX_LO_B23_SW2_BAND_2;
            set_tx_lo_filter_branch(idx, tx_band, lo_filter_branch);
            break;

        case 3:
            // For tx_band == 3, set TX_LO_B23_SW2, TX_LO_B23_SW1, TX_LO_SW1, and
            // TX_LO_SBHM_FB
            _regs.TX_LO_B23_SW2[idx] = hbx_cpld_regs_t::TX_LO_B23_SW2_BAND_3;
            set_tx_lo_filter_branch(idx, tx_band, lo_filter_branch);
            break;

        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

void hbx_cpld_ctrl::set_tx_lo_filter_branch(const uint8_t idx,
    const uint8_t tx_band,
    const std::vector<uint8_t>& lo_filter_branch)
{
    if (tx_band == 1) {
        // For tx_band == 1, lo_filter_branch must contain exactly 1 element
        UHD_ASSERT_THROW(lo_filter_branch.size() == 1);

        switch (lo_filter_branch[0]) {
            case 1:
                _regs.TX_LO_B1_FB[idx] = hbx_cpld_regs_t::TX_LO_B1_FB_FILTER_1;
                break;
            case 2:
                _regs.TX_LO_B1_FB[idx] = hbx_cpld_regs_t::TX_LO_B1_FB_FILTER_2;
                break;
            case 3:
                _regs.TX_LO_B1_FB[idx] = hbx_cpld_regs_t::TX_LO_B1_FB_FILTER_3;
                break;
            case 4:
                _regs.TX_LO_B1_FB[idx] = hbx_cpld_regs_t::TX_LO_B1_FB_FILTER_4;
                break;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    } else if (tx_band == 2 || tx_band == 3) {
        // For tx_band == 2 or 3, lo_filter_branch must contain exactly 2 elements
        UHD_ASSERT_THROW(lo_filter_branch.size() == 2);

        // Set TX_LO_B23_SW1 based on the first element
        switch (lo_filter_branch[0]) {
            case 1:
                _regs.TX_LO_B23_SW1[idx] = hbx_cpld_regs_t::TX_LO_B23_SW1_FILTER_BANK_1;
                _regs.TX_LO_SW1[idx]     = hbx_cpld_regs_t::TX_LO_SW1_FILTER_BANK_1;
                break;
            case 2:
                _regs.TX_LO_B23_SW1[idx] = hbx_cpld_regs_t::TX_LO_B23_SW1_FILTER_BANK_2;
                _regs.TX_LO_SW1[idx]     = hbx_cpld_regs_t::TX_LO_SW1_FILTER_BANK_2;
                break;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }

        // Set TX_LO_SBHM_FB based on the second element
        switch (lo_filter_branch[1]) {
            case 3:
                _regs.TX_LO_SBHM_FB[idx] = hbx_cpld_regs_t::TX_LO_SBHM_FB_FILTER_3;
                break;
            case 4:
                _regs.TX_LO_SBHM_FB[idx] = hbx_cpld_regs_t::TX_LO_SBHM_FB_FILTER_4;
                break;
            case 5:
                _regs.TX_LO_SBHM_FB[idx] = hbx_cpld_regs_t::TX_LO_SBHM_FB_FILTER_5;
                break;
            case 6:
                _regs.TX_LO_SBHM_FB[idx] = hbx_cpld_regs_t::TX_LO_SBHM_FB_FILTER_6;
                break;
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}

void hbx_cpld_ctrl::set_rx_rf_path_switches(const hbx_tune_map_item_t& tune_map_item)
{
    std::lock_guard<std::mutex> lock(_rx_lo_sw_mutex);

    for (const size_t idx : ATR_ADDRS) {
        // Set RX path switches
        set_rx_path_switches(idx,
            tune_map_item.rf_band,
            tune_map_item.rf_band1_subband,
            tune_map_item.rf_filter_branch);

        // Set RX LO switches
        set_rx_lo_switches(idx,
            tune_map_item.rf_band,
            tune_map_item.rf_band1_subband,
            tune_map_item.lo_filter_branch);
    }

    // Commit the changes to the CPLD registers
    commit();
}


void hbx_cpld_ctrl::set_rx_path_switches(const uint8_t idx,
    const uint8_t rx_band,
    const uint8_t rx_band1_subband,
    const uint8_t rx_filter_branch)
{
    switch (rx_band) {
        case 0:
            // Configure RX path switches for rx_band 0
            _regs.RX_SW_RF_LF[idx] = hbx_cpld_regs_t::RX_SW_RF_LF_BAND_0;
            _regs.RX_LF_SW1[idx]   = hbx_cpld_regs_t::RX_LF_SW1_ADC;
            break;

        case 1:
            // Configure RX path switches for rx_band 1
            switch (rx_band1_subband) {
                case 1:
                    _regs.RX_SW_RF_LF[idx] = hbx_cpld_regs_t::RX_SW_RF_LF_BAND_0;
                    _regs.RX_LF_SW1[idx]   = hbx_cpld_regs_t::RX_LF_SW1_LTC;
                    _regs.RX_B1_SW1[idx]   = hbx_cpld_regs_t::RX_B1_SW1_LF_PATH;

                    // Call set_rx_filter_branch
                    set_rx_filter_branch(
                        idx, rx_band, rx_band1_subband, rx_filter_branch);
                    _regs.RX_BB_SW1[idx] = hbx_cpld_regs_t::RX_BB_SW1_BAND_1;
                    break;

                case 2:
                    _regs.RX_SW_RF_LF[idx] = hbx_cpld_regs_t::RX_SW_RF_LF_ADMV;
                    _regs.RX_LF_SW1[idx]   = hbx_cpld_regs_t::RX_LF_SW1_ADC;
                    _regs.RX_B1_SW1[idx]   = hbx_cpld_regs_t::RX_B1_SW1_RF_PATH;

                    // Call set_rx_filter_branch
                    set_rx_filter_branch(
                        idx, rx_band, rx_band1_subband, rx_filter_branch);

                    _regs.RX_BB_SW1[idx] = hbx_cpld_regs_t::RX_BB_SW1_BAND_1;
                    break;

                case 3:
                    _regs.RX_SW_RF_LF[idx] = hbx_cpld_regs_t::RX_SW_RF_LF_ADMV;
                    _regs.RX_LF_SW1[idx]   = hbx_cpld_regs_t::RX_LF_SW1_ADC;
                    _regs.RX_B1_SW1[idx]   = hbx_cpld_regs_t::RX_B1_SW1_RF_PATH;

                    // Call set_rx_filter_branch
                    set_rx_filter_branch(
                        idx, rx_band, rx_band1_subband, rx_filter_branch);
                    _regs.RX_BB_SW1[idx] = hbx_cpld_regs_t::RX_BB_SW1_BAND_1;
                    break;

                default:
                    UHD_THROW_INVALID_CODE_PATH();
            }
            break;

        case 2:
            // Configure RX path switches for rx_band 2
            _regs.RX_SW_RF_LF[idx] = hbx_cpld_regs_t::RX_SW_RF_LF_ADMV;
            _regs.RX_BB_SW1[idx]   = hbx_cpld_regs_t::RX_BB_SW1_BAND_2;
            break;

        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

void hbx_cpld_ctrl::set_rx_filter_branch(const uint8_t idx,
    const uint8_t rx_band,
    const uint8_t rx_band1_subband,
    const uint8_t rx_filter_branch)
{
    switch (rx_band) {
        case 0:
            // No RX filter branch switches need to be set for RX band 0
            break;

        case 1:
            // RX band 1 requires setting RX filter branch switches
            if (rx_band1_subband == 1 || rx_band1_subband == 2) {
                // Subbands 1 and 2 share the same configuration for RX_B1_SW2 and
                // RX_B1_SW5
                _regs.RX_B1_SW2[idx] = hbx_cpld_regs_t::RX_B1_SW2_FB1_F1_TO_F8;
                _regs.RX_B1_SW5[idx] = hbx_cpld_regs_t::RX_B1_SW5_FB1_F1_TO_F8;

                // Set RX_B1_SW4 and RX_B1_SW3 based on rx_filter_branch
                switch (rx_filter_branch) {
                    case 1:
                        _regs.RX_B1_SW3[idx] = hbx_cpld_regs_t::RX_B1_SW3_FILTER_1;
                        _regs.RX_B1_SW4[idx] = hbx_cpld_regs_t::RX_B1_SW4_FILTER_1;
                        break;
                    case 2:
                        _regs.RX_B1_SW3[idx] = hbx_cpld_regs_t::RX_B1_SW3_FILTER_2;
                        _regs.RX_B1_SW4[idx] = hbx_cpld_regs_t::RX_B1_SW4_FILTER_2;
                        break;
                    case 3:
                        _regs.RX_B1_SW3[idx] = hbx_cpld_regs_t::RX_B1_SW3_FILTER_3;
                        _regs.RX_B1_SW4[idx] = hbx_cpld_regs_t::RX_B1_SW4_FILTER_3;
                        break;
                    case 4:
                        _regs.RX_B1_SW3[idx] = hbx_cpld_regs_t::RX_B1_SW3_FILTER_4;
                        _regs.RX_B1_SW4[idx] = hbx_cpld_regs_t::RX_B1_SW4_FILTER_4;
                        break;
                    case 5:
                        _regs.RX_B1_SW3[idx] = hbx_cpld_regs_t::RX_B1_SW3_FILTER_5;
                        _regs.RX_B1_SW4[idx] = hbx_cpld_regs_t::RX_B1_SW4_FILTER_5;
                        break;
                    case 6:
                        _regs.RX_B1_SW3[idx] = hbx_cpld_regs_t::RX_B1_SW3_FILTER_6;
                        _regs.RX_B1_SW4[idx] = hbx_cpld_regs_t::RX_B1_SW4_FILTER_6;
                        break;
                    case 7:
                        _regs.RX_B1_SW3[idx] = hbx_cpld_regs_t::RX_B1_SW3_FILTER_7;
                        _regs.RX_B1_SW4[idx] = hbx_cpld_regs_t::RX_B1_SW4_FILTER_7;
                        break;
                    case 8:
                        _regs.RX_B1_SW3[idx] = hbx_cpld_regs_t::RX_B1_SW3_FILTER_8;
                        _regs.RX_B1_SW4[idx] = hbx_cpld_regs_t::RX_B1_SW4_FILTER_8;
                        break;
                    default:
                        UHD_THROW_INVALID_CODE_PATH();
                }
            } else if (rx_band1_subband == 3) {
                // Subband 3 has a different configuration for RX_B1_SW2 and RX_B1_SW5
                _regs.RX_B1_SW2[idx] = hbx_cpld_regs_t::RX_B1_SW2_FB1_F9;
                _regs.RX_B1_SW5[idx] = hbx_cpld_regs_t::RX_B1_SW5_FB1_F9;
            } else {
                UHD_THROW_INVALID_CODE_PATH();
            }
            break;

        case 2:
            // No RX filter branch switches need to be set for RX band 2
            break;

        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

void hbx_cpld_ctrl::set_rx_lo_switches(const uint8_t idx,
    const uint8_t rx_band,
    const uint8_t rx_band1_subband,
    const std::vector<uint8_t>& lo_filter_branch)
{
    switch (rx_band) {
        case 0:
            // LO is not used for RX band 0
            break;

        case 1:
            // RX band 1 requires setting LO switches
            if (rx_band1_subband == 1 || rx_band1_subband == 2) {
                _regs.RX_LO_B12_SW[idx] = hbx_cpld_regs_t::RX_LO_B12_SW_LF_PATH;
                _regs.RX_LO_SW1[idx]    = hbx_cpld_regs_t::RX_LO_SW1_BAND_1;

                // Call set_rx_lo_filter_branch
                set_rx_lo_filter_branch(idx, rx_band, rx_band1_subband, lo_filter_branch);
            } else if (rx_band1_subband == 3) {
                _regs.RX_LO_B12_SW[idx]  = hbx_cpld_regs_t::RX_LO_B12_SW_RF_PATH;
                _regs.RX_LO_B23_SW2[idx] = hbx_cpld_regs_t::RX_LO_B23_SW2_BAND_1;

                // Call set_rx_lo_filter_branch
                set_rx_lo_filter_branch(idx, rx_band, rx_band1_subband, lo_filter_branch);
            } else {
                UHD_THROW_INVALID_CODE_PATH();
            }
            break;

        case 2:
            // RX band 2 requires setting LO switches
            _regs.RX_LO_B23_SW2[idx] = hbx_cpld_regs_t::RX_LO_B23_SW2_BAND_2;

            // Call set_rx_lo_filter_branch
            set_rx_lo_filter_branch(idx, rx_band, rx_band1_subband, lo_filter_branch);
            break;

        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

void hbx_cpld_ctrl::set_rx_lo_filter_branch(const uint8_t idx,
    const uint8_t rx_band,
    const uint8_t rx_band1_subband,
    const std::vector<uint8_t>& lo_filter_branch)
{
    switch (rx_band) {
        case 0:
            // No LO filter branch switches need to be set for RX band 0
            break;

        case 1:
            // RX band 1 requires setting LO filter branch switches
            if (rx_band1_subband == 1 || rx_band1_subband == 2) {
                // Ensure lo_filter_branch has exactly one element
                if (lo_filter_branch.size() != 1) {
                    UHD_THROW_INVALID_CODE_PATH();
                }

                // Set RX_LO_B1_FB based on the single element in lo_filter_branch
                switch (lo_filter_branch[0]) {
                    case 1:
                        _regs.RX_LO_B1_FB[idx] = hbx_cpld_regs_t::RX_LO_B1_FB_FILTER_1;
                        break;
                    case 2:
                        _regs.RX_LO_B1_FB[idx] = hbx_cpld_regs_t::RX_LO_B1_FB_FILTER_2;
                        break;
                    case 3:
                        _regs.RX_LO_B1_FB[idx] = hbx_cpld_regs_t::RX_LO_B1_FB_FILTER_3;
                        break;
                    case 4:
                        _regs.RX_LO_B1_FB[idx] = hbx_cpld_regs_t::RX_LO_B1_FB_FILTER_4;
                        break;
                    default:
                        UHD_THROW_INVALID_CODE_PATH();
                }
            } else if (rx_band1_subband == 3) {
                // Ensure lo_filter_branch has exactly two elements
                if (lo_filter_branch.size() != 2) {
                    UHD_THROW_INVALID_CODE_PATH();
                }

                // Set RX_LO_SW1 and RX_LO_B23_SW1 based on the first element in
                // lo_filter_branch
                switch (lo_filter_branch[0]) {
                    case 1:
                        _regs.RX_LO_SW1[idx] = hbx_cpld_regs_t::RX_LO_SW1_B12_PRE_DB_F1;
                        _regs.RX_LO_B23_SW1[idx] =
                            hbx_cpld_regs_t::RX_LO_B23_SW1_PRE_DB_F1;
                        break;
                    case 2:
                        _regs.RX_LO_SW1[idx] = hbx_cpld_regs_t::RX_LO_SW1_B12_PRE_DB_F2;
                        _regs.RX_LO_B23_SW1[idx] =
                            hbx_cpld_regs_t::RX_LO_B23_SW1_PRE_DB_F2;
                        break;
                    default:
                        UHD_THROW_INVALID_CODE_PATH();
                }

                // Set RX_LO_SBHM_FB based on the second element in lo_filter_branch
                switch (lo_filter_branch[1]) {
                    case 3:
                        _regs.RX_LO_SBHM_FB[idx] =
                            hbx_cpld_regs_t::RX_LO_SBHM_FB_FILTER_3;
                        break;
                    case 4:
                        _regs.RX_LO_SBHM_FB[idx] =
                            hbx_cpld_regs_t::RX_LO_SBHM_FB_FILTER_4;
                        break;
                    case 5:
                        _regs.RX_LO_SBHM_FB[idx] =
                            hbx_cpld_regs_t::RX_LO_SBHM_FB_FILTER_5;
                        break;
                    case 6:
                        _regs.RX_LO_SBHM_FB[idx] =
                            hbx_cpld_regs_t::RX_LO_SBHM_FB_FILTER_6;
                        break;
                    default:
                        UHD_THROW_INVALID_CODE_PATH();
                }
            } else {
                UHD_THROW_INVALID_CODE_PATH();
            }
            break;

        case 2:
            // RX band 2 requires setting LO filter branch switches
            // Ensure lo_filter_branch has exactly two elements
            if (lo_filter_branch.size() != 2) {
                UHD_THROW_INVALID_CODE_PATH();
            }

            // Set RX_LO_SW1 and RX_LO_B23_SW1 based on the first element in
            // lo_filter_branch
            switch (lo_filter_branch[0]) {
                case 1:
                    _regs.RX_LO_SW1[idx]     = hbx_cpld_regs_t::RX_LO_SW1_B12_PRE_DB_F1;
                    _regs.RX_LO_B23_SW1[idx] = hbx_cpld_regs_t::RX_LO_B23_SW1_PRE_DB_F1;
                    break;
                case 2:
                    _regs.RX_LO_SW1[idx]     = hbx_cpld_regs_t::RX_LO_SW1_B12_PRE_DB_F2;
                    _regs.RX_LO_B23_SW1[idx] = hbx_cpld_regs_t::RX_LO_B23_SW1_PRE_DB_F2;
                    break;
                default:
                    UHD_THROW_INVALID_CODE_PATH();
            }

            // Set RX_LO_SBHM_FB based on the second element in lo_filter_branch
            switch (lo_filter_branch[1]) {
                case 3:
                    _regs.RX_LO_SBHM_FB[idx] = hbx_cpld_regs_t::RX_LO_SBHM_FB_FILTER_3;
                    break;
                case 4:
                    _regs.RX_LO_SBHM_FB[idx] = hbx_cpld_regs_t::RX_LO_SBHM_FB_FILTER_4;
                    break;
                case 5:
                    _regs.RX_LO_SBHM_FB[idx] = hbx_cpld_regs_t::RX_LO_SBHM_FB_FILTER_5;
                    break;
                case 6:
                    _regs.RX_LO_SBHM_FB[idx] = hbx_cpld_regs_t::RX_LO_SBHM_FB_FILTER_6;
                    break;
                default:
                    UHD_THROW_INVALID_CODE_PATH();
            }
            break;

        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
}

uint8_t hbx_cpld_ctrl::set_dsa(const uint8_t idx, const dsa_t dsa, const uint8_t value)
{
    // Set the DSA value in the CPLD registers
    switch (dsa) {
        case dsa_t::TX_RF_DSA:
            _regs.TX_RF_DSA[idx] = value;
            break;
        case dsa_t::RX_RF_DSA:
            _regs.RX_RF_DSA[idx] = value;
            break;
        case dsa_t::RX_LF_DSA1:
            _regs.RX_LF_DSA1[idx] = value;
            break;
        case dsa_t::RX_LF_DSA2:
            _regs.RX_LF_DSA2[idx] = value;
            break;
        case dsa_t::TX_LO_DSA:
            _regs.TX_LO_DSA[idx] = value;
            break;
        case dsa_t::RX_LO_DSA:
            _regs.RX_LO_DSA[idx] = value;
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }

    // Commit the changes to the CPLD registers
    commit();

    return value;
}

double hbx_cpld_ctrl::read_lo_pd_val(const direction_t trx, const pd_read_fct_t& read_fn)
{
    //! Holds the last known switch positions for TX and RX
    hbx_cpld_regs_t::RX_LO_SW1_t rx_lo_sw1_state =
        hbx_cpld_regs_t::RX_LO_SW1_t::RX_LO_SW1_POWER_DETECTOR;
    hbx_cpld_regs_t::TX_LO_SW1_t tx_lo_sw1_state =
        hbx_cpld_regs_t::TX_LO_SW1_t::TX_LO_SW1_POWER_DETECTOR;
    std::lock_guard<std::mutex> lock(
        trx == TX_DIRECTION ? _tx_lo_sw_mutex : _rx_lo_sw_mutex);

    if (trx == RX_DIRECTION) {
        rx_lo_sw1_state = _regs.RX_LO_SW1[ATR_ADDR_RX];
        _regs.RX_LO_SW1[ATR_ADDR_RX] =
            hbx_cpld_regs_t::RX_LO_SW1_t::RX_LO_SW1_POWER_DETECTOR;
    } else if (trx == TX_DIRECTION) {
        tx_lo_sw1_state = _regs.TX_LO_SW1[ATR_ADDR_TX];
        _regs.TX_LO_SW1[ATR_ADDR_TX] =
            hbx_cpld_regs_t::TX_LO_SW1_t::TX_LO_SW1_POWER_DETECTOR;
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
    // No commit() required, the following method does that one anyway.
    // Switch to the right ATR state to have the LO DSA set correctly
    set_atr_mode(hbx_cpld_ctrl::atr_t::SW_DEFINED);
    // This will cause FP LEDs to flicker quickly, but that should barely be
    // visible.
    set_atr_state(trx == RX_DIRECTION ? ATR_ADDR_RX : ATR_ADDR_TX);

    // Ensure that switches are restored after reading the power detector value
    auto restore_switches = uhd::utils::scope_exit::make([&]() {
        if (trx == RX_DIRECTION) {
            // Restore RX_LO_SW1 from saved state
            _regs.RX_LO_SW1[ATR_ADDR_RX] = rx_lo_sw1_state;
        } else if (trx == TX_DIRECTION) {
            // Restore TX_LO_SW1 from saved state
            _regs.TX_LO_SW1[ATR_ADDR_TX] = tx_lo_sw1_state;
        } else {
            UHD_THROW_INVALID_CODE_PATH();
        }
        // Again, no commit() required, set_atr_mode() does that for us.
        set_atr_mode(hbx_cpld_ctrl::atr_t::CLASSIC_ATR);
    });

    // Read the power detector value using the provided read function
    return read_fn();
}

/**************************************************************************
 * Internal SYNC signal control
 *************************************************************************/
void hbx_cpld_ctrl::set_internal_sync_clk(const bool enable)
{
    _regs.SYNC_CLK = enable ? _regs.SYNC_CLK_ENABLE : _regs.SYNC_CLK_DISABLE;
    commit(CHAN0);
}

hbx_cpld_ctrl::spi_transactor::spi_transactor(size_t start_address,
    poke_fn_type&& poke_fn,
    peek_fn_type&& peek_fn,
    const bool queue)
    : _start_address(start_address)
    , _poke32(std::move(poke_fn))
    , _peek32(std::move(peek_fn))
    , _queue(queue)
{
    std::stringstream ss;
    ss << "spi_0x" << std::hex << _start_address;
    _log_id = ss.str();

    uint32_t spi_info = _peek32(_start_address);
    // In HBX SPI info registers, the first 8 bits will always contain the data width
    // and the next 8 bits will always have the address width
    _data_width           = spi_info & 0xFF; // Bits 0-7
    _address_width        = (spi_info >> 8) & 0xFF; // Bits 8-15
    const auto fifo_depth = (spi_info >> 16) & 0xFFFF; // Bits 16-31

    if (queue && fifo_depth == 0) {
        UHD_LOG_WARNING(_log_id,
            "SPI transactor created with queue enabled but FIFO depth is 0. "
            "Disabling queue. Timed commands are not supported on this SPI interface.");
        _queue = false;
    }

    UHD_LOG_TRACE(_log_id,
        "SPI Transactor created with address width: "
            << _address_width << ", data width: " << _data_width
            << ", fifo depth: " << fifo_depth << ", start address: " << _start_address);
}

void hbx_cpld_ctrl::spi_transactor::poll_spi_ready(const bool is_read)
{
    // Check if SPI_READY bit is set indicating hardware component is ready for
    // transaction. If is_read is true, also check for SPI_READ_READY bit.
    auto start_time   = std::chrono::steady_clock::now();
    auto elapsed_time = std::chrono::steady_clock::now() - start_time;
    bool ready        = false;
    while (!ready) {
        elapsed_time = std::chrono::steady_clock::now() - start_time;
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed_time).count()
            > HBX_SPI_READY_TIMEOUT_MS) {
            // SPI_READY bit not set within HBX_SPI_READY_TIMEOUT_MS
            UHD_LOG_THROW(uhd::runtime_error,
                _log_id,
                "spi_transactor at start address 0x"
                    << std::hex << _start_address << " was not ready "
                    << (is_read ? "to read data" : "for an SPI transaction")
                    << " within timeout period.");
        }
        uint32_t status = _peek32(_start_address + SPI_STATUS_OFFSET);
        ready           = (status & (1 << SPI_READY_BIT)) != 0
                && (!is_read || (status & (1 << SPI_READ_READY_BIT)) != 0);
    }
}

void hbx_cpld_ctrl::spi_transactor::spi_transact(
    const bool is_read, const uint32_t addr, const uint32_t data)
{
    if (!_queue) {
        poll_spi_ready();
    }
    // As SPI is ready, setup the SPI transaction by writing into the SPI_SETUP
    // register. The first _data_width number of bits is the data and the next
    // _address_width number of bits is the address.
    // Bit 30 indicates the type of transaction i.e READ=1 or WRITE=0.
    // Bit 31 being set indicates start of transaction.
    uint32_t setup_value = (1 << 31) | ((is_read ? 1 : 0) << 30)
                           | ((addr & ((1 << _address_width) - 1)) << _data_width)
                           | (data & ((1 << _data_width) - 1));

    _poke32(_start_address + SPI_SETUP_OFFSET, setup_value, CHAN0);
    // In the write case we are done at this point

    if (is_read) {
        poll_spi_ready(true);

        uint32_t status        = _peek32(_start_address + SPI_STATUS_OFFSET);
        _spi_read_data         = status & ((1 << _data_width) - 1); // Extract data
        uint32_t spi_read_addr = (status >> _data_width)
                                 & ((1 << _address_width) - 1); // Extract address
        UHD_ASSERT_THROW(spi_read_addr == addr); // Assert addresses match
    }
}

void hbx_cpld_ctrl::spi_transactor::spi_write(const uint32_t addr, const uint32_t data)
{
    spi_transact(false, addr, data);
}

uint32_t hbx_cpld_ctrl::spi_transactor::spi_read(const uint32_t addr)
{
    spi_transact(true, addr);
    return _spi_read_data;
}


/******************************************************************************
 * Private methods
 *****************************************************************************/

void hbx_cpld_ctrl::commit(const bool save_all)
{
    UHD_LOG_TRACE(_log_id,
        "Storing register cache " << (save_all ? "completely" : "selectively")
                                  << " to register...");
    const auto changed_addrs = save_all ? _regs.get_all_addrs<size_t>()
                                        : _regs.get_changed_addrs<size_t>();
    for (const auto addr : changed_addrs) {
        _poke32(addr, _regs.get_reg(addr), save_all ? NO_CHAN : CHAN0);
    }
    _regs.save_state();
    UHD_LOG_TRACE(_log_id,
        "Storing cache complete: "
        "Updated "
            << changed_addrs.size() << " registers.");
}

void hbx_cpld_ctrl::update_field(const std::string& field, const size_t idx)
{
    const uint16_t addr     = _regs.get_addr(field) + 4 * idx;
    const uint32_t chip_val = _peek32(addr);
    _regs.set_reg(addr, chip_val);
}
}}} // namespace uhd::usrp::hbx
