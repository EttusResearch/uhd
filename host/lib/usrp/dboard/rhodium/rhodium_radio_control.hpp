//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_RHODIUM_RADIO_CTRL_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_RHODIUM_RADIO_CTRL_IMPL_HPP

#include "rhodium_constants.hpp"
#include "rhodium_cpld_ctrl.hpp"
#include "rhodium_cpld_regs.hpp"
#include <uhd/types/serial.hpp>
#include <uhd/usrp/gpio_defs.hpp>
#include <uhdlib/rfnoc/radio_control_impl.hpp>
#include <uhdlib/rfnoc/rpc_block_ctrl.hpp>
#include <uhdlib/usrp/common/lmx2592.hpp>
#include <uhdlib/usrp/common/mpmd_mb_controller.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <uhdlib/usrp/cores/rx_frontend_core_3000.hpp>
#include <uhdlib/usrp/cores/tx_frontend_core_200.hpp>
#include <mutex>

namespace uhd { namespace rfnoc {

/*! \brief Provide access to an Rhodium radio.
 */
class rhodium_radio_control_impl : public radio_control_impl
{
public:
    typedef std::shared_ptr<rhodium_radio_control_impl> sptr;

    //! Frequency bands for RX. Bands are a function of the analog filter banks
    enum class rx_band {
        RX_BAND_INVALID,
        RX_BAND_0,
        RX_BAND_1,
        RX_BAND_2,
        RX_BAND_3,
        RX_BAND_4,
        RX_BAND_5,
        RX_BAND_6,
        RX_BAND_7
    };

    //! Frequency bands for TX. Bands are a function of the analog filter banks
    enum class tx_band {
        TX_BAND_INVALID,
        TX_BAND_0,
        TX_BAND_1,
        TX_BAND_2,
        TX_BAND_3,
        TX_BAND_4,
        TX_BAND_5,
        TX_BAND_6,
        TX_BAND_7
    };

    /************************************************************************
     * Structors
     ***********************************************************************/
    rhodium_radio_control_impl(make_args_ptr make_args);
    ~rhodium_radio_control_impl() override;

    /************************************************************************
     * RF API calls
     ***********************************************************************/
    // Note: We use the cached values in radio_ctrl_impl, so most getters are
    // not reimplemented here
    double set_rate(double rate) override;

    // Setters
    void set_tx_antenna(const std::string& ant, const size_t chan) override;
    void set_rx_antenna(const std::string& ant, const size_t chan) override;
    double set_tx_frequency(const double freq, const size_t chan) override;
    double set_rx_frequency(const double freq, const size_t chan) override;
    void set_tx_tune_args(const uhd::device_addr_t&, const size_t chan) override;
    void set_rx_tune_args(const uhd::device_addr_t&, const size_t chan) override;
    double set_tx_gain(const double gain, const size_t chan) override;
    double set_rx_gain(const double gain, const size_t chan) override;

    // Getters
    std::vector<std::string> get_tx_antennas(const size_t) const override;
    std::vector<std::string> get_rx_antennas(const size_t) const override;
    uhd::freq_range_t get_tx_frequency_range(const size_t) const override;
    uhd::freq_range_t get_rx_frequency_range(const size_t) const override;
    uhd::gain_range_t get_tx_gain_range(const size_t) const override;
    uhd::gain_range_t get_rx_gain_range(const size_t) const override;
    uhd::meta_range_t get_tx_bandwidth_range(size_t) const override;
    uhd::meta_range_t get_rx_bandwidth_range(size_t) const override;

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
    double set_tx_lo_freq(
        const double freq, const std::string& name, const size_t chan) override;
    double get_tx_lo_freq(const std::string& name, const size_t chan) override;
    // LO Export Control
    void set_tx_lo_export_enabled(
        const bool enabled, const std::string& name, const size_t chan) override;
    void set_rx_lo_export_enabled(
        const bool enabled, const std::string& name, const size_t chan) override;
    bool get_tx_lo_export_enabled(const std::string& name, const size_t chan) override;
    bool get_rx_lo_export_enabled(const std::string& name, const size_t chan) override;

    /**************************************************************************
     * GPIO Controls
     *************************************************************************/
    std::vector<std::string> get_gpio_banks() const override;
    void set_gpio_attr(
        const std::string& bank, const std::string& attr, const uint32_t value) override;
    uint32_t get_gpio_attr(const std::string& bank, const std::string& attr) override;

