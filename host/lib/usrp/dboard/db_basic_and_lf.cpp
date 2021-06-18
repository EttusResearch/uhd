//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "db_basic_and_lf.hpp"
#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::usrp::dboard::basic_and_lf;

/***********************************************************************
 * The basic and lf boards:
 *   They share a common class because only the frequency bounds differ.
 **********************************************************************/
class basic_rx : public rx_dboard_base
{
public:
    basic_rx(ctor_args_t args, double max_freq);
    ~basic_rx(void) override;

private:
    double _max_freq;
};

class basic_tx : public tx_dboard_base
{
public:
    basic_tx(ctor_args_t args, double max_freq);
    ~basic_tx(void) override;

private:
    double _max_freq;
};

/***********************************************************************
 * Register the basic and LF dboards
 **********************************************************************/
static dboard_base::sptr make_basic_rx(dboard_base::ctor_args_t args)
{
    return dboard_base::sptr(new basic_rx(args, BASIC_MAX_BANDWIDTH));
}

static dboard_base::sptr make_basic_tx(dboard_base::ctor_args_t args)
{
    return dboard_base::sptr(new basic_tx(args, BASIC_MAX_BANDWIDTH));
}

static dboard_base::sptr make_lf_rx(dboard_base::ctor_args_t args)
{
    return dboard_base::sptr(new basic_rx(args, LF_MAX_BANDWIDTH));
}

static dboard_base::sptr make_lf_tx(dboard_base::ctor_args_t args)
{
    return dboard_base::sptr(new basic_tx(args, LF_MAX_BANDWIDTH));
}

UHD_STATIC_BLOCK(reg_basic_and_lf_dboards)
{
    dboard_manager::register_dboard(
        BASIC_TX_PID, &make_basic_tx, "Basic TX", antenna_mode_to_conn.keys());
    dboard_manager::register_dboard(
        BASIC_RX_PID, &make_basic_rx, "Basic RX", antenna_mode_to_conn.keys());
    dboard_manager::register_dboard(
        LF_TX_PID, &make_lf_tx, "LF TX", antenna_mode_to_conn.keys());
    dboard_manager::register_dboard(
        LF_RX_PID, &make_lf_rx, "LF RX", antenna_mode_to_conn.keys());
    dboard_manager::register_dboard(
        BASIC_TX_RFNOC_PID, &make_basic_tx, "Basic TX", tx_frontends);
    dboard_manager::register_dboard(
        BASIC_RX_RFNOC_PID, &make_basic_rx, "Basic RX", rx_frontends);
    dboard_manager::register_dboard(
        LF_TX_RFNOC_PID, &make_lf_tx, "LF TX", tx_frontends);
    dboard_manager::register_dboard(
        LF_RX_RFNOC_PID, &make_lf_rx, "LF RX", rx_frontends);
}

/***********************************************************************
 * Basic and LF RX dboard
 **********************************************************************/
