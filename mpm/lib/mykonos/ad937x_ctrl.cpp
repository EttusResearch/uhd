//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "ad937x_device.hpp"
#include "adi/mykonos.h"
#include "mpm/ad937x/ad937x_ctrl.hpp"
#include <mpm/exception.hpp>
#include "../../../host/include/uhd/utils/math.hpp"

#include <boost/format.hpp>
#include <sstream>
#include <set>
#include <functional>
#include <iostream>
#include <algorithm>
#include <thread>
#include <chrono>


using namespace mpm::chips;
using namespace mpm::ad937x::device;
//Init cals mask
const uint32_t ad937x_ctrl::TX_BB_FILTER            = ::TX_BB_FILTER;
const uint32_t ad937x_ctrl::ADC_TUNER               = ::ADC_TUNER;
const uint32_t ad937x_ctrl::TIA_3DB_CORNER          = ::TIA_3DB_CORNER;
const uint32_t ad937x_ctrl::DC_OFFSET               = ::DC_OFFSET;
const uint32_t ad937x_ctrl::TX_ATTENUATION_DELAY    = ::TX_ATTENUATION_DELAY;
const uint32_t ad937x_ctrl::RX_GAIN_DELAY           = ::RX_GAIN_DELAY;
const uint32_t ad937x_ctrl::FLASH_CAL               = ::FLASH_CAL;
const uint32_t ad937x_ctrl::PATH_DELAY              = ::PATH_DELAY;
const uint32_t ad937x_ctrl::TX_LO_LEAKAGE_INTERNAL  = ::TX_LO_LEAKAGE_INTERNAL;
const uint32_t ad937x_ctrl::TX_LO_LEAKAGE_EXTERNAL  = ::TX_LO_LEAKAGE_EXTERNAL;
const uint32_t ad937x_ctrl::TX_QEC_INIT             = ::TX_QEC_INIT;
const uint32_t ad937x_ctrl::LOOPBACK_RX_LO_DELAY    = ::LOOPBACK_RX_LO_DELAY;
const uint32_t ad937x_ctrl::LOOPBACK_RX_RX_QEC_INIT = ::LOOPBACK_RX_RX_QEC_INIT;
const uint32_t ad937x_ctrl::RX_LO_DELAY             = ::RX_LO_DELAY;
const uint32_t ad937x_ctrl::RX_QEC_INIT             = ::RX_QEC_INIT;
const uint32_t ad937x_ctrl::DPD_INIT                = ::DPD_INIT;
const uint32_t ad937x_ctrl::CLGC_INIT               = ::CLGC_INIT;
const uint32_t ad937x_ctrl::VSWR_INIT               = ::VSWR_INIT;
//Tracking Cals mask
const uint32_t ad937x_ctrl::TRACK_RX1_QEC           = ::TRACK_RX1_QEC;
const uint32_t ad937x_ctrl::TRACK_RX2_QEC           = ::TRACK_RX2_QEC;
const uint32_t ad937x_ctrl::TRACK_ORX1_QEC          = ::TRACK_ORX1_QEC;
const uint32_t ad937x_ctrl::TRACK_ORX2_QEC          = ::TRACK_ORX2_QEC;
const uint32_t ad937x_ctrl::TRACK_TX1_LOL           = ::TRACK_TX1_LOL;
const uint32_t ad937x_ctrl::TRACK_TX2_LOL           = ::TRACK_TX2_LOL;
const uint32_t ad937x_ctrl::TRACK_TX1_QEC           = ::TRACK_TX1_QEC;
const uint32_t ad937x_ctrl::TRACK_TX2_QEC           = ::TRACK_TX2_QEC;
const uint32_t ad937x_ctrl::TRACK_TX1_DPD           = ::TRACK_TX1_DPD;
const uint32_t ad937x_ctrl::TRACK_TX2_DPD           = ::TRACK_TX2_DPD;
const uint32_t ad937x_ctrl::TRACK_TX1_CLGC          = ::TRACK_TX1_CLGC;
const uint32_t ad937x_ctrl::TRACK_TX2_CLGC          = ::TRACK_TX2_CLGC;
const uint32_t ad937x_ctrl::TRACK_TX1_VSWR          = ::TRACK_TX1_VSWR;
const uint32_t ad937x_ctrl::TRACK_TX2_VSWR          = ::TRACK_TX2_VSWR;
const uint32_t ad937x_ctrl::TRACK_ORX1_QEC_SNLO     = ::TRACK_ORX1_QEC_SNLO;
const uint32_t ad937x_ctrl::TRACK_ORX2_QEC_SNLO     = ::TRACK_ORX2_QEC_SNLO;
const uint32_t ad937x_ctrl::TRACK_SRX_QEC           = ::TRACK_SRX_QEC;
const uint32_t ad937x_ctrl::DEFAULT_INIT_CALS_MASKS =
        ad937x_ctrl::TX_BB_FILTER |
        ad937x_ctrl::ADC_TUNER |
        ad937x_ctrl::TIA_3DB_CORNER |
        ad937x_ctrl::DC_OFFSET |
        ad937x_ctrl::TX_ATTENUATION_DELAY |
        ad937x_ctrl::RX_GAIN_DELAY |
        ad937x_ctrl::FLASH_CAL |
        ad937x_ctrl::PATH_DELAY |
        ad937x_ctrl::TX_LO_LEAKAGE_INTERNAL |
        ad937x_ctrl::TX_QEC_INIT |
        ad937x_ctrl::LOOPBACK_RX_LO_DELAY |
        ad937x_ctrl::RX_QEC_INIT
        ;
