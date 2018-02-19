//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "magnesium_cpld_ctrl.hpp"
#include "magnesium_constants.hpp"
#include <uhd/utils/log.hpp>
#include <boost/format.hpp>
#include <chrono>

namespace {
    //! Address of the CPLD scratch register
    const uint8_t CPLD_REGS_SCRATCH = 0x0040;

    //! Address of the CPLD reset register
    const uint8_t CPLD_REGS_RESET   = 0x0041;
}

magnesium_cpld_ctrl::magnesium_cpld_ctrl(
        write_spi_t write_fn,
        read_spi_t read_fn
)
{
    _write_fn = [write_fn](const uint8_t addr, const uint32_t data){
        UHD_LOG_TRACE("MG_CPLD",
            str(boost::format("Writing to CPLD: 0x%02X -> 0x%04X")
                % uint32_t(addr) % data));
        const uint32_t spi_transaction = 0
            | ((addr & 0x7F) << 16)
            | data
        ;
        write_fn(spi_transaction);
    };
    _read_fn = [read_fn](const uint8_t addr){
        UHD_LOG_TRACE("MG_CPLD",
            str(boost::format("Reading from CPLD address 0x%02X")
                % uint32_t(addr)));
        const uint32_t spi_transaction = (1<<23)
            | ((addr & 0x7F) << 16)
        ;
        return read_fn(spi_transaction);
    };

    reset();
    _loopback_test();
}

/******************************************************************************
 * API
 *****************************************************************************/
void magnesium_cpld_ctrl::reset()
{
    std::lock_guard<std::mutex> l(_set_mutex);
    UHD_LOG_TRACE("MG_CPLD", "Resetting CPLD to default state");
    // Note: We won't use _regs and commit() here, because the reset will screw
    // up the state of the CPLD w.r.t. to the local cache anyway.
    _write_fn(CPLD_REGS_RESET, 1);
    _write_fn(CPLD_REGS_RESET, 0);
    // Now make sure the local state and the CPLD state are the same by pushing
    // our local changes out:
    _regs = magnesium_cpld_regs_t();
    commit(true);
}

uint16_t magnesium_cpld_ctrl::get_reg(const uint8_t addr)
{
    std::lock_guard<std::mutex> l(_set_mutex);
    return _read_fn(addr);
}

void magnesium_cpld_ctrl::set_scratch(const uint16_t val)
{
    std::lock_guard<std::mutex> l(_set_mutex);
    _regs.scratch = val;
    commit();
}

uint16_t magnesium_cpld_ctrl::get_scratch()
{
    return get_reg(CPLD_REGS_SCRATCH);
}

void magnesium_cpld_ctrl::set_tx_switches(
    const chan_sel_t chan,
    const tx_sw1_t tx_sw1,
    const tx_sw2_t tx_sw2,
    const tx_sw3_t tx_sw3,
    const lowband_mixer_path_sel_t select_lowband_mixer_path,
    const bool enb_lowband_mixer,
    const atr_state_t atr_state,
    const bool defer_commit
) {
    std::lock_guard<std::mutex> l(_set_mutex);
    if (chan == CHAN1 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch1_idle_tx_sw1 = magnesium_cpld_regs_t::ch1_idle_tx_sw1_t(tx_sw1);
            _regs.ch1_idle_tx_sw2 = magnesium_cpld_regs_t::ch1_idle_tx_sw2_t(tx_sw2);
            _regs.ch1_idle_tx_sw3 = magnesium_cpld_regs_t::ch1_idle_tx_sw3_t(tx_sw3);
            _regs.ch1_idle_tx_lowband_mixer_path_select = magnesium_cpld_regs_t::ch1_idle_tx_lowband_mixer_path_select_t(select_lowband_mixer_path);
            _regs.ch1_idle_tx_mixer_en = enb_lowband_mixer;
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch1_on_tx_sw1 = magnesium_cpld_regs_t::ch1_on_tx_sw1_t(tx_sw1);
            _regs.ch1_on_tx_sw2 = magnesium_cpld_regs_t::ch1_on_tx_sw2_t(tx_sw2);
            _regs.ch1_on_tx_sw3 = magnesium_cpld_regs_t::ch1_on_tx_sw3_t(tx_sw3);
            _regs.ch1_on_tx_lowband_mixer_path_select =
                magnesium_cpld_regs_t::ch1_on_tx_lowband_mixer_path_select_t(select_lowband_mixer_path);
            _regs.ch1_on_tx_mixer_en = enb_lowband_mixer;
        }
    }
    if (chan == CHAN2 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch2_idle_tx_sw1 = magnesium_cpld_regs_t::ch2_idle_tx_sw1_t(tx_sw1);
            _regs.ch2_idle_tx_sw2 = magnesium_cpld_regs_t::ch2_idle_tx_sw2_t(tx_sw1);
            _regs.ch2_idle_tx_sw3 = magnesium_cpld_regs_t::ch2_idle_tx_sw3_t(tx_sw1);
            _regs.ch2_idle_tx_lowband_mixer_path_select =
                magnesium_cpld_regs_t::ch2_idle_tx_lowband_mixer_path_select_t(select_lowband_mixer_path);
            _regs.ch2_idle_tx_mixer_en = enb_lowband_mixer;
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch2_on_tx_sw1 = magnesium_cpld_regs_t::ch2_on_tx_sw1_t(tx_sw1);
            _regs.ch2_on_tx_sw2 = magnesium_cpld_regs_t::ch2_on_tx_sw2_t(tx_sw2);
            _regs.ch2_on_tx_sw3 = magnesium_cpld_regs_t::ch2_on_tx_sw3_t(tx_sw3);
            _regs.ch2_on_tx_lowband_mixer_path_select =
                magnesium_cpld_regs_t::ch2_on_tx_lowband_mixer_path_select_t(select_lowband_mixer_path);
            _regs.ch2_on_tx_mixer_en = enb_lowband_mixer;
        }
    }

    if (not defer_commit) {
        commit();
    }
}

