//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "ad937x_device.hpp"
#include "adi/mykonos.h"
#include "adi/mykonos_gpio.h"
#include "adi/mykonos_debug/mykonos_dbgjesd.h"
#include "config/ad937x_config_t.hpp"
#include "config/ad937x_default_config.hpp"
#include <boost/format.hpp>

#include <functional>
#include <iostream>
#include <thread>
#include <fstream>

using namespace mpm::ad937x::device;
using namespace mpm::ad937x::gpio;
using namespace uhd;

const double ad937x_device::MIN_FREQ = 300e6;
const double ad937x_device::MAX_FREQ = 6e9;
const double ad937x_device::MIN_RX_GAIN = 0.0;
const double ad937x_device::MAX_RX_GAIN = 30.0;
const double ad937x_device::RX_GAIN_STEP = 0.5;
const double ad937x_device::MIN_TX_GAIN = 0.0;
const double ad937x_device::MAX_TX_GAIN = 41.95;
const double ad937x_device::TX_GAIN_STEP = 0.05;

static const double RX_DEFAULT_FREQ = 2.5e9;
static const double TX_DEFAULT_FREQ = 2.5e9;
static const double RX_DEFAULT_GAIN = 0;
static const double TX_DEFAULT_GAIN = 0;

static const uint32_t AD9371_PRODUCT_ID = 0x3;
static const size_t ARM_BINARY_SIZE = 98304;

static const uint32_t PLL_LOCK_TIMEOUT_MS = 200;

/******************************************************
Helper functions
******************************************************/

// Macro to call an API function via lambda
#define CALL_API(function_call) \
    _call_api_function([&,this]{return function_call;})

// helper function to unify error handling
void ad937x_device::_call_api_function(const std::function<mykonosErr_t()>& func)
{
    const auto error = func();
    if (error != MYKONOS_ERR_OK)
    {
        throw mpm::runtime_error(getMykonosErrorMessage(error));
    }
}

// Macro to call a GPIO API function via lambda
#define CALL_GPIO_API(function_call) \
    _call_gpio_api_function([&,this]{return function_call;})

// helper function to unify error handling, GPIO version
void ad937x_device::_call_gpio_api_function(const std::function<mykonosGpioErr_t()>& func)
{
    const auto error = func();
    if (error != MYKONOS_ERR_GPIO_OK)
    {
        throw mpm::runtime_error(getGpioMykonosErrorMessage(error));
    }
}

// _move_to_config_state() and _restore_from_config_state() are a pair of functions
// that should be called at the beginning and end (respectively) of any configuration
// function that requires the AD9371 to be in the radioOff state.  _restore should be
// called with the return value of _move.

// read the current state of the AD9371 and change it to radioOff (READY)
// returns the current state, to later be consumed by _restore_from_config_state()
ad937x_device::radio_state_t ad937x_device::_move_to_config_state()
{
    uint32_t status;
    CALL_API(MYKONOS_getRadioState(mykonos_config.device, &status));
    if ((status & 0x3) == 0x3)
    {
        stop_radio();
        return radio_state_t::ON;
    }
    else {
        return radio_state_t::OFF;
    }
}

// restores the state from before a call to _move_to_config_state
// if ON, move to radioOn, otherwise this function is a no-op
void ad937x_device::_restore_from_config_state(
        const ad937x_device::radio_state_t state
) {
    if (state == radio_state_t::ON)
    {
        start_radio();
    }
}

std::string ad937x_device::_get_arm_binary_path() const
{
    // TODO: possibly add more options here, for now it's always in
    // /lib/firmware or we explode
    return "/lib/firmware/adi/mykonos-m3.bin";
}

std::vector<uint8_t> ad937x_device::_get_arm_binary()
{
    const auto path = _get_arm_binary_path();
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        throw mpm::runtime_error(
                "Could not open AD9371 ARM binary at path " + path);
    }

    // TODO: add check that opened file size is equal to ARM_BINARY_SIZE
    std::vector<uint8_t> binary(ARM_BINARY_SIZE);
    file.read(reinterpret_cast<char*>(binary.data()), ARM_BINARY_SIZE);
    if (file.bad())
    {
        throw mpm::runtime_error(
                "Error reading AD9371 ARM binary at path " + path);
    }
    return binary;
}

