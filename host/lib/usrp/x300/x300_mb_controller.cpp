//
// Copyright 2019-2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_mb_controller.hpp"
#include "x300_fw_common.h"
#include "x300_regs.hpp"
#include <uhd/exception.hpp>
#include <uhdlib/utils/narrow.hpp>
#include <chrono>
#include <thread>

uhd::uart_iface::sptr x300_make_uart_iface(uhd::wb_iface::sptr iface);

using namespace uhd;
using namespace uhd::rfnoc;
using namespace uhd::usrp::x300;
using namespace std::chrono_literals;

namespace {
constexpr uint32_t DONT_LOOK_FOR_GPSDO = 0x1234abcdul;

constexpr uint32_t ADC_SELF_TEST_DURATION = 100; // ms

// When these regs are fixed, there is another fixme below to actually init the
// timekeepers
constexpr uint32_t TK_NUM_TIMEKEEPERS     = 12; // Read-only
constexpr uint32_t TK_REG_BASE            = 100;
constexpr uint32_t TK_REG_OFFSET          = 48;
constexpr uint32_t TK_REG_TICKS_NOW_LO    = 0x00; // Read-only
constexpr uint32_t TK_REG_TICKS_NOW_HI    = 0x04; // Read-only
constexpr uint32_t TK_REG_TICKS_EVENT_LO  = 0x08; // Write-only
constexpr uint32_t TK_REG_TICKS_EVENT_HI  = 0x0C; // Write-only
constexpr uint32_t TK_REG_TICKS_CTRL      = 0x10; // Write-only
constexpr uint32_t TK_REG_TICKS_PPS_LO    = 0x14; // Read-only
constexpr uint32_t TK_REG_TICKS_PPS_HI    = 0x18; // Read-only
constexpr uint32_t TK_REG_TICKS_PERIOD_LO = 0x1C; // Read-Write
constexpr uint32_t TK_REG_TICKS_PERIOD_HI = 0x20; // Read-Write

constexpr char LOG_ID[] = "X300::MB_CTRL";

constexpr char GPIO_SRC_BANK[]     = "FP0";
constexpr char GPIO_SRC_RFA[]      = "RFA";
constexpr char GPIO_SRC_RFB[]      = "RFB";
constexpr size_t GPIO_SRC_NUM_PINS = 12;

} // namespace


/******************************************************************************
 * Structors
 *****************************************************************************/
x300_mb_controller::x300_mb_controller(const size_t hw_rev,
    const std::string product_name,
    uhd::i2c_iface::sptr zpu_i2c,
    uhd::wb_iface::sptr zpu_ctrl,
    x300_clock_ctrl::sptr clock_ctrl,
    uhd::usrp::mboard_eeprom_t mb_eeprom,
    x300_device_args_t args)
    : _hw_rev(hw_rev)
    , _product_name(product_name)
    , _zpu_i2c(zpu_i2c)
    , _zpu_ctrl(zpu_ctrl)
    , _clock_ctrl(clock_ctrl)
    , _mb_eeprom(mb_eeprom)
    , _args(args)
{
    _fw_regmap = std::make_shared<fw_regmap_t>();
    _fw_regmap->initialize(*_zpu_ctrl.get(), true);
    _fw_regmap->ref_freq_reg.write(
        fw_regmap_t::ref_freq_reg_t::REF_FREQ, uint32_t(args.get_system_ref_rate()));

    // Initialize clock source to generate a valid radio clock. This may change
    // after configuration is done.
    // This will configure the LMK and wait for lock
    x300_mb_controller::set_clock_source(args.get_clock_source());
    x300_mb_controller::set_time_source(args.get_time_source());

    const size_t num_tks = _zpu_ctrl->peek32(SR_ADDR(SET0_BASE, TK_NUM_TIMEKEEPERS));
    for (size_t i = 0; i < num_tks; i++) {
        register_timekeeper(i,
            std::make_shared<x300_timekeeper>(
                i, _zpu_ctrl, clock_ctrl->get_master_clock_rate()));
    }

    init_gps();
    _radio_refs.reserve(2);
}

x300_mb_controller::~x300_mb_controller() {}

/******************************************************************************
 * Timekeeper APIs
 *****************************************************************************/
