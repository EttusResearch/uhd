//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_MAGNESIUM_CPLD_CTRL_HPP
#define INCLUDED_LIBUHD_MAGNESIUM_CPLD_CTRL_HPP

#include "adf4351_regs.hpp"
#include <uhd/types/serial.hpp>
#include "magnesium_cpld_regs.hpp"
#include <mutex>
#include <memory>

//! Controls the CPLD on a Magnesium daughterboard
//
// Setters are thread-safe through lock guards. This lets a CPLD control object
// be shared by multiple owners.
class magnesium_cpld_ctrl
{
public:
    /**************************************************************************
     * Types
     *************************************************************************/
    using sptr = std::shared_ptr<magnesium_cpld_ctrl>;
    //! SPI write functor: Can take a SPI transaction and clock it out
    using write_spi_t = std::function<void(uint32_t)>;
    //! SPI read functor: Return SPI
    using read_spi_t = std::function<uint32_t(uint32_t)>;

    //! ATR state: The CPLD has 2 states for RX and TX each, not like the radio
    //  which has 4 states (one for every RX/TX state combo).
    enum atr_state_t {
        IDLE,
        ON,
        ANY
    };

    //! Channel select: One CPLD controls both channels on a daughterboard
    enum chan_sel_t {
        CHAN1,
        CHAN2,
        BOTH
    };

    enum tx_sw1_t {
        TX_SW1_SHUTDOWNTXSW1 = 0,
        TX_SW1_FROMTXFILTERLP1700MHZ = 1,
        TX_SW1_FROMTXFILTERLP3400MHZ = 2,
        TX_SW1_FROMTXFILTERLP0800MHZ = 3
    };

    enum tx_sw2_t {
        TX_SW2_TOTXFILTERLP3400MHZ = 1,
        TX_SW2_TOTXFILTERLP1700MHZ = 2,
        TX_SW2_TOTXFILTERLP0800MHZ = 4,
        TX_SW2_TOTXFILTERLP6400MHZ = 8
    };

    enum tx_sw3_t {
        TX_SW3_TOTXFILTERBANKS = 0,
        TX_SW3_BYPASSPATHTOTRXSW = 1
    };

    enum sw_trx_t {
        SW_TRX_FROMLOWERFILTERBANKTXSW1 = 0,
        SW_TRX_FROMTXUPPERFILTERBANKLP6400MHZ = 1,
        SW_TRX_RXCHANNELPATH = 2,
        SW_TRX_BYPASSPATHTOTXSW3 = 3
    };

    enum rx_sw1_t {
        RX_SW1_TXRXINPUT = 0,
        RX_SW1_RXLOCALINPUT = 1,
        RX_SW1_TRXSWITCHOUTPUT = 2,
        RX_SW1_RX2INPUT = 3
    };

    enum rx_sw2_t {
        RX_SW2_SHUTDOWNSW2 = 0,
        RX_SW2_LOWERFILTERBANKTOSWITCH3 = 1,
        RX_SW2_BYPASSPATHTOSWITCH6 = 2,
        RX_SW2_UPPERFILTERBANKTOSWITCH4 = 3
    };

    enum rx_sw3_t {
        RX_SW3_FILTER2100X2850MHZ = 0,
        RX_SW3_FILTER0490LPMHZ = 1,
        RX_SW3_FILTER1600X2250MHZ = 2,
        RX_SW3_FILTER0440X0530MHZ = 4,
        RX_SW3_FILTER0650X1000MHZ = 5,
        RX_SW3_FILTER1100X1575MHZ = 6,
        RX_SW3_SHUTDOWNSW3 = 7
    };

    enum rx_sw4_t {
        RX_SW4_FILTER2100X2850MHZFROM = 1,
        RX_SW4_FILTER1600X2250MHZFROM = 2,
        RX_SW4_FILTER2700HPMHZ = 4
    };

    enum rx_sw5_t {
        RX_SW5_FILTER0440X0530MHZFROM = 1,
        RX_SW5_FILTER1100X1575MHZFROM = 2,
        RX_SW5_FILTER0490LPMHZFROM = 4,
        RX_SW5_FILTER0650X1000MHZFROM = 8
    };

    enum rx_sw6_t {
        RX_SW6_LOWERFILTERBANKFROMSWITCH5 = 1,
        RX_SW6_UPPERFILTERBANKFROMSWITCH4 = 2,
        RX_SW6_BYPASSPATHFROMSWITCH2 = 4
    };