void ad937x_device::_verify_product_id()
{
    const uint8_t product_id = get_product_id();
    if (product_id != AD9371_PRODUCT_ID) {
        throw mpm::runtime_error(str(
            boost::format("AD9371 product ID does not match expected ID! "
                          "Read: %X Expected: %X")
            % int(product_id) % int(AD9371_PRODUCT_ID)
        ));
    }
}

void ad937x_device::_verify_multichip_sync_status(const multichip_sync_t mcs)
{
    const uint8_t status_expected =
        (mcs == multichip_sync_t::FULL) ? 0x0B : 0x0A;
    const uint8_t status_mask = status_expected; // all 1s expected, mask is the same

    const uint8_t mcs_status = get_multichip_sync_status();
    if ((mcs_status & status_mask) != status_expected)
    {
        throw mpm::runtime_error(str(
            boost::format("Multichip sync failed! Read: %X Expected: %X")
            % int(mcs_status) % int(status_expected)
        ));
    }
}

// RX Gain values are table entries given in mykonos_user.h
// An array of gain values is programmed at initialization,
// which the API will then use for its gain values
// In general, Gain Value = (255 - Gain Table Index)
uint8_t ad937x_device::_convert_rx_gain_to_mykonos(const double gain)
{
    // TODO: derive 195 constant from gain table
    // gain should be a value 0-60, add 195 to make 195-255
    return static_cast<uint8_t>((gain * 2) + 195);
}

double ad937x_device::_convert_rx_gain_from_mykonos(const uint8_t gain)
{
    return (static_cast<double>(gain) - 195) / 2.0;
}

// TX gain is completely different from RX gain for no good reason so deal with it
// TX is set as attenuation using a value from 0-41950 mdB
// Only increments of 50 mdB are valid
uint16_t ad937x_device::_convert_tx_gain_to_mykonos(const double gain)
{
    // attenuation is inverted and in mdB not dB
    return static_cast<uint16_t>((MAX_TX_GAIN - (gain)) * 1e3);
}

double ad937x_device::_convert_tx_gain_from_mykonos(const uint16_t gain)
{
    return (MAX_TX_GAIN - (static_cast<double>(gain) / 1e3));
}

void ad937x_device::_apply_gain_pins(
        const direction_t direction,
        const chain_t chain
) {
    // get this channels configuration
    const auto chan = gain_ctrl.config.at(direction).at(chain);

    // TX direction does not support different steps per direction
    if (direction == TX_DIRECTION) {
        MPM_ASSERT_THROW(chan.inc_step == chan.dec_step);
    }

    const auto state = _move_to_config_state();

    switch (direction)
    {
        case RX_DIRECTION:
        {
            std::function<decltype(MYKONOS_setRx1GainCtrlPin)> func;
            switch (chain)
            {
            case chain_t::ONE:
                func = MYKONOS_setRx1GainCtrlPin;
                break;
            case chain_t::TWO:
                func = MYKONOS_setRx2GainCtrlPin;
                break;
            }
            CALL_GPIO_API(func(mykonos_config.device,
                    chan.inc_step,
                    chan.dec_step,
                    chan.inc_pin,
                    chan.dec_pin,
                    chan.enable));
            break;
        }
        case TX_DIRECTION:
        {
            // TX sets attenuation, but the configuration should be stored correctly
            switch (chain)
            {
            case chain_t::ONE:
                // TX1 has an extra parameter "useTx1ForTx2" that we do not support
                CALL_GPIO_API(MYKONOS_setTx1AttenCtrlPin(
                        mykonos_config.device,
                        chan.inc_step,
                        chan.inc_pin,
                        chan.dec_pin,
                        chan.enable,
                        0));
                break;
            case chain_t::TWO:
                CALL_GPIO_API(MYKONOS_setTx2AttenCtrlPin(
                        mykonos_config.device,
                        chan.inc_step,
                        chan.inc_pin,
                        chan.dec_pin,
                        chan.enable));
                break;
            }
            break;
        }
    }
    _restore_from_config_state(state);
}



