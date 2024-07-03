//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/direction.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/cast.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/dboard/fbx/fbx_expert.hpp>
#include <uhdlib/utils/interpolation.hpp>
#include <uhdlib/utils/log.hpp>
#include <algorithm>
#include <array>

using namespace uhd;

namespace uhd { namespace usrp { namespace fbx {
namespace {

/*********************************************************************
 *   Misc/calculative helper functions
 **********************************************************************/

/*
 * I and Q may be swapped depending on the frequency we are operating on. According to
 * measurements up to the 8th Nyquist zone we have to switch like this:
 * RX: First Nyquist zone IQ swapped, all others are okay as is.
 * TX: First Nyquist zone is okay, all others are swapped.
 * So the swapping is exactly mirrored for RX and TX.
 */
bool _is_band_inverted(
    const uhd::direction_t trx, const double freq, const double rfdc_rate)
{
    constexpr std::array<bool, 8> invert_zones = {
        true, false, false, false, false, false, false, false};

    // Find out in which Nyquist zone we are (counting from 0):
    const size_t nyquist_zone = freq / (rfdc_rate / 2);

    const bool rx_invert = invert_zones[nyquist_zone];
    const bool tx_invert = !invert_zones[nyquist_zone];

    if (trx == RX_DIRECTION) {
        return rx_invert;
    } else {
        return tx_invert;
    }
}

std::string _get_trx_string(const direction_t dir)
{
    if (dir == RX_DIRECTION) {
        return "rx";
    } else if (dir == TX_DIRECTION) {
        return "tx";
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}
} // namespace

/*!---------------------------------------------------------
 * EXPERT RESOLVE FUNCTIONS
 *
 * This sections contains all expert resolve functions.
 * These methods are triggered by any of the bound accessors becoming "dirty",
 * or changing value
 * --------------------------------------------------------
 */
void fbx_scheduling_expert::resolve()
{
    // We currently have no fancy scheduling, but here is where we'd add it if
    // we need to do that (e.g., plan out SYNC pulse timing vs. NCO timing etc.)
    _frontend_time = _command_time;
}

void fbx_tx_programming_expert::resolve()
{
    for (const size_t idx : ATR_ADDRS) {
        _fbx_ctrl->set_tx_antenna_switches(_chan, idx, _antenna);
    }
    // We do not update LEDs on switching TX antenna value by definition
}

void fbx_rx_programming_expert::resolve()
{
    for (const size_t idx : ATR_ADDRS) {
        // If using the TX/RX terminal, only configure the ATR RX state since the state of
        // the switch at other times is controlled by TX
        if (_antenna != ANTENNA_TXRX || idx == ATR_ADDR_RX) {
            _fbx_ctrl->set_rx_antenna_switches(_chan, idx, _antenna);
        }
    }
    _update_leds();
}

void fbx_rx_programming_expert::_update_leds()
{
    // We default to the RX1 for all rX antenna values that are not TX/RX0
    const bool rx_on_trx = _antenna == ANTENNA_TXRX;
    // clang-format off
    // G==Green, R==Red                        RX2         TX/RX-G    TX/RX-R
    _fbx_ctrl->set_leds(_chan, ATR_ADDR_0X, false,      false,     false);
    _fbx_ctrl->set_leds(_chan, ATR_ADDR_RX, !rx_on_trx, rx_on_trx, false);
    _fbx_ctrl->set_leds(_chan, ATR_ADDR_TX, false,      false,     true );
    _fbx_ctrl->set_leds(_chan, ATR_ADDR_XX, !rx_on_trx, rx_on_trx, true );
    // clang-format on
}

void fbx_band_inversion_expert::resolve()
{
    const bool band_inverted = _is_band_inverted(_trx, _frequency, _rfdc_rate);
    _rpcc->enable_iq_swap(band_inverted, _get_trx_string(_trx), _chan);
}

void fbx_rfdc_freq_expert::resolve()
{
    const double desired_rfdc_freq = [&]() -> double {
        if (_rfdc_freq_desired.is_dirty() && !_frequency_desired.is_dirty()) {
            return FBX_FREQ_RANGE.clip(_rfdc_freq_desired);
        }
        return FBX_FREQ_RANGE.clip(_frequency_desired);
    }();

    _rfdc_freq_coerced = _rpcc->rfdc_set_nco_freq(
        _get_trx_string(_trx), _db_idx, _chan, desired_rfdc_freq);
    _if_frequency_coerced = _rfdc_freq_coerced;
    _coerced_frequency    = _if_frequency_coerced;

    // Both are dirty during the session start and we don't want to spam the user with
    // logs there.
    if (!(_rfdc_freq_desired.is_dirty() && _frequency_desired.is_dirty())) {
        UHD_LOG_IF_GUIDED(
            auto nyquist_boundary = _rfdc_rate / 2;
            uint32_t nyquist_zone =
                std::floor(_if_frequency_coerced / nyquist_boundary) + 1;
            // Provide more information about implications of chosen frequency.
            UHD_LOG_GUIDE(_get_trx_string(_trx)
                          << " frequency " << (_if_frequency_coerced / 1e6)
                          << " MHz on DB " << _db_idx << ", channel " << _chan
                          << " is in the " << uhd::cast::to_ordinal_string(nyquist_zone)
                          << " Nyquist zone.");
            // warnings according to
            // https://kb.ettus.com/About_Sampling_Rates_and_Master_Clock_Rates_for_the_USRP_X440#Aliases_and_Nyquist_Zones
            if ((nyquist_zone > 2 && _trx == TX_DIRECTION)
                || (nyquist_zone > 3 && _trx == RX_DIRECTION)) {
                UHD_LOG_GUIDE(
                    _get_trx_string(_trx)
                    << " operation in this Nyquist zone will result in decreased RF "
                       "performance.");
            };
            // passband_freq is the if_frequency relative to the start of the Nyquist zone
            auto passband_freq = static_cast<uint32_t>(std::round(_if_frequency_coerced))
                                 % static_cast<uint32_t>(nyquist_boundary);
            if (passband_freq < 0.1 * nyquist_boundary
                || passband_freq > 0.9 * nyquist_boundary) {
                auto pass_low_mhz =
                    (nyquist_zone * nyquist_boundary - 0.9 * nyquist_boundary) / 1e6;
                auto pass_high_mhz =
                    (nyquist_zone * nyquist_boundary - 0.1 * nyquist_boundary) / 1e6;
                UHD_LOG_GUIDE(_get_trx_string(_trx)
                              << " frequency " << (_if_frequency_coerced / 1e6)
                              << " MHz is outside the 80% Nyquist passband ("
                              << pass_low_mhz << " MHz to " << pass_high_mhz
                              << " MHz for this Nyquist zone). This "
                              << "may result in aliasing and undesired effects.");
            });
    }
}

void fbx_sync_expert::resolve()
{
    // Local helper consts
    // clang-format off
    constexpr std::array<std::array<rfdc_control::rfdc_type, 2>, FBX_MAX_NUM_CHANS> all_ncos{{
        {rfdc_control::rfdc_type::RX0, rfdc_control::rfdc_type::TX0},
        {rfdc_control::rfdc_type::RX1, rfdc_control::rfdc_type::TX1},
        {rfdc_control::rfdc_type::RX2, rfdc_control::rfdc_type::TX2},
        {rfdc_control::rfdc_type::RX3, rfdc_control::rfdc_type::TX3}
    }};
    // clang-format on
    // We might have fewer channels than the maximum, so limit our list
    std::vector<std::array<rfdc_control::rfdc_type, 2>> ncos = {
        all_ncos.begin(), all_ncos.begin() + _num_chans};

    // Now do some timing checks
    std::vector<bool> chan_needs_sync;
    for (size_t i = 0; i < _num_chans; i++) {
        chan_needs_sync.push_back(_fe_time.at(i) != uhd::time_spec_t::ASAP);
    }
    // If there's no command time, no need to synchronize anything
    if (std::none_of(
            chan_needs_sync.begin(), chan_needs_sync.end(), [](bool v) { return v; })) {
        UHD_LOG_TRACE(get_name(), "No command time: Skipping phase sync.");
        return;
    }

    // Create map of time specs with corresponding channels. Example:
    // t0 = 0, t1 = 1, t2 = 1, t3 = 3 (with t* being times and numbers being channels)
    // => chans_to_sync == {0: (0,), 1: (1,2), 3: (3,)}
    std::map<uhd::time_spec_t, std::vector<size_t>> chans_to_sync;
    for (size_t i = 0; i < _fe_time.size(); i++) {
        chans_to_sync[_fe_time.at(i)].push_back(i);
    }

    std::set<rfdc_control::rfdc_type> ncos_to_sync;
    std::set<rfdc_control::rfdc_type> gearboxes_to_sync;
    for (auto times_chans : chans_to_sync) {
        ncos_to_sync.clear();
        gearboxes_to_sync.clear();
        for (auto chan : times_chans.second) {
            for (const auto& nco_idx : ncos[chan]) {
                if (_nco_freqs.at(nco_idx).is_dirty()) {
                    ncos_to_sync.insert(nco_idx);
                }
            }
        }
        UHD_LOG_TRACE(get_name(),
            "Syncing " << times_chans.second.size()
                       << " channel(s): " << ncos_to_sync.size() << " NCO(s).");

        // Sync the gearboxes:
        // Gearboxes are special, because they only need to be synchronized once
        // per session, assuming the command time has been set. Unfortunately we
        // have no way here to know if the timekeeper time was updated, but it is
        // well documented that in order to synchronize devices, one first has to
        // make sure the timekeepers are running in sync (by calling
        // set_time_next_pps() accordingly).
        // The logic we use here is that we will always have to update the NCO when
        // doing a synced tune, so we update all the gearboxes for the NCOs -- but
        // only if they have not yet been synchronized.
        if (!_adcs_synced || !_dacs_synced) {
            for (size_t chan = 0; chan < _num_chans; chan++) {
                // In ncos, element 0 in each array element is RX
                const auto rx = ncos[chan].at(0);
                if (ncos_to_sync.count(rx) && !_adcs_synced) {
                    gearboxes_to_sync.insert(rx);
                    // Technically, they're not synced yet but this saves us from
                    // having to look up which RFDCs map to RX again later
                    _adcs_synced = true;
                }
                // In ncos, element 0 in each array element is TX
                const auto tx = ncos[chan].at(1);
                if (ncos_to_sync.count(tx) && !_dacs_synced) {
                    gearboxes_to_sync.insert(tx);
                    // Technically, they're not synced yet but this saves us from
                    // having to look up which RFDCs map to TX again later
                    _dacs_synced = true;
                }
            }
            if (!gearboxes_to_sync.empty()) {
                UHD_LOG_TRACE(get_name(),
                    "Resetting "
                        << gearboxes_to_sync.size() << " gearboxes. ADCs synced: "
                        << (_adcs_synced ? "True" : "False")
                        << "; DACs synced: " << (_dacs_synced ? "True" : "False"));
                _rfdcc->reset_gearboxes(
                    std::vector<rfdc_control::rfdc_type>(
                        gearboxes_to_sync.cbegin(), gearboxes_to_sync.cend()),
                    times_chans.first);
            }
        }

        if (!ncos_to_sync.empty()) {
            _rfdcc->reset_ncos(std::vector<rfdc_control::rfdc_type>(
                                   ncos_to_sync.cbegin(), ncos_to_sync.cend()),
                times_chans.first);
        }
    }
}
}}} // namespace uhd::usrp::fbx