uint64_t x300_mb_controller::x300_timekeeper::get_ticks_now()
{
    uint32_t ticks_lo = _zpu_ctrl->peek32(get_tk_addr(TK_REG_TICKS_NOW_LO));
    uint32_t ticks_hi = _zpu_ctrl->peek32(get_tk_addr(TK_REG_TICKS_NOW_HI));
    return uint64_t(ticks_lo) | (uint64_t(ticks_hi) << 32);
}

uint64_t x300_mb_controller::x300_timekeeper::get_ticks_last_pps()
{
    uint32_t ticks_lo = _zpu_ctrl->peek32(get_tk_addr(TK_REG_TICKS_PPS_LO));
    uint32_t ticks_hi = _zpu_ctrl->peek32(get_tk_addr(TK_REG_TICKS_PPS_HI));
    return uint64_t(ticks_lo) | (uint64_t(ticks_hi) << 32);
}

void x300_mb_controller::x300_timekeeper::set_ticks_now(const uint64_t ticks)
{
    _zpu_ctrl->poke32(
        get_tk_addr(TK_REG_TICKS_EVENT_LO), narrow_cast<uint32_t>(ticks & 0xFFFFFFFF));
    _zpu_ctrl->poke32(
        get_tk_addr(TK_REG_TICKS_EVENT_HI), narrow_cast<uint32_t>(ticks >> 32));
    _zpu_ctrl->poke32(get_tk_addr(TK_REG_TICKS_CTRL), narrow_cast<uint32_t>(0x1));
}

void x300_mb_controller::x300_timekeeper::set_ticks_next_pps(const uint64_t ticks)
{
    _zpu_ctrl->poke32(
        get_tk_addr(TK_REG_TICKS_EVENT_LO), narrow_cast<uint32_t>(ticks & 0xFFFFFFFF));
    _zpu_ctrl->poke32(
        get_tk_addr(TK_REG_TICKS_EVENT_HI), narrow_cast<uint32_t>(ticks >> 32));
    _zpu_ctrl->poke32(get_tk_addr(TK_REG_TICKS_CTRL), narrow_cast<uint32_t>(0x2));
}

void x300_mb_controller::x300_timekeeper::set_period(const uint64_t period_ns)
{
    _zpu_ctrl->poke32(get_tk_addr(TK_REG_TICKS_PERIOD_LO),
        narrow_cast<uint32_t>(period_ns & 0xFFFFFFFF));
    _zpu_ctrl->poke32(
        get_tk_addr(TK_REG_TICKS_PERIOD_HI), narrow_cast<uint32_t>(period_ns >> 32));
}

uint32_t x300_mb_controller::x300_timekeeper::get_tk_addr(const uint32_t tk_addr)
{
    return SR_ADDR(SET0_BASE, TK_REG_BASE + TK_REG_OFFSET * _tk_idx + tk_addr);
}

/******************************************************************************
 * Motherboard Control API (see mb_controller.hpp)
 *****************************************************************************/
void x300_mb_controller::init()
{
    if (_radio_refs.empty()) {
        UHD_LOG_WARNING(LOG_ID, "No radio registered! Skipping ADC checks.");
        return;
    }
    // Check ADCs
    if (_args.get_ext_adc_self_test()) {
        extended_adc_test(_args.get_ext_adc_self_test_duration() / _radio_refs.size());
    } else if (_args.get_self_cal_adc_delay()) {
        constexpr bool apply_delay = true;
        self_cal_adc_xfer_delay(apply_delay);
    } else {
        for (auto& radio : _radio_refs) {
            radio->self_test_adc(ADC_SELF_TEST_DURATION);
        }
    }
}

std::string x300_mb_controller::get_mboard_name() const
{
    return _product_name;
}

void x300_mb_controller::set_time_source(const std::string& source)
{
    if (source == "internal") {
        _fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::PPS_SELECT,
            fw_regmap_t::clk_ctrl_reg_t::SRC_INTERNAL);
    } else if (source == "external") {
        _fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::PPS_SELECT,
            fw_regmap_t::clk_ctrl_reg_t::SRC_EXTERNAL);
    } else if (source == "gpsdo") {
        _fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::PPS_SELECT,
            fw_regmap_t::clk_ctrl_reg_t::SRC_GPSDO);
    } else {
        throw uhd::key_error("update_time_source: unknown source: " + source);
    }

    _current_time_src = source;

    /* TODO - Implement intelligent PPS detection
    //check for valid pps
    if (!is_pps_present(mb)) {
        throw uhd::runtime_error((boost::format("The %d PPS was not detected.  Please
    check the PPS source and try again.") % source).str());
    }
    */
}