/******************************************************
Initialization functions
******************************************************/

ad937x_device::ad937x_device(
    mpm::types::regs_iface* iface,
    const size_t deserializer_lane_xbar,
    const gain_pins_t gain_pins) :
    full_spi_settings(iface),
    mykonos_config(&full_spi_settings.spi_settings, deserializer_lane_xbar),
    gain_ctrl(gain_pins)
{
}
void ad937x_device::_setup_rf(){
    // TODO: add setRfPllLoopFilter here

    // Set frequencies
    tune(uhd::RX_DIRECTION, RX_DEFAULT_FREQ, false);
    tune(uhd::TX_DIRECTION, TX_DEFAULT_FREQ, false);

    if (!get_pll_lock_status(CLK_SYNTH | RX_SYNTH | TX_SYNTH | SNIFF_SYNTH, true))
    {
        throw mpm::runtime_error("PLLs did not lock after initial tuning!");
    }

    // Set gain control GPIO pins
    _apply_gain_pins(uhd::RX_DIRECTION, chain_t::ONE);
    _apply_gain_pins(uhd::RX_DIRECTION, chain_t::TWO);
    _apply_gain_pins(uhd::TX_DIRECTION, chain_t::ONE);
    _apply_gain_pins(uhd::TX_DIRECTION, chain_t::TWO);

    CALL_GPIO_API(MYKONOS_setupGpio(mykonos_config.device));

    // Set manual gain values
    set_gain(uhd::RX_DIRECTION, chain_t::ONE, RX_DEFAULT_GAIN);
    set_gain(uhd::RX_DIRECTION, chain_t::TWO, RX_DEFAULT_GAIN);
    set_gain(uhd::TX_DIRECTION, chain_t::ONE, TX_DEFAULT_GAIN);
    set_gain(uhd::TX_DIRECTION, chain_t::TWO, TX_DEFAULT_GAIN);


}

void ad937x_device::setup_cal(
    const uint32_t init_cals_mask,
    const uint32_t tracking_cals_mask,
    const uint32_t timeout
) {
    // Run and wait for init cals
    CALL_API(MYKONOS_runInitCals(mykonos_config.device, init_cals_mask));
    uint8_t errorFlag = 0, errorCode = 0;
    CALL_API(MYKONOS_waitInitCals(mykonos_config.device,
                timeout, &errorFlag, &errorCode));

    if ((errorFlag != 0) || (errorCode != 0)) {
        throw mpm::runtime_error("Init cals failed!");
        // TODO: add more debugging information here
    }
    CALL_API(MYKONOS_enableTrackingCals(mykonos_config.device,
                tracking_cals_mask));
    // ready for radioOn
}

uint8_t ad937x_device::set_lo_source(
        const uhd::direction_t direction,
        const uint8_t pll_source
) {
    switch (direction){
        case TX_DIRECTION:
            mykonos_config.device->tx->txPllUseExternalLo = pll_source;
            return pll_source;
        case RX_DIRECTION:
            mykonos_config.device->rx->rxPllUseExternalLo = pll_source;
            return pll_source;
    default:
        MPM_THROW_INVALID_CODE_PATH();
    }
}

uint8_t ad937x_device::get_lo_source(const uhd::direction_t direction) const {
    switch (direction){
        case TX_DIRECTION:
            return mykonos_config.device->tx->txPllUseExternalLo;
        case RX_DIRECTION:
            return mykonos_config.device->rx->rxPllUseExternalLo;
    default:
        MPM_THROW_INVALID_CODE_PATH();
    }
}
void ad937x_device::begin_initialization()
{
    CALL_API(MYKONOS_initialize(mykonos_config.device));

    _verify_product_id();

    if (!get_pll_lock_status(CLK_SYNTH))
    {
        throw mpm::runtime_error("AD937x CLK_SYNTH PLL failed to lock");
    }

    uint8_t mcs_status = 0;
    CALL_API(MYKONOS_enableMultichipSync(mykonos_config.device, 1, &mcs_status));
}

