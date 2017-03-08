#include "adi/mykonos.h"

#include "ad937x_ctrl.hpp"
#include "ad937x_device.hpp"
#include <sstream>
#include <functional>
#include <iostream>
#include <cmath>

uhd::meta_range_t ad937x_ctrl::get_rf_freq_range(void)
{
    return uhd::meta_range_t(300e6, 6e9);
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
        return uhd::meta_range_t(0, 30, 0.5);
    case uhd::direction_t::TX_DIRECTION:
        return uhd::meta_range_t(0, 41.95, 0.05);
    default:
        throw uhd::runtime_error("ad937x_ctrl got an invalid channel string.");
        return uhd::meta_range_t();
    }
}

std::vector<size_t> ad937x_ctrl::_get_valid_fir_lengths(const std::string& which)
{
    auto dir = _get_direction_from_antenna(which);
    switch (dir)
    {
    case uhd::direction_t::RX_DIRECTION:
        return { 24, 48, 72 };
    case uhd::direction_t::TX_DIRECTION:
        return { 16, 32, 48, 64, 80, 96 };
    default:
        throw uhd::runtime_error("ad937x_ctrl got an invalid channel string.");
        return std::vector<size_t>();
    }
}

uhd::direction_t ad937x_ctrl::_get_direction_from_antenna(const std::string& antenna)
{
    auto sub = antenna.substr(0, 2);
    if (sub == "RX") {
        return uhd::direction_t::RX_DIRECTION;
    }
    else if (sub == "TX") {
        return uhd::direction_t::TX_DIRECTION;
    }
    else {
        throw uhd::runtime_error("ad937x_ctrl got an invalid channel string.");
    }
    return uhd::direction_t::RX_DIRECTION;
}

ad937x_device::chain_t ad937x_ctrl::_get_chain_from_antenna(const std::string& antenna)
{
    auto sub = antenna.substr(2, 1);
    if (sub == "1") {
        return ad937x_device::chain_t::ONE;
    }
    else if (sub == "2") {
        return ad937x_device::chain_t::TWO;
    }
    else {
        throw uhd::runtime_error("ad937x_ctrl got an invalid channel string.");
    }
    return ad937x_device::chain_t::ONE;
}

class ad937x_ctrl_impl : public ad937x_ctrl
{
public:
    // change to uhd::spi_iface
    static sptr make(spi_lock::sptr spi_l, mpm::spi_iface::sptr iface);

    ad937x_ctrl_impl(spi_lock::sptr spi_l, mpm::spi_iface::sptr iface) :
        spi_l(spi_l),
        iface(iface)
    {

    }

    virtual uint8_t get_product_id()
    {
        std::lock_guard<spi_lock>(*spi_l);
        return device.get_product_id();
    }

    virtual uint8_t get_device_rev()
    {
        std::lock_guard<spi_lock>(*spi_l);
        return device.get_device_rev();
    }
    virtual std::string get_api_version()
    {
        std::lock_guard<spi_lock>(*spi_l);
        auto api = device.get_api_version();
        std::ostringstream ss;
        ss  << api.silicon_ver << "."
            << api.major_ver << "."
            << api.minor_ver << "."
            << api.build_ver;
        return ss.str();
    }

    virtual std::string get_arm_version()
    {
        std::lock_guard<spi_lock>(*spi_l);
        auto arm = device.get_arm_version();
        std::ostringstream ss;
        ss  << arm.major_ver << "."
            << arm.minor_ver << "."
            << arm.rc_ver;
        return ss.str();
    }

    virtual double set_bw_filter(const std::string &which, const double value)
    {
        // TODO implement
        return double();
    }

    virtual double set_gain(const std::string &which, const double value)
    {
        auto dir = _get_direction_from_antenna(which);
        auto chain = _get_chain_from_antenna(which);
        return device.set_gain(dir, chain, value);
    }

    virtual void set_agc(const std::string &which, const bool enable)
    {
        auto dir = _get_direction_from_antenna(which);
        if (dir != uhd::direction_t::RX_DIRECTION)
        {
            throw uhd::runtime_error("ad937x_ctrl::set_agc was called on a non-RX channel");
        }
        return device.set_agc(dir, enable);
    }

    virtual void set_agc_mode(const std::string &which, const std::string &mode)
    {
        auto dir = _get_direction_from_antenna(which);
        if (dir != uhd::direction_t::RX_DIRECTION)
        {
            throw uhd::runtime_error("ad937x_ctrl::set_agc was called on a non-RX channel");
        }

        ad937x_device::gain_mode_t gain_mode;
        if (mode == "automatic")
        {
            gain_mode = ad937x_device::gain_mode_t::AUTOMATIC;
        }
        else if (mode == "manual") {
            gain_mode = ad937x_device::gain_mode_t::MANUAL;
        }
        else if (mode == "hybrid") {
            gain_mode = ad937x_device::gain_mode_t::HYBRID;
        }
        else {
            throw uhd::runtime_error("ad937x_ctrl::set_agc_mode was called on a non-RX channel");
        }

        device.set_agc_mode(dir, gain_mode);
    }

    virtual double set_clock_rate(const double value)
    {
        auto rates = get_clock_rates();
        auto coerced_value = value;
        if (std::find(rates.begin(), rates.end(), value) == rates.end())
        {
            coerced_value = rates[0];
        }

        return device.set_clock_rate(coerced_value);
    }

    virtual void enable_channel(const std::string &which, const bool enable)
    {
        auto dir = _get_direction_from_antenna(which);
        auto chain = _get_chain_from_antenna(which);
        return device.enable_channel(dir, chain, enable);
    }

    virtual double set_freq(const std::string &which, const double value)
    {
        auto dir = _get_direction_from_antenna(which);
        auto clipped_value = get_rf_freq_range().clip(value);
        return device.tune(dir, clipped_value);
    }

    virtual double get_freq(const std::string &which)
    {
        auto dir = _get_direction_from_antenna(which);
        return device.get_freq(dir);
    }

    virtual void set_fir(const std::string &which, int8_t gain, const std::vector<int16_t> & fir)
    {
        auto lengths = _get_valid_fir_lengths(which);
        if (std::find(lengths.begin(), lengths.end(), fir.size()) == lengths.end())
        {
            throw uhd::value_error("ad937x_ctrl::set_fir got filter of invalid length");
        }

        ad937x_fir(gain, fir);

    }
    virtual std::vector<int16_t> get_fir(const std::string &which) = 0;

    virtual int16_t get_temperature() = 0;
    
private:
    ad937x_device device;
    spi_lock::sptr spi_l;
    mpm::spi_iface::sptr iface;
};

const double ad937x_ctrl_impl::MIN_FREQ = 300e6;
const double ad937x_ctrl_impl::MAX_FREQ = 6e9;

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