    /**************************************************************************
     * EEPROM API
     *************************************************************************/
    void set_db_eeprom(const uhd::eeprom_map_t& db_eeprom) override;
    uhd::eeprom_map_t get_db_eeprom() override;

    /**************************************************************************
     * Sensor API
     *************************************************************************/
    std::vector<std::string> get_rx_sensor_names(size_t chan) const override;
    uhd::sensor_value_t get_rx_sensor(const std::string& name, size_t chan) override;
    std::vector<std::string> get_tx_sensor_names(size_t chan) const override;
    uhd::sensor_value_t get_tx_sensor(const std::string& name, size_t chan) override;

    /**************************************************************************
     * Radio Identification API Calls
     *************************************************************************/
    std::string get_slot_name() const override
    {
        return _radio_slot;
    }
    size_t get_chan_from_dboard_fe(
        const std::string& fe, const uhd::direction_t direction) const override;
    std::string get_dboard_fe_from_chan(
        const size_t chan, const uhd::direction_t direction) const override;
    std::string get_fe_name(
        const size_t chan, const uhd::direction_t direction) const override;

    /**************************************************************************
     * Calibration API Calls
     *************************************************************************/
    void set_tx_dc_offset(const std::complex<double>& offset, size_t chan) override;
    meta_range_t get_tx_dc_offset_range(size_t chan) const override;
    void set_tx_iq_balance(const std::complex<double>& correction, size_t chan) override;
    void set_rx_dc_offset(const bool enb, size_t chan = ALL_CHANS) override;
    void set_rx_dc_offset(const std::complex<double>& offset, size_t chan) override;
    meta_range_t get_rx_dc_offset_range(size_t chan) const override;
    void set_rx_iq_balance(const std::complex<double>& correction, size_t chan) override;

    /************************************************************************
     * ??? calls
     ***********************************************************************/
    // LO Distribution Control
    void set_tx_lo_output_enabled(
        const bool enabled, const std::string& port_name, const size_t chan);
    void set_rx_lo_output_enabled(
        const bool enabled, const std::string& port_name, const size_t chan);
    bool get_tx_lo_output_enabled(const std::string& port_name, const size_t chan);
    bool get_rx_lo_output_enabled(const std::string& port_name, const size_t chan);

    // LO Gain Control

    //! Set the external gain for a TX LO
    //  Out of range values will be coerced
    double set_tx_lo_gain(const double gain, const std::string& name, const size_t chan);

    //! Set the external gain for an RX LO
    //  Out of range values will be coerced
    double set_rx_lo_gain(const double gain, const std::string& name, const size_t chan);

    double get_tx_lo_gain(const std::string& name, const size_t chan);
    double get_rx_lo_gain(const std::string& name, const size_t chan);

    // LO Output Power Control

    //! Set the output power setting of a TX LO
    //  Out of range values will be coerced
    double set_tx_lo_power(
        const double power, const std::string& name, const size_t chan);

    //! Set the output power setting of a RX LO
    //  Out of range values will be coerced
    double set_rx_lo_power(
        const double power, const std::string& name, const size_t chan);

    double get_tx_lo_power(const std::string& name, const size_t chan);
    double get_rx_lo_power(const std::string& name, const size_t chan);


private:
    /**************************************************************************
     * noc_block_base API
     *************************************************************************/
    //! Safely shut down all peripherals
    //
    // Reminder: After this is called, no peeks and pokes are allowed!
    void deinit() override
    {
        RFNOC_LOG_TRACE("deinit()");
        // Remove access to all peripherals
        _wb_iface.reset();
        _spi.reset();
        _tx_lo.reset();
        _rx_lo.reset();
        _cpld.reset();
        _gpio.reset();
        _fp_gpio.reset();
        _rx_fe_core.reset();
        _tx_fe_core.reset();
    }

    /**************************************************************************
     * Helpers
     *************************************************************************/
    //! Initialize all the peripherals connected to this block
    void _init_peripherals();

    //! Sync up with MPM
    void _init_mpm();

    //! Set state of this class to sensible defaults
    void _init_defaults();

    //! Init a subtree for the RF frontends
    void _init_frontend_subtree(uhd::property_tree::sptr subtree);

    //! Initialize property tree
    void _init_prop_tree();

    //! Discover and initialize any mpm sensors
    void _init_mpm_sensors(const direction_t dir, const size_t chan_idx);