void ad937x_device::finish_initialization()
{
    _verify_multichip_sync_status(multichip_sync_t::PARTIAL);

    CALL_API(MYKONOS_initArm(mykonos_config.device));
    auto binary = _get_arm_binary();
    CALL_API(MYKONOS_loadArmFromBinary(
        mykonos_config.device,
        binary.data(),
        binary.size()));

    // TODO: check ARM version before or after the load of the ARM
    // currently binary has no readable version number until after it's loaded
    //Run setup RF
    _setup_rf();
}

void ad937x_device::start_jesd_tx()
{
    CALL_API(MYKONOS_enableSysrefToRxFramer(mykonos_config.device, 1));
}

void ad937x_device::start_jesd_rx()
{
    CALL_API(MYKONOS_enableSysrefToDeframer(mykonos_config.device, 0));
    CALL_API(MYKONOS_resetDeframer(mykonos_config.device));
    CALL_API(MYKONOS_enableSysrefToDeframer(mykonos_config.device, 1));
}

void ad937x_device::start_radio()
{
    CALL_API(MYKONOS_radioOn(mykonos_config.device));
}

void ad937x_device::stop_radio()
{
    CALL_API(MYKONOS_radioOff(mykonos_config.device));
}



/******************************************************
Get status functions
******************************************************/

uint8_t ad937x_device::get_multichip_sync_status()
{
    uint8_t mcs_status = 0;
    // to check status, just call the enable function with a 0 instead of a 1, seems good
    CALL_API(MYKONOS_enableMultichipSync(mykonos_config.device, 0, &mcs_status));
    return mcs_status;
}

uint8_t ad937x_device::get_framer_status()
{
    uint8_t status = 0;
    CALL_API(MYKONOS_readRxFramerStatus(mykonos_config.device, &status));
    return status;
}

uint8_t ad937x_device::get_deframer_status()
{
    uint8_t status = 0;
    CALL_API(MYKONOS_readDeframerStatus(mykonos_config.device, &status));
    return status;
}

uint16_t ad937x_device::get_ilas_config_match()
{
    uint16_t ilas_status = 0;
    CALL_API(MYKONOS_jesd204bIlasCheck(mykonos_config.device, &ilas_status));
    return ilas_status;
}

uint8_t ad937x_device::get_product_id()
{
    uint8_t id;
    CALL_API(MYKONOS_getProductId(mykonos_config.device, &id));
    return id;
}

uint8_t ad937x_device::get_device_rev()
{
    uint8_t rev;
    CALL_API(MYKONOS_getDeviceRev(mykonos_config.device, &rev));
    return rev;
}

api_version_t ad937x_device::get_api_version()
{
    api_version_t api;
    CALL_API(MYKONOS_getApiVersion(
        mykonos_config.device,
        &api.silicon_ver,
        &api.major_ver,
        &api.minor_ver,
        &api.build_ver));
    return api;
}

arm_version_t ad937x_device::get_arm_version()
{
    arm_version_t arm;
    mykonosBuild_t build;
    CALL_API(MYKONOS_getArmVersion(
        mykonos_config.device,
        &arm.major_ver,
        &arm.minor_ver,
        &arm.rc_ver,
        &build));

    switch (build)
    {
    case MYK_BUILD_RELEASE:
        arm.build_type = mpm::ad937x::device::build_type_t::RELEASE;
        break;
    case MYK_BUILD_DEBUG:
        arm.build_type = mpm::ad937x::device::build_type_t::DEBUG;
        break;
    case MYK_BUILD_TEST_OBJECT:
        arm.build_type = mpm::ad937x::device::build_type_t::TEST_OBJECT;
        break;
    default:
        MPM_THROW_INVALID_CODE_PATH();
    }

    return arm;
}


/******************************************************
Set configuration functions
******************************************************/

void ad937x_device::enable_jesd_loopback(const uint8_t enable)
{
    const auto state = _move_to_config_state();
    CALL_API(MYKONOS_setRxFramerDataSource(mykonos_config.device, enable));
    _restore_from_config_state(state);
}

