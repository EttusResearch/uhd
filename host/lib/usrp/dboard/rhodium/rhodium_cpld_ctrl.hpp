//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RHODIUM_CPLD_CTRL_HPP
#define INCLUDED_LIBUHD_RHODIUM_CPLD_CTRL_HPP

#include "adf4351_regs.hpp"
#include "rhodium_cpld_regs.hpp"
#include <uhd/types/direction.hpp>
#include <uhd/types/serial.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

//! Controls the CPLD on a Rhodium daughterboard
//
// Setters are thread-safe through lock guards. This lets a CPLD control object
// be shared by multiple owners.
class rhodium_cpld_ctrl
{
public:
    /**************************************************************************
     * Types
     *************************************************************************/
    using sptr = std::shared_ptr<rhodium_cpld_ctrl>;
    //! SPI write function: Can take a SPI transaction and clock it out
    using write_spi_t = std::function<void(uint32_t)>;
    //! SPI read function: Return SPI
    using read_spi_t = std::function<uint32_t(uint32_t)>;

    enum gain_band_t {
        LOW,
        HIGH,
    };

    enum tx_sw1_t {
        TX_SW1_TOLOWBAND     = 0,
        TX_SW1_TOSWITCH2     = 1,
        TX_SW1_TOCALLOOPBACK = 2,
        TX_SW1_ISOLATION     = 3
    };

    enum tx_sw2_t {
        TX_SW2_FROMSWITCH3           = 0,
        TX_SW2_FROMTXFILTERLP6000MHZ = 1,
        TX_SW2_FROMTXFILTERLP4100MHZ = 2,
        TX_SW2_FROMTXFILTERLP3000MHZ = 3
    };

    enum tx_sw3_sw4_t {
        TX_SW3_SW4_FROMTXFILTERLP1000MHZ = 1,
        TX_SW3_SW4_FROMTXFILTERLP0650MHZ = 2,
        TX_SW3_SW4_FROMTXFILTERLP1900MHZ = 4,
        TX_SW3_SW4_FROMTXFILTERLP1350MHZ = 8
    };

    enum tx_sw5_t {
        TX_SW5_TOTXFILTERLP3000MHZ = 0,
        TX_SW5_TOTXFILTERLP4100MHZ = 1,
        TX_SW5_TOTXFILTERLP6000MHZ = 2,
        TX_SW5_TOSWITCH4           = 3
    };

    enum rx_sw1_t {
        RX_SW1_FROMCALLOOPBACK = 0,
        RX_SW1_FROMRX2INPUT    = 1,
        RX_SW1_ISOLATION       = 2,
        RX_SW1_FROMTXRXINPUT   = 3
    };

    enum rx_sw2_sw7_t {
        RX_SW2_SW7_LOWBANDFILTERBANK  = 0,
        RX_SW2_SW7_HIGHBANDFILTERBANK = 1
    };

    enum rx_sw3_t {
        RX_SW3_TOSWITCH4            = 0,
        RX_SW3_TOFILTER4500X6000MHZ = 1,
        RX_SW3_TOFILTER3000X4500MHZ = 2,
        RX_SW3_TOFILTER2050X3000MHZ = 3
    };

    enum rx_sw4_sw5_t {
        RX_SW4_SW5_FILTER0760X1100MHZ = 1,
        RX_SW4_SW5_FILTER0450X0760MHZ = 2,
        RX_SW4_SW5_FILTER1410X2050MHZ = 4,
        RX_SW4_SW5_FILTER1100X1410MHZ = 8
    };

    enum rx_sw6_t {
        RX_SW6_FROMFILTER2050X3000MHZ = 0,
        RX_SW6_FROMFILTER3000X4500MHZ = 1,
        RX_SW6_FROMFILTER4500X6000MHZ = 2,
        RX_SW6_FROMSWITCH5            = 3,
    };

    enum cal_iso_sw_t { CAL_ISO_ISOLATION = 0, CAL_ISO_CALLOOPBACK = 1 };

    enum tx_hb_lb_sel_t { TX_HB_LB_SEL_LOWBAND = 0, TX_HB_LB_SEL_HIGHBAND = 1 };

    enum tx_lo_input_sel_t { TX_LO_INPUT_SEL_INTERNAL = 0, TX_LO_INPUT_SEL_EXTERNAL = 1 };

    enum rx_hb_lb_sel_t { RX_HB_LB_SEL_LOWBAND = 0, RX_HB_LB_SEL_HIGHBAND = 1 };

    enum rx_lo_input_sel_t { RX_LO_INPUT_SEL_INTERNAL = 1, RX_LO_INPUT_SEL_EXTERNAL = 0 };

    enum rx_demod_adj { RX_DEMOD_OPEN = 0, RX_DEMOD_200OHM = 1, RX_DEMOD_1500OHM = 2 };

    enum tx_lo_filter_sel_t {
        TX_LO_FILTER_SEL_0_9GHZ_LPF  = 0,
        TX_LO_FILTER_SEL_5_85GHZ_LPF = 1,
        TX_LO_FILTER_SEL_2_25GHZ_LPF = 2,
        TX_LO_FILTER_SEL_ISOLATION   = 3
    };

