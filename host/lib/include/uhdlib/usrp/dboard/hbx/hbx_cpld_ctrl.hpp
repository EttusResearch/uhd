//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "hbx_constants.hpp"
#include <uhd/features/internal_sync_iface.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/time_spec.hpp>
#include <hbx_cpld_regs.hpp>
#include <unordered_map>
#include <array>
#include <chrono>
#include <functional>
#include <mutex>

namespace uhd { namespace usrp { namespace hbx {

/*! HBX Control class
 *
 * This class is very similar to the ZBX CPLD Control class.
 */
class hbx_cpld_ctrl : public internal_sync_actor
{
public:
    using sptr = std::shared_ptr<hbx_cpld_ctrl>;
    enum chan_t { CHAN0, NO_CHAN };
    enum init_t { INIT, FINALIZE };
    enum class dsa_t {
        TX_RF_DSA,
        RX_RF_DSA,
        RX_LF_DSA1,
        RX_LF_DSA2,
        TX_LO_DSA,
        RX_LO_DSA
    };
    enum class atr_t { CLASSIC_ATR, SW_DEFINED };

    using mixer_callback_t = std::function<void(const init_t)>;
    using pd_read_fct_t    = std::function<double()>;

    using poke_fn_type =
        std::function<void(const uint32_t, const uint32_t, const chan_t)>;
    using peek_fn_type  = std::function<uint32_t(const uint32_t)>;
    using sleep_fn_type = std::function<void(const uhd::time_spec_t&)>;

    hbx_cpld_ctrl(poke_fn_type&& poke_fn,
        peek_fn_type&& peek_fn,
        sleep_fn_type&& sleep_fn,
        const std::string& log_id);

    ~hbx_cpld_ctrl(void) = default;

    /*!
     * Get address from CPLD regmap
     */
    uint16_t get_addr(const std::string& field);

    /*!
     * Mixer init callback method
     *
     * \param finalize false when the callback is called before any SPI steps and true
     * afterwards
     * \param trx The direction of the mixer (TX or RX)
     */
    void mixer_init_callback(const init_t step, const direction_t trx);

    /*! Setting switches required for antenna mode switching, transmitting side
     *
     * \param idx Table index
     * \param loopback true if the loopback is enabled, false for antenna
     * \param tx true if the TX path is selected, false for RX
     */
    void set_tx_antenna_switches(const uint8_t idx, const bool loopback, const bool tx);

    /*! Setting switches required for antenna mode switching, receiving side
     *
     * \param idx Table index
     * \param setting The RX antenna switch setting
     * \param sync_inj true if the sync injection is enabled
     */
    void set_rx_antenna_switches(
        const uint8_t idx, const rx_ant_t setting, const bool sync_inj);

    /*! Setting switches for LO import and export
     *
     * \param lo_export true if the LO is exported to the outside world
     * \param lo_import true if the LO is imported from the outside world
     * \param trx The direction of the LO (TX or RX)
     */
    void set_lo_switches_and_leds(
        const bool lo_export, const bool lo_import, const direction_t trx);

    /**************************************************************************
     * LED controls
     *************************************************************************/
    /*! Turn the TX/RX LEDs on or off
     *
     * There are two LEDs on HBX, the TRX LED has a red and green component.
     *
     * Note that toggling any of the LED settings to 'true' won't necessarily
     * turn on the LED. The current config register for this channel must
     * also match \p idx in order for this state to be applied.
     *
     * \param idx The LED table index that is configured
     * \param rx On-state of the green RX2 LED
     * \param trx_rx On-state of the green TX/RX LED
     * \param trx_tx On-state of the red TX/RX LED
     */
    void set_leds(const uint8_t idx, const bool rx, const bool trx_rx, const bool trx_tx);

    /*!
     * Set the TX RF path switches based on the provided tune map item.
     *
     * \param tune_map_item The `hbx_tune_map_item_t` containing the switch configuration.
     */
    void set_tx_rf_path_switches(const hbx_tune_map_item_t& tune_map_item);

    /*!
     * Set the RX RF path switches based on the provided tune map item.
     *
     * \param tune_map_item The `hbx_tune_map_item_t` containing the configuration.
     */
    void set_rx_rf_path_switches(const hbx_tune_map_item_t& tune_map_item);

