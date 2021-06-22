// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "zbx_constants.hpp"
#include "zbx_lo_ctrl.hpp"
#include <uhd/types/direction.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/time_spec.hpp>
#include <unordered_map>
#include <zbx_cpld_regs.hpp>
#include <array>
#include <functional>
#include <mutex>

namespace uhd { namespace usrp { namespace zbx {

/*! ZBX CPLD Control Class
 *
 * A note on table indexing: Many settings take an index paramater, usually
 * called 'idx'. These settings can be configured in 256 different ways. The
 * configuration that is chosen out of those 256 different ones depends on the
 * current config register (see get_current_config()). This register itself
 * depends on the ATR mode the CPLD is in.
 * When the ATR mode is atr_mode::CLASSIC_ATR, then only the first four indexes
 * are used, and depend on the current ATR state of the radio (RX, TX, full duplex,
 * or idle). If the mode is atr_mode::FPGA_STATE, then only the first 16 indexes
 * are used, and the setting that is applied follows the four ATR pins with no
 * specific mapping to RX or TX states. If the mode is atr_mode::SW_DEFINED,
 * then the ATR pins are ignored, and the state is configured by set_sw_config().
 */
class zbx_cpld_ctrl
{
public:
    enum chan_t { CHAN0, CHAN1, BOTH_CHANS, NO_CHAN };
    enum spi_xact_t { READ, WRITE };
    // Note: The values in this enum must match the values in the CPLD regmaps.
    enum class atr_mode { SW_DEFINED = 0, CLASSIC_ATR = 1, FPGA_STATE = 2 };
    enum class dsa_type { DSA1, DSA2, DSA3A, DSA3B };
    enum class atr_mode_target { DSA, PATH_LED };

    // The RX gain settings have four DSAs
    using rx_dsa_type = std::array<uint32_t, 4>;
    // The TX gain settings have two DSAs and one amp-path with 3 possible
    // settings.
    using tx_dsa_type = std::array<uint32_t, 3>;

    using poke_fn_type =
        std::function<void(const uint32_t, const uint32_t, const chan_t)>;
    using peek_fn_type  = std::function<uint32_t(const uint32_t)>;
    using sleep_fn_type = std::function<void(const uhd::time_spec_t&)>;

    //! Maps a DSA name ("DSA1", "DSA2", etc.) to its equivalent dsa_type
    static const std::unordered_map<std::string, dsa_type> dsa_map;

    zbx_cpld_ctrl(poke_fn_type&& poke_fn,
        peek_fn_type&& peek_fn,
        sleep_fn_type&& sleep_fn,
        const std::string& log_id);

    ~zbx_cpld_ctrl(void) = default;

    //! Write a value to the scratch register
    void set_scratch(const uint32_t value);

    //! Read back the value from the scratch register
    uint32_t get_scratch();

    /*! Configure the ATR mode of a channel
     *
     * This configures how the DSAs, LEDs, and switches are controlled by the
     * ATR pins going into the CPLD.
     *
     * See the CPLD register map for more information. In a nutshell, this will
     * define how the current config register is populated (ATR pins or
     * set_sw_config()).
     *
     * \param channel The channel for which this setting applies (either 0 or 1)
     * \param target The target for this setting. With atr_mode_target::DSA, it
     *               will change the mode for the attenuators. With PATH_LED, it
     *               will change the mode for the RF path and LED controls.
     * \param mode The ATR mode for this channel and target.
     */
    void set_atr_mode(
        const size_t channel, const atr_mode_target target, const atr_mode mode);

    /*! Choose the SW configuration of the CPLD
     *
     * When the ATR mode is anything other than SW_DEFINED, this has no effect.
     * When the ATR mode is SW_DEFINED, this will choose which DSA/LED/switch
     * configuration to apply to hardware.
     *
     * \param channel The RF channel for which this applies (0 or 1)
     * \param target The target for this setting. With atr_mode_target::DSA, it
     *               will change the mode for the attenuators. With PATH_LED, it
     *               will change the mode for the RF path and LED controls.
     * \param rf_config The selected RF configuration
     */
    void set_sw_config(
        const size_t channel, const atr_mode_target target, const uint8_t rf_config);