double ad937x_device::set_clock_rate(const double req_rate)
{
    const auto rate = static_cast<uint32_t>(req_rate / 1000.0);

    const auto state = _move_to_config_state();
    mykonos_config.device->clocks->deviceClock_kHz = rate;
    CALL_API(MYKONOS_initDigitalClocks(mykonos_config.device));
    _restore_from_config_state(state);

    return static_cast<double>(rate);
}

void ad937x_device::enable_channel(
        const direction_t direction,
        const chain_t chain,
        const bool enable
) {
    // TODO:
    // Turns out the only code in the API that actually sets the channel enable settings
    // is _initialize(). Need to figure out how to deal with this.
    // mmeserve 8/24/2017
    // While it is possible to change the enable state after disabling the radio, we'll probably
    // always use the GPIO pins to do so. Delete this function at a later time.
}

double ad937x_device::tune(
        const direction_t direction,
        const double value,
        const bool wait_for_lock = false
) {
    // I'm not sure why we set the PLL value in the config AND as a function parameter
    // but here it is

    mykonosRfPllName_t pll;
    uint8_t locked_pll;
    uint64_t* config_value;
    const uint64_t integer_value = static_cast<uint64_t>(value);
    switch (direction)
    {
    case TX_DIRECTION:
        pll = TX_PLL;
        locked_pll = TX_SYNTH;
        config_value = &(mykonos_config.device->tx->txPllLoFrequency_Hz);
        break;
    case RX_DIRECTION:
        pll = RX_PLL;
        locked_pll = RX_SYNTH;
        config_value = &(mykonos_config.device->rx->rxPllLoFrequency_Hz);
        break;
    default:
        MPM_THROW_INVALID_CODE_PATH();
    }

    const auto state = _move_to_config_state();
    *config_value = integer_value;
    CALL_API(MYKONOS_setRfPllFrequency(mykonos_config.device, pll, integer_value));

    if (wait_for_lock)
    {
        if (!get_pll_lock_status(locked_pll, true))
        {
            throw mpm::runtime_error("PLL did not lock");
        }
    }
    _restore_from_config_state(state);

    return get_freq(direction);
}

double ad937x_device::set_bw_filter(
        const direction_t direction,
        const double value
) {

    switch (direction)
    {
        case TX_DIRECTION:
        {
            mykonos_config.device->tx->txProfile->rfBandwidth_Hz = value;
            mykonos_config.device->tx->txProfile->txBbf3dBCorner_kHz = value/1000;
            mykonos_config.device->tx->txProfile->txDac3dBCorner_kHz = value/1000;
            break;
        }
        case RX_DIRECTION:
        {
            mykonos_config.device->rx->rxProfile->rfBandwidth_Hz = value;
            mykonos_config.device->rx->rxProfile->rxBbf3dBCorner_kHz = value/1000;
            break;
        }
    }
    const auto state = _move_to_config_state();
    CALL_API(MYKONOS_writeArmProfile(mykonos_config.device));
    _restore_from_config_state(state);
    return value; // TODO: what is coercer value?
}


double ad937x_device::set_gain(
        const direction_t direction,
        const chain_t chain,
        const double value
) {
    double coerced_value;
    const auto state = _move_to_config_state();
    switch (direction)
    {
    case TX_DIRECTION:
    {
        const uint16_t attenuation = _convert_tx_gain_to_mykonos(value);
        coerced_value = static_cast<double>(attenuation);

        std::function<mykonosErr_t(mykonosDevice_t*, uint16_t)> func;
        switch (chain)
        {
        case chain_t::ONE:
            func = MYKONOS_setTx1Attenuation;
            break;
        case chain_t::TWO:
            func = MYKONOS_setTx2Attenuation;
            break;
        default:
            MPM_THROW_INVALID_CODE_PATH();
        }
        CALL_API(func(mykonos_config.device, attenuation));
        break;
    }
    case RX_DIRECTION:
    {
        const uint8_t gain = _convert_rx_gain_to_mykonos(value);
        coerced_value = static_cast<double>(gain);

        std::function<mykonosErr_t(mykonosDevice_t*, uint8_t)> func;
        switch (chain)
        {
        case chain_t::ONE:
            func = MYKONOS_setRx1ManualGain;
            break;
        case chain_t::TWO:
            func = MYKONOS_setRx2ManualGain;
            break;
        default:
            MPM_THROW_INVALID_CODE_PATH();
        }
        CALL_API(func(mykonos_config.device, gain));
        break;
    }
    default:
        MPM_THROW_INVALID_CODE_PATH();
    }
    _restore_from_config_state(state);

    return get_gain(direction, chain);
}

