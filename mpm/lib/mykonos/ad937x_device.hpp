#pragma once

// TODO: fix path of UHD includes
#include <../../host/include/uhd/types/direction.hpp>
#include <../../host/include/uhd/types/ranges.hpp>
#include <../../host/include/uhd/exception.hpp>

#include "config/ad937x_config_t.hpp"
#include "config/ad937x_fir.h"
#include "adi/t_mykonos.h"
#include "../spi/spi_lock.h"
#include "../spi/spi_config.h"
#include <mpm/spi_iface.hpp>
#include <mpm/spi/adi_ctrl.hpp>
#include <boost/noncopyable.hpp>
#include <functional>

struct ad937x_api_version_t {
    uint32_t silicon_ver;
    uint32_t major_ver;
    uint32_t minor_ver;
    uint32_t build_ver;
};

struct ad937x_arm_version_t {
    uint32_t major_ver;
    uint32_t minor_ver;
    uint32_t rc_ver;
};

class ad937x_device : public boost::noncopyable
{
public:
    enum class gain_mode_t { MANUAL, AUTOMATIC, HYBRID };
    enum class chain_t { ONE, TWO };

    typedef std::shared_ptr<ad937x_device> sptr;
    virtual ~ad937x_device(void) {};
    
    static uhd::meta_range_t get_rf_freq_range(void);
    static uhd::meta_range_t get_bw_filter_range(void);
    static std::vector<double> get_clock_rates(void);

    uhd::meta_range_t get_gain_range(uhd::direction_t direction, chain_t chain);

    uint8_t get_product_id() = 0;
    uint8_t get_device_rev() = 0;
    ad937x_api_version_t get_api_version() = 0;
    ad937x_arm_version_t get_arm_version() = 0;

    double set_bw_filter(const uhd::direction_t direction, const chain_t chain, const double value) = 0;
    double set_gain(const uhd::direction_t direction, const chain_t chain, const double value) = 0;

    void set_agc(const uhd::direction_t direction, const bool enable) = 0;
    void set_agc_mode(const uhd::direction_t direction, const gain_mode_t mode) = 0;

    double set_clock_rate(const double value) = 0;
    void enable_channel(const uhd::direction_t direction, const chain_t chain, const bool enable) = 0;

    double tune(const uhd::direction_t direction, const double value) = 0;
    double get_freq(const uhd::direction_t direction) = 0;

    void set_fir(const uhd::direction_t direction, const chain_t chain, const ad937x_fir & fir) = 0;
    ad937x_fir get_fir(const uhd::direction_t direction, const chain_t chain) = 0;

    int16_t get_temperature() = 0;    

private:
    const static double MIN_FREQ;
    const static double MAX_FREQ;

    virtual void initialize() = 0;

    spi_lock::sptr spi_l;
    mpm::spi_iface::sptr iface;
    mpm_spiSettings_t mpm_sps;

    ad937x_config_t mykonos_config;

    void call_api_function(std::function<mykonosErr_t()> func);

    static uint8_t _convert_rx_gain(double inGain, double &coercedGain);
    static uint16_t _convert_tx_gain(double inGain, double &coercedGain);

    void _set_active_tx_chains(bool tx1, bool tx2);
    void _set_active_rx_chains(bool rx1, bool rx2);

    const static spi_device_settings_t spi_device_settings;
};

const double ad937x_ctrl::MIN_FREQ = 300e6;
const double ad937x_ctrl::MAX_FREQ = 6e9;