    /*! Read back the current config register
     *
     * \param channel The RF channel for which this applies (0 or 1)
     * \param target The target for this setting. With atr_mode_target::DSA, it
     *               will change the mode for the attenuators. With PATH_LED, it
     *               will change the mode for the RF path and LED controls.
     */
    uint8_t get_current_config(const size_t channel, const atr_mode_target target);

    /*! Set all RX DSAs directly
     *
     * This will directly update the DSA tables at the given index. In other
     * words, this setting will directly be applied to hardware.
     *
     * \param channel daughterboard channel to program
     * \param idx Table index
     * \param dsa_steps DSA step values
     */
    void set_tx_gain_switches(
        const size_t channel, const uint8_t idx, const tx_dsa_type& dsa_steps);

    /*! Set all TX DSAs directly
     *
     * This will directly update the DSA tables at the given index. In other
     * words, this setting will directly be applied to hardware.
     *
     * \param channel daughterboard channel to program
     * \param idx Table index
     * \param dsa_steps DSA step values
     */
    void set_rx_gain_switches(
        const size_t channel, const uint8_t idx, const rx_dsa_type& dsa_steps);

    /*! Set all RX DSAs using the CPLD table
     *
     * This will read DSA settings from the lookup table at position \p table_idx
     * and write them to the DSAs at position \p idx.
     *
     * \param channel daughterboard channel to program
     * \param idx DSA table index
     * \param table_idx Lookup table index
     */
    void set_rx_gain_switches(
        const size_t channel, const uint8_t idx, const uint8_t table_idx);

    /*! Set all TX DSAs using the CPLD table
     *
     * This will read DSA settings from the lookup table at position \p table_idx
     * and write them to the DSAs at position \p idx.
     *
     * \param channel daughterboard channel to program
     * \param idx DSA table index
     * \param table_idx Lookup table index
     */
    void set_tx_gain_switches(
        const size_t channel, const uint8_t idx, const uint8_t table_idx);

    /*! Set a specific TX DSA
     *
     * \returns the coerced value that's written to the DSA
     */
    uint8_t set_tx_dsa(const size_t channel,
        const uint8_t idx,
        const dsa_type tx_dsa,
        const uint8_t att);

    /*! Set a specific RX DSA
     *
     * \returns the coerced value that's written to the DSA
     */
    uint8_t set_rx_dsa(const size_t channel,
        const uint8_t idx,
        const dsa_type rx_dsa,
        const uint8_t att);

    /*! Set a specific TX DSA
     *
     * \returns the coerced value that's written to the DSA
     */
    uint8_t get_tx_dsa(const size_t channel,
        const uint8_t idx,
        const dsa_type tx_dsa,
        const bool update_cache = false);

    /*! Set a specific RX DSA
     *
     * \returns the coerced value that's written to the DSA
     */
    uint8_t get_rx_dsa(const size_t channel,
        const uint8_t idx,
        const dsa_type rx_dsa,
        const bool update_cache = false);

    /*! Setting switches required for antenna mode switching, transmitting side
     *
     * Note: If the antenna is set to TX/RX, this also configures the TX
     * amplifier. This unfortunate API coupling is due to the fact that the
     * same switch that chooses the antenna path also switches the amplifier in
     * and out.
     *
     * \param channel daughterboard channel to program
     * \param idx Table index
     * \param amp The amplifier configuration
     * \param antenna desired antenna mode
     */
    void set_tx_antenna_switches(const size_t channel,
        const uint8_t idx,
        const std::string& antenna,
        const tx_amp amp);

    /*! Setting switches required for antenna mode switching, receiving side
     *
     * \param channel daughterboard channel to program
     * \param idx Table index
     * \param gain desired antenna mode
     * \param is_highband highband or lowband settings
     */
    void set_rx_antenna_switches(
        const size_t channel, const uint8_t idx, const std::string& antenna);

    /*! Return the current amp settings
     */
    tx_amp get_tx_amp_settings(
        const size_t channel, const uint8_t idx, const bool update_cache);

    /*! Setting switches required for rf filter changes, receiving side
     *
     * \param channel daughterboard channel to program
     * \param idx Table index
     * \param rf_fir rf filter value
     */
    void set_rx_rf_filter(
        const size_t channel, const uint8_t idx, const uint8_t rf_fir);