basic_rx::basic_rx(ctor_args_t args, double max_freq)
    : rx_dboard_base(args), _max_freq(max_freq)
{
    // Examine the frontend to use the RFNoC or the N210 implementation
    // (preserves legacy behavior)
    const std::string fe_name(get_subdev_name());
    const bool is_rfnoc_dev = std::find(rx_frontends.begin(), rx_frontends.end(), fe_name)
                              != rx_frontends.end();
    const std::string ant_mode(is_rfnoc_dev ? "AB" : fe_name);
    const std::string fe_conn(antenna_mode_to_conn[ant_mode]);
    const std::string db_name([this]() {
        switch (get_rx_id().to_uint16()) {
            case BASIC_RX_PID:
            case BASIC_RX_RFNOC_PID:
                return str(boost::format("%s (%s)") % "BasicRX" % get_subdev_name());
            case LF_RX_PID:
            case LF_RX_RFNOC_PID:
                return str(boost::format("%s (%s)") % "LFRX" % get_subdev_name());
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }());
    UHD_LOG_TRACE("BASICRX",
        "Initializing driver for: " << db_name << " IQ connection type: " << ant_mode);
    UHD_LOG_TRACE("BASICRX", "Is RFNoC Device: " << (is_rfnoc_dev ? "Yes" : "No"));

    std::vector<std::string> antenna_options = antenna_mode_to_conn.keys();

    ////////////////////////////////////////////////////////////////////
    // Register properties
    ////////////////////////////////////////////////////////////////////
    this->get_rx_subtree()->create<std::string>("name").set(db_name);
    this->get_rx_subtree()->create<std::string>("id").set(
        (get_rx_id().to_uint16() & 0xFF) == BASIC_RX_PID ? "basicrx" : "lfrx");
    this->get_rx_subtree()->create<int>("gains"); // phony property so this dir exists
    this->get_rx_subtree()->create<double>("freq/value").set_publisher([]() {
        return 0.0;
    });
    this->get_rx_subtree()
        ->create<meta_range_t>("freq/range")
        .set(freq_range_t(-_max_freq, +_max_freq));
    this->get_rx_subtree()
        ->create<std::string>("antenna/value")
        .set(is_rfnoc_dev ? ant_mode : "");
    this->get_rx_subtree()
        ->create<std::vector<std::string>>("antenna/options")
        .set(is_rfnoc_dev ? antenna_options : std::vector<std::string>(1, ""));
    this->get_rx_subtree()->create<int>("sensors"); // phony property so this dir exists
    this->get_rx_subtree()
        ->create<std::string>("connection")
        .set(antenna_mode_to_conn[ant_mode]);
    this->get_rx_subtree()->create<bool>("enabled").set(true); // always enabled
    this->get_rx_subtree()->create<bool>("use_lo_offset").set(false);
    this->get_rx_subtree()
        ->create<double>("bandwidth/value")
        .set(antenna_mode_bandwidth_scalar.at(ant_mode) * _max_freq);
    this->get_rx_subtree()
        ->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(antenna_mode_bandwidth_scalar.at(ant_mode) * _max_freq,
            antenna_mode_bandwidth_scalar.at(ant_mode) * _max_freq));
    if (is_rfnoc_dev) {
        this->get_rx_subtree()
            ->access<std::string>("antenna/value")
            .add_coerced_subscriber([this](const std::string& ant) {
                this->get_rx_subtree()
                    ->access<std::string>("connection")
                    .set(antenna_mode_to_conn[ant]);
                this->get_rx_subtree()
                    ->access<double>("bandwidth/value")
                    .set(antenna_mode_bandwidth_scalar.at(ant) * _max_freq);
                this->get_rx_subtree()
                    ->access<meta_range_t>("bandwidth/range")
                    .set(freq_range_t(antenna_mode_bandwidth_scalar.at(ant) * _max_freq,
                        antenna_mode_bandwidth_scalar.at(ant) * _max_freq));
                UHD_LOG_TRACE("BASICRX",
                    "Changing antenna mode on channel: " << get_subdev_name()
                                                         << " IQ connection type: "
                                                         << antenna_mode_to_conn[ant]);
            });
    }

    // disable RX dboard clock by default
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, false);

    // set GPIOs to output 0x0000 to decrease noise pickup
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, 0x0000);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, 0xFFFF);
    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, 0x0000);
}

basic_rx::~basic_rx(void)
{
    /* NOP */
}

/***********************************************************************
 * Basic and LF TX dboard
 **********************************************************************/
