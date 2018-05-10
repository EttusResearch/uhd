//
// Copyright 2010-2012 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/types/dict.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/utils/assert_has.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/usrp/dboard_base.hpp>
#include <uhd/usrp/dboard_manager.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>

using namespace uhd;
using namespace uhd::usrp;

/***********************************************************************
 * Constants
 **********************************************************************/
namespace {
    constexpr uint32_t BASIC_TX_PID = 0x0000;
    constexpr uint32_t BASIC_RX_PID = 0x0001;
    constexpr uint32_t LF_TX_PID    = 0x000E;
    constexpr uint32_t LF_RX_PID    = 0x000F;

    constexpr double BASIC_MAX_BANDWIDTH = 250e6; // Hz
    constexpr double LF_MAX_BANDWIDTH    = 32e6; // Hz


    const std::map<std::string, double> subdev_bandwidth_scalar{
        {"A", 1.0},
        {"B", 1.0},
        {"AB", 2.0},
        {"BA", 2.0}
    };

    const uhd::dict<std::string, std::string> sd_name_to_conn =
        boost::assign::map_list_of
        ("AB", "IQ")
        ("BA", "QI")
        ("A",  "I")
        ("B",  "Q")
    ;
}


/***********************************************************************
 * The basic and lf boards:
 *   They share a common class because only the frequency bounds differ.
 **********************************************************************/
class basic_rx : public rx_dboard_base{
public:
    basic_rx(ctor_args_t args, double max_freq);
    virtual ~basic_rx(void);

private:
    void set_rx_ant(const std::string& ant);

    double _max_freq;
};

class basic_tx : public tx_dboard_base{
public:
    basic_tx(ctor_args_t args, double max_freq);
    virtual ~basic_tx(void);

private:
    double _max_freq;
};

/***********************************************************************
 * Register the basic and LF dboards
 **********************************************************************/
static dboard_base::sptr make_basic_rx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new basic_rx(args, BASIC_MAX_BANDWIDTH));
}

static dboard_base::sptr make_basic_tx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new basic_tx(args, BASIC_MAX_BANDWIDTH));
}

static dboard_base::sptr make_lf_rx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new basic_rx(args, LF_MAX_BANDWIDTH));
}

static dboard_base::sptr make_lf_tx(dboard_base::ctor_args_t args){
    return dboard_base::sptr(new basic_tx(args, LF_MAX_BANDWIDTH));
}

UHD_STATIC_BLOCK(reg_basic_and_lf_dboards){
    dboard_manager::register_dboard(BASIC_TX_PID, &make_basic_tx, "Basic TX", sd_name_to_conn.keys());
    dboard_manager::register_dboard(BASIC_RX_PID, &make_basic_rx, "Basic RX", sd_name_to_conn.keys());
    dboard_manager::register_dboard(LF_TX_PID,    &make_lf_tx,    "LF TX",    sd_name_to_conn.keys());
    dboard_manager::register_dboard(LF_RX_PID,    &make_lf_rx,    "LF RX",    sd_name_to_conn.keys());
}

/***********************************************************************
 * Basic and LF RX dboard
 **********************************************************************/
basic_rx::basic_rx(ctor_args_t args, double max_freq)
    : rx_dboard_base(args),
    _max_freq(max_freq)
{
    const std::string fe_name(get_subdev_name());
    const std::string fe_conn(sd_name_to_conn[fe_name]);
    const std::string db_name(str(
        boost::format("%s (%s)")
        % ((get_rx_id() == BASIC_RX_PID) ? "BasicRX" : "LFRX")
        % fe_name));
    UHD_LOG_TRACE("BASICRX",
        "Initializing driver for: " << db_name <<
        " IQ connection type: " << fe_conn);
    const bool has_fe_conn_settings =
        get_iface()->has_set_fe_connection(dboard_iface::UNIT_RX);
    UHD_LOG_TRACE("BASICRX",
        "Access to FE connection settings: "
        << (has_fe_conn_settings ? "Yes" : "No"));

    std::vector<std::string> antenna_options = has_fe_conn_settings
        ? sd_name_to_conn.keys()
        : std::vector<std::string>(1, "");

    ////////////////////////////////////////////////////////////////////
    // Register properties
    ////////////////////////////////////////////////////////////////////
    this->get_rx_subtree()->create<std::string>("name").set(db_name);
    this->get_rx_subtree()->create<int>("gains"); //phony property so this dir exists
    this->get_rx_subtree()->create<double>("freq/value")
        .set_publisher([](){ return 0.0; });
    this->get_rx_subtree()->create<meta_range_t>("freq/range")
        .set(freq_range_t(-_max_freq, +_max_freq));
    this->get_rx_subtree()->create<std::string>("antenna/value")
        .set(has_fe_conn_settings ? fe_name : "");
    if (has_fe_conn_settings) {
        this->get_rx_subtree()->access<std::string>("antenna/value")
            .add_coerced_subscriber([this](const std::string& ant){
                this->set_rx_ant(ant);
            })
        ;
    }
    this->get_rx_subtree()->create<std::vector<std::string>>("antenna/options")
        .set(antenna_options);
    this->get_rx_subtree()->create<int>("sensors"); //phony property so this dir exists
    this->get_rx_subtree()->create<std::string>("connection")
        .set(sd_name_to_conn[get_subdev_name()]);
    this->get_rx_subtree()->create<bool>("enabled")
        .set(true); //always enabled
    this->get_rx_subtree()->create<bool>("use_lo_offset")
        .set(false);
    this->get_rx_subtree()->create<double>("bandwidth/value")
        .set(subdev_bandwidth_scalar.at(get_subdev_name())*_max_freq);
    this->get_rx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(
            subdev_bandwidth_scalar.at(get_subdev_name())*_max_freq,
            subdev_bandwidth_scalar.at(get_subdev_name())*_max_freq));

    //disable RX dboard clock by default
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_RX, false);

    //set GPIOs to output 0x0000 to decrease noise pickup
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_RX, 0x0000);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_RX, 0xFFFF);
    this->get_iface()->set_gpio_out(dboard_iface::UNIT_RX, 0x0000);
}