std::string x300_mb_controller::get_time_source() const
{
    return _current_time_src;
}

std::vector<std::string> x300_mb_controller::get_time_sources() const
{
    return {"internal", "external", "gpsdo"};
}

void x300_mb_controller::set_clock_source(const std::string& source)
{
    UHD_LOG_TRACE("X300::MB_CTRL", "Setting clock source to " << source);
    // Optimize for the case when the current source is internal and we are trying
    // to set it to internal. This is the only case where we are guaranteed that
    // the clock has not gone away so we can skip setting the MUX and reseting the LMK.
    const bool reconfigure_clks = (_current_refclk_src != "internal")
                                  or (source != "internal");
    if (reconfigure_clks) {
        // Update the clock MUX on the motherboard to select the requested source
        if (source == "internal") {
            _fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::CLK_SOURCE,
                fw_regmap_t::clk_ctrl_reg_t::SRC_INTERNAL);
            _fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::TCXO_EN, 1);
        } else if (source == "external") {
            _fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::CLK_SOURCE,
                fw_regmap_t::clk_ctrl_reg_t::SRC_EXTERNAL);
            _fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::TCXO_EN, 0);
        } else if (source == "gpsdo") {
            _fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::CLK_SOURCE,
                fw_regmap_t::clk_ctrl_reg_t::SRC_GPSDO);
            _fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::TCXO_EN, 0);
        } else {
            throw uhd::key_error("set_clock_source: unknown source: " + source);
        }
        _fw_regmap->clock_ctrl_reg.flush();

        // Reset the LMK to make sure it re-locks to the new reference
        _clock_ctrl->reset_clocks();
    }

    // Wait for the LMK to lock (always, as a sanity check that the clock is useable)
    //* Currently the LMK can take as long as 30 seconds to lock to a reference but we
    // don't
    //* want to wait that long during initialization.
    // TODO: Need to verify timeout and settings to make sure lock can be achieved in
    // < 1.0 seconds
    double timeout = _initialization_done ? 30.0 : 1.0;

    // The programming code in x300_clock_ctrl is not compatible with revs <= 4 and may
    // lead to locking issues. So, disable the ref-locked check for older (unsupported)
    // boards.
    if (_hw_rev > 4) {
        if (not wait_for_clk_locked(fw_regmap_t::clk_status_reg_t::LMK_LOCK, timeout)) {
            // failed to lock on reference
            if (_initialization_done) {
                throw uhd::runtime_error(
                    (boost::format("Reference Clock PLL failed to lock to %s source.")
                        % source)
                        .str());
            } else {
                // TODO: Re-enable this warning when we figure out a reliable lock time
                // UHD_LOGGER_WARNING("X300::MB_CTRL") << "Reference clock failed to lock
                // to " + source + " during device initialization.  " <<
                //    "Check for the lock before operation or ignore this warning if using
                //    another clock source." ;
            }
        }
    }

    if (reconfigure_clks) {
        // Reset the radio clock PLL in the FPGA
        _zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_SW_RST), ZPU_SR_SW_RST_RADIO_CLK_PLL);
        _zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_SW_RST), 0);

        // Wait for radio clock PLL to lock
        if (not wait_for_clk_locked(
                fw_regmap_t::clk_status_reg_t::RADIO_CLK_LOCK, 0.01)) {
            throw uhd::runtime_error(
                (boost::format("Reference Clock PLL in FPGA failed to lock to %s source.")
                    % source)
                    .str());
        }

        // Reset the IDELAYCTRL used to calibrate the data interface delays
        _zpu_ctrl->poke32(
            SR_ADDR(SET0_BASE, ZPU_SR_SW_RST), ZPU_SR_SW_RST_ADC_IDELAYCTRL);
        _zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_SW_RST), 0);

        // Wait for the ADC IDELAYCTRL to be ready
        if (not wait_for_clk_locked(
                fw_regmap_t::clk_status_reg_t::IDELAYCTRL_LOCK, 0.01)) {
            throw uhd::runtime_error(
                (boost::format(
                     "ADC Calibration Clock in FPGA failed to lock to %s source.")
                    % source)
                    .str());
        }

        // Reset ADCs and DACs
        reset_codecs();
    }

    // Update cache value
    _current_refclk_src = source;
}

