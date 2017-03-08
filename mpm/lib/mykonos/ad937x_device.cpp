#include "adi/mykonos.h"

#include "ad937x_device.hpp"
#include <functional>
#include <iostream>
#include <cmath>

/*
ad937x_ctrl::sptr ad937x_ctrl_impl::make(
    spi_lock::sptr spi_l,
    mpm::spi_iface::sptr iface)
{
    return std::make_shared<ad937x_ctrl_impl>(spi_l, iface);
}


void ad937x_ctrl::initialize()
{
    //headlessinit(mykonos_config.device);
    // TODO: finish initialization
    {
        std::lock_guard<spi_lock> lock(*spi_l);
        call_api_function(std::bind(MYKONOS_initialize, mykonos_config.device));
    }    
    get_product_id();
}

ad937x_ctrl::ad937x_ctrl(
    spi_lock::sptr spi_l, 
    mpm::spi_iface::sptr iface) :
    spi_l(spi_l),
    iface(iface)
{
    mpm_sps.spi_iface = iface.get();

    //TODO assert iface->get_chip_select() is 1-8
    mpm_sps.spi_settings.chipSelectIndex = static_cast<uint8_t>(iface->get_chip_select());
    mpm_sps.spi_settings.writeBitPolarity = 1;
    mpm_sps.spi_settings.longInstructionWord = 1; // set to 1 by initialize
    mpm_sps.spi_settings.MSBFirst = 
        (iface->get_endianness() == mpm::spi_iface::spi_endianness_t::LSB_FIRST) ? 0 : 1;
    mpm_sps.spi_settings.CPHA = 0; // set to 0 by initialize
    mpm_sps.spi_settings.CPOL = 0; // set to 0 by initialize
    mpm_sps.spi_settings.enSpiStreaming = 1;
    mpm_sps.spi_settings.autoIncAddrUp = 1;
    mpm_sps.spi_settings.fourWireMode = 
        (iface->get_wire_mode() == mpm::spi_iface::spi_wire_mode_t::THREE_WIRE_MODE) ? 0 : 1;
    mpm_sps.spi_settings.spiClkFreq_Hz = 25000000; 

    initialize();
}

// helper function to unify error handling
// bind is bad, but maybe this is justifiable
void ad937x_ctrl::call_api_function(std::function<mykonosErr_t()> func)
{
    auto error = func();
    if (error != MYKONOS_ERR_OK)
    {
        std::cout << getMykonosErrorMessage(error);
        // TODO: make UHD exception
        //throw std::exception(getMykonosErrorMessage(error));
    }
}

uint8_t ad937x_ctrl::get_product_id()
{
    std::lock_guard<spi_lock> lock(*spi_l);
    uint8_t id;
    call_api_function(std::bind(MYKONOS_getProductId, mykonos_config.device, &id));
    return id;
}

double ad937x_ctrl::set_clock_rate(const double req_rate)
{
    auto rate = static_cast<decltype(mykonos_config.device->clocks->deviceClock_kHz)>(req_rate / 1000);
    mykonos_config.device->clocks->deviceClock_kHz = rate;
    {
        std::lock_guard<spi_lock> lock(*spi_l);
        call_api_function(std::bind(MYKONOS_initDigitalClocks, mykonos_config.device));
    }
    return static_cast<decltype(set_clock_rate(0))>(rate);
}

void ad937x_ctrl::_set_active_tx_chains(bool tx1, bool tx2)
{
    decltype(mykonos_config.device->tx->txChannels) newTxChannel;
    if (tx1 && tx2)
    {
        newTxChannel = TX1_TX2;
    }
    else if (tx1) {
        newTxChannel = TX1;
    }
    else if (tx2) {
        newTxChannel = TX2;
    }
    else {
        newTxChannel = TXOFF;
    }
    mykonos_config.device->tx->txChannels = newTxChannel;
}

void ad937x_ctrl::_set_active_rx_chains(bool rx1, bool rx2)
{
    decltype(mykonos_config.device->rx->rxChannels) newRxChannel;
    if (rx1 && rx2)
    {
        newRxChannel = RX1_RX2;
    }
    else if (rx1) {
        newRxChannel = RX1;
    }
    else if (rx2) {
        newRxChannel = RX2;
    }
    else {
        newRxChannel = RXOFF;
    }
    mykonos_config.device->rx->rxChannels = newRxChannel;
}

void ad937x_ctrl::set_active_chains(direction_t direction, bool channel1, bool channel2)
{
    switch (direction)
    {
    case TX: _set_active_tx_chains(channel1, channel2); break;
    case RX: _set_active_rx_chains(channel1, channel2); break;
    default:
        // TODO: bad code path exception
        throw std::exception();
    }
    // TODO: make this apply the setting
}

double ad937x_ctrl::tune(direction_t direction, const double value)
{
    // I'm not really sure why we set the PLL value in the config AND as a function parameter
    // but here it is

    mykonosRfPllName_t pll;
    uint64_t integer_value = static_cast<uint64_t>(value);
    switch (direction)
    {
    case TX:
        pll = TX_PLL;
        mykonos_config.device->tx->txPllLoFrequency_Hz = integer_value;
        break;
    case RX:
        pll = RX_PLL;
        mykonos_config.device->rx->rxPllLoFrequency_Hz = integer_value;
        break;
    default:
        // TODO: bad code path exception
        throw std::exception();
    }

    {
        std::lock_guard<spi_lock> lock(*spi_l);
        call_api_function(std::bind(MYKONOS_setRfPllFrequency, mykonos_config.device, pll, integer_value));
    }

    // TODO: coercion here causes extra device accesses, when the formula is provided on pg 119 of the user guide
    // Furthermore, because coerced is returned as an integer, it's not even accurate
    uint64_t coerced_pll;
    {
        std::lock_guard<spi_lock> lock(*spi_l);
        call_api_function(std::bind(MYKONOS_getRfPllFrequency, mykonos_config.device, pll, &coerced_pll));
    }
    return static_cast<double>(coerced_pll);
}

double ad937x_ctrl::get_freq(direction_t direction)
{
    mykonosRfPllName_t pll;
    switch (direction)
    {
    case TX: pll = TX_PLL; break;
    case RX: pll = RX_PLL; break;
    default:
        // TODO: bad code path exception
        throw std::exception();
    }

    // TODO: coercion here causes extra device accesses, when the formula is provided on pg 119 of the user guide
    // Furthermore, because coerced is returned as an integer, it's not even accurate
    uint64_t coerced_pll;
    {
        std::lock_guard<spi_lock> lock(*spi_l);
        call_api_function(std::bind(MYKONOS_getRfPllFrequency, mykonos_config.device, pll, &coerced_pll));
    }
    return static_cast<double>(coerced_pll);
    return double();
}

// RX Gain values are table entries given in mykonos_user.h
// An array of gain values is programmed at initialization, which the API will then use for its gain values
// In general, Gain Value = (255 - Gain Table Index)
uint8_t ad937x_ctrl::_convert_rx_gain(double inGain, double &coercedGain)
{
    // TODO: use uhd::meta_range?
    const static double min_gain = 0;
    const static double max_gain = 30;
    const static double gain_step = 0.5;

    coercedGain = inGain;
    if (coercedGain < min_gain)
    {
        coercedGain = min_gain;
    }
    if (coercedGain > max_gain)
    {
        coercedGain = max_gain;
    }

    // round to nearest step
    coercedGain = std::round(coercedGain * (1.0 / gain_step)) / (1.0 / gain_step);

    // gain should be a value 0-60, add 195 to make 195-255
    return static_cast<uint8_t>((coercedGain * 2) + 195);
}

// TX gain is completely different from RX gain for no good reason so deal with it
// TX is set as attenuation using a value from 0-41950 mdB
// Only increments of 50 mdB are valid
uint16_t ad937x_ctrl::_convert_tx_gain(double inGain, double &coercedGain)
{
    // TODO: use uhd::meta_range?
    const static double min_gain = 0;
    const static double max_gain = 41.95;
    const static double gain_step = 0.05;

    coercedGain = inGain;
    if (coercedGain < min_gain)
    {
        coercedGain = min_gain;
    }
    if (coercedGain > max_gain)
    {
        coercedGain = max_gain;
    }

    coercedGain = std::round(coercedGain * (1.0 / gain_step)) / (1.0 / gain_step);

    // attenuation is inverted and in mdB not dB
    return static_cast<uint16_t>((max_gain - (coercedGain)) * 1000);
}


double ad937x_ctrl::set_gain(direction_t direction, chain_t chain, const double value)
{
    double coerced_value;
    switch (direction)
    {
    case TX:
    {
        uint16_t attenuation = _convert_tx_gain(value, coerced_value);
        std::function<mykonosErr_t(mykonosDevice_t*, uint16_t)> func;
        switch (chain)
        {
        case CHAIN_1:
            func = MYKONOS_setTx1Attenuation;
            break;
        case CHAIN_2:
            func = MYKONOS_setTx2Attenuation;
            break;
        default:
            // TODO: bad code path exception
            throw std::exception();
        }
        std::lock_guard<spi_lock> lock(*spi_l);
        call_api_function(std::bind(func, mykonos_config.device, attenuation));
        break;
    }
    case RX:
    {
        uint8_t gain = _convert_rx_gain(value, coerced_value);
        std::function<mykonosErr_t(mykonosDevice_t*, uint8_t)> func;
        switch (chain)
        {
        case CHAIN_1:
            func = MYKONOS_setRx1ManualGain;
            break;
        case CHAIN_2:
            func = MYKONOS_setRx2ManualGain;
            break;
        default:
            // TODO: bad code path exception
            throw std::exception();
        }
        std::lock_guard<spi_lock> lock(*spi_l);
        call_api_function(std::bind(func, mykonos_config.device, gain));
        break;
    }
    default:
        // TODO: bad code path exception
        throw std::exception();
    }
    return coerced_value;
}

double ad937x_ctrl::set_agc_mode(direction_t direction, chain_t chain, gain_mode_t mode)
{
    std::lock_guard<spi_lock> lock(*spi_l);
    switch (direction)
    {
    case RX:
        switch (mode)
        {
        case GAIN_MODE_MANUAL:
            call_api_function(std::bind(MYKONOS_resetRxAgc, mykonos_config.device));
            break;
        case GAIN_MODE_SLOW_AGC:
        case GAIN_MODE_FAST_AGC:
            // TODO: differentiate these
            call_api_function(std::bind(MYKONOS_setupRxAgc, mykonos_config.device));
            break;
        default:
            // TODO: bad code path exception
            throw std::exception();
        }
    default:
        // TODO: bad code path exception
        throw std::exception();
    }
    return double();
}

ad937x_ctrl::sptr ad937x_ctrl::make(spi_lock::sptr spi_l, mpm::spi_iface::sptr iface)
{
    return std::make_shared<ad937x_ctrl>(spi_l, iface);
}
*/