    /*!
     * Set the value of the specified DSA
     *
     * This function sets the value of the specified DSA. This method takes the ATR state
     * index idx as parameter so that per state the DSA settings can be different. By
     * default however, the DSA settings are the same for all ATR states because the DSAs
     * themselves are too slow to enable a clear signal immediately when RX or TX are
     * starting. Still for custom use cases, we don't want to block the usage.
     *
     * \param idx The ATR state index to be configured
     * \param dsa The DSA to set (TX_RF_DSA, RX_RF_DSA, RX_LF_DSA1, RX_LF_DSA2, TX_LO_DSA,
     * RX_LO_DSA)
     * \param value The value to set the DSA to
     */
    uint8_t set_dsa(const uint8_t idx, const dsa_t dsa, const uint8_t value);

    /*!
     * Set the desired ATR mode
     *
     * The possible ATR modes are:
     * - Classic (FPGA sets according to streaming state, IDLE, RX, TX, TRX)
     * - SW (Software)
     *
     * \param mode The desired ATR mode
     */
    void set_atr_mode(atr_t mode);

    /*!
     * Set the ATR state
     *
     * This function sets the ATR state idx if the ATR mode is set to SW. If the ATR mode
     * is set to CLASSIC_ATR, this function does nothing.
     *
     * \param idx The ATR state idx to set (0-255)
     */
    void set_atr_state(const uint8_t idx);

    /**************************************************************************
     * LO power detector read
     *************************************************************************/
    /*!
     * Get the LO power detector value for the specified direction.
     *
     * This function reads the LO power detector value for the specified direction
     * (RX or TX)..
     *
     * \param trx The direction of the LO power detector (RX or TX)
     * \param read_fn The function to read the LO power detector value from the hardware
     * \return The LO power detector value as a double
     */
    double read_lo_pd_val(const direction_t trx, const pd_read_fct_t& read_fn);

    /**************************************************************************
     * Internal SYNC signal control
     *************************************************************************/
    /*!
     * Enables or disables the internal sync clock (5 MHz) on this daughterboard.
     *
     * \param enable
     */
    void set_internal_sync_clk(const bool enable) override;

    class spi_transactor
    {
    public:
        /*!
         * Create an SPI transactor object for the SPI engine of a hardware component.
         * Based on the information in the info register located at the start_address,
         * the address width and data width for any SPI transactions are retrieved.
         *
         * \param start_address The address associated with the SPI engine of a component
         * (like RX LO, TX LO, ...)
         * \param peek_fn The peek function to read data from the hardware component.
         * \param poke_fn The poke function to write data to the hardware component.
         * \param queue If true, transactions can be queued without waiting for SPI ready
         *
         */
        spi_transactor(size_t start_address,
            poke_fn_type&& poke_fn,
            peek_fn_type&& peek_fn,
            const bool queue = false);

        ~spi_transactor(void) = default;

        /*!
         * Perform an SPI write transaction.
         *
         * This function writes data to a specified addr of the hardware component using
         * an SPI transaction.
         *
         * \param addr The register address on the hardware component where the data will
         * be written. Hardware component is determined by the _start_address.
         * \param data The data to write to the specified address. This is the value that
         * will be written to the register at the addr.
         */
        void spi_write(const uint32_t addr, const uint32_t data);

        /*!
         * Perform an SPI read transaction.
         *
         * This function reads data from a specified addr of the hardware component using
         * an SPI transaction.
         *
         * \param addr The register address on the hardware component where the data will
         * be read from. Hardware component is determined by the _start_address.
         */
        uint32_t spi_read(const uint32_t addr);


    private:
        // The logging ID for this object including the SPI start address
        std::string _log_id;
        // Each component in HBX (e.g TX LO, RX LO) has its own SPI engine and we can
        // distinguish between them via the start_address
        size_t _start_address;

        // Width of the address and data fields for the particular component
        size_t _address_width;
        size_t _data_width;

        //! Poker object
        poke_fn_type _poke32;

        //! Peeker object
        peek_fn_type _peek32;

        // Stores data read from an SPI transaction
        uint32_t _spi_read_data;

        // Tells us if we can queue transactions or not
        bool _queue;

        // In HBX, all SPI engines have their SPI setup and SPI status registers at
        // defined offsets from the start_address
        static constexpr uint32_t SPI_SETUP_OFFSET  = 0x0004;
        static constexpr uint32_t SPI_STATUS_OFFSET = 0x0008;

        // Timeout value for waiting for SPI to be ready for a transaction
        static constexpr uint32_t HBX_SPI_READY_TIMEOUT_MS = 100;

