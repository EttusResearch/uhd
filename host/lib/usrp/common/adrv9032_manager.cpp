//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/direction.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhdlib/usrp/common/adrv9032_ctrl.hpp>
#include <uhdlib/usrp/common/adrv9032_manager.hpp>
#include <functional>
#include <mutex>

namespace uhd { namespace usrp {

class adrv9032_manager_impl : public adrv9032_manager
{
public:
    adrv9032_manager_impl(spi_core_adrv::sptr spi,
        std::function<void(uint8_t)> reset_poke_fn,
        double master_clock_rate,
        std::function<void(size_t, uint32_t)> channel_enable_fn,
        std::function<void(const uhd::time_spec_t&)> sleep_fn,
        std::string init_cal_args,
        std::string tracking_cal_args)
        : _adrv9032_ctrl()
        , _mutex(std::make_shared<std::mutex>())
        , _tx_nco_freq(0.0)
        , _rx_nco_freq(0.0)
        , _channel_enable_fn(channel_enable_fn)
        , _sleep_fn(sleep_fn)
    {
        _adrv9032_ctrl = adrv9032_ctrl::make(
            channel_enable_fn, sleep_fn, init_cal_args, tracking_cal_args);
        _initialize(spi, reset_poke_fn, master_clock_rate);
    }

    double set_frequency(uhd::direction_t dir,
        const double freq,
        const size_t chan,
        const bool timed_tuning) override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        double coerced_freq = adrv9032_freq_range.clip(freq, true);
        // Calculation is done for Tx NCO, but then needs to be negated for Rx since it is
        // a shift. So when the requested frequency is higher than the LO max, Tx needs a
        // positive shift, but Rx needs a negative shift, and then vice versa for lower
        // than LO min.

        // Note on the usage of _sleep_fn: We cannot queue up back to back
        // operations that require the CPU on the ADRV9032 as we need to wait for the CPU
        // to be ready.
        if (coerced_freq > adrv9032_lo_freq_range.stop()) {
            check_adrv9032_error(_adrv9032_ctrl->set_rf_lo_frequency(
                dir, chan, adrv9032_lo_freq_range.stop(), timed_tuning));
            double nco_freq = coerced_freq - adrv9032_lo_freq_range.stop();
            if (timed_tuning) {
                _sleep_fn(ADRV9032_CMD_SLEEP_TIME);
            }
            if (dir == TX_DIRECTION) {
                check_adrv9032_error(
                    _adrv9032_ctrl->set_tx_nco_frequency(chan, nco_freq, timed_tuning));
                _tx_nco_freq = nco_freq;
            } else {
                check_adrv9032_error(
                    _adrv9032_ctrl->set_rx_nco_frequency(chan, -nco_freq, timed_tuning));
                _rx_nco_freq = -nco_freq;
            }
        } else if (coerced_freq < adrv9032_lo_freq_range.start()) {
            check_adrv9032_error(_adrv9032_ctrl->set_rf_lo_frequency(
                dir, chan, adrv9032_lo_freq_range.start(), timed_tuning));
            double nco_freq = coerced_freq - adrv9032_lo_freq_range.start();
            if (timed_tuning) {
                _sleep_fn(ADRV9032_CMD_SLEEP_TIME);
            }
            if (dir == TX_DIRECTION) {
                check_adrv9032_error(
                    _adrv9032_ctrl->set_tx_nco_frequency(chan, nco_freq, timed_tuning));
                _tx_nco_freq = nco_freq;
            } else {
                check_adrv9032_error(
                    _adrv9032_ctrl->set_rx_nco_frequency(chan, -nco_freq, timed_tuning));
                _rx_nco_freq = -nco_freq;
            }
        } else {
            check_adrv9032_error(_adrv9032_ctrl->set_rf_lo_frequency(
                dir, chan, coerced_freq, timed_tuning));
            if (dir == TX_DIRECTION
                && !uhd::math::frequencies_are_equal(_tx_nco_freq, 0.0)) {
                if (timed_tuning) {
                    _sleep_fn(ADRV9032_CMD_SLEEP_TIME);
                }
                check_adrv9032_error(
                    _adrv9032_ctrl->set_tx_nco_frequency(chan, 0.0, timed_tuning));
                _tx_nco_freq = 0.0;
            } else if (!uhd::math::frequencies_are_equal(_rx_nco_freq, 0.0)) {
                if (timed_tuning) {
                    _sleep_fn(ADRV9032_CMD_SLEEP_TIME);
                }
                check_adrv9032_error(
                    _adrv9032_ctrl->set_rx_nco_frequency(chan, 0.0, timed_tuning));
                _rx_nco_freq = 0.0;
            }
        }
        if (timed_tuning) {
            // If we are doing fast tuning, take our own coercion rather than querying
            // back from the chip.
            if (dir == TX_DIRECTION) {
                return coerced_freq + _tx_nco_freq;
            } else {
                return coerced_freq + _rx_nco_freq;
            }
        } else {
            // If we are not doing timed tuning, we can query back from the chip.
            double ret_freq = 0.0;
            check_adrv9032_error(
                _adrv9032_ctrl->get_rf_lo_frequency(dir, chan, ret_freq));
            if (dir == TX_DIRECTION) {
                check_adrv9032_error(
                    _adrv9032_ctrl->get_tx_nco_frequency(chan, _tx_nco_freq));
                ret_freq += _tx_nco_freq;
            } else {
                check_adrv9032_error(
                    _adrv9032_ctrl->get_rx_nco_frequency(chan, _rx_nco_freq));
                ret_freq -= _rx_nco_freq;
            }
            return ret_freq;
        }
    }

