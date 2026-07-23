//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/types/serial.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/utils/noncopyable.hpp>
#include <uhdlib/usrp/cores/spi_core_adrv.hpp>
#include <adi_adrv903x_datainterface_types.h>
#include <adi_adrv903x_dev_temp_types.h>
#include <adi_adrv903x_error_type_action.h>
#include <adi_common_error_types.h>
#include <functional>
#include <memory>

typedef struct uhd_adrv9032_hal_cfg
{
    spi_core_adrv::sptr spi_iface; /*!< SPI Interface */
    int spi_slave; /*!< SPI Slave */
    uhd::spi_config_t spi_config; /*!< SPI Configuration */
    std::function<void(uint8_t)>
        reset_poke_fn; /*!< Function to toggle Reset Line to Chip */
} uhd_adrv9032_hal_cfg_t;

namespace uhd { namespace usrp {

// ADRV9032 Radios can be pin-controlled, pin0 is Tx, pin1 is Rx.
static constexpr int ADRV9032_CHAN_TX  = (1 << 0);
static constexpr int ADRV9032_CHAN_RX  = (1 << 1);
static constexpr int ADRV9032_CHAN_ALL = ADRV9032_CHAN_TX | ADRV9032_CHAN_RX;
// This is needed for timed tuning since we need to ensure the CPU on the ADRV9032 has
// time to process subsequent commands. Normally this is done by waiting for the CPU to be
// ready, but this can't be done for timed commands since there cannot be blocking reads.
// In practice, this value needs to be 4ms, doubling to ensure a buffer.
static constexpr double ADRV9032_CMD_SLEEP_TIME = 0.008;

class adrv9032_ctrl : public uhd::noncopyable
{
public:
    typedef std::shared_ptr<adrv9032_ctrl> sptr;

    static sptr make(std::function<void(size_t, uint32_t)> channel_enable_fn,
        std::function<void(const uhd::time_spec_t&)> sleep_fn,
        std::string init_cal_args,
        std::string tracking_cal_args);
    virtual adi_adrv903x_ErrAction_e hardware_open(spi_core_adrv::sptr spi,
        std::function<void(uint8_t)> reset_poke_fn,
        double master_clock_rate)                    = 0;
    virtual adi_adrv903x_ErrAction_e mcs_check()     = 0;
    virtual adi_adrv903x_ErrAction_e mcs_end()       = 0;
    virtual adi_adrv903x_ErrAction_e post_mcs_init() = 0;

    // JESD Functions
    virtual adi_adrv903x_ErrAction_e jesd_serializer_reset() = 0;
    virtual adi_adrv903x_ErrAction_e jesd_deframer_link_state_enable(
        const adi_adrv903x_DeframerSel_e deframer_sel, bool enable) = 0;
    virtual adi_adrv903x_ErrAction_e jesd_get_framer_sync_mode(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t& mode) = 0;
    virtual adi_adrv903x_ErrAction_e jesd_set_framer_sync_mode(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t mode) = 0;
    virtual adi_adrv903x_ErrAction_e jesd_get_framer_sync_status(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t& status) = 0;
    virtual adi_adrv903x_ErrAction_e jesd_set_framer_sync_status(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t status) = 0;
    virtual adi_adrv903x_ErrAction_e jesd_framer_link_state_enable(
        const adi_adrv903x_FramerSel_e framer_sel, bool enable) = 0;
    virtual adi_adrv903x_ErrAction_e jesd_deframer_sysref_request_enable(
        const adi_adrv903x_DeframerSel_e deframer_sel, bool enable) = 0;
    virtual adi_adrv903x_ErrAction_e jesd_framer_sysref_request_enable(
        const adi_adrv903x_FramerSel_e framer_sel, bool enable) = 0;
    virtual adi_adrv903x_ErrAction_e jesd_run_deframer_init_cals(
        const adi_adrv903x_DeframerSel_e deframer) = 0;
    virtual adi_adrv903x_ErrAction_e jesd_get_framer_status(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t& status) = 0;
    virtual adi_adrv903x_ErrAction_e jesd_get_deframer_status(
        const adi_adrv903x_DeframerSel_e deframer_sel, uint8_t& status) = 0;
    virtual adi_adrv903x_ErrAction_e jesd_deframer_error_clear(
        const adi_adrv903x_DeframerSel_e deframer_sel) = 0;

    virtual adi_adrv903x_ErrAction_e set_rf_lo_frequency(uhd::direction_t dir,
        const size_t chan,
        const double freq,
        const bool timed_tuning) = 0;
    virtual adi_adrv903x_ErrAction_e get_rf_lo_frequency(
        uhd::direction_t dir, const size_t chan, double& freq) = 0;
    virtual adi_adrv903x_ErrAction_e set_tx_nco_frequency(
        const size_t chan, const double freq, const bool timed_tuning) = 0;
    virtual adi_adrv903x_ErrAction_e get_tx_nco_frequency(
        const size_t chan, double& freq) = 0;
    virtual adi_adrv903x_ErrAction_e set_rx_nco_frequency(
        const size_t chan, const double freq, const bool timed_tuning) = 0;
    virtual adi_adrv903x_ErrAction_e get_rx_nco_frequency(
        const size_t chan, double& freq) = 0;
    virtual adi_adrv903x_ErrAction_e set_chan_pll_source(
        uhd::direction_t dir, const size_t chan, const std::string& source) = 0;
    virtual adi_adrv903x_ErrAction_e get_chan_pll_source(
        uhd::direction_t dir, const size_t chan, std::string& source) = 0;
    virtual adi_adrv903x_ErrAction_e get_pll_locked(
        uhd::direction_t dir, const size_t chan, bool& locked)           = 0;
    virtual adi_adrv903x_ErrAction_e initialize_tx_update_atten_config() = 0;
    virtual adi_adrv903x_ErrAction_e set_tx_gain(
        const double gain, const size_t chan)                                     = 0;
    virtual adi_adrv903x_ErrAction_e get_tx_gain(const size_t chan, double& gain) = 0;
    virtual adi_adrv903x_ErrAction_e set_rx_gain(
        const double gain, const size_t chan)                                         = 0;
    virtual adi_adrv903x_ErrAction_e get_rx_gain(const size_t chan, double& gain)     = 0;
    virtual adi_adrv903x_ErrAction_e set_rx_agc(const bool enable, const size_t chan) = 0;
    virtual adi_adrv903x_ErrAction_e get_rx_agc(const size_t chan, bool& enable)      = 0;
    virtual adi_adrv903x_ErrAction_e set_pin_control(
        const bool enable, const uint32_t rxChanMask, const uint32_t txChanMask) = 0;
    virtual adi_adrv903x_ErrAction_e enable_txrx(
        const bool enable, const uint32_t rxChanMask, const uint32_t txChanMask) = 0;
    virtual adi_adrv903x_ErrAction_e enable_tracking_cals(void)                  = 0;
    virtual adi_adrv903x_ErrAction_e get_temperature(int16_t& temperature)       = 0;
    virtual adi_adrv903x_ErrAction_e get_firmware_version(std::string& version)  = 0;
    virtual ~adrv9032_ctrl(void) {}
};
}} // namespace uhd::usrp