void ad937x_device::set_agc_mode(
        const direction_t direction,
        const gain_mode_t mode
) {
    mykonosGainMode_t mykonos_mode;
    switch (direction)
    {
    case RX_DIRECTION:
        switch (mode)
        {
        case gain_mode_t::MANUAL:
            mykonos_mode = MGC;
            break;
        case gain_mode_t::AUTOMATIC:
            mykonos_mode = AGC;
            break;
        case gain_mode_t::HYBRID:
            mykonos_mode = HYBRID;
            break;
        default:
            MPM_THROW_INVALID_CODE_PATH();
        }
        break;
    default:
        MPM_THROW_INVALID_CODE_PATH();
    }

    const auto state = _move_to_config_state();
    CALL_API(MYKONOS_setRxGainControlMode(mykonos_config.device, mykonos_mode));
    _restore_from_config_state(state);
}

void ad937x_device::set_fir(
    const direction_t direction,
    int8_t gain,
    const std::vector<int16_t> & fir)
{
    switch (direction)
    {
    case TX_DIRECTION:
        mykonos_config.tx_fir_config.set_fir(gain, fir);
        break;
    case RX_DIRECTION:
        mykonos_config.rx_fir_config.set_fir(gain, fir);
        break;
    default:
        MPM_THROW_INVALID_CODE_PATH();
    }

    // TODO: reload this on device
}

void ad937x_device::set_gain_pin_step_sizes(
        const direction_t direction,
        const chain_t chain,
        const double inc_step,
        const double dec_step
) {
    if (direction == RX_DIRECTION)
    {
        gain_ctrl.config.at(direction).at(chain).inc_step =
            static_cast<uint8_t>(inc_step / 0.5);
        gain_ctrl.config.at(direction).at(chain).dec_step =
            static_cast<uint8_t>(dec_step / 0.5);
    }
    else if (direction == TX_DIRECTION) {
        // !!! TX is attenuation direction, so the pins are flipped !!!
        gain_ctrl.config.at(direction).at(chain).dec_step =
            static_cast<uint8_t>(inc_step / 0.05);
        gain_ctrl.config.at(direction).at(chain).inc_step =
            static_cast<uint8_t>(dec_step / 0.05);
    }
    else {
        MPM_THROW_INVALID_CODE_PATH();
    }
    _apply_gain_pins(direction, chain);
}

void ad937x_device::set_enable_gain_pins(
        const direction_t direction,
        const chain_t chain,
        const bool enable
) {
    gain_ctrl.config.at(direction).at(chain).enable = enable;
    _apply_gain_pins(direction, chain);
}



/******************************************************
Get configuration functions
******************************************************/
double ad937x_device::get_freq(const direction_t direction)
{
    mykonosRfPllName_t pll;
    switch (direction)
    {
    case TX_DIRECTION: pll = TX_PLL; break;
    case RX_DIRECTION: pll = RX_PLL; break;
    default:
        MPM_THROW_INVALID_CODE_PATH();
    }

    // TODO: because coerced_pll is returned as an integer, it's not accurate
    uint64_t coerced_pll;
    CALL_API(MYKONOS_getRfPllFrequency(mykonos_config.device, pll, &coerced_pll));
    return static_cast<double>(coerced_pll);
}

