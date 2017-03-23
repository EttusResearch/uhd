//
// Copyright 2017 Ettus Research (National Instruments)
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "ad937x_device.hpp"
#include "adi/mykonos.h"
#include "adi/mykonos_gpio.h"

#include <functional>
#include <iostream>

const double ad937x_device::MIN_FREQ = 300e6;
const double ad937x_device::MAX_FREQ = 6e9;
const double ad937x_device::MIN_RX_GAIN = 0.0;
const double ad937x_device::MAX_RX_GAIN = 30.0;
const double ad937x_device::RX_GAIN_STEP = 0.5;
const double ad937x_device::MIN_TX_GAIN = 0.0;
const double ad937x_device::MAX_TX_GAIN = 41.95;
const double ad937x_device::TX_GAIN_STEP = 0.05;

static const double RX_DEFAULT_FREQ = 1e9;
static const double TX_DEFAULT_FREQ = 1e9;

// TODO: get the actual device ID
static const uint32_t AD9371_PRODUCT_ID = 0x1F;

// TODO: move this to whereever we declare the ARM binary
static const size_t ARM_BINARY_SIZE = 98304;

static const uint32_t INIT_CAL_TIMEOUT_MS = 10000;

static const uint32_t INIT_CALS =
    TX_BB_FILTER |
    ADC_TUNER |
    TIA_3DB_CORNER |
    DC_OFFSET |
    TX_ATTENUATION_DELAY |
    RX_GAIN_DELAY |
    FLASH_CAL |
    PATH_DELAY |
    TX_LO_LEAKAGE_INTERNAL |
//  TX_LO_LEAKAGE_EXTERNAL |
    TX_QEC_INIT |
    LOOPBACK_RX_LO_DELAY |
    LOOPBACK_RX_RX_QEC_INIT |
    RX_LO_DELAY |
    RX_QEC_INIT |
//  DPD_INIT |
//  CLGC_INIT |
//  VSWR_INIT |
    0;

static const uint32_t TRACKING_CALS =
    TRACK_RX1_QEC |
    TRACK_RX2_QEC |
    TRACK_ORX1_QEC |
    TRACK_ORX2_QEC |
//  TRACK_TX1_LOL |
//  TRACK_TX2_LOL |
    TRACK_TX1_QEC |
    TRACK_TX2_QEC |
//  TRACK_TX1_DPD |
//  TRACK_TX2_DPD |
//  TRACK_TX1_CLGC |
//  TRACK_TX2_CLGC |
//  TRACK_TX1_VSWR |
//  TRACK_TX2_VSWR |
//  TRACK_ORX1_QEC_SNLO |
//  TRACK_ORX2_QEC_SNLO |
//  TRACK_SRX_QEC |
    0;

// helper function to unify error handling
void ad937x_device::_call_api_function(std::function<mykonosErr_t()> func)
{
    auto error = func();
    if (error != MYKONOS_ERR_OK)
    {
        std::cout << getMykonosErrorMessage(error);
        // TODO: make UHD exception
        //throw std::exception(getMykonosErrorMessage(error));
    }
}

// helper function to unify error handling, GPIO version
void ad937x_device::_call_gpio_api_function(std::function<mykonosGpioErr_t()> func)
{
    auto error = func();
    if (error != MYKONOS_ERR_GPIO_OK)
    {
        std::cout << getGpioMykonosErrorMessage(error);
        // TODO: make UHD exception
        //throw std::exception(getMykonosErrorMessage(error));
    }
}