basic_rx::~basic_rx(void){
    /* NOP */
}

void basic_rx::set_rx_ant(const std::string& ant)
{
    UHD_ASSERT_THROW(get_iface()->has_set_fe_connection(dboard_iface::UNIT_RX));
    UHD_LOG_TRACE("BASICRX",
        "Setting antenna value to: " << ant);
    get_iface()->set_fe_connection(
        dboard_iface::UNIT_RX,
        get_subdev_name(),
        usrp::fe_connection_t(sd_name_to_conn[ant], 0.0 /* IF */)
    );
}

/***********************************************************************
 * Basic and LF TX dboard
 **********************************************************************/
basic_tx::basic_tx(ctor_args_t args, double max_freq) : tx_dboard_base(args){
    _max_freq = max_freq;
    //this->get_iface()->set_clock_enabled(dboard_iface::UNIT_TX, true);

    ////////////////////////////////////////////////////////////////////
    // Register properties
    ////////////////////////////////////////////////////////////////////
    if (get_tx_id() == BASIC_TX_PID) {
        this->get_tx_subtree()->create<std::string>("name").set(
            std::string(str(boost::format("BasicTX (%s)") % get_subdev_name()
        )));
    }
    else {
        this->get_tx_subtree()->create<std::string>("name").set(
            std::string(str(boost::format("LFTX (%s)") % get_subdev_name()
        )));
    }

    this->get_tx_subtree()->create<int>("gains"); //phony property so this dir exists
    this->get_tx_subtree()->create<double>("freq/value")
        .set_publisher([](){ return 0.0; });
    this->get_tx_subtree()->create<meta_range_t>("freq/range")
        .set(freq_range_t(-_max_freq, +_max_freq));
    this->get_tx_subtree()->create<std::string>("antenna/value")
        .set("");
    this->get_tx_subtree()->create<std::vector<std::string>>("antenna/options")
        .set({""});
    this->get_tx_subtree()->create<int>("sensors"); //phony property so this dir exists
    this->get_tx_subtree()->create<std::string>("connection")
        .set(sd_name_to_conn[get_subdev_name()]);
    this->get_tx_subtree()->create<bool>("enabled")
        .set(true); //always enabled
    this->get_tx_subtree()->create<bool>("use_lo_offset")
        .set(false);
    this->get_tx_subtree()->create<double>("bandwidth/value")
        .set(subdev_bandwidth_scalar.at(get_subdev_name())*_max_freq);
    this->get_tx_subtree()->create<meta_range_t>("bandwidth/range")
        .set(freq_range_t(
            subdev_bandwidth_scalar.at(get_subdev_name())*_max_freq,
            subdev_bandwidth_scalar.at(get_subdev_name())*_max_freq));

    //disable TX dboard clock by default
    this->get_iface()->set_clock_enabled(dboard_iface::UNIT_TX, false);

    //set GPIOs to output 0x0000 to decrease noise pickup
    this->get_iface()->set_pin_ctrl(dboard_iface::UNIT_TX, 0x0000);
    this->get_iface()->set_gpio_ddr(dboard_iface::UNIT_TX, 0xFFFF);
    this->get_iface()->set_gpio_out(dboard_iface::UNIT_TX, 0x0000);
}

basic_tx::~basic_tx(void){
    /* NOP */
}