    enum lowband_mixer_path_sel_t {
        LOWBAND_MIXER_PATH_SEL_BYPASS = 0,
        LOWBAND_MIXER_PATH_SEL_LOBAND = 1
    };


    /*! Constructor.
     *
     * \param write_spi_fn SPI write functor
     * \param read_spi_fn SPI read functor
     */
    magnesium_cpld_ctrl(
        write_spi_t write_spi_fn,
        read_spi_t read_spi_fn
    );

    /**************************************************************************
     * API
     *************************************************************************/
    //! Reset all registers to their default state
    void reset();

    //! Return the current value of register at \p addr.
    //
    // Note: This will initiate a SPI transaction, it doesn't read from the
    // internal register cache. However, it won't actually update the register
    // cache.
    uint16_t get_reg(const uint8_t addr);

    //! Set the value of the scratch reg (no effect, for debugging only)
    void set_scratch(const uint16_t val);

    //! Get the value of the scratch reg.
    //
    // This should be zero unless set_scratch() was called beforehand (note
    // that _loopback_test() will also call set_scratch()). If set_scratch()
    // was previously called, this should return the previously written value.
    //
    // Note: This will call get_reg(), and not simply return the value of the
    // internal cache.
    uint16_t get_scratch();

    /*! Frequency-related settings, transmit side
     *
     * \param chan Which channel do these settings apply to? Use BOTH to set
     *             both channels at once.
     * \param tx_sw1 Filter bank switch 1
     * \param tx_sw2 Filter bank switch 2
     * \param tx_sw3 Filter bank switch 3
     * \param select_lowband_mixer_path Enable the lowband mixer path
     * \param enb_lowband_mixer Enable the actual lowband mixer
     * \param atr_state If IDLE, only update the idle register. If ON, only
     *                  enable the on register. If ANY, update both.
     */
    void set_tx_switches(
        const chan_sel_t chan,
        const tx_sw1_t tx_sw1,
        const tx_sw2_t tx_sw2,
        const tx_sw3_t tx_sw3,
        const lowband_mixer_path_sel_t select_lowband_mixer_path,
        const bool enb_lowband_mixer,
        const atr_state_t atr_state = ANY,
        const bool defer_commit = false
    );

    /*! Frequency-related settings, receive side
     *
     * Note: RX switch 1 is on set_rx_atr_bits().
     *
     * \param chan Which channel do these settings apply to? Use BOTH to set
     *             both channels at once.
     * \param rx_sw2 Filter bank switch 2
     * \param rx_sw3 Filter bank switch 3
     * \param rx_sw4 Filter bank switch 4
     * \param rx_sw5 Filter bank switch 5
     * \param rx_sw6 Filter bank switch 6
     * \param select_lowband_mixer_path Enable the lowband mixer path
     * \param enb_lowband_mixer Enable the actual lowband mixer
     * \param atr_state If IDLE, only update the idle register. If ON, only
     *                  enable the on register. If ANY, update both.
     */
    void set_rx_switches(
        const chan_sel_t chan,
        const rx_sw2_t rx_sw2,
        const rx_sw3_t rx_sw3,
        const rx_sw4_t rx_sw4,
        const rx_sw5_t rx_sw5,
        const rx_sw6_t rx_sw6,
        const lowband_mixer_path_sel_t select_lowband_mixer_path,
        const bool enb_lowband_mixer,
        const atr_state_t atr_state = ANY,
        const bool defer_commit = false
    );

    /*! ATR settings: LEDs, PAs, LNAs, ... for TX side
     *
     * Note: These ATR states are not frequency dependent.
     *
     * Note on the tx_myk_enb bits: The AD9371 requires those pins to stay
     * high for longer than we can guarantee with out clock-cycle accurate
     * TX timing, so let's keep it turned on all the time.
     *
     * \param chan Which channel do these settings apply to? Use BOTH to set
     *             both channels at once.
     * \param atr_state TX state for which these settings apply.
     * \param tx_led State of the TX LED for this ATR state (on or off)
     * \param tx_pa_enb State of the TX PA for this ATR state (on or off)
     * \param tx_amp_enb State of the TX amp for this ATR state (on or off)
     * \param tx_myk_enb State of the AD9371 TX enable pin for this ATR state
     */
    void set_tx_atr_bits(
        const chan_sel_t chan,
        const atr_state_t atr_state,
        const bool tx_led,
        const bool tx_pa_enb,
        const bool tx_amp_enb,
        const bool tx_myk_enb,
        const bool defer_commit = false
    );