void ad937x_device::_initialize()
{
    _call_api_function(std::bind(MYKONOS_resetDevice, mykonos_config.device));

    if (get_product_id() != AD9371_PRODUCT_ID)
    {
        throw uhd::runtime_error("AD9371 product ID does not match expected ID!");
    }

    _call_api_function(std::bind(MYKONOS_initialize, mykonos_config.device));

    if (!get_pll_lock_status(pll_t::CLK_SYNTH))
    {
        throw uhd::runtime_error("AD937x CLK_SYNTH PLL failed to lock in initialize()");
    }

    std::vector<uint8_t> binary(98304, 0);
    _load_arm(binary);

    tune(uhd::RX_DIRECTION, RX_DEFAULT_FREQ);
    tune(uhd::TX_DIRECTION, TX_DEFAULT_FREQ);

    // TODO: wait 200ms or change to polling
    if (!get_pll_lock_status(pll_t::RX_SYNTH))
    {
        throw uhd::runtime_error("AD937x RX PLL failed to lock in initialize()");
    }
    if (!get_pll_lock_status(pll_t::TX_SYNTH))
    {
        throw uhd::runtime_error("AD937x TX PLL failed to lock in initialize()");
    }

    // TODO: ADD GPIO CTRL setup here

    set_gain(uhd::RX_DIRECTION, chain_t::ONE, 0);
    set_gain(uhd::RX_DIRECTION, chain_t::TWO, 0);
    set_gain(uhd::TX_DIRECTION, chain_t::ONE, 0);
    set_gain(uhd::TX_DIRECTION, chain_t::TWO, 0);

    _run_initialization_calibrations();

    // TODO: do external LO leakage calibration here if hardware supports it
    // I don't think we do?

    _start_jesd();
    _enable_tracking_calibrations();

    // radio is ON!
    _call_api_function(std::bind(MYKONOS_radioOn, mykonos_config.device));

    // TODO: ordering of this doesn't seem right, intuitively, verify this works
    _call_api_function(std::bind(MYKONOS_setObsRxPathSource, mykonos_config.device, OBS_RXOFF));
    _call_api_function(std::bind(MYKONOS_setObsRxPathSource, mykonos_config.device, OBS_INTERNALCALS));
}

// TODO: review const-ness in this function with respect to ADI API
void ad937x_device::_load_arm(std::vector<uint8_t> & binary)
{
    _call_api_function(std::bind(MYKONOS_initArm, mykonos_config.device));

    if (binary.size() == ARM_BINARY_SIZE)
    {
        throw uhd::runtime_error("ad937x_device ARM is not the correct size!");
    }

    _call_api_function(std::bind(MYKONOS_loadArmFromBinary, mykonos_config.device, &binary[0], binary.size()));
}

void ad937x_device::_run_initialization_calibrations()
{
    _call_api_function(std::bind(MYKONOS_runInitCals, mykonos_config.device, INIT_CALS));

    uint8_t errorFlag = 0;
    uint8_t errorCode = 0;
    _call_api_function(
        std::bind(MYKONOS_waitInitCals,
            mykonos_config.device,
            INIT_CAL_TIMEOUT_MS,
            &errorFlag,
            &errorCode));

    if ((errorFlag != 0) || (errorCode != 0))
    {
        mykonosInitCalStatus_t initCalStatus = { 0 };
        _call_api_function(std::bind(MYKONOS_getInitCalStatus, mykonos_config.device, &initCalStatus));

        // abort init cals
        uint32_t initCalsCompleted = 0;
        _call_api_function(std::bind(MYKONOS_abortInitCals, mykonos_config.device, &initCalsCompleted));
        // init cals completed contains mask of cals that did finish

        uint16_t errorWord = 0;
        uint16_t statusWord = 0;
        _call_api_function(std::bind(MYKONOS_readArmCmdStatus, mykonos_config.device, &errorWord, &statusWord));

        uint8_t status = 0;
        _call_api_function(std::bind(MYKONOS_readArmCmdStatusByte, mykonos_config.device, 2, &status));
    }
}

void ad937x_device::_start_jesd()
{
    // Stop and/or disable SYSREF
    // ensure BBIC JESD is reset and ready to receive CGS characters

    // prepare to transmit CGS when sysref starts
    _call_api_function(std::bind(MYKONOS_enableSysrefToRxFramer, mykonos_config.device, 1));

    // prepare to transmit CGS when sysref starts
    //_call_api_function(std::bind(MYKONOS_enableSysrefToObsRxFramer, mykonos_config.device, 1));

    // prepare to transmit CGS when sysref starts
    _call_api_function(std::bind(MYKONOS_enableSysrefToDeframer, mykonos_config.device, 0));

    _call_api_function(std::bind(MYKONOS_resetDeframer, mykonos_config.device));
    _call_api_function(std::bind(MYKONOS_enableSysrefToDeframer, mykonos_config.device, 1));

    // make sure BBIC JESD framer is actively transmitting CGS
    // Start SYSREF

    // verify sync code here
    // verify links
    uint8_t framerStatus = 0;
    _call_api_function(std::bind(MYKONOS_readRxFramerStatus, mykonos_config.device, &framerStatus));

    uint8_t deframerStatus = 0;
    _call_api_function(std::bind(MYKONOS_readDeframerStatus, mykonos_config.device, &deframerStatus));
}