basic_tx::basic_tx(ctor_args_t args, double max_freq) : tx_dboard_base(args)
{
    // Examine the frontend to use the RFNoC or the N210 implementation
    // (preserves legacy behavior)
    const std::string fe_name(get_subdev_name());
    const bool is_rfnoc_dev = std::find(tx_frontends.begin(), tx_frontends.end(), fe_name)
                              != tx_frontends.end();
    const std::string ant_mode(is_rfnoc_dev ? "AB" : fe_name);
    _max_freq = max_freq;
    const std::string db_name([this]() {
        switch (get_tx_id().to_uint16()) {
            case BASIC_TX_PID:
            case BASIC_TX_RFNOC_PID:
                return str(boost::format("%s (%s)") % "BasicTX" % get_subdev_name());
            case LF_TX_PID:
            case LF_TX_RFNOC_PID:
                return str(boost::format("%s (%s)") % "LFTX" % get_subdev_name());
            default:
                UHD_THROW_INVALID_CODE_PATH();
        }
    }());
    UHD_LOG_TRACE("BASICTX",
        "Initializing driver for: " << db_name << " IQ connection type: " << ant_mode);
    UHD_LOG_TRACE("BASICTX", "Is RFNoC Device: " << (is_rfnoc_dev ? "Yes" : "No"));

    std::vector<std::string> antenna_options = antenna_mode_to_conn.keys();

    ////////////////////////////////////////////////////////////////////
    // Register properties
    ////////////////////////////////////////////////////////////////////
    this->get_tx_subtree()->create<std::string>("name").set(db_name);
    this->get_tx_subtree()->create<std::string>("id").set(
        (get_tx_id().to_uint16() & 0xFF) == BASIC_TX_PID ? "basicrx" : "lfrx");
    this->get_tx_subtree()->create<int>("gains"); // phony property so this dir exists
    this->get_tx_subtree()->create<double>("freq/value").set_publisher([]() {
        return 0.0;
    });
    this->get_tx_subtree()
        ->create<meta_range_t>("freq/range")
        .set(freq_range_t(-_max_freq, +_max_freq));
    this->get_tx_subtree()
        ->create<std::string>("antenna/value")
        .set(is_rfnoc_dev ? ant_mode : "");
    this->get_tx_subtree()
        ->create<std::vector<std::string>>("antenna/options")
        .set(is_rfnoc_dev ? antenna_options : std::vector<std::string>(1, ""));
    this->get_tx_subtree()->create<int>("sensors"); // phony property so this dir exists
    this->get_tx_subtree()
        ->create<std::string>("connection")
        .set(antenna_mode_to_conn[ant_mode]);
    this->get_tx_subtree()->create<bool>("enabled").set(true); // always enabled
    this->get_tx_subtree()->create<bool>("use_lo_offset").set(false);
    this->get_tx_subtree()
        ->create<double>("bandwidth/value")
        .set(antenna_mode_bandwidth_scalar.at(ant_mode) * _max_freq);
    this->get_tx_subtree()
        ->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(antenna_mode_bandwidth_scalar.at(ant_mode) * _max_freq,
            antenna_mode_bandwidth_scalar.at(ant_mode) * _max_freq));
    if (is_rfnoc_dev) {
        this->get_tx_subtree()
            ->access<std::string>("antenna/value")
            .add_coerced_subscriber([this](const std::string& ant) {
                this->get_tx_subtree()
                    ->access<std::string>("connection")
                    .set(antenna_mode_to_conn[ant]);
                this->get_tx_subtree()
                    ->access<double>("bandwidth/value")
                    .set(antenna_mode_bandwidth_scalar.at(ant) * _max_freq);
                this->get_tx_subtree()
                    ->access<meta_range_t>("bandwidth/range")
                    .set(freq_range_t(antenna_mode_bandwidth_scalar.at(ant) * _max_freq,
                        antenna_mode_bandwidth_scalar.at(ant) * _max_freq));
                UHD_LOG_TRACE("BASICTX",
                    "Changing antenna mode for channel: " << get_subdev_name()
                                                          << " IQ connection type: "
                                                          << antenna_mode_to_conn[ant]);
            });
    }

    // disable TX dboard clock by default
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_TX, false);

    // set GPIOs to output 0x0000 to decrease noise pickup
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, 0x0000);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, 0xFFFF);
    this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, 0x0000);
}

basic_tx::~basic_tx(void)
{
    /* NOP */
}