    //! Get the frequency range for an LO
    freq_range_t _get_lo_freq_range(const std::string& name) const;

    //! Get the current lowband intermediate frequency
    double _get_lowband_lo_freq() const;

    //! Configure LO1's export
    void _set_lo1_export_enabled(const bool enabled, const direction_t dir);

    //! Validate that port_name is valid, and that LO distribution functions
    //  can be called in this instance
    void _validate_output_port(
        const std::string& port_name, const std::string& function_name);

    //! Configure LO Distribution board's termination switches
    void _set_lo_output_enabled(
        const bool enabled, const std::string& port_name, const direction_t dir);

    bool _get_lo_output_enabled(const std::string& port_name, const direction_t dir);

    //! Configure LO1's output power
    //  Out of range values will be coerced to [0-63]
    double _set_lo1_power(const double power, const direction_t dir);

    //! Flash all front end LEDs at 1 Hz for the specified amount of time
    void _identify_with_leds(double identify_duration);

    //! Configure ATR registers and update the cached antenna value from the
    //  new antenna value.
    //  ATR registers control SW10 and the frontend LEDs.
    void _update_atr(const std::string& ant, const direction_t dir);

    //! Configure DSP core corrections based on current frequency
    void _update_corrections(const double freq, const direction_t dir, const bool enable);

    //! Map a frequency in Hz to an rx_band value. Will return
    //  rx_band::INVALID_BAND if the frequency is out of range.
    static rx_band _map_freq_to_rx_band(const double freq);
    //! Map a frequency in Hz to an tx_band value. Will return
    //  tx_band::INVALID_BAND if the frequency is out of range.
    static tx_band _map_freq_to_tx_band(const double freq);

    //! Return if the given rx frequency is in lowband
    //  NOTE: Returns false if frequency is out of Rh's rx frequency range
    static bool _is_rx_lowband(const double freq);
    //! Return if the given tx frequency is in lowband
    //  NOTE: Returns false if frequency is out of Rh's tx frequency range
    static bool _is_tx_lowband(const double freq);

    //! Return the gain range of the LMX LO
    static uhd::gain_range_t _get_lo_gain_range();
    //! Return the power setting range of the LMX LO
    static uhd::gain_range_t _get_lo_power_range();

    //! Lookup the LO DSA setting from LO frequency
    int _get_lo_dsa_setting(const double freq, const direction_t dir);

    //! Lookup the LO output power setting from LO frequency
    unsigned int _get_lo_power_setting(const double freq);

    bool _get_spur_dodging_enabled(const uhd::direction_t dir) const;
    double _get_spur_dodging_threshold(const uhd::direction_t dir) const;
    bool _get_highband_spur_reduction_enabled(const uhd::direction_t dir) const;
    bool _get_timed_command_enabled() const;

    /**************************************************************************
     * Sensors
     *************************************************************************/
    //! Return LO lock status. Factors in current band (low/high) and
    // direction (TX/RX)
    bool get_lo_lock_status(const direction_t dir) const;

    /**************************************************************************
     * Frontend Controls
     *************************************************************************/

    void _set_tx_fe_connection(const std::string& conn);
    void _set_rx_fe_connection(const std::string& conn);
    std::string _get_tx_fe_connection() const;
    std::string _get_rx_fe_connection() const;

    /**************************************************************************
     * CPLD Controls (implemented in rhodium_radio_ctrl_cpld.cpp)
     *************************************************************************/
    void _update_rx_freq_switches(const double freq);

    void _update_tx_freq_switches(const double freq);

    void _update_rx_input_switches(const std::string& input);

    void _update_tx_output_switches(const std::string& output);

    /**************************************************************************
     * Private attributes
     *************************************************************************/
    //! Locks access to the antenna cached values
    std::mutex _ant_mutex;

    //! Letter representation of the radio we're currently running
    std::string _radio_slot;

    //! Prepended for all dboard RPC calls
    std::string _rpc_prefix;

    //! Daughterboard info from MPM
    std::map<std::string, std::string> _dboard_info;

    //! Reference to the MB controller
    mpmd_mb_controller::sptr _n320_mb_control;

    //! Reference to the MB timekeeper
    uhd::rfnoc::mpmd_mb_controller::mpmd_timekeeper::sptr _n3xx_timekeeper;

    //! Reference to the RPC client
    uhd::rpc_client::sptr _rpcc;