const uint32_t ad937x_ctrl::DEFAULT_TRACKING_CALS_MASKS =
        ad937x_ctrl::TRACK_RX1_QEC |
        ad937x_ctrl::TRACK_RX2_QEC |
        ad937x_ctrl::TRACK_TX1_QEC |
        ad937x_ctrl::TRACK_TX2_QEC
        ;
const uint32_t ad937x_ctrl::DEFAULT_INIT_CALS_TIMEOUT = 60000;
static uhd::direction_t _get_direction_from_antenna(const std::string& antenna)
{
    auto sub = antenna.substr(0, 2);
    if (sub == "RX") {
        return uhd::direction_t::RX_DIRECTION;
    }
    else if (sub == "TX") {
        return uhd::direction_t::TX_DIRECTION;
    }
    else {
        throw mpm::runtime_error("ad937x_ctrl got an invalid channel string.");
    }
    return uhd::direction_t::RX_DIRECTION;
}

static chain_t _get_chain_from_antenna(const std::string& antenna)
{
    auto sub = antenna.substr(2, 1);
    if (sub == "1") {
        return chain_t::ONE;
    }
    else if (sub == "2") {
        return chain_t::TWO;
    }
    else {
        throw mpm::runtime_error("ad937x_ctrl got an invalid channel string.");
    }
    return chain_t::ONE;
}

std::set<size_t> _get_valid_fir_lengths(const std::string& which)
{
    auto dir = _get_direction_from_antenna(which);
    switch (dir)
    {
    case uhd::direction_t::RX_DIRECTION:
        return{ 24, 48, 72 };
    case uhd::direction_t::TX_DIRECTION:
        return{ 16, 32, 48, 64, 80, 96 };
    default:
        MPM_THROW_INVALID_CODE_PATH();
        return std::set<size_t>();
    }
}

uhd::meta_range_t _get_valid_rx_gain_steps()
{
    // 0-7 step size is valid, in 0.5 dB increments
    return uhd::meta_range_t(0, 3.5, 0.5);
}

uhd::meta_range_t _get_valid_tx_gain_steps()
{
    // 0-31 step size is valid, in 0.05 dB increments
    return uhd::meta_range_t(0, 1.55, 0.05);
}

uhd::meta_range_t ad937x_ctrl::get_rf_freq_range(void)
{
    return uhd::meta_range_t(ad937x_device::MIN_FREQ, ad937x_device::MAX_FREQ);
}


uhd::meta_range_t ad937x_ctrl::get_bw_filter_range(void)
{
    // TODO: fix
    return uhd::meta_range_t(0, 1);
}

std::vector<double> ad937x_ctrl::get_clock_rates(void)
{
    // TODO: fix
    return { 125e6 };
}

uhd::meta_range_t ad937x_ctrl::get_gain_range(const std::string &which)
{
    auto dir = _get_direction_from_antenna(which);
    switch (dir)
    {
    case uhd::direction_t::RX_DIRECTION:
        return uhd::meta_range_t(
                ad937x_device::MIN_RX_GAIN,
                ad937x_device::MAX_RX_GAIN,
                ad937x_device::RX_GAIN_STEP
        );
    case uhd::direction_t::TX_DIRECTION:
        return uhd::meta_range_t(
                ad937x_device::MIN_TX_GAIN,
                ad937x_device::MAX_TX_GAIN,
                ad937x_device::TX_GAIN_STEP
        );
    default:
        MPM_THROW_INVALID_CODE_PATH();
        return uhd::meta_range_t();
    }
}

class ad937x_ctrl_impl : public ad937x_ctrl
{
public:
    ad937x_ctrl_impl(
        std::shared_ptr<std::mutex> spi_mutex,
        const size_t deserializer_lane_xbar,
        mpm::types::regs_iface::sptr iface,
        mpm::ad937x::gpio::gain_pins_t gain_pins) :
        spi_mutex(spi_mutex),
        device(iface.get(), deserializer_lane_xbar, gain_pins),
        _iface(iface)
    {
        /* nop */
    }