std::string x300_mb_controller::get_clock_source() const
{
    return _current_refclk_src;
}

std::vector<std::string> x300_mb_controller::get_clock_sources() const
{
    return {"internal", "external", "gpsdo"};
}

void x300_mb_controller::set_sync_source(
    const std::string& clock_source, const std::string& time_source)
{
    device_addr_t sync_args;
    sync_args["clock_source"] = clock_source;
    sync_args["time_source"]  = time_source;
    set_sync_source(sync_args);
}

void x300_mb_controller::set_sync_source(const device_addr_t& sync_source)
{
    if (sync_source.has_key("clock_source")) {
        set_clock_source(sync_source["clock_source"]);
    }
    if (sync_source.has_key("time_source")) {
        set_time_source(sync_source["time_source"]);
    }
}

device_addr_t x300_mb_controller::get_sync_source() const
{
    const std::string clock_source = get_clock_source();
    const std::string time_source  = get_time_source();
    device_addr_t sync_source;
    sync_source["clock_source"] = clock_source;
    sync_source["time_source"]  = time_source;
    return sync_source;
}

std::vector<device_addr_t> x300_mb_controller::get_sync_sources()
{
    const std::vector<std::pair<std::string, std::string>> clock_time_src_pairs = {
        // Clock source, Time source
        {"internal", "internal"},
        {"external", "internal"},
        {"external", "external"},
        {"gpsdo", "gpsdo"},
        {"gpsdo", "internal"}};

    // Now convert to vector of device_addr_t
    std::vector<device_addr_t> sync_sources;
    for (const auto& ct_pair : clock_time_src_pairs) {
        device_addr_t sync_source;
        sync_source["clock_source"] = ct_pair.first;
        sync_source["time_source"]  = ct_pair.second;
        sync_sources.push_back(sync_source);
    }
    return sync_sources;
}

void x300_mb_controller::set_clock_source_out(const bool enb)
{
    _clock_ctrl->set_ref_out(enb);
}

void x300_mb_controller::set_time_source_out(const bool enb)
{
    _fw_regmap->clock_ctrl_reg.write(
        fw_regmap_t::clk_ctrl_reg_t::PPS_OUT_EN, enb ? 1 : 0);
}

sensor_value_t x300_mb_controller::get_sensor(const std::string& name)
{
    if (name == "ref_locked") {
        return sensor_value_t("Ref", get_ref_locked(), "locked", "unlocked");
    }
    if (name == "temp_fpga") {
        // FPGA XADC Code is a 12-bit value
        uint32_t fpga_temp_adc_code =
            _zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_XADC_VALS)) & 0xFFF;
        // Formula for conversion taken from AMD UG480 Equation 1-2.
        double temp_degC = ((fpga_temp_adc_code * 503.975) / 4096.0) - 273.15;
        return sensor_value_t("FPGA TEMP", temp_degC, "C");
    }
    // There are only GPS sensors, temp_fpga, and ref_locked, so we can take a shortcut
    // here and directly ask the GPS for its sensor value:
    if (_sensors.count(name)) {
        return _gps->get_sensor(name);
    }
    throw uhd::key_error(std::string("Invalid sensor name: ") + name);
}

std::vector<std::string> x300_mb_controller::get_sensor_names()
{
    return std::vector<std::string>(_sensors.cbegin(), _sensors.cend());
}

uhd::usrp::mboard_eeprom_t x300_mb_controller::get_eeprom()
{
    return _mb_eeprom;
}