    enum rx_lo_filter_sel_t {
        RX_LO_FILTER_SEL_0_9GHZ_LPF  = 0,
        RX_LO_FILTER_SEL_5_85GHZ_LPF = 1,
        RX_LO_FILTER_SEL_2_25GHZ_LPF = 2,
        RX_LO_FILTER_SEL_ISOLATION   = 3
    };

    /*! Constructor.
     *
     * \param write_spi_fn SPI write function
     * \param read_spi_fn SPI read function
     */
    rhodium_cpld_ctrl(write_spi_t write_spi_fn, read_spi_t read_spi_fn);

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

    //! Set the value of the scratch register (has no effect on chip functions)
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
     * \param tx_sw2 Filter bank switch 2
     * \param tx_sw3_sw4 Filter bank switch 3 and 4
     * \param tx_sw5 Filter bank switch 5
     * \param tx_hb_lb_sel Power on the highband or lowband amplifier
     * \param tx_lo_filter_sel Select LPF filter for LO
     */
    void set_tx_switches(const tx_sw2_t tx_sw2,
        const tx_sw3_sw4_t tx_sw3_sw4,
        const tx_sw5_t tx_sw5,
        const tx_hb_lb_sel_t tx_hb_lb_sel,
        const bool defer_commit = false);

    /*! Frequency-related settings, receive side
     *
     * \param rx_sw2_sw7 Filter bank switch 2 and 7
     * \param rx_sw3 Filter bank switch 3
     * \param rx_sw4_sw5 Filter bank switch 4 and 5
     * \param rx_sw6 Filter bank switch 6
     * \param rx_hb_lb_sel Power on the highband or lowband amplifier
     * \param rx_lo_filter_sel Select LPF filter for LO
     */
    void set_rx_switches(const rx_sw2_sw7_t rx_sw2_sw7,
        const rx_sw3_t rx_sw3,
        const rx_sw4_sw5_t rx_sw4_sw5,
        const rx_sw6_t rx_sw6,
        const rx_hb_lb_sel_t rx_hb_lb_sel,
        const bool defer_commit = false);

    /*! Input switches for RX side
     *
     * Note: These are not frequency dependent.
     *
     * \param rx_sw1 Input selection of RX path
     * \param cal_iso_sw Terminates the calibration loopback path
     */
    void set_rx_input_switches(const rx_sw1_t rx_sw1,
        const cal_iso_sw_t cal_iso_sw,
        const bool defer_commit = false);

    /*! Output switches for TX side
     *
     * Note: These are not frequency dependent.
     *
     * \param tx_sw1 Output selection of TX path
     */
    void set_tx_output_switches(const tx_sw1_t tx_sw1, const bool defer_commit = false);

    /*! Input switch for RX LO
     *
     * \param rx_lo_input_sel Selects RX LO source
     */
    void set_rx_lo_source(
        const rx_lo_input_sel_t rx_lo_input_sel, const bool defer_commit = false);

    /*! Input switch for TX LO
     *
     * \param tx_lo_input_sel Selects TX LO source
     */
    void set_tx_lo_source(
        const tx_lo_input_sel_t tx_lo_input_sel, const bool defer_commit = false);

    /*! Configure RX LO filter, synth, and mixer settings
     *
     * \param freq RX LO Frequency
     */
    void set_rx_lo_path(const double freq, const bool defer_commit = false);

    /*! Configure TX LO filter, synth, and mixer settings
     *
     * \param freq TX LO Frequency
     */
    void set_tx_lo_path(const double freq, const bool defer_commit = false);


    /*! Gain index setting for the RF frontend
     *
     * Sets the gain index to one of the predefined values that have been
     * loaded into the CPLD by gain table loader in MPM.
     *
     * \param index Index of the gain table entry to apply (0-60)
     * \param band Selects which table to use (lowband or highband)
     * \param dir Selects which RF frontend to apply to (RX or TX)
     */
    void set_gain_index(const uint32_t index,
        const gain_band_t band,
        const uhd::direction_t dir,
        const bool defer_commit = false);

    /*! Gain setting for LO1
     *
     * Sets the attenuation of the RX LO1 DSA or TX LO1 DSA.
     *
     * Note: This function uses gain as a parameter, although it is
     * setting an attenuation.
     *
     * \param index Gain value to apply (0-30)
     * \param dir Selects which LO to apply to (RX, TX, or DX)
     */
    void set_lo_gain(const uint32_t index,
        const uhd::direction_t dir,
        const bool defer_commit = false);

private:
    //! Write function: Take address / data pair, craft SPI transaction
    using write_reg_fn_t = std::function<void(uint32_t, uint32_t)>;
    //! Write function: Send bits directly to CPLD
    using write_raw_fn_t = std::function<void(uint32_t)>;
    //! Read function: Return value given address
    using read_reg_fn_t = std::function<uint32_t(uint32_t)>;

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

    //! Write function for regs pokes
    write_reg_fn_t _write_reg_fn;
    //! Read function for regs peeks
    read_reg_fn_t _read_reg_fn;
    //! Write function for raw poke command
    write_raw_fn_t _write_raw_fn;

    //! Current state of the CPLD registers (for write operations only)
    rhodium_cpld_regs_t _regs;

    //! Queue of gain commands to be written at next commit
    std::vector<uint32_t> _gain_queue;

    //! Lock access to setters
    std::mutex _set_mutex;
};

#endif /* INCLUDED_LIBUHD_RHODIUM_CPLD_CTRL_HPP */
