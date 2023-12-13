// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "fbx_constants.hpp"
#include <uhd/types/direction.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/time_spec.hpp>
#include <fbx_regs.hpp>
#include <unordered_map>
#include <array>
#include <chrono>
#include <functional>
#include <mutex>

namespace uhd { namespace usrp { namespace fbx {

/*! FBX Control class
 *
 * This class is very similar to the ZBX CPLD Control class. The main difference is that
 * we do not have a CPLD on the FBX daughterboard. Therefore all control is done through
 * the FPGA via GPIO lines towards the daughterboard where switches and LEDs are
 * controlled (hence the naming).
 */
class fbx_ctrl
{
public:
    using sptr = std::shared_ptr<fbx_ctrl>;
    enum chan_t { CHAN0, CHAN1, CHAN2, CHAN3, ALL_CHANS, NO_CHAN };
    // Note: The values in this enum must match the values in the regmaps.

    using poke_fn_type =
        std::function<void(const uint32_t, const uint32_t, const chan_t)>;
    using peek_fn_type  = std::function<uint32_t(const uint32_t)>;
    using sleep_fn_type = std::function<void(const uhd::time_spec_t&)>;

    fbx_ctrl(poke_fn_type&& poke_fn,
        peek_fn_type&& peek_fn,
        sleep_fn_type&& sleep_fn,
        const std::string& log_id);

    ~fbx_ctrl(void) = default;

    /*! Setting switches required for antenna mode switching, transmitting side
     *
     * \param channel daughterboard channel to program
     * \param idx Table index
     * \param antenna desired antenna mode
     */
    void set_tx_antenna_switches(
        const size_t channel, const uint8_t idx, const std::string& antenna);

    /*! Setting switches required for antenna mode switching, receiving side
     *
     * \param channel daughterboard channel to program
     * \param idx Table index
     * \param antenna desired antenna mode
     */
    void set_rx_antenna_switches(
        const size_t channel, const uint8_t idx, const std::string& antenna);

    /*! The last sync switch is a common one among all channels. Therefore to know if
     * another channel has changed its setting we can query it here to return a correct
     * antenna name (SYNC_INT vs. SYNC_EXT).
     *
     * \return Antenna name according to sync switch setting
     */
    std::string get_rx_sync_switch_state();

    /*! Peek a register for the value 1 until we either get that value or timeout.
     * \param addr: Register address to peek
     * \param timeout_ms: Timeout in milliseconds
     * \param check: Callback that get register value and return whether value
     *               is accepted or not
     * \return true on success (check accepted register value), false otherwise
     */
    bool poll_reg_with_timeout(
        const std::string& reg, uint32_t timeout_ms, std::function<bool(uint32_t)> check);

    /**************************************************************************
     * LED controls
     *************************************************************************/
    /*! Turn the LEDs on or off
     *
     * There are two LEDs on FBX, the TRX LED has a red and green component.
     *
     * TODO: Check if this statement from ZBX holds true for FBX, too:
     * Note that toggling any of the LED settings to 'true' won't necessarily
     * turn on the LED. The current config register for this channel must
     * also match \p idx in order for this state to be applied.
     *
     * \param channel The channel for which these settings apply
     * \param idx The LED table index that is configured
     * \param rx On-state of the green RX2 LED
     * \param trx_rx On-state of the green TX/RX LED
     * \param trx_tx On-state of the red TX/RX LED
     */
    void set_leds(const size_t channel,
        const uint8_t idx,
        const bool rx,
        const bool trx_rx,
        const bool trx_tx);


    /**************************************************************************
     * Internal SYNC signal control
     *************************************************************************/
    /*!
     * Enables or disables the internal sync clock (5 MHz) on this daughterboard.
     *
     * \param enable
     */
    void set_internal_sync_clk(const bool enable);


private:
    /*! Dump the state of the registers into the actual registers
     *
     * \param chan Which channel does this change pertain to? This is forwarded
     *             to _poke32().
     * \param save_all If true, save all registers. If false, only change those
     *                 that changed since last save_state() call.
     *                 Note that if save_all is true, the chan parameter does not
     *                 really apply, because all registers (for all channels) are
     *                 written to. Therefore, only use save_all==true in
     *                 combination with NO_CHAN.
     */
    void commit(const chan_t chan = NO_CHAN, const bool save_all = false);

    /*! Check if any channel is connected to the last sync switch, so we know if we can
     * turn it off for better isolation or leave it switched to where ever it is
     * currently.
     *
     * \return Anyone connected (true) or not (false)
     */
    bool any_ch_connected_to_sync();

    //! Initially set the correct ATR option
    void set_classic_atr();

    // Cached register state
    fbx_regs_t _regs = fbx_regs_t();

    //! Poker object
    poke_fn_type _poke32;

    //! Peeker object
    peek_fn_type _peek32;

    //! Hardware-timed sleep, used to throttle pokes
    sleep_fn_type _sleep;

    // infos about the daughtherboard revision
    std::string _db_rev_info;

    const std::string _log_id;
};


}}} // namespace uhd::usrp::fbx