    /*! ATR settings: TRX switch
     *
     * Note: This ATR state is frequency dependent.
     *
     * \param chan Which channel do these settings apply to? Use BOTH to set
     *             both channels at once.
     * \param atr_state TX state for which these settings apply.
     * \param trx_sw State of the TRX switch for this ATR state
     */
    void set_trx_sw_atr_bits(
        const chan_sel_t chan,
        const atr_state_t atr_state,
        const sw_trx_t trx_sw,
        const bool defer_commit = false
    );

    /*! ATR settings: LEDs, input switches for RX side
     *
     * Note: These ATR states are not frequency dependent, but need to change
     * when the antenna input is switched.
     *
     * \param chan Which channel do these settings apply to? Use BOTH to set
     *             both channels at once.
     * \param atr_state ATR state for which these settings apply.
     * \param rx_sw1 Filter bank sw1 of RX path
     * \param rx_led State of the RX LED for this ATR state (on or off). This
     *               is the LED on the TX/RX port.
     * \param rx2_led State of the RX LED for this ATR state (on or off). This
     *               is the LED on the RX2 port.
     */
    void set_rx_input_atr_bits(
        const chan_sel_t chan,
        const atr_state_t atr_state,
        const rx_sw1_t rx_sw1,
        const bool rx_led,
        const bool rx2_led,
        const bool defer_commit = false
    );

    /*! ATR settings: Amp, Mykonos settings for RX side
     *
     * Note: These ATR states are not frequency dependent (or dependent on
     * anything other than RX ATR state).
     *
     * Note on the rx_myk_enb bits: The AD9371 requires those pins to stay
     * high for longer than we can guarantee without clock-cycle accurate
     * RX timing, so let's keep it turned on all the time.
     *
     * \param chan Which channel do these settings apply to? Use BOTH to set
     *             both channels at once.
     * \param atr_state ATR state for which these settings apply.
     * \param rx_amp_enb State of RX amp for this ATR state (on or off).
     * \param rx_myk_enb State of the AD9371 RX enable pin for this ATR state
     */
    void set_rx_atr_bits(
        const chan_sel_t chan,
        const atr_state_t atr_state,
        const bool rx_amp_enb,
        const bool rx_myk_en,
        const bool defer_commit = false
    );

    /*! ATR settings: LNAs for RX side
     *
     * Note: These ATR states are frequency dependent.
     *
     * Note on the rx_myk_enb bits: The AD9371 requires those pins to stay
     * high for longer than we can guarantee with out clock-cycle accurate
     * RX timing, so let's keep it turned on all the time.
     *
     * \param chan Which channel do these settings apply to? Use BOTH to set
     *             both channels at once.
     * \param atr_state ATR state for which these settings apply.
     * \param rx_lna1_enb State of RX LNA 1 for this ATR state (on or off).
     *                    This is the high-band LNA.
     * \param rx_lna2_enb State of RX LNA 2 for this ATR state (on or off).
     *                    This is the low-band LNA.
     */
    void set_rx_lna_atr_bits(
        const chan_sel_t chan,
        const atr_state_t atr_state,
        const bool rx_lna1_enb,
        const bool rx_lna2_enb,
        const bool defer_commit = false
    );

private:
    //! Write functor: Take address / data pair, craft SPI transaction
    using write_fn_t = std::function<void(uint32_t, uint32_t)>;
    //! Read functor: Return value given address
    using read_fn_t = std::function<uint32_t(uint32_t)>;

    //! Dump the state of the registers into the CPLD
    //
    // \param save_all If true, save all registers. If false, only change those
    //                 that changes recently.
    void commit(const bool save_all = false);

    //! Writes to the scratch reg and reads again. Throws on failure.
    //
    // Note: This is not thread-safe. Accesses to the scratch reg are not
    // atomic. Only call this from a thread-safe environment, please.
    void _loopback_test();

    //! Write functor for regs pokes
    write_fn_t _write_fn;
    //! Read functor for regs peeks
    read_fn_t _read_fn;

    //! Current state of the CPLD registers (for write operations only)
    magnesium_cpld_regs_t _regs;

    //! Lock access to setters
    std::mutex _set_mutex;
};

#endif /* INCLUDED_LIBUHD_MAGNESIUM_CPLD_CTRL_HPP */
// vim: sw=4 et:
