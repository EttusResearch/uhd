//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "n230_impl.hpp"
#include "n230_fw_ctrl_iface.hpp"
#include "n230_defaults.h"
#include "n230_fpga_defs.h"
#include "n230_fw_defs.h"
#include "n230_fw_host_iface.h"

#include <uhdlib/usrp/common/validate_subdev_spec.hpp>

#include <uhd/utils/static.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>

#include <uhd/types/sensors.hpp>
#include <uhd/types/ranges.hpp>
#include <uhd/types/direction.hpp>
#include <uhd/usrp/mboard_eeprom.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/gps_ctrl.hpp>

#include <boost/format.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio.hpp> //used for htonl and ntohl
#include <boost/make_shared.hpp>

#include <chrono>
#include <thread>

namespace uhd { namespace usrp { namespace n230 {

using namespace uhd::transport;
namespace asio = boost::asio;

//----------------------------------------------------------
// Static device registration with framework
//----------------------------------------------------------
UHD_STATIC_BLOCK(register_n230_device)
{
    device::register_device(&n230_impl::n230_find, &n230_impl::n230_make, device::USRP);
}

//----------------------------------------------------------
// Device discovery
//----------------------------------------------------------
uhd::device_addrs_t n230_impl::n230_find(const uhd::device_addr_t &multi_dev_hint)
{
    //handle the multi-device discovery
    device_addrs_t hints = separate_device_addr(multi_dev_hint);
    if (hints.size() > 1){
        device_addrs_t found_devices;
        std::string error_msg;
        for(const device_addr_t &hint_i:  hints){
            device_addrs_t found_devices_i = n230_find(hint_i);
            if (found_devices_i.size() != 1) error_msg += str(boost::format(
                "Could not resolve device hint \"%s\" to a single device."
            ) % hint_i.to_string());
            else found_devices.push_back(found_devices_i[0]);
        }
        if (found_devices.empty()) return device_addrs_t();
        if (not error_msg.empty()) throw uhd::value_error(error_msg);
        return device_addrs_t(1, combine_device_addrs(found_devices));
    }

    //initialize the hint for a single device case
    UHD_ASSERT_THROW(hints.size() <= 1);
    hints.resize(1); //in case it was empty
    device_addr_t hint = hints[0];
    device_addrs_t n230_addrs;

    //return an empty list of addresses when type is set to non-n230
    if (hint.has_key("type") and hint["type"] != "n230") return n230_addrs;

    //Return an empty list of addresses when a resource is specified,
    //since a resource is intended for a different, non-networked, device.
    if (hint.has_key("resource")) return n230_addrs;

    //if no address was specified, send a broadcast on each interface
    if (not hint.has_key("addr")) {
        for(const if_addrs_t &if_addrs:  get_if_addrs()) {
            //avoid the loopback device
            if (if_addrs.inet == asio::ip::address_v4::loopback().to_string()) continue;

            //create a new hint with this broadcast address
            device_addr_t new_hint = hint;
            new_hint["addr"] = if_addrs.bcast;

            //call discover with the new hint and append results
            device_addrs_t new_n230_addrs = n230_find(new_hint);
            n230_addrs.insert(n230_addrs.begin(),
                new_n230_addrs.begin(), new_n230_addrs.end()
            );
        }
        return n230_addrs;
    }

    std::vector<std::string> discovered_addrs =
        n230_fw_ctrl_iface::discover_devices(
            hint["addr"], BOOST_STRINGIZE(N230_FW_COMMS_UDP_PORT), N230_FW_PRODUCT_ID);

    for(const std::string& addr:  discovered_addrs)
    {
        device_addr_t new_addr;
        new_addr["type"] = "n230";
        new_addr["addr"] = addr;

        //Attempt a simple 2-way communication with a connected socket.
        //Reason: Although the USRP will respond the broadcast above,
        //we may not be able to communicate directly (non-broadcast).
        udp_simple::sptr ctrl_xport = udp_simple::make_connected(new_addr["addr"], BOOST_STRINGIZE(N230_FW_COMMS_UDP_PORT));

        //Corner case: If two devices have the same IP but different MAC
        //addresses and are used back-to-back it takes a while for ARP tables
        //on the host to update in which period brodcasts will respond but
        //connected communication can fail. Retry the following call to allow
        //the stack to update
        size_t first_conn_retries = 10;
        n230_fw_ctrl_iface::sptr fw_ctrl;
        while (first_conn_retries > 0) {
            try {
                fw_ctrl = n230_fw_ctrl_iface::make(ctrl_xport, N230_FW_PRODUCT_ID, false /*verbose*/);
                break;
            } catch (uhd::io_error& ex) {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                first_conn_retries--;
            }
        }
        if (first_conn_retries > 0) {
            uint32_t compat_reg = fw_ctrl->peek32(fw::reg_addr(fw::WB_SBRB_BASE, fw::RB_ZPU_COMPAT));
            if (fw::get_prod_num(compat_reg) == fw::PRODUCT_NUM) {
                if (!n230_resource_manager::is_device_claimed(fw_ctrl)) {
                    //Not claimed by another process or host
                    try {
                        //Try to read the EEPROM to get the name and serial
                        n230_eeprom_manager eeprom_mgr(new_addr["addr"]);
                        const mboard_eeprom_t& eeprom = eeprom_mgr.get_mb_eeprom();
                        new_addr["name"] = eeprom["name"];
                        new_addr["serial"] = eeprom["serial"];
                    }
                    catch(const std::exception &)
                    {
                        //set these values as empty string so the device may still be found
                        //and the filter's below can still operate on the discovered device
                        new_addr["name"] = "";
                        new_addr["serial"] = "";
                    }
                    //filter the discovered device below by matching optional keys
                    if ((not hint.has_key("name")    or hint["name"]    == new_addr["name"]) and
                        (not hint.has_key("serial")  or hint["serial"]  == new_addr["serial"]))
                    {
                        n230_addrs.push_back(new_addr);
                    }
                }
            }
        }
    }

    return n230_addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
device::sptr n230_impl::n230_make(const device_addr_t &device_addr)
{
    return device::sptr(new n230_impl(device_addr));
}

/***********************************************************************
 * n230_impl::ctor
 **********************************************************************/
n230_impl::n230_impl(const uhd::device_addr_t& dev_addr)
{
    UHD_LOGGER_INFO("N230") << "N230 initialization sequence...";
    _dev_args.parse(dev_addr);
    _tree = uhd::property_tree::make();

    //TODO: Only supports one motherboard per device class.
    const fs_path mb_path = "/mboards/0";

    //Initialize addresses
    std::vector<std::string> ip_addrs(1, dev_addr["addr"]);
    if (dev_addr.has_key("second_addr")) {
        ip_addrs.push_back(dev_addr["second_addr"]);
    }

    //Read EEPROM and perform version checks before talking to HW
    _eeprom_mgr = boost::make_shared<n230_eeprom_manager>(ip_addrs[0]);
    const mboard_eeprom_t& mb_eeprom = _eeprom_mgr->get_mb_eeprom();
    bool recover_mb_eeprom = dev_addr.has_key("recover_mb_eeprom");
    if (recover_mb_eeprom) {
        UHD_LOGGER_WARNING("N230") << "UHD is operating in EEPROM Recovery Mode which disables hardware version "
                            "checks.\nOperating in this mode may cause hardware damage and unstable "
                            "radio performance!";
    }
    uint16_t hw_rev = boost::lexical_cast<uint16_t>(mb_eeprom["revision"]);
    uint16_t hw_rev_compat = boost::lexical_cast<uint16_t>(mb_eeprom["revision_compat"]);
    if (not recover_mb_eeprom) {
        if (hw_rev_compat > N230_HW_REVISION_COMPAT) {
            throw uhd::runtime_error(str(boost::format(
                "Hardware is too new for this software. Please upgrade to a driver that supports hardware revision %d.")
                % hw_rev));
        }
    }

    //Initialize all subsystems
    _resource_mgr = boost::make_shared<n230_resource_manager>(ip_addrs, _dev_args.get_safe_mode());
    _stream_mgr = boost::make_shared<n230_stream_manager>(_dev_args, _resource_mgr, _tree);

    //Build property tree
    _initialize_property_tree(mb_path);

    //Debug loopback mode
    switch(_dev_args.get_loopback_mode()) {
    case n230_device_args_t::LOOPBACK_RADIO:
        UHD_LOGGER_INFO("N230") << "DEBUG: Running in TX->RX Radio loopback mode.";
        _resource_mgr->get_frontend_ctrl().set_self_test_mode(LOOPBACK_RADIO);
        break;
    case n230_device_args_t::LOOPBACK_CODEC:
        UHD_LOGGER_INFO("N230") << "DEBUG: Running in TX->RX CODEC loopback mode.";
        _resource_mgr->get_frontend_ctrl().set_self_test_mode(LOOPBACK_CODEC);
        break;
    default:
        _resource_mgr->get_frontend_ctrl().set_self_test_mode(LOOPBACK_DISABLED);
        break;
    }
}

/***********************************************************************
 * n230_impl::dtor
 **********************************************************************/
n230_impl::~n230_impl()
{
    _stream_mgr.reset();
    _eeprom_mgr.reset();
    _resource_mgr.reset();
    _tree.reset();
}

/***********************************************************************
 * n230_impl::get_rx_stream
 **********************************************************************/
rx_streamer::sptr n230_impl::get_rx_stream(const uhd::stream_args_t &args)
{
    return _stream_mgr->get_rx_stream(args);
}

/***********************************************************************
 * n230_impl::get_tx_stream
 **********************************************************************/
tx_streamer::sptr n230_impl::get_tx_stream(const uhd::stream_args_t &args)
{
    return _stream_mgr->get_tx_stream(args);
}

/***********************************************************************
 * n230_impl::recv_async_msg
 **********************************************************************/
bool n230_impl::recv_async_msg(uhd::async_metadata_t &async_metadata, double timeout)
{
    return _stream_mgr->recv_async_msg(async_metadata, timeout);
}

/***********************************************************************
 * _initialize_property_tree
 **********************************************************************/
void n230_impl::_initialize_property_tree(const fs_path& mb_path)
{
    //------------------------------------------------------------------
    // General info
    //------------------------------------------------------------------
    _tree->create<std::string>("/name").set("N230 Device");

    _tree->create<std::string>(mb_path / "name").set("N230");
    _tree->create<std::string>(mb_path / "codename").set("N230");
    _tree->create<std::string>(mb_path / "dboards").set("none");    //No dboards.

    _tree->create<std::string>(mb_path / "fw_version").set(str(boost::format("%u.%u")
        % _resource_mgr->get_version(FIRMWARE, COMPAT_MAJOR)
        % _resource_mgr->get_version(FIRMWARE, COMPAT_MINOR)));
    _tree->create<std::string>(mb_path / "fw_version_hash").set(str(boost::format("%s")
        % _resource_mgr->get_version_hash(FIRMWARE)));
    _tree->create<std::string>(mb_path / "fpga_version").set(str(boost::format("%u.%u")
        % _resource_mgr->get_version(FPGA, COMPAT_MAJOR)
        % _resource_mgr->get_version(FPGA, COMPAT_MINOR)));
    _tree->create<std::string>(mb_path / "fpga_version_hash").set(str(boost::format("%s")
        % _resource_mgr->get_version_hash(FPGA)));

    _tree->create<double>(mb_path / "link_max_rate").set(_resource_mgr->get_max_link_rate());

    //------------------------------------------------------------------
    // EEPROM
    //------------------------------------------------------------------
    _tree->create<mboard_eeprom_t>(mb_path / "eeprom")
        .set(_eeprom_mgr->get_mb_eeprom())  //Set first...
        .add_coerced_subscriber(boost::bind(&n230_eeprom_manager::write_mb_eeprom, _eeprom_mgr, _1));  //..then enable writer

    //------------------------------------------------------------------
    // Create codec nodes
    //------------------------------------------------------------------
    const fs_path rx_codec_path = mb_path / ("rx_codecs") / "A";
    _tree->create<std::string>(rx_codec_path / "name")
        .set("N230 RX dual ADC");
    _tree->create<int>(rx_codec_path / "gains");    //Empty because gains are in frontend

    const fs_path tx_codec_path = mb_path / ("tx_codecs") / "A";
    _tree->create<std::string>(tx_codec_path / "name")
        .set("N230 TX dual DAC");
    _tree->create<int>(tx_codec_path / "gains");    //Empty because gains are in frontend

    //------------------------------------------------------------------
    // Create clock and time control nodes
    //------------------------------------------------------------------
    _tree->create<double>(mb_path / "tick_rate")
        .set_coercer(boost::bind(&n230_clk_pps_ctrl::set_tick_rate, _resource_mgr->get_clk_pps_ctrl_sptr(), _1))
        .set_publisher(boost::bind(&n230_clk_pps_ctrl::get_tick_rate, _resource_mgr->get_clk_pps_ctrl_sptr()))
        .add_coerced_subscriber(boost::bind(&n230_stream_manager::update_tick_rate, _stream_mgr, _1));

    //Register time now and pps onto available radio cores
    //radio0 is the master
    _tree->create<time_spec_t>(mb_path / "time" / "cmd");
    _tree->create<time_spec_t>(mb_path / "time" / "now")
        .set_publisher(boost::bind(&time_core_3000::get_time_now, _resource_mgr->get_radio(0).time));
    _tree->create<time_spec_t>(mb_path / "time" / "pps")
        .set_publisher(boost::bind(&time_core_3000::get_time_last_pps, _resource_mgr->get_radio(0).time));

    //Setup time source props
    _tree->create<std::string>(mb_path / "time_source" / "value")
        .add_coerced_subscriber(boost::bind(&n230_impl::_check_time_source, this, _1))
        .add_coerced_subscriber(boost::bind(&n230_clk_pps_ctrl::set_pps_source, _resource_mgr->get_clk_pps_ctrl_sptr(), _1))
        .set(n230::DEFAULT_TIME_SRC);
    const std::vector<std::string> time_sources{"none", "external", "gpsdo"};
    _tree->create<std::vector<std::string> >(mb_path / "time_source" / "options")
        .set(time_sources);

    //Setup reference source props
    _tree->create<std::string>(mb_path / "clock_source" / "value")
        .add_coerced_subscriber(boost::bind(&n230_impl::_check_clock_source, this, _1))
        .add_coerced_subscriber(boost::bind(&n230_clk_pps_ctrl::set_clock_source, _resource_mgr->get_clk_pps_ctrl_sptr(), _1))
        .set(n230::DEFAULT_CLOCK_SRC);
    const std::vector<std::string> clock_sources{"internal", "external", "gpsdo"};
    _tree->create<std::vector<std::string> >(mb_path / "clock_source" / "options")
        .set(clock_sources);
    _tree->create<sensor_value_t>(mb_path / "sensors" / "ref_locked")
        .set_publisher(boost::bind(&n230_clk_pps_ctrl::get_ref_locked, _resource_mgr->get_clk_pps_ctrl_sptr()));

    //------------------------------------------------------------------
    // Create frontend mapping
    //------------------------------------------------------------------
    _tree->create<subdev_spec_t>(mb_path / "rx_subdev_spec")
        .set(subdev_spec_t())
        .add_coerced_subscriber(boost::bind(&n230_impl::_update_rx_subdev_spec, this, _1));
    _tree->create<subdev_spec_t>(mb_path / "tx_subdev_spec")
        .set(subdev_spec_t())
        .add_coerced_subscriber(boost::bind(&n230_impl::_update_tx_subdev_spec, this, _1));

    //------------------------------------------------------------------
    // Create a fake dboard to put frontends in
    //------------------------------------------------------------------
    //For completeness we give it a fake EEPROM as well
    dboard_eeprom_t db_eeprom;  //Default state: ID is 0xffff, Version and serial empty
    _tree->create<dboard_eeprom_t>(mb_path / "dboards" / "A" / "rx_eeprom").set(db_eeprom);
    _tree->create<dboard_eeprom_t>(mb_path / "dboards" / "A" / "tx_eeprom").set(db_eeprom);
    _tree->create<dboard_eeprom_t>(mb_path / "dboards" / "A" / "gdb_eeprom").set(db_eeprom);

    //------------------------------------------------------------------
    // Create radio specific nodes
    //------------------------------------------------------------------
    for (size_t radio_instance = 0; radio_instance < fpga::NUM_RADIOS; radio_instance++) {
        _initialize_radio_properties(mb_path, radio_instance);
    }
    //Update tick rate on newly created radio objects
    _tree->access<double>(mb_path / "tick_rate").set(_dev_args.get_master_clock_rate());

    //------------------------------------------------------------------
    // Initialize subdev specs
    //------------------------------------------------------------------
    subdev_spec_t rx_spec, tx_spec;
    for(const std::string &fe:  _tree->list(mb_path / "dboards" / "A" / "rx_frontends"))
    {
        rx_spec.push_back(subdev_spec_pair_t("A", fe));
    }
    for(const std::string &fe:  _tree->list(mb_path / "dboards" / "A" / "tx_frontends"))
    {
        tx_spec.push_back(subdev_spec_pair_t("A", fe));
    }
    _tree->access<subdev_spec_t>(mb_path / "rx_subdev_spec").set(rx_spec);
    _tree->access<subdev_spec_t>(mb_path / "tx_subdev_spec").set(tx_spec);

    //------------------------------------------------------------------
    // MiniSAS GPIO
    //------------------------------------------------------------------
    for(const usrp::gpio_atr::gpio_attr_map_t::value_type attr:  usrp::gpio_atr::gpio_attr_map) {
    switch (attr.first){
                case usrp::gpio_atr::GPIO_SRC:
                case usrp::gpio_atr::GPIO_CTRL:
                    _tree->create<std::vector<std::string>>(mb_path / "gpio" / "FP0" /  attr.second)
                         .set(std::vector<std::string>(32, usrp::gpio_atr::default_attr_value_map.at(attr.first)));
                    _tree->create<std::vector<std::string>>(mb_path / "gpio" / "FP1" /  attr.second)
                         .set(std::vector<std::string>(32, usrp::gpio_atr::default_attr_value_map.at(attr.first)));
                    break;
                case usrp::gpio_atr::GPIO_DDR:
                    _tree->create<std::vector<std::string>>(mb_path / "gpio" / "FP0" /  attr.second)
                         .set(std::vector<std::string>(32, usrp::gpio_atr::default_attr_value_map.at(attr.first)))
                         .add_coerced_subscriber([this, attr](const std::vector<std::string> str_val){
                            uint32_t val = 0;
                            for(size_t i = 0 ; i < str_val.size() ; i++){
                                val += usrp::gpio_atr::gpio_attr_value_pair.at(attr.second).at(str_val[i])<<i;
                            }
                            _resource_mgr->get_minisas_gpio_ctrl_sptr(0)->set_gpio_attr(attr.first, val);
                         });
                    _tree->create<std::vector<std::string>>(mb_path / "gpio" / "FP1" /  attr.second)
                         .set(std::vector<std::string>(32, usrp::gpio_atr::default_attr_value_map.at(attr.first)))
                         .add_coerced_subscriber([this, attr](const std::vector<std::string> str_val){
                            uint32_t val = 0;
                            for(size_t i = 0 ; i < str_val.size() ; i++){
                                val += usrp::gpio_atr::gpio_attr_value_pair.at(attr.second).at(str_val[i])<<i;
                            }
                            _resource_mgr->get_minisas_gpio_ctrl_sptr(1)->set_gpio_attr(attr.first, val);
                         });
                    break;
                case usrp::gpio_atr::GPIO_OUT:
                    _tree->create<uint32_t>(mb_path / "gpio" / "FP0" /  attr.second)
                         .set(0)
                         .add_coerced_subscriber([this, attr](const uint32_t val){
                             _resource_mgr->get_minisas_gpio_ctrl_sptr(0)->set_gpio_attr(attr.first, val);
                         });
                     _tree->create<uint32_t>(mb_path / "gpio" / "FP1" /  attr.second)
                         .set(0)
                         .add_coerced_subscriber([this, attr](const uint32_t val){
                             _resource_mgr->get_minisas_gpio_ctrl_sptr(1)->set_gpio_attr(attr.first, val);
                         });
                         break;
                case usrp::gpio_atr::GPIO_READBACK:
                    _tree->create<uint32_t>(mb_path / "gpio" / "FP0" / "READBACK")
                        .set_publisher(boost::bind(&gpio_atr::gpio_atr_3000::read_gpio, _resource_mgr->get_minisas_gpio_ctrl_sptr(0)));
                    _tree->create<uint32_t>(mb_path / "gpio" / "FP1" / "READBACK")
                        .set_publisher(boost::bind(&gpio_atr::gpio_atr_3000::read_gpio, _resource_mgr->get_minisas_gpio_ctrl_sptr(1)));
                    break;
                default:
                    _tree->create<uint32_t>(mb_path / "gpio" / "FP0" /  attr.second)
                         .set(0);
                    _tree->create<uint32_t>(mb_path / "gpio" / "FP1" /  attr.second)
                         .set(0);
                    break;
            }
    }
    //------------------------------------------------------------------
    // GPSDO sensors
    //------------------------------------------------------------------
    if (_resource_mgr->is_gpsdo_present()) {
        uhd::gps_ctrl::sptr gps_ctrl = _resource_mgr->get_gps_ctrl();
        for(const std::string &name:  gps_ctrl->get_sensors())
        {
            _tree->create<sensor_value_t>(mb_path / "sensors" / name)
                .set_publisher(boost::bind(&gps_ctrl::get_sensor, gps_ctrl, name));
        }
    }
}

/***********************************************************************
 * _initialize_radio_properties
 **********************************************************************/
void n230_impl::_initialize_radio_properties(const fs_path& mb_path, size_t instance)
{
    radio_resource_t& perif = _resource_mgr->get_radio(instance);

    //Time
    _tree->access<time_spec_t>(mb_path / "time" / "cmd")
        .add_coerced_subscriber(boost::bind(&radio_ctrl_core_3000::set_time, perif.ctrl, _1));
    _tree->access<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&radio_ctrl_core_3000::set_tick_rate, perif.ctrl, _1));
    _tree->access<time_spec_t>(mb_path / "time" / "now")
        .add_coerced_subscriber(boost::bind(&time_core_3000::set_time_now, perif.time, _1));
    _tree->access<time_spec_t>(mb_path / "time" / "pps")
        .add_coerced_subscriber(boost::bind(&time_core_3000::set_time_next_pps, perif.time, _1));