void magnesium_cpld_ctrl::set_rx_switches(
    const chan_sel_t chan,
    const rx_sw2_t rx_sw2,
    const rx_sw3_t rx_sw3,
    const rx_sw4_t rx_sw4,
    const rx_sw5_t rx_sw5,
    const rx_sw6_t rx_sw6,
    const lowband_mixer_path_sel_t select_lowband_mixer_path,
    const bool enb_lowband_mixer,
    const atr_state_t atr_state,
    const bool defer_commit
) {
    std::lock_guard<std::mutex> l(_set_mutex);
    if (chan == CHAN1 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch1_idle_rx_sw2 = magnesium_cpld_regs_t::ch1_idle_rx_sw2_t(rx_sw2);
            _regs.ch1_idle_rx_sw3 = magnesium_cpld_regs_t::ch1_idle_rx_sw3_t(rx_sw3);
            _regs.ch1_idle_rx_sw4 = magnesium_cpld_regs_t::ch1_idle_rx_sw4_t(rx_sw4);
            _regs.ch1_idle_rx_sw5 = magnesium_cpld_regs_t::ch1_idle_rx_sw5_t(rx_sw5);
            _regs.ch1_idle_rx_sw6 = magnesium_cpld_regs_t::ch1_idle_rx_sw6_t(rx_sw6);
            _regs.ch1_idle_rx_loband_mixer_path_sel = magnesium_cpld_regs_t::ch1_idle_rx_loband_mixer_path_sel_t(select_lowband_mixer_path);
            _regs.ch1_idle_rx_mixer_en = enb_lowband_mixer;
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch1_on_rx_sw2 = magnesium_cpld_regs_t::ch1_on_rx_sw2_t(rx_sw2);
            _regs.ch1_on_rx_sw3 = magnesium_cpld_regs_t::ch1_on_rx_sw3_t(rx_sw3);
            _regs.ch1_on_rx_sw4 = magnesium_cpld_regs_t::ch1_on_rx_sw4_t(rx_sw4);
            _regs.ch1_on_rx_sw5 = magnesium_cpld_regs_t::ch1_on_rx_sw5_t(rx_sw5);
            _regs.ch1_on_rx_sw6 = magnesium_cpld_regs_t::ch1_on_rx_sw6_t(rx_sw6);
            _regs.ch1_on_rx_loband_mixer_path_sel = magnesium_cpld_regs_t::ch1_on_rx_loband_mixer_path_sel_t(select_lowband_mixer_path);
            _regs.ch1_on_rx_mixer_en = enb_lowband_mixer;
        }
    }
    if (chan == CHAN2 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch2_idle_rx_sw2 = magnesium_cpld_regs_t::ch2_idle_rx_sw2_t(rx_sw2);
            _regs.ch2_idle_rx_sw3 = magnesium_cpld_regs_t::ch2_idle_rx_sw3_t(rx_sw3);
            _regs.ch2_idle_rx_sw4 = magnesium_cpld_regs_t::ch2_idle_rx_sw4_t(rx_sw4);
            _regs.ch2_idle_rx_sw5 = magnesium_cpld_regs_t::ch2_idle_rx_sw5_t(rx_sw5);
            _regs.ch2_idle_rx_sw6 = magnesium_cpld_regs_t::ch2_idle_rx_sw6_t(rx_sw6);
            _regs.ch2_idle_rx_loband_mixer_path_sel =
                magnesium_cpld_regs_t::ch2_idle_rx_loband_mixer_path_sel_t(select_lowband_mixer_path);
            _regs.ch2_idle_rx_mixer_en = enb_lowband_mixer;
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch2_on_rx_sw2 = magnesium_cpld_regs_t::ch2_on_rx_sw2_t(rx_sw2);
            _regs.ch2_on_rx_sw3 = magnesium_cpld_regs_t::ch2_on_rx_sw3_t(rx_sw3);
            _regs.ch2_on_rx_sw4 = magnesium_cpld_regs_t::ch2_on_rx_sw4_t(rx_sw4);
            _regs.ch2_on_rx_sw5 = magnesium_cpld_regs_t::ch2_on_rx_sw5_t(rx_sw5);
            _regs.ch2_on_rx_sw6 = magnesium_cpld_regs_t::ch2_on_rx_sw6_t(rx_sw6);
            _regs.ch2_on_rx_loband_mixer_path_sel = magnesium_cpld_regs_t::ch2_on_rx_loband_mixer_path_sel_t(select_lowband_mixer_path);
            _regs.ch2_on_rx_mixer_en = enb_lowband_mixer;
        }
    }
    if (not defer_commit) {
        commit();
    }
}

void magnesium_cpld_ctrl::set_tx_atr_bits(
    const chan_sel_t chan,
    const atr_state_t atr_state,
    const bool tx_led,
    const bool tx_pa_enb,
    const bool tx_amp_enb,
    const bool tx_myk_en,
    const bool defer_commit
) {
    std::lock_guard<std::mutex> l(_set_mutex);
    if (chan == CHAN1 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch1_idle_tx_led = tx_led;
            _regs.ch1_idle_tx_pa_en = tx_pa_enb;
            _regs.ch1_idle_tx_amp_en = tx_amp_enb;
            _regs.ch1_idle_tx_myk_en = tx_myk_en;
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch1_on_tx_led = tx_led;
            _regs.ch1_on_tx_pa_en = tx_pa_enb;
            _regs.ch1_on_tx_amp_en = tx_amp_enb;
            _regs.ch1_on_tx_myk_en = tx_myk_en;
        }
    }
    if (chan == CHAN2 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch2_idle_tx_led = tx_led;
            _regs.ch2_idle_tx_pa_en = tx_pa_enb;
            _regs.ch2_idle_tx_amp_en = tx_amp_enb;
            _regs.ch2_idle_tx_myk_en = tx_myk_en;
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch2_on_tx_led = tx_led;
            _regs.ch2_on_tx_pa_en = tx_pa_enb;
            _regs.ch2_on_tx_amp_en = tx_amp_enb;
            _regs.ch2_on_tx_myk_en = tx_myk_en;
        }
    }
    if (not defer_commit) {
        commit();
    }
}

void magnesium_cpld_ctrl::set_trx_sw_atr_bits(
    const chan_sel_t chan,
    const atr_state_t atr_state,
    const sw_trx_t trx_sw,
    const bool defer_commit
) {
    std::lock_guard<std::mutex> l(_set_mutex);
    if (chan == CHAN1 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch1_idle_sw_trx =
                magnesium_cpld_regs_t::ch1_idle_sw_trx_t(trx_sw);
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch1_on_sw_trx =
                magnesium_cpld_regs_t::ch1_on_sw_trx_t(trx_sw);
        }
    }
    if (chan == CHAN2 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch2_idle_sw_trx =
                magnesium_cpld_regs_t::ch2_idle_sw_trx_t(trx_sw);
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch2_on_sw_trx =
                magnesium_cpld_regs_t::ch2_on_sw_trx_t(trx_sw);
        }
    }
    if (not defer_commit) {
        commit();
    }
}

void magnesium_cpld_ctrl::set_rx_input_atr_bits(
    const chan_sel_t chan,
    const atr_state_t atr_state,
    const rx_sw1_t rx_sw1,
    const bool rx_led,
    const bool rx2_led,
    const bool defer_commit
) {
    std::lock_guard<std::mutex> l(_set_mutex);
    if (chan == CHAN1 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch1_idle_rx_sw1 =
                magnesium_cpld_regs_t::ch1_idle_rx_sw1_t(rx_sw1);
            _regs.ch1_idle_rx_led = rx_led;
            _regs.ch1_idle_rx2_led = rx2_led;
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch1_on_rx_sw1 =
                magnesium_cpld_regs_t::ch1_on_rx_sw1_t(rx_sw1);
            _regs.ch1_on_rx_led = rx_led;
            _regs.ch1_on_rx2_led = rx2_led;
        }
    }
    if (chan == CHAN2 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch2_idle_rx_sw1 =
                magnesium_cpld_regs_t::ch2_idle_rx_sw1_t(rx_sw1);
            _regs.ch2_idle_rx_led = rx_led;
            _regs.ch2_idle_rx2_led = rx2_led;
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch2_on_rx_sw1 =
                magnesium_cpld_regs_t::ch2_on_rx_sw1_t(rx_sw1);
            _regs.ch2_on_rx_led = rx_led;
            _regs.ch2_on_rx2_led = rx2_led;
        }
    }

    if (not defer_commit) {
        commit();
    }
}

void magnesium_cpld_ctrl::set_rx_atr_bits(
    const chan_sel_t chan,
    const atr_state_t atr_state,
    const bool rx_amp_enb,
    const bool rx_myk_en,
    const bool defer_commit
) {
    std::lock_guard<std::mutex> l(_set_mutex);
    if (chan == CHAN1 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch1_idle_rx_amp_en = rx_amp_enb;
            _regs.ch1_idle_rx_myk_en = rx_myk_en;
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch1_on_rx_amp_en = rx_amp_enb;
            _regs.ch1_on_rx_myk_en = rx_myk_en;
        }
    }
    if (chan == CHAN2 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch2_idle_rx_amp_en = rx_amp_enb;
            _regs.ch2_idle_rx_myk_en = rx_myk_en;
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch2_on_rx_amp_en = rx_amp_enb;
            _regs.ch2_on_rx_myk_en = rx_myk_en;
        }
    }

    if (not defer_commit) {
        commit();
    }
}

void magnesium_cpld_ctrl::set_rx_lna_atr_bits(
    const chan_sel_t chan,
    const atr_state_t atr_state,
    const bool rx_lna1_enb,
    const bool rx_lna2_enb,
    const bool defer_commit
) {
    std::lock_guard<std::mutex> l(_set_mutex);
    if (chan == CHAN1 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch1_idle_rx_lna1_en = rx_lna1_enb;
            _regs.ch1_idle_rx_lna2_en = rx_lna2_enb;
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch1_on_rx_lna1_en = rx_lna1_enb;
            _regs.ch1_on_rx_lna2_en = rx_lna2_enb;
        }
    }
    if (chan == CHAN2 or chan == BOTH) {
        if (atr_state == IDLE or atr_state == ANY) {
            _regs.ch2_idle_rx_lna1_en = rx_lna1_enb;
            _regs.ch2_idle_rx_lna2_en = rx_lna2_enb;
        }
        if (atr_state == ON or atr_state == ANY) {
            _regs.ch2_on_rx_lna1_en = rx_lna1_enb;
            _regs.ch2_on_rx_lna2_en = rx_lna2_enb;
        }
    }

    if (not defer_commit) {
        commit();
    }
}


/******************************************************************************
 * Private methods
 *****************************************************************************/
void magnesium_cpld_ctrl::_loopback_test()
{
    UHD_LOG_TRACE("MG_CPLD", "Performing CPLD scratch loopback test...");
    using namespace std::chrono;
    const uint16_t random_number = // Derived from current time
        uint16_t(system_clock::to_time_t(system_clock::now()) & 0xFFFF);
    set_scratch(random_number);
    const uint16_t actual = get_scratch();
    if (actual != random_number) {
        UHD_LOGGER_ERROR("MG_CPLD")
            << "CPLD scratch loopback failed! "
            << boost::format("Expected: 0x%04X Got: 0x%04X")
               % random_number % actual
        ;
        throw uhd::runtime_error("CPLD scratch loopback failed!");
    }
    UHD_LOG_TRACE("MG_CPLD", "CPLD scratch loopback test passed!");
}


void magnesium_cpld_ctrl::commit(const bool save_all)
{
    UHD_LOGGER_TRACE("MG_CPLD")
        << "Storing register cache "
        << (save_all ? "completely" : "selectively")
        << " to CPLD via SPI..."
    ;
    auto changed_addrs = save_all ?
        _regs.get_all_addrs() :
        _regs.get_changed_addrs<size_t>();
    for (const auto addr: changed_addrs) {
        _write_fn(addr, _regs.get_reg(addr));
    }
    _regs.save_state();
    UHD_LOG_TRACE("MG_CPLD",
        "Storing cache complete: " \
        "Updated " << changed_addrs.size() << " registers.");
}