void ad937x_device::_enable_tracking_calibrations()
{
    _call_api_function(std::bind(MYKONOS_enableTrackingCals, mykonos_config.device, TRACKING_CALS));
}

ad937x_device::ad937x_device(uhd::spi_iface::sptr iface) :
    full_spi_settings(iface),
    mykonos_config(&full_spi_settings.spi_settings)
{
    _initialize();
}

uint8_t ad937x_device::get_product_id()
{
    uint8_t id;
    _call_api_function(std::bind(MYKONOS_getProductId, mykonos_config.device, &id));
    return id;
}

uint8_t ad937x_device::get_device_rev()
{
    uint8_t rev;
    _call_api_function(std::bind(MYKONOS_getDeviceRev, mykonos_config.device, &rev));
    return rev;
}

ad937x_device::api_version_t ad937x_device::get_api_version()
{
    api_version_t api;
    _call_api_function(std::bind(MYKONOS_getApiVersion,
        mykonos_config.device,
        &api.silicon_ver,
        &api.major_ver,
        &api.minor_ver,
        &api.build_ver));
    return api;
}

ad937x_device::arm_version_t ad937x_device::get_arm_version()
{
    arm_version_t arm;
    _call_api_function(std::bind(MYKONOS_getArmVersion,
        mykonos_config.device,
        &arm.major_ver,
        &arm.minor_ver,
        &arm.rc_ver));
    return arm;
}

double ad937x_device::set_clock_rate(double req_rate)
{
    auto rate = static_cast<uint32_t>(req_rate / 1000.0);
    mykonos_config.device->clocks->deviceClock_kHz = rate;
    _call_api_function(std::bind(MYKONOS_initDigitalClocks, mykonos_config.device));
    return static_cast<double>(rate);
}

void ad937x_device::enable_channel(uhd::direction_t direction, chain_t chain, bool enable)
{
    // TODO:
    // Turns out the only code in the API that actually sets the channel enable settings
    // _initialize(). Need to figure out how to deal with this.
}

double ad937x_device::tune(uhd::direction_t direction, double value)
{
    // I'm not sure why we set the PLL value in the config AND as a function parameter
    // but here it is

    mykonosRfPllName_t pll;
    uint64_t integer_value = static_cast<uint64_t>(value);
    switch (direction)
    {
    case uhd::TX_DIRECTION:
        pll = TX_PLL;
        mykonos_config.device->tx->txPllLoFrequency_Hz = integer_value;
        break;
    case uhd::RX_DIRECTION:
        pll = RX_PLL;
        mykonos_config.device->rx->rxPllLoFrequency_Hz = integer_value;
        break;
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }

    _call_api_function(std::bind(MYKONOS_setRfPllFrequency, mykonos_config.device, pll, integer_value));

    // TODO: coercion here causes extra device accesses, when the formula is provided on pg 119 of the user guide
    // Furthermore, because coerced is returned as an integer, it's not even accurate
    uint64_t coerced_pll;
    _call_api_function(std::bind(MYKONOS_getRfPllFrequency, mykonos_config.device, pll, &coerced_pll));
    return static_cast<double>(coerced_pll);
}

double ad937x_device::get_freq(uhd::direction_t direction)
{
    mykonosRfPllName_t pll;
    switch (direction)
    {
    case uhd::TX_DIRECTION: pll = TX_PLL; break;
    case uhd::RX_DIRECTION: pll = RX_PLL; break;
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }

    // TODO: coercion here causes extra device accesses, when the formula is provided on pg 119 of the user guide
    // Furthermore, because coerced is returned as an integer, it's not even accurate
    uint64_t coerced_pll;
    _call_api_function(std::bind(MYKONOS_getRfPllFrequency, mykonos_config.device, pll, &coerced_pll));
    return static_cast<double>(coerced_pll);
}