    //RX DSP
    _tree->access<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&rx_vita_core_3000::set_tick_rate, perif.framer, _1))
        .add_coerced_subscriber(boost::bind(&rx_dsp_core_3000::set_tick_rate, perif.ddc, _1));
    const fs_path rx_dsp_path = mb_path / "rx_dsps" / str(boost::format("%u") % instance);
    _tree->create<meta_range_t>(rx_dsp_path / "rate" / "range")
        .set_publisher(boost::bind(&rx_dsp_core_3000::get_host_rates, perif.ddc));
    _tree->create<double>(rx_dsp_path / "rate" / "value")
        .set_coercer(boost::bind(&rx_dsp_core_3000::set_host_rate, perif.ddc, _1))
        .add_coerced_subscriber(boost::bind(&n230_stream_manager::update_rx_samp_rate, _stream_mgr, instance, _1))
        .set(n230::DEFAULT_RX_SAMP_RATE);
    _tree->create<double>(rx_dsp_path / "freq" / "value")
        .set_coercer(boost::bind(&rx_dsp_core_3000::set_freq, perif.ddc, _1))
        .set(n230::DEFAULT_DDC_FREQ);
    _tree->create<meta_range_t>(rx_dsp_path / "freq" / "range")
        .set_publisher(boost::bind(&rx_dsp_core_3000::get_freq_range, perif.ddc));
    _tree->create<stream_cmd_t>(rx_dsp_path / "stream_cmd")
        .add_coerced_subscriber(boost::bind(&rx_vita_core_3000::issue_stream_command, perif.framer, _1));

