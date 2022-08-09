//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/defaults.hpp>
#include <uhd/rfnoc/multichan_register_iface.hpp>
#include <uhd/rfnoc/radio_control.hpp>
#include <uhdlib/features/discoverable_feature_registry.hpp>
#include <uhdlib/usrp/common/pwr_cal_mgr.hpp>
#include <uhd/rfnoc/rf_control/core_iface.hpp>
#include <uhdlib/rfnoc/rf_control/gain_profile_iface.hpp>
#include <unordered_map>
#include <mutex>

#define RFNOC_RADIO_CONSTRUCTOR(CLASS_NAME) \
    CLASS_NAME##_impl(make_args_ptr make_args) : radio_control_impl(std::move(make_args))

namespace uhd { namespace rfnoc {

/*! Base class of radio controllers
 *
 * All radio control classes should derive from this class (e.g., the X300 radio
 * controller, etc.)
 *
 * Many of the radio_control API calls have virtual (default) implementations
 * here, but they can be overridden.
 */
class radio_control_impl : public radio_control,
                           public ::uhd::features::discoverable_feature_registry
{
public:
    /**************************************************************************
     * Structors
     *************************************************************************/
    radio_control_impl(make_args_ptr make_args);

    void deinit() override {}

    ~radio_control_impl() override {}

    /**************************************************************************
     * Stream control API calls
     *************************************************************************/
    void issue_stream_cmd(
        const uhd::stream_cmd_t& stream_cmd, const size_t port) override;

    void enable_rx_timestamps(const bool enable, const size_t chan) override;

    /**************************************************************************
     * Rate-Related API Calls
     *************************************************************************/
    double set_rate(const double rate) override;
    double get_rate() const override;
    meta_range_t get_rate_range() const override;
    size_t get_spc() const override;

    /**************************************************************************
     * Time-Related API Calls
     *************************************************************************/
    uint64_t get_ticks_now() override;
    uhd::time_spec_t get_time_now() override;

    /**************************************************************************
     * RF-specific API calls
     *************************************************************************/
    // Setters
    void set_tx_antenna(const std::string& ant, const size_t chan) override;
    void set_rx_antenna(const std::string& ant, const size_t chan) override;
    double set_tx_frequency(const double freq, const size_t chan) override;
    double set_rx_frequency(const double freq, const size_t chan) override;
    void set_tx_tune_args(const uhd::device_addr_t&, const size_t chan) override;
    void set_rx_tune_args(const uhd::device_addr_t&, const size_t chan) override;
    double set_tx_gain(const double gain, const size_t chan) override;
    double set_tx_gain(
        const double gain, const std::string& name, const size_t chan) override;
    double set_rx_gain(const double gain, const size_t chan) override;
    double set_rx_gain(
        const double gain, const std::string& name, const size_t chan) override;
    void set_rx_agc(const bool enable, const size_t chan) override;
    double set_tx_bandwidth(const double bandwidth, const size_t chan) override;
    double set_rx_bandwidth(const double bandwidth, const size_t chan) override;
    void set_tx_gain_profile(const std::string& profile, const size_t chan) override;
    void set_rx_gain_profile(const std::string& profile, const size_t chan) override;
    void set_rx_power_reference(const double power_dbm, const size_t chan) override;
    void set_tx_power_reference(const double power_dbm, const size_t chan) override;

    // Getters
    std::string get_tx_antenna(const size_t chan) const override;
    std::string get_rx_antenna(const size_t chan) const override;
    std::vector<std::string> get_tx_antennas(const size_t chan) const override;
    std::vector<std::string> get_rx_antennas(const size_t chan) const override;
    double get_tx_frequency(const size_t) override;
    double get_rx_frequency(const size_t) override;
    uhd::freq_range_t get_tx_frequency_range(const size_t chan) const override;
    uhd::freq_range_t get_rx_frequency_range(const size_t chan) const override;
    std::vector<std::string> get_tx_gain_names(const size_t) const override;
    std::vector<std::string> get_rx_gain_names(const size_t) const override;
    double get_tx_gain(const size_t) override;
    double get_tx_gain(const std::string&, size_t) override;
    double get_rx_gain(const size_t) override;
    double get_rx_gain(const std::string&, size_t) override;
    uhd::gain_range_t get_tx_gain_range(const size_t) const override;
    uhd::gain_range_t get_tx_gain_range(const std::string&, const size_t) const override;
    uhd::gain_range_t get_rx_gain_range(const size_t) const override;
    uhd::gain_range_t get_rx_gain_range(const std::string&, const size_t) const override;
    std::vector<std::string> get_tx_gain_profile_names(const size_t chan) const override;
    std::vector<std::string> get_rx_gain_profile_names(const size_t chan) const override;
    std::string get_tx_gain_profile(const size_t chan) const override;
    std::string get_rx_gain_profile(const size_t chan) const override;
    double get_tx_bandwidth(const size_t) override;
    double get_rx_bandwidth(const size_t) override;
    meta_range_t get_tx_bandwidth_range(size_t chan) const override;
    meta_range_t get_rx_bandwidth_range(size_t chan) const override;
    bool has_rx_power_reference(const size_t chan) override;
    bool has_tx_power_reference(const size_t chan) override;
    double get_rx_power_reference(const size_t chan) override;
    double get_tx_power_reference(const size_t chan) override;
    std::vector<std::string> get_rx_power_ref_keys(const size_t) override;
    std::vector<std::string> get_tx_power_ref_keys(const size_t) override;
    meta_range_t get_rx_power_range(const size_t chan) override;
    meta_range_t get_tx_power_range(const size_t chan) override;

    /**************************************************************************
     * LO Controls
     *************************************************************************/
    std::vector<std::string> get_rx_lo_names(const size_t chan) const override;
    std::vector<std::string> get_rx_lo_sources(
        const std::string& name, const size_t chan) const override;
    freq_range_t get_rx_lo_freq_range(
        const std::string& name, const size_t chan) const override;
    void set_rx_lo_source(
        const std::string& src, const std::string& name, const size_t chan) override;
    const std::string get_rx_lo_source(
        const std::string& name, const size_t chan) override;
    void set_rx_lo_export_enabled(
        bool enabled, const std::string& name, const size_t chan) override;
    bool get_rx_lo_export_enabled(const std::string& name, const size_t chan) override;
    double set_rx_lo_freq(
        double freq, const std::string& name, const size_t chan) override;
    double get_rx_lo_freq(const std::string& name, const size_t chan) override;
    std::vector<std::string> get_tx_lo_names(const size_t chan) const override;
    std::vector<std::string> get_tx_lo_sources(
        const std::string& name, const size_t chan) const override;
    freq_range_t get_tx_lo_freq_range(
        const std::string& name, const size_t chan) override;
    void set_tx_lo_source(
        const std::string& src, const std::string& name, const size_t chan) override;
    const std::string get_tx_lo_source(
        const std::string& name, const size_t chan) override;
    void set_tx_lo_export_enabled(
        const bool enabled, const std::string& name, const size_t chan) override;
    bool get_tx_lo_export_enabled(const std::string& name, const size_t chan) override;
    double set_tx_lo_freq(
        const double freq, const std::string& name, const size_t chan) override;
    double get_tx_lo_freq(const std::string& name, const size_t chan) override;

    /**************************************************************************
     * Calibration-Related API Calls
     *************************************************************************/
    void set_tx_dc_offset(const std::complex<double>& offset, size_t chan) override;
    meta_range_t get_tx_dc_offset_range(size_t chan) const override;
    void set_tx_iq_balance(const std::complex<double>& correction, size_t chan) override;
    void set_rx_dc_offset(const bool enb, size_t chan = ALL_CHANS) override;
    void set_rx_dc_offset(const std::complex<double>& offset, size_t chan) override;
    meta_range_t get_rx_dc_offset_range(size_t chan) const override;
    void set_rx_iq_balance(const bool enb, size_t chan) override;
    void set_rx_iq_balance(const std::complex<double>& correction, size_t chan) override;

    /**************************************************************************
     * GPIO Controls
     *************************************************************************/
    std::vector<std::string> get_gpio_banks() const override;
    void set_gpio_attr(
        const std::string& bank, const std::string& attr, const uint32_t value) override;
    uint32_t get_gpio_attr(const std::string& bank, const std::string& attr) override;

    /**************************************************************************
     * Sensor API
     *************************************************************************/
    std::vector<std::string> get_rx_sensor_names(size_t chan) const override;
    uhd::sensor_value_t get_rx_sensor(const std::string& name, size_t chan) override;
    std::vector<std::string> get_tx_sensor_names(size_t chan) const override;
    uhd::sensor_value_t get_tx_sensor(const std::string& name, size_t chan) override;

    /**************************************************************************
     * Identification API
     *************************************************************************/
    std::string get_fe_name(
        const size_t chan, const uhd::direction_t direction) const override
    {
        return get_dboard_fe_from_chan(chan, direction);
    }

    /**************************************************************************
     * EEPROM API
     *************************************************************************/
    void set_db_eeprom(const uhd::eeprom_map_t& db_eeprom) override;
    uhd::eeprom_map_t get_db_eeprom() override;

    /***********************************************************************
     * Reg Map
     **********************************************************************/
    static const uint16_t MAJOR_COMPAT;
    static const uint16_t MINOR_COMPAT;

    /*! Register map common to all radios
     *
     * See rfnoc_block_radio_regs.vh for details
     */
    struct regmap
    {
        enum
        {
            REG_COMPAT_NUM = 0x00, // Compatibility number register offset
            REG_TIME_LO    = 0x04, // Time lower bits
            REG_TIME_HI    = 0x08, // Time upper bits
            REG_RADIO_WIDTH =
                0x1000 + 0x04, // Upper 16 bits is sample width, lower 16 bits is NSPC

            RADIO_BASE_ADDR = 0x1000,
            REG_CHAN_OFFSET = 128,
            RADIO_ADDR_W    = 7, // Address space size per radio

            // General Radio Registers
            REG_LOOPBACK_EN = 0x00, // Loopback enable (connect Tx output to Rx input)

            // Note on the RX and TX Control Registers: These are per-channel,
            // which means the values here are offsets. The base address per
            // channel is RADIO_BASE_ADDR + i * REG_CHAN_OFFSET, where i is the
            // channel index.

            // RX Control Registers
            REG_RX_STATUS = 0x10, // Status of Rx radio
            REG_RX_CMD    = 0x14, // The next radio command to execute
            REG_RX_CMD_NUM_WORDS_LO =
                0x18, // Number of radio words for the next command (low word)
            REG_RX_CMD_NUM_WORDS_HI =
                0x1C, // Number of radio words for the next command (high word)
            REG_RX_CMD_TIME_LO =
                0x20, // Time for the next command (low word)
            REG_RX_CMD_TIME_HI =
                0x24, // Time for the next command (high word)
            REG_RX_MAX_WORDS_PER_PKT =
                0x28, // Maximum packet length to build from Rx data
            REG_RX_ERR_PORT = 0x2C, // Port ID for error reporting
            REG_RX_ERR_REM_PORT =
                0x30, // Remote port ID for error reporting
            REG_RX_ERR_REM_EPID =
                0x34, // Remote EPID (endpoint ID) for error reporting
            REG_RX_ERR_ADDR =
                0x38, // Offset to which to write error code (ADDR+0) and time (ADDR+8)
            REG_RX_DATA = 0x3C,
            REG_RX_HAS_TIME =
                0x70, // Set to one if radio output packets should have timestamps

            // TX Control Registers
            REG_TX_IDLE_VALUE =
                0x40, // Value to output when transmitter is idle
            REG_TX_ERROR_POLICY = 0x44, // Tx error policy
            REG_TX_ERR_PORT     = 0x48, // Port ID for error reporting
            REG_TX_ERR_REM_PORT =
                0x4C, // Remote port ID for error reporting
            REG_TX_ERR_REM_EPID =
                0x50, // Remote EPID (endpoint ID) for error reporting
            REG_TX_ERR_ADDR =
                0x54, // Offset to which to write error code (ADDR+0) and time (ADDR+8)

            RX_CMD_STOP   = 0, // Stop acquiring at end of next packet
            RX_CMD_FINITE = 1, // Acquire NUM_SAMPS then stop
            RX_CMD_CONTINUOUS = 2, // Acquire until stopped

            RX_CMD_TIMED_POS = 31,

            PERIPH_BASE       = 0x80000,
            PERIPH_REG_OFFSET = 8,

            SWREG_TX_ERR      = 0x0000,
            SWREG_RX_ERR      = 0x1000,
            SWREG_CHAN_OFFSET = 64
        };
    };

    struct err_codes
    {
        enum
        {
            //! Late command (stream command arrived after indicated time)
            ERR_RX_LATE_CMD = 1,
            //! FIFO overflow
            ERR_RX_OVERRUN = 2,
            // FIFO underrun (data not available when needed)
            ERR_TX_UNDERRUN = 1,
            //! Late data (arrived after indicated time)
            ERR_TX_LATE_DATA = 2,
            //! Acknowledge a TX burst with an EOB
            EVENT_TX_BURST_ACK = 3
        };
    };

    //! Tree path to the dboard-specific properties
    static const uhd::fs_path DB_PATH;
    //! Tree path to the radio frontends' properties
    static const uhd::fs_path FE_PATH;

protected:
    /*! Helper function for property propagation: Like set_rate(), but called
     * during a different context.
     *
     * This function is called from the samp_rate property resolver. The
     * difference to set_rate() is that the latter is a user API, and may
     * trigger different kinds of warnings or errors.
     * If the radio supports changing its sampling rate at runtime, it is OK to
     * call set_rate() within this function.
     *
     * Default implementation is to simply return the current rate.
     */
    virtual double coerce_rate(const double /* rate */)
    {
        return _rate;
    }

    /*
     * Returns the number of bytes to be processed in each clock cycle.
     */
    size_t get_atomic_item_size() const
    {
        return (_samp_width / 8) * _spc;
    }
    //! Properties for samp_rate (one per port)
    std::vector<property_t<double>> _samp_rate_in;
    //! Properties for samp_rate (one per port)
    std::vector<property_t<double>> _samp_rate_out;

    //! Block-specific register interface
    multichan_register_iface _radio_reg_iface;

    //! Power manager for RX power cal. If the radio doesn't have a power API,
    // simply leave these empty.
    std::vector<uhd::usrp::pwr_cal_mgr::sptr> _rx_pwr_mgr;
    //! Power manager for TX power cal. If the radio doesn't have a power API,
    // simply leave these empty.
    std::vector<uhd::usrp::pwr_cal_mgr::sptr> _tx_pwr_mgr;

    rf_control::gain_profile_iface::sptr _tx_gain_profile_api;
    rf_control::gain_profile_iface::sptr _rx_gain_profile_api;

private:
    //! Validator for the async messages
    //
    // We only know about overruns, underruns, and late commands/packets.
    bool async_message_validator(uint32_t addr, const std::vector<uint32_t>& data);

    //! Receiver for the async messages
    //
    // This block will receive all async messages. The following async messages
    // are expected to show up:
    // - Overrun info
    // - Underrun info
    // - Late data packets
    void async_message_handler(uint32_t addr,
        const std::vector<uint32_t>& data,
        boost::optional<uint64_t> timestamp);

    //! Return the maximum samples per packet of size \p bytes
    //
    // Given a packet of size \p bytes, how many samples can we fit in there?
    // This gives the answer, factoring in item size and samples per clock.
    //
    // \param bytes Number of bytes we can fill with samples (excluding bytes
    //              required for CHDR headers!)
    int get_max_spp(const size_t bytes);

    //! FPGA compat number
    const uint32_t _fpga_compat;

    //! Copy of the REG_RADIO_WIDTH register
    const uint32_t _radio_width;

    //! Sample width (total width, sc16 == 32 bits per complex sample)
    const uint32_t _samp_width;

    //! Samples per cycle
    const uint32_t _spc;

    std::vector<property_t<size_t>> _atomic_item_size_in;
    std::vector<property_t<size_t>> _atomic_item_size_out;
    std::vector<property_t<int>> _spp_prop;
    //! Properties for type_in (one per port)
    std::vector<property_t<io_type_t>> _type_in;
    //! Properties for type_out (one per port)
    std::vector<property_t<io_type_t>> _type_out;

    mutable std::mutex _cache_mutex;
    double _rate = 1.0;
    std::unordered_map<size_t, std::string> _tx_antenna;
    std::unordered_map<size_t, std::string> _rx_antenna;
    std::unordered_map<size_t, double> _tx_freq;
    std::unordered_map<size_t, double> _rx_freq;
    std::unordered_map<size_t, double> _tx_gain;
    std::unordered_map<size_t, double> _rx_gain;
    std::unordered_map<size_t, double> _tx_bandwidth;
    std::unordered_map<size_t, double> _rx_bandwidth;

    std::vector<uhd::stream_cmd_t> _last_stream_cmd;
};

}} // namespace uhd::rfnoc