bool ad937x_device::get_pll_lock_status(pll_t pll)
{
    uint8_t pll_status;
    _call_api_function(std::bind(MYKONOS_checkPllsLockStatus, mykonos_config.device, &pll_status));
    switch (pll)
    {
    case pll_t::CLK_SYNTH:
        return (pll_status & 0x01) ? 1 : 0;
    case pll_t::RX_SYNTH:
        return (pll_status & 0x02) ? 1 : 0;
    case pll_t::TX_SYNTH:
        return (pll_status & 0x04) ? 1 : 0;
    case pll_t::SNIFF_SYNTH:
        return (pll_status & 0x08) ? 1 : 0;
    case pll_t::CALPLL_SDM:
        return (pll_status & 0x10) ? 1 : 0;
    default:
        UHD_THROW_INVALID_CODE_PATH();
        return false;
    }
}

double ad937x_device::set_bw_filter(uhd::direction_t direction, chain_t chain, double value)
{
    // TODO: implement
    return double();
}

// RX Gain values are table entries given in mykonos_user.h
// An array of gain values is programmed at initialization, which the API will then use for its gain values
// In general, Gain Value = (255 - Gain Table Index)
uint8_t ad937x_device::_convert_rx_gain(double gain)
{
    // gain should be a value 0-60, add 195 to make 195-255
    return static_cast<uint8_t>((gain * 2) + 195);
}

// TX gain is completely different from RX gain for no good reason so deal with it
// TX is set as attenuation using a value from 0-41950 mdB
// Only increments of 50 mdB are valid
uint16_t ad937x_device::_convert_tx_gain(double gain)
{
    // attenuation is inverted and in mdB not dB
    return static_cast<uint16_t>((MAX_TX_GAIN - (gain)) * 1e3);
}


double ad937x_device::set_gain(uhd::direction_t direction, chain_t chain, double value)
{
    double coerced_value;
    switch (direction)
    {
    case uhd::TX_DIRECTION:
    {
        uint16_t attenuation = _convert_tx_gain(value);
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
            UHD_THROW_INVALID_CODE_PATH();
        }
        _call_api_function(std::bind(func, mykonos_config.device, attenuation));
        break;
    }
    case uhd::RX_DIRECTION:
    {
        uint8_t gain = _convert_rx_gain(value);
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
            UHD_THROW_INVALID_CODE_PATH();
        }
        _call_api_function(std::bind(func, mykonos_config.device, gain));
        break;
    }
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }
    return coerced_value;
}

void ad937x_device::set_agc_mode(uhd::direction_t direction, gain_mode_t mode)
{
    switch (direction)
    {
    case uhd::RX_DIRECTION:
        switch (mode)
        {
        case gain_mode_t::MANUAL:
            _call_api_function(std::bind(MYKONOS_setRxGainControlMode, mykonos_config.device, MGC));
            break;
        case gain_mode_t::AUTOMATIC:
            _call_api_function(std::bind(MYKONOS_setRxGainControlMode, mykonos_config.device, AGC));
            break;
        case gain_mode_t::HYBRID:
            _call_api_function(std::bind(MYKONOS_setRxGainControlMode, mykonos_config.device, HYBRID));
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
        }
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }
}

void ad937x_device::set_fir(
    const uhd::direction_t direction,
    const chain_t chain,
    int8_t gain,
    const std::vector<int16_t> & fir)
{
    switch (direction)
    {
    case uhd::TX_DIRECTION:
        mykonos_config.tx_fir_config.set_fir(gain, fir);
        break;
    case uhd::RX_DIRECTION:
        mykonos_config.rx_fir_config.set_fir(gain, fir);
        break;
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }
}

std::vector<int16_t> ad937x_device::get_fir(
    const uhd::direction_t direction,
    const chain_t chain,
    int8_t &gain)
{
    switch (direction)
    {
    case uhd::TX_DIRECTION:
        return mykonos_config.tx_fir_config.get_fir(gain);
    case uhd::RX_DIRECTION:
        return mykonos_config.rx_fir_config.get_fir(gain);
    default:
        UHD_THROW_INVALID_CODE_PATH();
    }
}

int16_t ad937x_device::get_temperature()
{
    // TODO: deal with the status.tempValid flag
    mykonosTempSensorStatus_t status;
    _call_gpio_api_function(std::bind(MYKONOS_readTempSensor, mykonos_config.device, &status));
    return status.tempCode;
}