    //TX DSP
    _tree->access<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&tx_dsp_core_3000::set_tick_rate, perif.duc, _1));
    const fs_path tx_dsp_path = mb_path / "tx_dsps" / str(boost::format("%u") % instance);
    _tree->create<meta_range_t>(tx_dsp_path / "rate" / "range")
        .set_publisher(boost::bind(&tx_dsp_core_3000::get_host_rates, perif.duc));
    _tree->create<double>(tx_dsp_path / "rate" / "value")
        .set_coercer(boost::bind(&tx_dsp_core_3000::set_host_rate, perif.duc, _1))
        .add_coerced_subscriber(boost::bind(&n230_stream_manager::update_tx_samp_rate, _stream_mgr, instance, _1))
        .set(n230::DEFAULT_TX_SAMP_RATE);
    _tree->create<double>(tx_dsp_path / "freq" / "value")
        .set_coercer(boost::bind(&tx_dsp_core_3000::set_freq, perif.duc, _1))
        .set(n230::DEFAULT_DUC_FREQ);
    _tree->create<meta_range_t>(tx_dsp_path / "freq" / "range")
        .set_publisher(boost::bind(&tx_dsp_core_3000::get_freq_range, perif.duc));

    //RF Frontend Interfacing
    for (direction_t direction : std::vector<direction_t>{RX_DIRECTION, TX_DIRECTION}) {
        const std::string dir_str = (direction == RX_DIRECTION) ? "rx" : "tx";
        const std::string key = boost::to_upper_copy(dir_str) + str(boost::format("%u") % (instance + 1));
        const fs_path rf_fe_path = mb_path / "dboards" / "A" / (dir_str + "_frontends") / ((instance==0)?"A":"B");

        //CODEC subtree
        _resource_mgr->get_codec_mgr().populate_frontend_subtree(_tree->subtree(rf_fe_path), key, direction);

        //User settings
        _tree->create<uhd::wb_iface::sptr>(rf_fe_path / "user_settings" / "iface")
            .set(perif.user_settings);

        //Setup antenna stuff
        if (key[0] == 'R') {
            _tree->create<std::vector<std::string> >(rf_fe_path / "antenna" / "options")
                .set({"TX/RX", "RX2"});
            _tree->create<std::string>(rf_fe_path / "antenna" / "value")
                .add_coerced_subscriber(boost::bind(&n230_frontend_ctrl::set_antenna_sel, _resource_mgr->get_frontend_ctrl_sptr(), instance, _1))
                .set("RX2");
        }
        if (key[0] == 'T') {
            _tree->create<std::vector<std::string> >(rf_fe_path / "antenna" / "options")
                .set({"TX/RX"});
            _tree->create<std::string>(rf_fe_path / "antenna" / "value")
                .set("TX/RX");
        }
    }
}