    double get_frequency(uhd::direction_t dir, const size_t chan) const override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        double freq = 0.0;
        check_adrv9032_error(_adrv9032_ctrl->get_rf_lo_frequency(dir, chan, freq));
        freq += (dir == TX_DIRECTION) ? _tx_nco_freq : -_rx_nco_freq;
        return freq;
    }

    double set_lo_freq(uhd::direction_t dir,
        double freq,
        const std::string& name,
        const size_t chan,
        bool timed_tuning) override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        double ret_freq = 0.0;
        if (name == "RFLO") {
            freq = adrv9032_lo_freq_range.clip(freq, true);
            check_adrv9032_error(
                _adrv9032_ctrl->set_rf_lo_frequency(dir, chan, freq, timed_tuning));
            if (timed_tuning) {
                // If we are doing fast tuning, take our own coercion rather than querying
                // back from the chip.
                return freq;
            } else {
                check_adrv9032_error(
                    _adrv9032_ctrl->get_rf_lo_frequency(dir, chan, ret_freq));
                return ret_freq;
            }
        } else if (name == "NCO") {
            if (dir == TX_DIRECTION) {
                check_adrv9032_error(
                    _adrv9032_ctrl->set_tx_nco_frequency(chan, freq, timed_tuning));
                if (timed_tuning) {
                    _tx_nco_freq = freq;
                } else {
                    check_adrv9032_error(
                        _adrv9032_ctrl->get_tx_nco_frequency(chan, _tx_nco_freq));
                }
                return _tx_nco_freq;
            } else {
                check_adrv9032_error(
                    _adrv9032_ctrl->set_rx_nco_frequency(chan, freq, timed_tuning));
                if (timed_tuning) {
                    _rx_nco_freq = freq;
                } else {
                    check_adrv9032_error(
                        _adrv9032_ctrl->get_rx_nco_frequency(chan, _rx_nco_freq));
                }
                return _rx_nco_freq;
            }
        } else {
            throw uhd::value_error("Invalid LO name: " + name);
        }
    }

    double get_lo_freq(
        uhd::direction_t dir, const std::string& name, const size_t chan) override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        double freq = 0.0;
        if (name == "RFLO") {
            check_adrv9032_error(_adrv9032_ctrl->get_rf_lo_frequency(dir, chan, freq));
        } else if (name == "NCO") {
            if (dir == TX_DIRECTION) {
                check_adrv9032_error(_adrv9032_ctrl->get_tx_nco_frequency(chan, freq));
            } else {
                check_adrv9032_error(_adrv9032_ctrl->get_rx_nco_frequency(chan, freq));
            }
        } else {
            throw uhd::value_error("Invalid LO name: " + name);
        }
        return freq;
    }

    std::string get_lo_source(
        uhd::direction_t dir, const std::string& name, const size_t chan) override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        std::string src = "";
        if (name == "RFLO") {
            check_adrv9032_error(_adrv9032_ctrl->get_chan_pll_source(dir, chan, src));
        } else if (name == "NCO") {
            src = "NCO";
        } else {
            throw uhd::value_error("Invalid LO name: " + name);
        }
        return src;
    }

    void set_lo_source(uhd::direction_t dir,
        const std::string& src,
        const std::string& name,
        const size_t chan) override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        if (name == "RFLO") {
            if (src != "LO0" && src != "LO1") {
                throw uhd::value_error("Invalid RFLO source: " + src);
            }
            check_adrv9032_error(_adrv9032_ctrl->set_chan_pll_source(dir, chan, src));
        } else if (name == "NCO") {
            if (src != "NC0") {
                throw uhd::value_error("Invalid NCO source: " + src);
                // There isn't actually any NCO source to set, but don't throw the Invalid
                // Name error since "NCO" is a valid name.
            }
        } else {
            throw uhd::value_error("Invalid LO name: " + name);
        }
    }

    void set_tx_gain(const double gain, const size_t chan) override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        check_adrv9032_error(_adrv9032_ctrl->set_tx_gain(gain, chan));
    }

    double get_tx_gain(const size_t chan) const override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        double gain = 0.0;
        check_adrv9032_error(_adrv9032_ctrl->get_tx_gain(chan, gain));
        return gain;
    }

    void set_rx_gain(const double gain, const size_t chan) override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        check_adrv9032_error(_adrv9032_ctrl->set_rx_gain(gain, chan));
    }

    double get_rx_gain(const size_t chan) const override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        double gain = 0.0;
        check_adrv9032_error(_adrv9032_ctrl->get_rx_gain(chan, gain));
        return gain;
    }

    bool get_pll_locked(uhd::direction_t dir, const size_t chan) override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        bool locked = false;
        check_adrv9032_error(_adrv9032_ctrl->get_pll_locked(dir, chan, locked));
        return locked;
    }

    void jesd_deframer_sysref_request_enable(
        const adi_adrv903x_DeframerSel_e deframer_sel, bool enable)
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        check_adrv9032_error(
            _adrv9032_ctrl->jesd_deframer_sysref_request_enable(deframer_sel, enable));
    }

    void jesd_framer_sysref_request_enable(
        const adi_adrv903x_FramerSel_e framer_sel, bool enable)
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        check_adrv9032_error(
            _adrv9032_ctrl->jesd_framer_sysref_request_enable(framer_sel, enable));
    }

    void jesd_deframer_link_state_enable(
        const adi_adrv903x_DeframerSel_e deframer_sel, bool enable)
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        check_adrv9032_error(
            _adrv9032_ctrl->jesd_deframer_link_state_enable(deframer_sel, enable));
    }

    uint8_t jesd_get_framer_sync_mode(const adi_adrv903x_FramerSel_e framer_sel)
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        uint8_t mode;
        check_adrv9032_error(_adrv9032_ctrl->jesd_get_framer_sync_mode(framer_sel, mode));
        return mode;
    }

    void jesd_set_framer_sync_mode(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t mode)
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        check_adrv9032_error(_adrv9032_ctrl->jesd_set_framer_sync_mode(framer_sel, mode));
    }

    uint8_t jesd_get_framer_sync_status(const adi_adrv903x_FramerSel_e framer_sel)
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        uint8_t status;
        check_adrv9032_error(
            _adrv9032_ctrl->jesd_get_framer_sync_status(framer_sel, status));
        return status;
    }

    void jesd_set_framer_sync_status(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t status)
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        check_adrv9032_error(
            _adrv9032_ctrl->jesd_set_framer_sync_status(framer_sel, status));
    }

    void jesd_framer_link_state_enable(
        const adi_adrv903x_FramerSel_e framer_sel, bool enable)
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        check_adrv9032_error(
            _adrv9032_ctrl->jesd_framer_link_state_enable(framer_sel, enable));
    }

    void jesd_run_deframer_init_cals(adi_adrv903x_DeframerSel_e deframer_sel)
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        check_adrv9032_error(_adrv9032_ctrl->jesd_run_deframer_init_cals(deframer_sel));
    }

    bool jesd_get_framer_status(const adi_adrv903x_FramerSel_e framer_sel)
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        uint8_t status = 0;
        check_adrv9032_error(_adrv9032_ctrl->jesd_get_framer_status(framer_sel, status));
        // ADRV9032 Framer Status Bits:
        // b1: SYSREF phase established by framer
        // b2: SYSREF phase error
        // b3: pclk slow error
        // b4: pclk fast error
        // b7: Link type: 0: 204B, 1: 204C
        // Check that the error bits are 0, SYSREF phase established and
        // the link type is setup as 204B
        if ((status & 0b10011100) != 0 || (((status & (1 << 1)) == 0))) {
            return false;
        } else {
            return true;
        }
    }

    bool jesd_get_deframer_status(const adi_adrv903x_DeframerSel_e deframer_sel)
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        uint8_t status = 0;
        check_adrv9032_error(
            _adrv9032_ctrl->jesd_get_deframer_status(deframer_sel, status));
        return (status == 0x1);
    }

    void jesd_deframer_error_clear(const adi_adrv903x_DeframerSel_e deframer_sel)
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        check_adrv9032_error(_adrv9032_ctrl->jesd_deframer_error_clear(deframer_sel));
    }

    void reset_jesd_serializer()
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        check_adrv9032_error(_adrv9032_ctrl->jesd_serializer_reset());
    }

    void enable_tracking_cals() override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        check_adrv9032_error(_adrv9032_ctrl->enable_tracking_cals());
    }

    int16_t get_average_temperature() override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        int16_t temperature = 0;
        check_adrv9032_error(_adrv9032_ctrl->get_temperature(temperature));
        return temperature;
    }

    std::string get_firmware_version(void) override
    {
        std::lock_guard<std::mutex> lock(*_mutex);
        std::string version;
        check_adrv9032_error(_adrv9032_ctrl->get_firmware_version(version));
        return version;
    }

