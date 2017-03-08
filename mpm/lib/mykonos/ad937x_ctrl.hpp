#pragma once

// TODO: fix path of UHD includes
#include <../../host/include/uhd/types/direction.hpp>
#include <../../host/include/uhd/types/ranges.hpp>
#include <../../host/include/uhd/exception.hpp>

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

class ad937x_ctrl : public boost::noncopyable
{
public:
    typedef std::shared_ptr<ad937x_ctrl> sptr;
    virtual ~ad937x_ctrl(void) {};

    static uhd::meta_range_t get_rf_freq_range(void);
    static uhd::meta_range_t get_bw_filter_range(void);
    static std::vector<double> get_clock_rates(void);
    static uhd::meta_range_t get_gain_range(const std::string &which);

    virtual uint8_t get_product_id() = 0;
    virtual uint8_t get_device_rev() = 0;
    virtual std::string get_api_version() = 0;
    virtual std::string get_arm_version() = 0;

    virtual double set_bw_filter(const std::string &which, const double value) = 0;
    virtual double set_gain(const std::string &which, const double value) = 0;

    virtual void set_agc(const std::string &which, const bool enable) = 0;
    virtual void set_agc_mode(const std::string &which, const std::string &mode) = 0;

    virtual double set_clock_rate(const double value) = 0;
    virtual void enable_channel(const std::string &which, const bool enable) = 0;

    virtual double set_freq(const std::string &which, const double value) = 0;
    virtual double get_freq(const std::string &which) = 0;

    virtual void set_fir(const std::string &which, const std::vector<int16_t> & fir) = 0;
    virtual std::vector<int16_t> get_fir(const std::string &which) = 0;

    virtual int16_t get_temperature() = 0;

protected:
    static uhd::direction_t _get_direction_from_antenna(const std::string& antenna);
    static ad937x_device::chain_t _get_chain_from_antenna(const std::string& antenna);

    static std::vector<size_t> _get_valid_fir_lengths(const std::string& which);
};