void n230_impl::_update_rx_subdev_spec(const uhd::usrp::subdev_spec_t &spec)
{
    //sanity checking
    if (spec.size()) validate_subdev_spec(_tree, spec, "rx");
    UHD_ASSERT_THROW(spec.size() <= fpga::NUM_RADIOS);

    if (spec.size() > 0) {
        UHD_ASSERT_THROW(spec[0].db_name == "A");
        UHD_ASSERT_THROW(spec[0].sd_name == "A");
    }
    if (spec.size() > 1) {
        //TODO we can support swapping at a later date, only this combo is supported
        UHD_ASSERT_THROW(spec[1].db_name == "A");
        UHD_ASSERT_THROW(spec[1].sd_name == "B");
    }

    _stream_mgr->update_stream_states();
}

void n230_impl::_update_tx_subdev_spec(const uhd::usrp::subdev_spec_t &spec)
{
    //sanity checking
    if (spec.size()) validate_subdev_spec(_tree, spec, "tx");
    UHD_ASSERT_THROW(spec.size() <= fpga::NUM_RADIOS);

    if (spec.size() > 0) {
        UHD_ASSERT_THROW(spec[0].db_name == "A");
        UHD_ASSERT_THROW(spec[0].sd_name == "A");
    }
    if (spec.size() > 1) {
        //TODO we can support swapping at a later date, only this combo is supported
        UHD_ASSERT_THROW(spec[1].db_name == "A");
        UHD_ASSERT_THROW(spec[1].sd_name == "B");
    }

    _stream_mgr->update_stream_states();
}

void n230_impl::_check_time_source(std::string source)
{
    if (source == "gpsdo")
    {
        uhd::gps_ctrl::sptr gps_ctrl = _resource_mgr->get_gps_ctrl();
        if (not (gps_ctrl and gps_ctrl->gps_detected()))
            throw uhd::runtime_error("GPSDO time source not available");
    }
}

void n230_impl::_check_clock_source(std::string source)
{
    if (source == "gpsdo")
    {
        uhd::gps_ctrl::sptr gps_ctrl = _resource_mgr->get_gps_ctrl();
        if (not (gps_ctrl and gps_ctrl->gps_detected()))
            throw uhd::runtime_error("GPSDO clock source not available");
    }
}

}}} //namespace