bool x300_mb_controller::synchronize(std::vector<mb_controller::sptr>& mb_controllers,
    const uhd::time_spec_t& time_spec,
    const bool quiet)
{
    if (!mb_controller::synchronize(mb_controllers, time_spec, quiet)) {
        return false;
    }

    std::vector<std::shared_ptr<x300_mb_controller>> mb_controller_copy;
    mb_controller_copy.reserve(mb_controllers.size());
    for (auto mb_controller : mb_controllers) {
        if (std::dynamic_pointer_cast<x300_mb_controller>(mb_controller)) {
            mb_controller_copy.push_back(
                std::dynamic_pointer_cast<x300_mb_controller>(mb_controller));
        }
    }
    // Now, mb_controller_copy contains only references of mb_controllers that
    // are actually x300_mb_controllers
    mb_controllers.clear();
    for (auto mb_controller : mb_controller_copy) {
        mb_controllers.push_back(mb_controller);
    }

    // Now we have the housekeeping out of the way, we can actually start
    // synchronizing. The X300 needs to sync its DACs. First, we get a reference
    // to all the radios (and thus to the DACs).
    std::vector<uhd::usrp::x300::x300_radio_mbc_iface*> radios;
    radios.reserve(2 * mb_controller_copy.size());
    for (auto& mbc : mb_controller_copy) {
        for (auto radio_ref : mbc->_radio_refs) {
            radios.push_back(radio_ref);
        }
    }

    UHD_LOG_TRACE(LOG_ID, "Running DAC sync on " << radios.size() << " radios.");

    // **PRECONDITION**
    // This function assumes that all the VITA times for "radios" are
    // synchronized to a common reference, which we did earlier.

    // Get a rough estimate of the cumulative command latency
    auto t_start = std::chrono::steady_clock::now();
    for (auto radio : radios) {
        radio->get_adc_rx_word(); // Discard value. We are just timing the call
    }
    auto t_elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now() - t_start);
    // Add 100% of headroom + uncertainty to the command time
    uint64_t t_sync_us = (t_elapsed.count() * 2) + 16000 /* Scheduler latency */;

    const double radio_clk_rate = _clock_ctrl->get_master_clock_rate();
    std::string err_str;
    // Try to sync 3 times before giving up
    constexpr size_t MAX_ATTEMPTS = 3;
    for (size_t attempt = 0; attempt < MAX_ATTEMPTS; attempt++) {
        try {
            // Reinitialize and resync all DACs
            for (auto radio : radios) {
                radio->sync_dac();
            }

            // Make sure FRAMEP/N is 0
            for (auto radio : radios) {
                radio->set_dac_sync(false);
            }

            // Pick radios[0] as the time reference.
            uhd::time_spec_t sync_time =
                mb_controller_copy.front()->get_timekeeper(0)->get_time_now()
                + uhd::time_spec_t(((double)t_sync_us) / 1e6);

            // Send the sync command
            for (auto radio : radios) {
                // Arm FRAMEP/N sync pulse by asserting a rising edge
                radio->set_dac_sync(true, sync_time);
            }

            // Reset FRAMEP/N to 0 after 2 clock cycles, and reset command time
            for (auto radio : radios) {
                radio->set_dac_sync(false, sync_time + (2.0 / radio_clk_rate));
            }

            // Wait and check status
            std::this_thread::sleep_for(std::chrono::microseconds(t_sync_us));
            for (auto radio : radios) {
                radio->dac_verify_sync();
            }

            UHD_LOG_TRACE(LOG_ID, "DAC sync passed on attempt " << attempt);
            return true;
        } catch (const uhd::runtime_error& e) {
            err_str = e.what();
            RFNOC_LOG_DEBUG("Retrying DAC synchronization: " << err_str);
        }
    }
    throw uhd::runtime_error(err_str);
}

std::vector<std::string> x300_mb_controller::get_gpio_banks() const
{
    return {GPIO_SRC_BANK};
}

std::vector<std::string> x300_mb_controller::get_gpio_srcs(const std::string& bank) const
{
    if (bank != GPIO_SRC_BANK) {
        UHD_LOG_ERROR(LOG_ID,
            "Invalid GPIO source bank: " << bank << ". Only supported bank is "
                                         << GPIO_SRC_BANK);
        throw uhd::runtime_error(std::string("Invalid GPIO source bank: ") + bank);
    }
    return {GPIO_SRC_RFA, GPIO_SRC_RFB};
}

std::vector<std::string> x300_mb_controller::get_gpio_src(const std::string& bank)
{
    if (bank != GPIO_SRC_BANK) {
        UHD_LOG_ERROR(LOG_ID,
            "Invalid GPIO source bank: " << bank << ". Only supported bank is "
                                         << GPIO_SRC_BANK);
        throw uhd::runtime_error(std::string("Invalid GPIO source bank: ") + bank);
    }
    uint32_t fp_gpio_src = _zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_FP_GPIO_SRC));
    const auto gpio_srcs = get_gpio_srcs(bank);
    std::vector<std::string> gpio_src;
    for (size_t ii = 0; ii < GPIO_SRC_NUM_PINS; ++ii) {
        const uint32_t this_src = (fp_gpio_src >> (2 * ii)) & 0x3;
        if (this_src > 1) {
            UHD_LOG_WARNING(LOG_ID,
                "get_gpio_src() read back invalid GPIO source index: "
                    << this_src << ". Falling back to " << (this_src & 0x1));
        }
        gpio_src.push_back(gpio_srcs[this_src & 0x1]);
    }
    return gpio_src;
}