    virtual void begin_initialization()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.begin_initialization();
    }

    virtual void finish_initialization()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.finish_initialization();
    }

    virtual void setup_cal(
            const uint32_t init_cals_mask,
            const uint32_t tracking_cals_mask,
            const uint32_t timeout
    ) {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.setup_cal(init_cals_mask, tracking_cals_mask, timeout);
    }

    virtual std::string set_lo_source(
            const std::string &which,
            const std::string &source
    ) {
        const auto dir = _get_direction_from_antenna(which);

        uint8_t pll_source = 0 ;
        if (source == "internal"){
            pll_source = 0;
        }
        else if (source == "external") {
            pll_source = 1;
        }
        else {
            throw mpm::runtime_error("invalid LO source");
        }

        std::lock_guard<std::mutex> lock(*spi_mutex);
        uint8_t retval = device.set_lo_source(dir, pll_source);
        if (retval == 0){
            return "internal";
        } else if (retval == 1){
            return "external";
        }else{
            throw mpm::runtime_error("invalid return from set LO source");
        }
    }

    virtual std::string get_lo_source(const std::string &which){
        const auto dir = _get_direction_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        uint8_t retval = device.get_lo_source(dir);
        if (retval == 0){
            return "internal";
        } else if (retval == 1) {
            return "external";
        } else {
            throw mpm::runtime_error("invalid return from get LO source");
        }
    }

    virtual void start_jesd_rx()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.start_jesd_rx();
    }

    virtual void start_jesd_tx()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.start_jesd_tx();
    }

    virtual void start_radio()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.start_radio();
    }

    virtual void stop_radio()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.stop_radio();
    }

    virtual uint8_t get_multichip_sync_status()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_multichip_sync_status();
    }

    virtual uint8_t get_framer_status()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_framer_status();
    }

    virtual uint8_t get_deframer_status()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_deframer_status();
    }

    virtual uint16_t get_ilas_config_match()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_ilas_config_match();
    }

    virtual void enable_jesd_loopback(const uint8_t enable)
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.enable_jesd_loopback(enable);
    }

    virtual uint8_t get_product_id()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_product_id();
    }

    virtual uint8_t get_device_rev()
    {
        std::this_thread::sleep_for(std::chrono::seconds(4));
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_device_rev();
    }

    virtual std::string get_api_version()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        const auto api = device.get_api_version();
        std::ostringstream ss;
        ss  << api.silicon_ver << "."
            << api.major_ver << "."
            << api.minor_ver << "."
            << api.build_ver;

        return ss.str();
    }

    virtual std::string get_arm_version()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        const auto arm = device.get_arm_version();
        std::ostringstream ss;
        ss  << (int)(arm.major_ver) << "."
            << (int)(arm.minor_ver) << "."
            << (int)(arm.rc_ver);

        switch (arm.build_type)
        {
        case mpm::ad937x::device::build_type_t::RELEASE:
            ss << " Release";
            break;
        case mpm::ad937x::device::build_type_t::DEBUG:
            ss << " Debug";
            break;
        case mpm::ad937x::device::build_type_t::TEST_OBJECT:
            ss << " Test Object";
            break;
        }

        return ss.str();
    }

    virtual double set_bw_filter(const std::string &which, const double value)
    {
        const auto dir = _get_direction_from_antenna(which);
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.set_bw_filter(dir, value);
    }

    virtual double set_gain(const std::string &which, const double value)
    {
        const auto dir = _get_direction_from_antenna(which);
        const auto chain = _get_chain_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.set_gain(dir, chain, value);
    }

    virtual double get_gain(const std::string &which)
    {
        const auto dir = _get_direction_from_antenna(which);
        const auto chain = _get_chain_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_gain(dir, chain);
    }

    // TODO: does agc mode need to have a which parameter?
    // this affects all RX channels on the device
    virtual void set_agc_mode(
            const std::string &which,
            const std::string &mode
    ) {
        const auto dir = _get_direction_from_antenna(which);
        if (dir != uhd::direction_t::RX_DIRECTION)
        {
            throw mpm::runtime_error("set_agc not valid for non-rx channels");
        }

        ad937x_device::gain_mode_t gain_mode;
        if (mode == "automatic") {
            gain_mode = ad937x_device::gain_mode_t::AUTOMATIC;
        }
        else if (mode == "manual") {
            gain_mode = ad937x_device::gain_mode_t::MANUAL;
        }
        else if (mode == "hybrid") {
            gain_mode = ad937x_device::gain_mode_t::HYBRID;
        }
        else {
            throw mpm::runtime_error("invalid agc mode");
        }

        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.set_agc_mode(dir, gain_mode);
    }

    virtual double set_clock_rate(double value)
    {
        const auto rates = get_clock_rates();
        if (std::find(rates.cbegin(), rates.cend(), value) == rates.end()) {
            value = *(rates.cbegin());
        }

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.set_clock_rate(value);
    }

    virtual void enable_channel(const std::string &which, const bool enable)
    {
        const auto dir = _get_direction_from_antenna(which);
        const auto chain = _get_chain_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.enable_channel(dir, chain, enable);
    }

    virtual double set_freq(
            const std::string &which,
            const double value,
            const bool wait_for_lock
    ) {
        const auto dir = _get_direction_from_antenna(which);
        const auto clipped_value = get_rf_freq_range().clip(value);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.tune(dir, clipped_value, wait_for_lock);
    }

    virtual double get_freq(const std::string &which)
    {
        const auto dir = _get_direction_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_freq(dir);
    }

    virtual bool get_lo_locked(const std::string &which)
    {
        const auto dir = _get_direction_from_antenna(which);
        const uint8_t pll_select = (dir == uhd::RX_DIRECTION) ?
            ad937x_device::RX_SYNTH :
            ad937x_device::TX_SYNTH;

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_pll_lock_status(pll_select);
    }

    virtual void set_fir(
            const std::string &which,
            const int8_t gain,
            const std::vector<int16_t>& fir
    ) {
        const auto dir = _get_direction_from_antenna(which);
        const auto lengths = _get_valid_fir_lengths(which);

        if (std::find(lengths.begin(), lengths.end(), fir.size()) == lengths.end())
        {
            throw mpm::value_error("invalid filter length");
        }

        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.set_fir(dir, gain, fir);
    }

    virtual std::vector<int16_t> get_fir(const std::string &which, int8_t &gain)
    {
        auto dir = _get_direction_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_fir(dir, gain);
    }

    virtual int16_t get_temperature()
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return device.get_temperature();
    }

    virtual void set_master_clock_rate(const double rate)
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        ad937x_device::mcr_t mcr;
        if (uhd::math::frequencies_are_equal(rate, 122.88e6)) {
            mcr = ad937x_device::MCR_122_88MHZ;
        }
        else if (uhd::math::frequencies_are_equal(rate, 125e6)) {
            mcr = ad937x_device::MCR_125_00MHZ;
        }
        else if (uhd::math::frequencies_are_equal(rate, 153.6e6)) {
            mcr = ad937x_device::MCR_153_60MHZ;
        }
        else {
            throw mpm::value_error(boost::str(
                boost::format("Requested invalid master clock rate: %f MHz")
                % (rate / 1e6)
            ));
        }

        device.set_master_clock_rate(mcr);
    }

    virtual void set_enable_gain_pins(
            const std::string &which,
            const bool enable
    ) {
        const auto dir = _get_direction_from_antenna(which);
        const auto chain = _get_chain_from_antenna(which);

        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.set_enable_gain_pins(dir, chain, enable);
    }

    virtual void set_gain_pin_step_sizes(
            const std::string &which,
            double inc_step,
            double dec_step
    ) {
        const auto dir = _get_direction_from_antenna(which);
        const auto chain = _get_chain_from_antenna(which);

        if (dir == uhd::RX_DIRECTION) {
            auto steps = _get_valid_rx_gain_steps();
            inc_step = steps.clip(inc_step);
            dec_step = steps.clip(dec_step);
        }
        else if (dir == uhd::TX_DIRECTION) {
            auto steps = _get_valid_tx_gain_steps();
            inc_step = steps.clip(inc_step);
            dec_step = steps.clip(dec_step);

            // double comparison here should be okay because of clipping
            if (inc_step != dec_step) {
                throw mpm::value_error(
                        "TX gain increment and decrement steps must be equal");
            }
        }

        std::lock_guard<std::mutex> lock(*spi_mutex);
        device.set_gain_pin_step_sizes(dir, chain, inc_step, dec_step);
    }

    uint8_t peek8(const uint32_t addr)
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        return _iface->peek8(addr);
    }

    void poke8(const uint32_t addr, const uint8_t val)
    {
        std::lock_guard<std::mutex> lock(*spi_mutex);
        _iface->poke8(addr, val);
    }

private:
    ad937x_device device;
    std::shared_ptr<std::mutex> spi_mutex;
    mpm::types::regs_iface::sptr _iface;
};

ad937x_ctrl::sptr ad937x_ctrl::make(
        std::shared_ptr<std::mutex> spi_mutex,
        const size_t deserializer_lane_xbar,
        mpm::types::regs_iface::sptr iface,
        mpm::ad937x::gpio::gain_pins_t gain_pins
) {
    return std::make_shared<ad937x_ctrl_impl>(
            spi_mutex,
            deserializer_lane_xbar,
            iface,
            gain_pins
    );
}