    /*! Setting switches required for if1 filter changes, receiving side
     *
     * \param channel daughterboard channel to program
     * \param idx Table index
     * \param if1_fir if1 filter value
     */
    void set_rx_if1_filter(
        const size_t channel, const uint8_t idx, const uint8_t if1_fir);

    /*! Setting switches required for if2 filter changes, receiving side
     *
     * \param channel daughterboard channel to program
     * \param idx Table index
     * \param if2_fir if2 filter value
     */
    void set_rx_if2_filter(
        const size_t channel, const uint8_t idx, const uint8_t if2_fir);

    /*! Setting switches required for rf filter changes, transmitting side
     *
     * \param channel daughterboard channel to program
     * \param idx Table index
     * \param rf_fir rf filter value
     */
    void set_tx_rf_filter(
        const size_t channel, const uint8_t idx, const uint8_t rf_fir);

    /*! Setting switches required for if1 filter changes, transmitting side
     *
     * \param channel daughterboard channel to program
     * \param idx Table index
     * \param if1_fir if1 filter value
     */
    void set_tx_if1_filter(
        const size_t channel, const uint8_t idx, const uint8_t if1_fir);

    /*! Setting switches required for if2 filter changes, transmitting side
     *
     * \param channel daughterboard channel to program
     * \param idx Table index
     * \param if2_fir if2 filter value
     */
    void set_tx_if2_filter(
        const size_t channel, const uint8_t idx, const uint8_t if2_fir);

    /**************************************************************************
     * LED controls
     *************************************************************************/
    /*! Turn the LEDs on or off
     *
     * There are two LEDs on ZBX, the TRX LED has a red and green component.
     *
     * Note that toggling any of the LED settings to 'true' won't necessarily
     * turn on the LED. The current CPLD config register for this channel must
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
     * LO controls
     *************************************************************************/
    //! Write to a register on an LO
    //
    // Note: All eight LOs are accessed through the same CPLD register. For
    // timed commands to the LOs, it is up to the call site to ensure that SPI
    // writes/reads do not get interleaved.
    //
    // Note: This will not poll the ready bit of the CPLD. To ensure valid
    // transactions, either manually call lo_spi_ready(), or make sure that SPI
    // commands are timed appropriately, i.e., new SPI transaction requests reach
    // the CPLD only after the previous transaction is comppleted.
    //
    // \param lo Which LO to write to.
    // \param addr The address of the LO register (see the LMX2572 datasheet)
    // \param data The data to write to the LO register (see the LMX2572 datasheet)
    void lo_poke16(const zbx_lo_t lo, const uint8_t addr, const uint16_t data);

    //! Read back from the LO
    //
    // Note: The LMX2572 has a MUXout pin, not just an SDO pin. This means the
    // call site needs to ensure that MUXout configuration is in the correct
    // state before calling this function (to either read back the lock status,
    // or the SPI read return value).
    //
    // Note: This will not poll the ready bit of the CPLD. To ensure valid
    // transactions, either manually call lo_spi_ready(), or make sure that SPI
    // commands are timed appropriately, i.e., new SPI transaction requests reach
    // the CPLD only after the previous transaction is comppleted.
    //
    // \param lo Which LO to read from
    // \param addr Which address on the LO to read from (see LMX2572 datasheet)
    // \param valid_timeout_ms After triggering the transaction, the function will
    //        wait for this many ms before throwing an exception. A zero timeout
    //        is possible, which means the first read to the LO_SPI_STATUS
    //        register must already have the ready bit high.
    uint16_t lo_peek16(const zbx_lo_t lo, const uint8_t addr);

    //! Returns true if the LO_SPI_READY bit is high, i.e., the LO SPI is ready
    // for a transaction
    bool lo_spi_ready();

    //! LO's incoming source control (external/internal) is actually found in
    // the CPLD path control register spaces
    //
    // \param idx Table index
    // \param lo Which LO to read from
    // \param lo_source Set LO source to internal/external
    void set_lo_source(
        const size_t idx, const zbx_lo_t lo, const zbx_lo_source_t lo_source);

    //! Retrieve lo source
    // \param idx Table index to read from
    // \param lo Which LO to read from
    zbx_lo_source_t get_lo_source(const size_t idx, zbx_lo_t lo);