void x300_mb_controller::set_gpio_src(
    const std::string& bank, const std::vector<std::string>& srcs)
{
    if (srcs.size() > GPIO_SRC_NUM_PINS) {
        UHD_LOG_WARNING(LOG_ID, "set_gpio_src(): Provided more sources than pins!");
    }
    uint32_t fp_gpio_src   = _zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_FP_GPIO_SRC));
    size_t pins_configured = 0;

    const auto gpio_srcs = get_gpio_srcs(bank);
    for (auto src : srcs) {
        const uint32_t pins = [src]() {
            if (src == GPIO_SRC_RFA) {
                return 0;
            }
            if (src == GPIO_SRC_RFB) {
                return 1;
            }
            UHD_LOG_ERROR(LOG_ID, "Invalid GPIO source provided: " << src);
            throw uhd::runtime_error("Invalid GPIO source provided!");
        }();
        uint32_t pin_mask = ~(uint32_t(0x3) << (2 * pins_configured));
        fp_gpio_src       = (fp_gpio_src & pin_mask) | (pins << 2 * pins_configured);
        pins_configured++;
        if (pins_configured > GPIO_SRC_NUM_PINS) {
            break;
        }
    }
    _zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_FP_GPIO_SRC), fp_gpio_src);
}

/******************************************************************************
 * Private Methods
 *****************************************************************************/
std::string x300_mb_controller::get_unique_id()
{
    return std::string("X300::MB_CTRL") + ""; // FIXME
}

void x300_mb_controller::init_gps()
{
    // otherwise if not disabled, look for the internal GPSDO
    if (_zpu_ctrl->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_GPSDO_STATUS))
        != DONT_LOOK_FOR_GPSDO) {
        UHD_LOG_TRACE("X300::MB_CTRL", "Detecting internal GPSDO....");
        try {
            // gps_ctrl will print its own log statements if a GPSDO was found
            _gps = gps_ctrl::make(x300_make_uart_iface(_zpu_ctrl));
        } catch (std::exception& e) {
            UHD_LOGGER_WARNING("X300::MB_CTRL")
                << "An error occurred making GPSDO control: " << e.what()
                << " Continuing without GPS.";
        }
        if (_gps and _gps->gps_detected()) {
            auto sensors = _gps->get_sensors();
            _sensors.insert(sensors.cbegin(), sensors.cend());
        } else {
            UHD_LOG_TRACE("X300::MB_CTRL",
                "No GPS found, setting register to save time on next run.");
            _zpu_ctrl->poke32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_GPSDO_STATUS),
                DONT_LOOK_FOR_GPSDO);
        }
    } else {
        UHD_LOG_TRACE("X300::MB_CTRL",
            "Not detecting internal GPSDO, previous run already failed to find it.");
    }
}

void x300_mb_controller::reset_codecs()
{
    for (auto& callback : _reset_cbs) {
        UHD_LOG_TRACE("X300::MB_CTRL", "Calling DAC/ADC reset callback");
        callback();
    }
}

bool x300_mb_controller::wait_for_clk_locked(uint32_t which, double timeout)
{
    const auto timeout_time = std::chrono::steady_clock::now()
                              + std::chrono::milliseconds(int64_t(timeout * 1000));
    do {
        if (_fw_regmap->clock_status_reg.read(which) == 1) {
            return true;
        }
        std::this_thread::sleep_for(5ms);
    } while (std::chrono::steady_clock::now() < timeout_time);

    // Check one last time
    return (_fw_regmap->clock_status_reg.read(which) == 1);
}

bool x300_mb_controller::is_pps_present()
{
    // The ZPU_RB_CLK_STATUS_PPS_DETECT bit toggles with each rising edge of the PPS.
    // We monitor it for up to 1.5 seconds looking for it to toggle.
    uint32_t pps_detect =
        _fw_regmap->clock_status_reg.read(fw_regmap_t::clk_status_reg_t::PPS_DETECT);
    const auto timeout_time = std::chrono::steady_clock::now() + 1500ms;
    while (std::chrono::steady_clock::now() < timeout_time) {
        std::this_thread::sleep_for(100ms);
        if (pps_detect
            != _fw_regmap->clock_status_reg.read(
                fw_regmap_t::clk_status_reg_t::PPS_DETECT))
            return true;
    }
    return false;
}

