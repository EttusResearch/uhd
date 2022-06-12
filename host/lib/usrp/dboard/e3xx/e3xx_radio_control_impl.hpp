//
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_RFNOC_E3XX_RADIO_CTRL_IMPL_HPP
#define INCLUDED_LIBUHD_RFNOC_E3XX_RADIO_CTRL_IMPL_HPP

#include "e3xx_ad9361_iface.hpp"
#include <uhd/rfnoc/filter_node.hpp>
#include <uhd/types/eeprom.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/usrp/gpio_defs.hpp>
#include <uhdlib/rfnoc/radio_control_impl.hpp>
#include <uhdlib/usrp/common/mpmd_mb_controller.hpp>
#include <uhdlib/usrp/cores/gpio_atr_3000.hpp>
#include <mutex>

namespace uhd { namespace rfnoc {

namespace e3xx_regs {
constexpr uint32_t PERIPH_BASE            = 0x80000;
constexpr uint32_t PERIPH_REG_OFFSET      = 8;
constexpr uint32_t PERIPH_REG_CHAN_OFFSET = 0x800;
constexpr uint32_t SR_LEDS                = PERIPH_BASE + 176 * PERIPH_REG_OFFSET;
constexpr uint32_t SR_FP_GPIO             = PERIPH_BASE + 184 * PERIPH_REG_OFFSET;
constexpr uint32_t SR_DB_GPIO             = PERIPH_BASE + 192 * PERIPH_REG_OFFSET;

constexpr uint32_t RB_DB_GPIO = PERIPH_BASE + 19 * PERIPH_REG_OFFSET;
constexpr uint32_t RB_FP_GPIO = PERIPH_BASE + 20 * PERIPH_REG_OFFSET;


} // namespace e3xx_regs

/*! \brief Provide access to an E3xx radio.
 */
class e3xx_radio_control_impl : public radio_control_impl,
                                public uhd::rfnoc::detail::filter_node
{
public:
    //! Frequency bands for RX. Bands are a function of the analog filter banks
    enum class rx_band { INVALID_BAND, LB_B2, LB_B3, LB_B4, LB_B5, LB_B6, LB_B7, HB };

    //! Frequency bands for TX. Bands are a function of the analog filter banks
    enum class tx_band {
        INVALID_BAND,
        LB_80,
        LB_160,
        LB_225,
        LB_400,
        LB_575,
        LB_1000,
        LB_1700,
        LB_2750,
        HB
    };

    /**************************************************************************
     * ATR/ Switches Types
     *************************************************************************/
    //! ATR state
    enum atr_state_t { IDLE, RX_ONLY, TX_ONLY, FULL_DUPLEX };

    //! Channel select:
    enum chan_sel_t { CHAN1, CHAN2, BOTH };

    /************************************************************************
     * Structors
     ***********************************************************************/
    e3xx_radio_control_impl(make_args_ptr make_args);
    ~e3xx_radio_control_impl() override;

    /************************************************************************
     * node_t && noc_block_base API calls
     ***********************************************************************/
    void deinit() override;

    bool check_topology(const std::vector<size_t>& connected_inputs,
        const std::vector<size_t>& connected_outputs) override;

    /************************************************************************
     * radio_control API calls
     ***********************************************************************/
    double set_rate(const double rate) override;
    uhd::meta_range_t get_rate_range() const override;

    // Setters
    void set_tx_antenna(const std::string& ant, const size_t chan) override;
    void set_rx_antenna(const std::string& ant, const size_t chan) override;
    double set_tx_frequency(const double freq, const size_t chan) override;
    double set_rx_frequency(const double freq, const size_t chan) override;
    double set_tx_gain(const double gain, const size_t chan) override;
    double set_rx_gain(const double gain, const size_t chan) override;
    void set_rx_agc(const bool enable, const size_t chan) override;
    double set_tx_bandwidth(const double bandwidth, const size_t chan) override;
    double set_rx_bandwidth(const double bandwidth, const size_t chan) override;

    // Getters
    std::vector<std::string> get_tx_antennas(const size_t chan) const override;
    std::vector<std::string> get_rx_antennas(const size_t chan) const override;
    uhd::freq_range_t get_tx_frequency_range(const size_t chan) const override;
    uhd::freq_range_t get_rx_frequency_range(const size_t chan) const override;
    uhd::gain_range_t get_tx_gain_range(const size_t) const override;
    uhd::gain_range_t get_rx_gain_range(const size_t) const override;
    meta_range_t get_tx_bandwidth_range(size_t chan) const override;
    meta_range_t get_rx_bandwidth_range(size_t chan) const override;

    /**************************************************************************
     * Calibration-Related API Calls
     *************************************************************************/
    void set_rx_dc_offset(const bool enb, size_t chan = ALL_CHANS) override;
    void set_rx_iq_balance(const bool enb, size_t chan) override;

    /**************************************************************************
     * GPIO Controls
     *************************************************************************/
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
     * Filter API
     *************************************************************************/
    std::vector<std::string> get_rx_filter_names(const size_t chan) const override;
    uhd::filter_info_base::sptr get_rx_filter(
        const std::string& name, const size_t chan) override;
    void set_rx_filter(const std::string& name,
        uhd::filter_info_base::sptr filter,
        const size_t chan) override;

    std::vector<std::string> get_tx_filter_names(const size_t chan) const override;
    uhd::filter_info_base::sptr get_tx_filter(
        const std::string& name, const size_t chan) override;
    void set_tx_filter(const std::string& name,
        uhd::filter_info_base::sptr filter,
        const size_t chan) override;

    /**************************************************************************
     * Radio Identification API Calls
     *************************************************************************/
    std::string get_slot_name() const override
    {
        return "A";
    }
    size_t get_chan_from_dboard_fe(
        const std::string& fe, const uhd::direction_t direction) const override;
    std::string get_dboard_fe_from_chan(
        const size_t chan, const uhd::direction_t direction) const override;
    std::string get_fe_name(
        const size_t chan, const uhd::direction_t direction) const override;

protected:
    //! Map a frequency in Hz to an rx_band value. Will return
    //  rx_band::INVALID_BAND if the frequency is out of range.
    rx_band map_freq_to_rx_band(const double freq);
    //! Map a frequency in Hz to an tx_band value. Will return
    //  tx_band::INVALID_BAND if the frequency is out of range.
    tx_band map_freq_to_tx_band(const double freq);

    virtual const std::string get_default_timing_mode() = 0;

    /*! Run a loopback self test.
     *
     * This will write data to the AD936x and read it back again.
     * If this test fails, it generally means the interface is broken,
     * so we assume it passes and throw otherwise. Running this requires
     * a core that we can peek and poke the loopback values into.
     *
     * \param iface An interface to the associated radio control core
     * \param iface The radio control core's address to write the loopback value
     * \param iface The radio control core's readback address to read back the returned
     * value
     *
     * \throws a uhd::runtime_error if the loopback value didn't match.
     */
    void loopback_self_test(const size_t chan);

    virtual uint32_t get_rx_switches(
        const size_t chan, const double freq, const std::string& ant) = 0;

    virtual uint32_t get_tx_switches(const size_t chan, const double freq) = 0;

    virtual uint32_t get_idle_switches() = 0;

    virtual uint32_t get_tx_led()   = 0;
    virtual uint32_t get_rx_led()   = 0;
    virtual uint32_t get_txrx_led() = 0;
    virtual uint32_t get_idle_led() = 0;

    //! Reference to the AD9361 controls
    // e3xx_ad9361_iface::uptr _ad9361;
    ad9361_ctrl::sptr _ad9361;

    //! Swap RFA and RFB for catalina
    bool _fe_swap;

    //! Init RPC-related items
    void _init_mpm();

private:
    /**************************************************************************
     * Helpers
     *************************************************************************/
    //! Initialize all the peripherals connected to this block
    void _init_peripherals();

    //! Set state of this class to sensible defaults
    void _init_defaults();

    //! Init a subtree for the RF frontends
    void _init_frontend_subtree(uhd::property_tree::sptr subtree, const size_t chan_idx);

    //! Initialize Catalina defaults
    void _init_codec();

    //! Initialize property tree
    void _init_prop_tree();

    //! Set streaming mode - active chains, channel_mode, timing_mode
    void set_streaming_mode(
        const bool tx1, const bool tx2, const bool rx1, const bool rx2);

    //! Set which channel mode is used
    void set_channel_mode(const std::string& channel_mode);

    /**************************************************************************
     * Misc Controls
     *************************************************************************/
    //! Blink the front-panel LEDs for \p identify_duration,
    //  and resume normal operation.
    void _identify_with_leds(const int identify_duration);

    void _set_atr_bits(const size_t chan);

    void set_db_eeprom(const uhd::eeprom_map_t& db_eeprom) override;

    uhd::eeprom_map_t get_db_eeprom() override;

    /**************************************************************************
     * Private attributes
     *************************************************************************/
    //! Locks access to setter APIs
    mutable std::recursive_mutex _set_lock;

    //! Prepended for all dboard RPC calls
    std::string _rpc_prefix = "db_0_";

    //! Reference to the MB controller
    uhd::rfnoc::mpmd_mb_controller::sptr _e3xx_mb_control;

    //! Reference to the MB timekeeper
    uhd::rfnoc::mpmd_mb_controller::mpmd_timekeeper::sptr _e3xx_timekeeper;

    //! Reference to wb_iface adapters
    std::vector<uhd::timed_wb_iface::sptr> _wb_ifaces;

    //! Reference to the RPC client
    uhd::rpc_client::sptr _rpcc;

    //! ATR controls. These control the AD9361 gain up/down bits.
    //  Every radio channel gets its own ATR state register.
    std::vector<usrp::gpio_atr::gpio_atr_3000::sptr> _db_gpio;

    // ATR controls for LEDs
    //  Every radio channel gets its own ATR state register.
    std::vector<usrp::gpio_atr::gpio_atr_3000::sptr> _leds_gpio;

    //! Front panel GPIO controller. Note that only one radio block per
    //  module can be the FP-GPIO master.
    usrp::gpio_atr::gpio_atr_3000::sptr _fp_gpio;

    //! Sampling rate
    double _master_clock_rate = 1.0;

    //! RX sensor names (they get cached)
    std::vector<std::string> _rx_sensor_names;

    //! TX sensor names (they get cached)
    std::vector<std::string> _tx_sensor_names;

    //! RX filter names (they get cached)
    std::vector<std::string> _rx_filter_names;

    //! TX filter names (they get cached)
    std::vector<std::string> _tx_filter_names;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_RFNOC_E3XX_RADIO_CTRL_IMPL_HPP */