bool ad937x_device::get_pll_lock_status(
        const uint8_t pll,
        const bool wait_for_lock
) {
    uint8_t pll_status;
    CALL_API(MYKONOS_checkPllsLockStatus(mykonos_config.device, &pll_status));

    if (not wait_for_lock)
    {
        return (pll_status & pll) == pll;
    }
    else {
        const auto lock_time =
            std::chrono::steady_clock::now()
            + std::chrono::milliseconds(PLL_LOCK_TIMEOUT_MS);
        bool locked = false;
        while (not locked and lock_time > std::chrono::steady_clock::now())
        {
            locked = get_pll_lock_status(pll);
        }

        if (!locked)
        {
            // last chance
            locked = get_pll_lock_status(pll);
        }
        return locked;
    }
}

double ad937x_device::get_gain(
        const direction_t direction,
        const chain_t chain
) {
    switch (direction)
    {
        case TX_DIRECTION:
        {
            std::function<mykonosErr_t(mykonosDevice_t*, uint16_t*)> func;
            switch (chain)
            {
            case chain_t::ONE:
                func = MYKONOS_getTx1Attenuation;
                break;
            case chain_t::TWO:
                func = MYKONOS_getTx2Attenuation;
                break;
            }
            uint16_t atten;
            CALL_API(func(mykonos_config.device, &atten));
            return _convert_tx_gain_from_mykonos(atten);
        }
        case RX_DIRECTION:
        {
            std::function<mykonosErr_t(mykonosDevice_t*, uint8_t*)> func;
            switch (chain)
            {
            case chain_t::ONE:
                func = MYKONOS_getRx1Gain;
                break;
            case chain_t::TWO:
                func = MYKONOS_getRx2Gain;
                break;
            }
            uint8_t gain;
            CALL_API(func(mykonos_config.device, &gain));
            return _convert_rx_gain_from_mykonos(gain);
        }
        default:
            MPM_THROW_INVALID_CODE_PATH();
    }
}

std::vector<int16_t> ad937x_device::get_fir(
    const direction_t direction,
    int8_t &gain)
{
    switch (direction)
    {
    case TX_DIRECTION:
        return mykonos_config.tx_fir_config.get_fir(gain);
    case RX_DIRECTION:
        return mykonos_config.rx_fir_config.get_fir(gain);
    default:
        MPM_THROW_INVALID_CODE_PATH();
    }
}

int16_t ad937x_device::get_temperature()
{
    // TODO: deal with the status.tempValid flag
    mykonosTempSensorStatus_t status;
    CALL_GPIO_API(MYKONOS_readTempSensor(mykonos_config.device, &status));
    return status.tempCode;
}