bool x300_mb_controller::get_ref_locked()
{
    _fw_regmap->clock_status_reg.refresh();
    return (_fw_regmap->clock_status_reg.get(fw_regmap_t::clk_status_reg_t::LMK_LOCK)
               == 1)
           && (_fw_regmap->clock_status_reg.get(
                   fw_regmap_t::clk_status_reg_t::RADIO_CLK_LOCK)
               == 1)
           && (_fw_regmap->clock_status_reg.get(
                   fw_regmap_t::clk_status_reg_t::IDELAYCTRL_LOCK)
               == 1);
}

void x300_mb_controller::self_cal_adc_xfer_delay(bool apply_delay)
{
    UHD_LOG_INFO("X300", "Running ADC transfer delay self-cal: ");

    // Effective resolution of the self-cal.
    constexpr size_t NUM_DELAY_STEPS = 100;

    double master_clk_period = (1.0e9 / _clock_ctrl->get_master_clock_rate()); // in ns
    double delay_start       = 0.0;
    double delay_range       = 2 * master_clk_period;
    double delay_incr        = delay_range / NUM_DELAY_STEPS;

    double cached_clk_delay = _clock_ctrl->get_clock_delay(X300_CLOCK_WHICH_ADC0);
    double fpga_clk_delay   = _clock_ctrl->get_clock_delay(X300_CLOCK_WHICH_FPGA);

    // Iterate through several values of delays and measure ADC data integrity
    std::vector<std::pair<double, bool>> results;
    for (size_t i = 0; i < NUM_DELAY_STEPS; i++) {
        // Delay the ADC clock (will set both Ch0 and Ch1 delays)
        double delay = _clock_ctrl->set_clock_delay(
            X300_CLOCK_WHICH_ADC0, delay_incr * i + delay_start);
        wait_for_clk_locked(fw_regmap_t::clk_status_reg_t::LMK_LOCK, 0.1);

        uint32_t err_code = 0;
        for (auto& radio : _radio_refs) {
            // Test each channel (I and Q) individually so as to not accidentally
            // trigger on the data from the other channel if there is a swap

            // -- Test I Channel --
            // Put ADC in ramp test mode. Tie the other channel to all ones.
            radio->set_adc_test_word("ramp", "ones");
            // Turn on the pattern checker in the FPGA. It will lock when it sees a
            // zero and count deviations from the expected value
            radio->set_adc_checker_enabled(false);
            radio->set_adc_checker_enabled(true);
            // 50ms @ 200MHz = 10 million samples
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (radio->get_adc_checker_locked(true /* I */)) {
                err_code += radio->get_adc_checker_error_code(true /* I */);
            } else {
                err_code += 100; // Increment error code by 100 to indicate no lock
            }

            // -- Test Q Channel --
            // Put ADC in ramp test mode. Tie the other channel to all ones.
            radio->set_adc_test_word("ones", "ramp");
            // Turn on the pattern checker in the FPGA. It will lock when it sees a
            // zero and count deviations from the expected value
            radio->set_adc_checker_enabled(false);
            radio->set_adc_checker_enabled(true);
            // 50ms @ 200MHz = 10 million samples
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (radio->get_adc_checker_locked(false /* Q */)) {
                err_code += radio->get_adc_checker_error_code(false /* Q */);
            } else {
                err_code += 100; // Increment error code by 100 to indicate no lock
            }
        }
        UHD_LOG_TRACE(
            LOG_ID, boost::format("XferDelay=%fns, Error=%d") % delay % err_code);
        results.push_back(std::pair<double, bool>(delay, err_code == 0));
    }

    // Calculate the valid window
    // When done win_start_idx will have the first delay value index that caused
    // no errors, and win_stop_idx will have the last valid delay value index
    int win_start_idx = -1, win_stop_idx = -1, cur_start_idx = -1, cur_stop_idx = -1;
    for (size_t i = 0; i < results.size(); i++) {
        std::pair<double, bool>& item = results[i];
        if (item.second) { // If data is stable
            if (cur_start_idx == -1) { // This is the first window
                cur_start_idx = i;
                cur_stop_idx  = i;
            } else { // We are extending the window
                cur_stop_idx = i;
            }
        } else {
            if (cur_start_idx == -1) { // We haven't yet seen valid data
                // Do nothing
            } else if (win_start_idx == -1) { // We passed the first valid window
                win_start_idx = cur_start_idx;
                win_stop_idx  = cur_stop_idx;
            } else { // Update cached window if current window is larger
                double cur_win_len =
                    results[cur_stop_idx].first - results[cur_start_idx].first;
                double cached_win_len =
                    results[win_stop_idx].first - results[win_start_idx].first;
                if (cur_win_len > cached_win_len) {
                    win_start_idx = cur_start_idx;
                    win_stop_idx  = cur_stop_idx;
                }
            }
            // Reset current window
            cur_start_idx = -1;
            cur_stop_idx  = -1;
        }
    }
    if (win_start_idx == -1) {
        throw uhd::runtime_error(
            "self_cal_adc_xfer_delay: Self calibration failed. Convergence error.");
    }

    double win_center =
        (results[win_stop_idx].first + results[win_start_idx].first) / 2.0;
    const double win_length = results[win_stop_idx].first - results[win_start_idx].first;
    if (win_length < master_clk_period / 4) {
        throw uhd::runtime_error("self_cal_adc_xfer_delay: Self calibration failed. "
                                 "Valid window too narrow.");
    }

    // Cycle slip the relative delay by a clock cycle to prevent sample misalignment
    // fpga_clk_delay > 0 and 0 < win_center < 2*(1/MCR) so one cycle slip is all we need
    bool cycle_slip = (win_center - fpga_clk_delay >= master_clk_period);
    if (cycle_slip) {
        win_center -= master_clk_period;
    }

    if (apply_delay) {
        // Apply delay
        win_center = _clock_ctrl->set_clock_delay(
            X300_CLOCK_WHICH_ADC0, win_center); // Sets ADC0 and ADC1
        wait_for_clk_locked(fw_regmap_t::clk_status_reg_t::LMK_LOCK, 0.1);
        // Validate
        for (auto radio_ref : _radio_refs) {
            radio_ref->self_test_adc(2000);
        }
    } else {
        // Restore delay
        _clock_ctrl->set_clock_delay(
            X300_CLOCK_WHICH_ADC0, cached_clk_delay); // Sets ADC0 and ADC1
    }

    // Teardown
    for (auto& radio : _radio_refs) {
        radio->set_adc_test_word("normal", "normal");
        radio->set_adc_checker_enabled(false);
    }
    UHD_LOGGER_INFO(LOG_ID)
        << (boost::format("ADC transfer delay self-cal done (FPGA->ADC=%.3fns%s, "
                          "Window=%.3fns)")
               % (win_center - fpga_clk_delay) % (cycle_slip ? " +cyc" : "")
               % win_length);
}