        // Constants for SPI_READY and SPI_DATA_VALID bits in the SPI_STATUS register
        // SPI_READY bit being set indicates that the hardware component is ready for an
        // SPI transaction
        static constexpr uint32_t SPI_READY_BIT = 31;
        // SPI_READ_READY bit being set indicates that the data is ready to be read back
        // from the hardware component
        static constexpr uint32_t SPI_READ_READY_BIT = 30;

        /*!
         * Main function for performing the SPI transaction.
         *
         * This function performs an SPI transaction of the specified type (READ or WRITE)
         * with the given address and data.
         *
         * \param is_read true for read, false for a write transaction.
         * \param addr The register address on the hardware component to transact with.
         * \param data The data to write (only required for write transactions).
         */
        void spi_transact(
            const bool is_read, const uint32_t addr, const uint32_t data = 0);

        // Functions to verify if the SPI engine is ready for a transaction or data is
        // ready to be read back
        void poll_spi_ready(const bool is_read = false);
    };


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
    void commit(const bool save_all = false);

    /*! Update a register field by peeking the corresponding CPLD register
     *
     * This will synchronize the state of the internal register cache with the actual
     * value from the CPLD. This will incur a single peek (non-timed) to the chip before
     * returning. All other bitfields in the same register will be updated as well.
     * */
    void update_field(const std::string& field, const size_t idx);

    // Cached register state
    hbx_cpld_regs_t _regs = hbx_cpld_regs_t();

    //! Poker object
    poke_fn_type _poke32;

    //! Peeker object
    peek_fn_type _peek32;

    //! Hardware-timed sleep, used to throttle pokes
    sleep_fn_type _sleep;

    //! infos about the daughtherboard revision
    std::string _db_rev_info;

    const std::string _log_id;

    // The LO switches are controlled when changing the bands, but also when reading the
    // LO power detector. This mutex ensures, they are not coming in their way.
    std::mutex _rx_lo_sw_mutex;
    std::mutex _tx_lo_sw_mutex;

    /*!
     * Set the TX signal path switches in the CPLD registers.
     *
     * Note: Those methods will only set the SW regmap, a call to commit() is required to
     * apply this to the hardware.
     * Note: While by default we only use the first four states to map the Classic ATR
     * (Idle, RX, TX, TRX), the index can in theory span up to 255. To leave this open for
     * user changes, we don't use an enum here.
     * \param idx The index of the ATR state to be configured
     * \param trx_band The band to be set
     * \param trx_band1_subband The subband to be set
     * \param trx_filter_branch The filter branch to be set
     */
    void set_tx_path_switches(
        const uint8_t idx, const uint8_t tx_band, const uint8_t tx_filter_branch);
    void set_tx_filter_branch(
        const uint8_t idx, const uint8_t tx_band, const uint8_t tx_filter_branch);
    void set_tx_lo_switches(const uint8_t idx,
        const uint8_t tx_band,
        const std::vector<uint8_t>& lo_filter_branch);
    void set_tx_lo_filter_branch(const uint8_t idx,
        const uint8_t tx_band,
        const std::vector<uint8_t>& lo_filter_branch);

    /*!
     * Set the RX signal path switches in the CPLD registers.
     *
     * Note: Those methods will only set the SW regmap, a call to commit() is required to
     * apply this to the hardware.
     * Note: While by default we only use the first four states to map the Classic ATR
     * (Idle, RX, TX, TRX), the index can in theory span up to 255. To leave this open for
     * user changes, we don't use an enum here.
     * \param idx The index of the ATR state to be configured
     * \param trx_band The band to be set
     * \param trx_band1_subband The subband to be set
     * \param trx_filter_branch The filter branch to be set
     */
    void set_rx_path_switches(const uint8_t idx,
        const uint8_t trx_band,
        const uint8_t rx_band1_subband,
        const uint8_t rx_filter_branch);
    void set_rx_filter_branch(const uint8_t idx,
        const uint8_t rx_band,
        const uint8_t rx_band1_subband,
        const uint8_t tx_filter_branch);
    void set_rx_lo_switches(const uint8_t idx,
        const uint8_t rx_band,
        const uint8_t rx_band1_subband,
        const std::vector<uint8_t>& lo_filter_branch);
    void set_rx_lo_filter_branch(const uint8_t idx,
        const uint8_t rx_band,
        const uint8_t rx_band1_subband,
        const std::vector<uint8_t>& lo_filter_branch);
};


}}} // namespace uhd::usrp::hbx