void ad937x_device::set_master_clock_rate(const mcr_t rate)
{
    switch (rate) {
    case MCR_125_00MHZ: {
        mykonos_config.device->clocks->deviceClock_kHz = 125000;
        mykonos_config.device->clocks->clkPllVcoFreq_kHz = 10000000;
        mykonos_config.device->clocks->clkPllVcoDiv = ::VCODIV_2;
        set_fir(TX_DIRECTION,
            mykonos_config.device->tx->txProfile->txFir->gain_dB,
            std::vector<int16_t>(ad937x_config_t::DEFAULT_TX_FIR,
                ad937x_config_t::DEFAULT_TX_FIR
                    + ad937x_config_t::DEFAULT_TX_FIR_SIZE)
            );
        mykonos_config.device->tx->txProfile->iqRate_kHz = 125000;
        mykonos_config.device->tx->txProfile->primarySigBandwidth_Hz = 20000000;
        mykonos_config.device->tx->txProfile->rfBandwidth_Hz = 102000000;
        mykonos_config.device->tx->txProfile->txDac3dBCorner_kHz = 722000;
        mykonos_config.device->tx->txProfile->txBbf3dBCorner_kHz = 51000;

        set_fir(RX_DIRECTION,
            mykonos_config.device->rx->rxProfile->rxFir->gain_dB,
            std::vector<int16_t>(ad937x_config_t::DEFAULT_RX_FIR,
                ad937x_config_t::DEFAULT_RX_FIR
                    + ad937x_config_t::DEFAULT_RX_FIR_SIZE)
            );

        mykonos_config.device->rx->rxProfile->iqRate_kHz = 125000;
        mykonos_config.device->rx->rxProfile->rxBbf3dBCorner_kHz = 102000;

        mykonos_config.device->obsRx->orxProfile->iqRate_kHz = 125000;
        mykonos_config.device->obsRx->orxProfile->rxBbf3dBCorner_kHz = 102000;
        break;
    }
    case MCR_122_88MHZ: {
        mykonos_config.device->clocks->deviceClock_kHz = 122880;
        mykonos_config.device->clocks->clkPllVcoFreq_kHz = 9830400;
        mykonos_config.device->clocks->clkPllVcoDiv = ::VCODIV_2;
        set_fir(TX_DIRECTION,
            mykonos_config.device->tx->txProfile->txFir->gain_dB,
            std::vector<int16_t>(ad937x_config_t::DEFAULT_TX_FIR,
                ad937x_config_t::DEFAULT_TX_FIR
                    + ad937x_config_t::DEFAULT_TX_FIR_SIZE)
            );
        mykonos_config.device->tx->txProfile->iqRate_kHz = 122880;
        mykonos_config.device->tx->txProfile->primarySigBandwidth_Hz = 20000000;
        mykonos_config.device->tx->txProfile->rfBandwidth_Hz = 100000000;
        mykonos_config.device->tx->txProfile->txDac3dBCorner_kHz = 710539;
        mykonos_config.device->tx->txProfile->txBbf3dBCorner_kHz = 50000;

        set_fir(RX_DIRECTION,
            mykonos_config.device->rx->rxProfile->rxFir->gain_dB,
            std::vector<int16_t>(ad937x_config_t::DEFAULT_RX_FIR,
                ad937x_config_t::DEFAULT_RX_FIR
                    + ad937x_config_t::DEFAULT_RX_FIR_SIZE)
            );
        mykonos_config.device->rx->rxProfile->iqRate_kHz = 122880;
        mykonos_config.device->rx->rxProfile->rxBbf3dBCorner_kHz = 100000;

        mykonos_config.device->obsRx->orxProfile->iqRate_kHz = 122880;
        mykonos_config.device->obsRx->orxProfile->rxBbf3dBCorner_kHz = 100000;
        break;
    }
    case MCR_153_60MHZ: {
        mykonos_config.device->clocks->deviceClock_kHz = 153600;
        mykonos_config.device->clocks->clkPllVcoFreq_kHz = 6144000;
        mykonos_config.device->clocks->clkPllVcoDiv = ::VCODIV_1;
        set_fir(TX_DIRECTION,
            mykonos_config.device->tx->txProfile->txFir->gain_dB,
            std::vector<int16_t>(ad937x_config_t::DEFAULT_TX_FIR_15366,
                ad937x_config_t::DEFAULT_TX_FIR_15366
                    + ad937x_config_t::DEFAULT_TX_FIR_SIZE)
            );
        mykonos_config.device->tx->txProfile->iqRate_kHz = 153600;
        mykonos_config.device->tx->txProfile->primarySigBandwidth_Hz = 10000000;
        mykonos_config.device->tx->txProfile->rfBandwidth_Hz = 100000000;
        mykonos_config.device->tx->txProfile->txDac3dBCorner_kHz = 100000;
        mykonos_config.device->tx->txProfile->txBbf3dBCorner_kHz = 100000;

        set_fir(RX_DIRECTION,
            mykonos_config.device->rx->rxProfile->rxFir->gain_dB,
            std::vector<int16_t>(ad937x_config_t::DEFAULT_RX_FIR_15366,
                ad937x_config_t::DEFAULT_RX_FIR_15366
                    + ad937x_config_t::DEFAULT_RX_FIR_SIZE)
            );
        mykonos_config.device->rx->rxProfile->iqRate_kHz = 153600;
        mykonos_config.device->rx->rxProfile->rxBbf3dBCorner_kHz = 100000;

        mykonos_config.device->obsRx->orxProfile->iqRate_kHz = 153600;
        mykonos_config.device->obsRx->orxProfile->rxBbf3dBCorner_kHz = 225000;
        break;
    }
    default:
        MPM_THROW_INVALID_CODE_PATH();
    }
}