void x300_mb_controller::extended_adc_test(double duration_s)
{
    static const size_t SECS_PER_ITER = 5;
    RFNOC_LOG_INFO(
        boost::format("Running Extended ADC Self-Test (Duration=%.0fs, %ds/iteration)...")
        % duration_s % SECS_PER_ITER);

    size_t num_iters    = static_cast<size_t>(ceil(duration_s / SECS_PER_ITER));
    size_t num_failures = 0;
    for (size_t iter = 0; iter < num_iters; iter++) {
        // Run self-test
        RFNOC_LOG_INFO(
            boost::format("Extended ADC Self-Test Iteration %06d... ") % (iter + 1));
        try {
            for (auto& radio : _radio_refs) {
                radio->self_test_adc(SECS_PER_ITER * 1000);
            }
            RFNOC_LOG_INFO(boost::format("Extended ADC Self-Test Iteration %06d passed ")
                           % (iter + 1));
        } catch (std::exception& e) {
            num_failures++;
            RFNOC_LOG_ERROR(e.what());
        }
    }
    if (num_failures == 0) {
        RFNOC_LOG_INFO("Extended ADC Self-Test PASSED");
    } else {
        const std::string err_msg =
            (boost::format("Extended ADC Self-Test FAILED!!! (%d/%d failures)")
                % num_failures % num_iters)
                .str();
        RFNOC_LOG_ERROR(err_msg);
        throw uhd::runtime_error(err_msg);
    }
}