    //! Reference to wb_iface compat adapter. This will call into this->regs()
    uhd::timed_wb_iface::sptr _wb_iface;

    //! Reference to the SPI core
    uhd::spi_iface::sptr _spi;

    //! Reference to the TX LO
    lmx2592_iface::sptr _tx_lo;

    //! Reference to the RX LO
    lmx2592_iface::sptr _rx_lo;

    //! Reference to the CPLD controls. Even if there's multiple radios,
    //  there's only one CPLD control.
    std::shared_ptr<rhodium_cpld_ctrl> _cpld;

    //! ATR controls. These control the external DSA and the AD9371 gain
    //  up/down bits. They do *not* control the ATR state of the CPLD, the
    //  tx/rx run states are hooked up directly to the CPLD.
    //
    //  Every radio channel gets its own ATR state register.
    usrp::gpio_atr::gpio_atr_3000::sptr _gpio;

    //! Front panel GPIO controller. Note that only one radio block per
    //  module can be the FP-GPIO master.
    usrp::gpio_atr::gpio_atr_3000::sptr _fp_gpio;

    //! One DSP core per channel
    rx_frontend_core_3000::sptr _rx_fe_core;
    tx_frontend_core_200::sptr _tx_fe_core;

    //! Sampling rate, and also ref clock frequency for the lowband LOs.
    double _master_clock_rate = 1.0;
    //! Saved frontend connection for DSP core
    std::string _rx_fe_connection;
    std::string _tx_fe_connection;
    //! Desired RF frequency
    std::map<direction_t, double> _desired_rf_freq = {
        {RX_DIRECTION, 2.44e9}, {TX_DIRECTION, 2.44e9}};
    //! Frequency at which gain setting was last applied.  The CPLD requires a new gain
    //  control write when switching between lowband and highband frequencies, so save
    //  the frequency when sending a gain control command.
    double _tx_frequency_at_last_gain_write = 0.0;
    double _rx_frequency_at_last_gain_write = 0.0;
    //! LO gain
    double _lo_rx_gain = 0.0;
    double _lo_tx_gain = 0.0;
    //! LO output power
    double _lo_rx_power = 0.0;
    double _lo_tx_power = 0.0;
    //! Gain profile
    std::map<direction_t, std::string> _gain_profile = {
        {RX_DIRECTION, "default"}, {TX_DIRECTION, "default"}};

    //! LO source
    std::string _rx_lo_source = "internal";
    std::string _tx_lo_source = "internal";

    //! LO export enabled
    bool _rx_lo_exported = false;
    bool _tx_lo_exported = false;

    //! LO state frequency
    double _rx_lo_freq = 0.0;
    double _tx_lo_freq = 0.0;

    //! LO Distribution board
    bool _lo_dist_present = false;

    //! LO Distribution board output status
    bool _lo_dist_rx_out_enabled[4] = {false, false, false, false};
    bool _lo_dist_tx_out_enabled[4] = {false, false, false, false};

    std::unordered_map<uhd::direction_t, uhd::device_addr_t, std::hash<size_t>>
        _tune_args{{uhd::RX_DIRECTION, uhd::device_addr_t()},
            {uhd::TX_DIRECTION, uhd::device_addr_t()}};

    //! Cache the contents of the DB EEPROM
    uhd::eeprom_map_t _db_eeprom;

    //! Cached list of RX sensor names
    std::vector<std::string> _rx_sensor_names{"lo_locked"};
    //! Cached list of TX sensor names
    std::vector<std::string> _tx_sensor_names{"lo_locked"};

    property_t<std::string> _spur_dodging_mode{SPUR_DODGING_PROP_NAME,
        RHODIUM_DEFAULT_SPUR_DOGING_MODE,
        {res_source_info::USER}};
    property_t<double> _spur_dodging_threshold{SPUR_DODGING_THRESHOLD_PROP_NAME,
        RHODIUM_DEFAULT_SPUR_DOGING_THRESHOLD,
        {res_source_info::USER}};
    property_t<std::string> _highband_spur_reduction_mode{
        HIGHBAND_SPUR_REDUCTION_PROP_NAME,
        RHODIUM_DEFAULT_HB_SPUR_REDUCTION_MODE,
        {res_source_info::USER}};

}; /* class rhodium_radio_control_impl */

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_RHODIUM_RADIO_CTRL_IMPL_HPP */