    //! Synchronize LOs
    //
    // This will assert a SYNC pulse on all the LOs listed in \p los.
    //
    // Note: This function will throw an exception if LO sync bypass is enabled
    // (see set_lo_sync_bypass()).
    //
    // A note on timing: Like most CPLD controls, the time is inherited from the
    // underlying register interface. That is to say, the APIs don't take a time
    // as an argument, but assume the command time is correctly applied.
    // The different channels of the ZBX (channel 0/1) may have different command
    // times. Because this API potentially affects both channels at once, the
    // channel index must be provided to determine which channel's time should
    // be used.
    //
    // \param ref_chan The channel that is used as a timing reference.
    // \param los A list of LOs to synchronize
    // \throws uhd::runtime_error if LO sync bypass is enabled.
    void pulse_lo_sync(const size_t ref_chan, const std::vector<zbx_lo_t>& los);

    //! Enable/disable LO sync bypass
    //
    // This is a ZBX-specific option, which will allow synchronizing the LOs via
    // the MB_SYNTH_SYNC pin instead of using a register. Enabling this will
    // disable the ability to call pulse_lo_sync().
    //
    // \param enable If true, enables the bypass. When false, disables the bypass
    //               and pulse_lo_sync() can be called.
    void set_lo_sync_bypass(const bool enable);

    /*! Write DSA table for TX frequency to DB CPLD
     */
    void update_tx_dsa_settings(
        const std::vector<uint32_t>& dsa1_table, const std::vector<uint32_t>& dsa2_table);

    /*! Write DSA table for RX frequency to DB CPLD
     */
    void update_rx_dsa_settings(const std::vector<uint32_t>& dsa1_table,
        const std::vector<uint32_t>& dsa2_table,
        const std::vector<uint32_t>& dsa3a_table,
        const std::vector<uint32_t>& dsa3b_table);

private:
    /*! Dump the state of the registers into the CPLD
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

    /*! Update a register field by peeking the corresponding CPLD register
     *
     * This will synchronize the state of the internal register cache with the
     * actual value from the CPLD. This will incur a single peek (non-timed) to
     * the chip before returning.
     */
    void update_field(const zbx_cpld_regs_t::zbx_cpld_field_t field, const size_t idx);

    /*! Perform an LO SPI transaction (interact with the LO_SPI_STATUS register)
     *
     * Note: This has the ability to throttle the SPI transactions. The reason
     * is that the peek/poke interface from UHD to the CPLD is faster than the
     * SPI interface from the CPLD to the LO. If two SPI writes were to be
     * sent without a throttle, the second one would clobber the first.  Never
     * call this with throttle == false if another SPI transaction is following!
     *
     * \param lo Which LO to address
     * \param addr 7-bit address of the LO's register
     * \param data 16-bit data to write (can be empty for reads)
     * \param write If true, write, else read
     * \param throttle If true, wait after writing, so that a following SPI
     *                 transaction won't clobber the previous one
     */
    void _lo_spi_transact(const zbx_lo_t lo,
        const uint8_t addr,
        const uint16_t data,
        const spi_xact_t xact_type,
        const bool throttle = true);

    /*! Write a list of values to a register.
     * The list start address is searched using reg_addr_name.
     * The method will raise an exception if values is longer than
     * the register size.
     * The caller is responsible to commit the data once values are written.
     * This allows multiple vector writes with a single commit.
     */
    void write_register_vector(
        const std::string& reg_addr_name, const std::vector<uint32_t>& values);

    // Cached register state
    zbx_cpld_regs_t _regs = zbx_cpld_regs_t();

    //! Poker object
    poke_fn_type _poke32;

    //! Peeker object
    peek_fn_type _peek32;

    //! Hardware-timed sleep, used to throttle pokes
    sleep_fn_type _sleep;

    // Address offset (on top of _db_cpld_offset) where the LO SPI register is
    const uint32_t _lo_spi_offset;

    // infos about the daughtherboard revision
    std::string _db_rev_info;

    const std::string _log_id;
};

}}} // namespace uhd::usrp::zbx

namespace uhd { namespace usrp { namespace zbx {
// << Operator overload for expert's node printing (zbx_lo_source_t property)
// Any added expert nodes of type enum class will have to define this
std::ostream& operator<<(
    std::ostream& os, const ::uhd::usrp::zbx::zbx_cpld_ctrl::atr_mode& lo_source);
}}} // namespace uhd::usrp::zbx