private:
    adrv9032_ctrl::sptr _adrv9032_ctrl;
    std::shared_ptr<std::mutex> _mutex;
    double _tx_nco_freq;
    double _rx_nco_freq;
    std::function<void(size_t, uint32_t)> _channel_enable_fn;
    std::function<void(const uhd::time_spec_t&)> _sleep_fn;

    void _initialize(spi_core_adrv::sptr spi,
        std::function<void(uint8_t)> reset_poke_fn,
        double master_clock_rate)
    {
        UHD_LOG_INFO("ADRV9032", "Initializing ADRV9032...");
        check_adrv9032_error(
            _adrv9032_ctrl->hardware_open(spi, reset_poke_fn, master_clock_rate));
        check_adrv9032_error(_adrv9032_ctrl->mcs_check());
        check_adrv9032_error(_adrv9032_ctrl->mcs_end());
        check_adrv9032_error(_adrv9032_ctrl->post_mcs_init());

        check_adrv9032_error(_adrv9032_ctrl->initialize_tx_update_atten_config());

        // Set Pin Control for all Channels.
        check_adrv9032_error((_adrv9032_ctrl->set_pin_control(true,
            ADI_ADRV903X_RX0 | ADI_ADRV903X_RX4,
            ADI_ADRV903X_TX0 | ADI_ADRV903X_TX4)));

        _channel_enable_fn(0, ADRV9032_CHAN_ALL); // Enable Tx and Rx Channel 0
        _channel_enable_fn(1, ADRV9032_CHAN_ALL); // Enable Tx and Rx Channel 1
    }
};

adrv9032_manager::sptr adrv9032_manager::make(spi_core_adrv::sptr spi,
    std::function<void(uint8_t)> reset_poke_fn,
    double master_clock_rate,
    std::function<void(size_t, uint32_t)> channel_enable_fn,
    std::function<void(const uhd::time_spec_t&)> increase_command_time,
    std::string init_cal_args,
    std::string tracking_cal_args)
{
    return std::make_shared<adrv9032_manager_impl>(spi,
        reset_poke_fn,
        master_clock_rate,
        channel_enable_fn,
        increase_command_time,
        init_cal_args,
        tracking_cal_args);
}

}} // namespace uhd::usrp
