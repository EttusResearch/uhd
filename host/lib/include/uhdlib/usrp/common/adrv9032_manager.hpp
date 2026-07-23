//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include "adrv9032_ctrl.hpp"
#include <uhd/types/ranges.hpp>
#include <uhd/types/time_spec.hpp>
#include <adi_adrv903x_datainterface.h>
#include <adi_adrv903x_dev_temp_types.h>
#include <memory>

inline void check_adrv9032_error(adi_adrv903x_ErrAction_e func_result)
{
    switch (func_result) {
        case ADI_ADRV903X_ERR_ACT_NONE:
            return;
        case ADI_ADRV903X_ERR_ACT_RESET_DEVICE:
            throw uhd::runtime_error(
                "Error calling ADRV903x function: HW/SW Reset Required");
        case ADI_ADRV903X_ERR_ACT_RESET_FEATURE:
            throw uhd::runtime_error(
                "Error calling ADRV903x function: Feature Reset Required");
        case ADI_ADRV903X_ERR_ACT_RESET_INTERFACE:
            throw uhd::runtime_error(
                "Error calling ADRV903x function: Interface Reset Required");
        case ADI_ADRV903X_ERR_ACT_CHECK_FEATURE:
            throw uhd::runtime_error(
                "Error calling ADRV903x function: Feature is reporting an Error");
        case ADI_ADRV903X_ERR_ACT_CHECK_INTERFACE:
            throw uhd::runtime_error(
                "Error calling ADRV903x function: Interface is reporting an Error");
        case ADI_ADRV903X_ERR_ACT_OPEN_DEVICE:
            throw uhd::runtime_error("Error calling ADRV9032 function: Device Not Open");
        case ADI_ADRV903X_ERR_ACT_CHECK_MULTIVERSIONING:
            throw uhd::runtime_error(
                "Error calling ADRV903x function: Note: No other error data set");
        case ADI_ADRV903X_ERR_ACT_CHECK_PARAM:
            throw uhd::runtime_error("Error calling ADRV903x function: Invalid "
                                     "Parameter passed to function");
    }
}

namespace uhd { namespace usrp {

static const freq_range_t adrv9032_freq_range(350e6, 7.225e9, 1e3);
static const freq_range_t adrv9032_lo_freq_range(450e6, 7.125e9, 1e3);

class adrv9032_manager
{
public:
    typedef std::shared_ptr<adrv9032_manager> sptr;

    static sptr make(spi_core_adrv::sptr spi,
        std::function<void(uint8_t)> reset_poke_fn,
        double master_clock_rate,
        std::function<void(size_t, uint32_t)> channel_enable_fn,
        std::function<void(const uhd::time_spec_t&)> sleep_fn,
        std::string init_cal_args,
        std::string tracking_cal_args);

    virtual ~adrv9032_manager(void){};

    virtual double set_frequency(uhd::direction_t dir,
        const double freq,
        const size_t chan,
        bool timed_tuning)                                                      = 0;
    virtual double get_frequency(uhd::direction_t dir, const size_t chan) const = 0;
    virtual double set_lo_freq(uhd::direction_t dir,
        double freq,
        const std::string& name,
        const size_t chan,
        bool timed_tuning)                                                      = 0;
    virtual double get_lo_freq(
        uhd::direction_t dir, const std::string& name, const size_t chan) = 0;
    virtual std::string get_lo_source(
        uhd::direction_t dir, const std::string& name, const size_t chan) = 0;
    virtual void set_lo_source(uhd::direction_t dir,
        const std::string& src,
        const std::string& name,
        const size_t chan)                                                = 0;
    virtual void set_tx_gain(const double gain, const size_t chan)        = 0;
    virtual double get_tx_gain(const size_t chan) const                   = 0;
    virtual void set_rx_gain(const double gain, const size_t chan)        = 0;
    virtual double get_rx_gain(const size_t chan) const                   = 0;
    virtual bool get_pll_locked(uhd::direction_t dir, const size_t chan)  = 0;
    virtual void jesd_deframer_sysref_request_enable(
        const adi_adrv903x_DeframerSel_e deframer_sel, bool enable) = 0;
    virtual void jesd_framer_sysref_request_enable(
        const adi_adrv903x_FramerSel_e framer_sel, bool enable) = 0;
    virtual void jesd_deframer_link_state_enable(
        const adi_adrv903x_DeframerSel_e deframer_sel, bool enable) = 0;
    virtual uint8_t jesd_get_framer_sync_mode(
        const adi_adrv903x_FramerSel_e framer_sel) = 0;
    virtual void jesd_set_framer_sync_mode(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t mode) = 0;
    virtual uint8_t jesd_get_framer_sync_status(
        const adi_adrv903x_FramerSel_e framer_sel) = 0;
    virtual void jesd_set_framer_sync_status(
        const adi_adrv903x_FramerSel_e framer_sel, uint8_t status) = 0;
    virtual void jesd_framer_link_state_enable(
        const adi_adrv903x_FramerSel_e framer_sel, bool enable) = 0;
    virtual void jesd_run_deframer_init_cals(
        const adi_adrv903x_DeframerSel_e deframer_sel)                             = 0;
    virtual bool jesd_get_framer_status(const adi_adrv903x_FramerSel_e framer_sel) = 0;
    virtual bool jesd_get_deframer_status(
        const adi_adrv903x_DeframerSel_e deframer_sel) = 0;
    virtual void jesd_deframer_error_clear(
        const adi_adrv903x_DeframerSel_e deframer_sel) = 0;
    virtual void reset_jesd_serializer(void)           = 0;
    virtual void enable_tracking_cals(void)            = 0;
    virtual int16_t get_average_temperature()          = 0;
    virtual std::string get_firmware_version(void)     = 0;
};

}} // namespace uhd::usrp
