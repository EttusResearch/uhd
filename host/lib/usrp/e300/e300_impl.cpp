//
// Copyright 2013-2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "e300_impl.hpp"
#include "e300_defaults.hpp"
#include "e300_fpga_defs.hpp"
#include "e300_spi.hpp"
#include "e300_regs.hpp"
#include "e300_eeprom_manager.hpp"
#include "e300_sensor_manager.hpp"
#include "e300_common.hpp"
#include "e300_remote_codec_ctrl.hpp"


#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_zero_copy.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/sensors.hpp>
#include <boost/make_shared.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/functional/hash.hpp>
#include <boost/bind.hpp>
#include <boost/make_shared.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/asio.hpp>
#include <fstream>
#include <chrono>
#include <thread>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::usrp::gpio_atr;
using namespace uhd::transport;
namespace fs = boost::filesystem;
namespace asio = boost::asio;

//! mapping of frontend to radio perif index
static const size_t FE0 = 1;
static const size_t FE1 = 0;

namespace uhd { namespace usrp { namespace e300 {

/***********************************************************************
 * Discovery
 **********************************************************************/

static std::vector<std::string> discover_ip_addrs(
    const std::string& addr_hint, const std::string& port)
{
    std::vector<std::string> addrs;

    // Create a UDP transport to communicate:
    // Some devices will cause a throw when opened for a broadcast address.
    // We print and recover so the caller can loop through all bcast addrs.
    uhd::transport::udp_simple::sptr udp_bcast_xport;
    try {
        udp_bcast_xport = uhd::transport::udp_simple::make_broadcast(addr_hint, port);
    } catch(const std::exception &e) {
        UHD_LOGGER_ERROR("E300") << boost::format("Cannot open UDP transport on %s for discovery%s")
        % addr_hint % e.what() ;
        return addrs;
    } catch(...) {
        UHD_LOGGER_ERROR("E300") << "E300 Network discovery unknown error";
        return addrs;
    }

    // TODO: Do not abuse the I2C transport here ...
    // we send a read request to i2c address 0x51,
    // to read register 0
    i2c_transaction_t req;
    req.type = i2c::READ | i2c::ONEBYTE;
    req.addr = 0x51; // mboard's eeprom address, we don't really care
    req.reg = 4;

    // send dummy request
    try {
    udp_bcast_xport->send(boost::asio::buffer(&req, sizeof(req)));
    } catch (const std::exception &ex) {
        UHD_LOGGER_ERROR("E300") << "E300 Network discovery error " << ex.what();
        return addrs;
    } catch(...) {
        UHD_LOGGER_ERROR("E300") << "E300 Network discovery unknown error";
        return addrs;
    }

    // loop for replies until timeout
    while (true) {
        uint8_t buff[sizeof(i2c_transaction_t)] = {};
        const size_t nbytes = udp_bcast_xport->recv(boost::asio::buffer(buff), 0.050);
        if (nbytes == 0)
            break; //No more responses

        const i2c_transaction_t *reply = reinterpret_cast<const i2c_transaction_t*>(buff);
        if (req.addr == reply->addr)
           addrs.push_back(udp_bcast_xport->get_recv_addr());
    }

    return addrs;
}

static bool is_loopback(const if_addrs_t &if_addrs)
{
       return if_addrs.inet == asio::ip::address_v4::loopback().to_string();
}

device_addrs_t e300_find(const device_addr_t &multi_dev_hint)
{
    // handle multi device discovery
    device_addrs_t hints = separate_device_addr(multi_dev_hint);

    if (hints.size() > 1) {
        device_addrs_t found_devices;
        std::string err_msg;
        for(const device_addr_t &hint_i:  hints)
        {
            device_addrs_t found_devices_i = e300_find(hint_i);
            if(found_devices_i.size() != 1)
                err_msg += str(boost::format(
                    "Could not resolve device hint \"%s\" to a single device.")
                    % hint_i.to_string());
            else
                found_devices.push_back(found_devices_i[0]);
            if (found_devices.empty())
                return device_addrs_t();

            if (not err_msg.empty())
                throw uhd::value_error(err_msg);
        }
        return device_addrs_t(1, combine_device_addrs(found_devices));
    }

    // initialize the hint for a single device case
    UHD_ASSERT_THROW(hints.size() <= 1);
    hints.resize(1); // in case it was empty
    device_addr_t hint = hints[0];
    device_addrs_t e300_addrs;

    // return an empty list of addresses when type is set to non-e300
    if (hint.has_key("type") and hint["type"] != "e3x0")
        return e300_addrs;

    const bool loopback_only =
        get_if_addrs().size() == 1 and is_loopback(get_if_addrs().at(0));

    // if we don't have connectivity, we might as well skip the network part
    if (not loopback_only) {
        // if no address or node has been specified, send a broadcast
        if ((not hint.has_key("addr")) and (not hint.has_key("node"))) {
            for(const if_addrs_t &if_addrs:  get_if_addrs())
            {
                // avoid the loopback device
                if (is_loopback(if_addrs))
                    continue;

                // create a new hint with this broadcast address
                device_addr_t new_hint = hint;
                new_hint["addr"] = if_addrs.bcast;

                // call discover with the new hint and append results
                device_addrs_t new_e300_addrs = e300_find(new_hint);
                e300_addrs.insert(e300_addrs.begin(),
                    new_e300_addrs.begin(), new_e300_addrs.end());

            }
            return e300_addrs;
        }

        std::vector<std::string> ip_addrs = discover_ip_addrs(
            hint["addr"], E300_SERVER_I2C_PORT);

        for(const std::string &ip_addr:  ip_addrs)
        {
            device_addr_t new_addr;
            new_addr["type"] = "e3x0";
            new_addr["addr"] = ip_addr;

            // see if we can read the eeprom
            try {
                e300_eeprom_manager eeprom_manager(
                    i2c::make_simple_udp(new_addr["addr"], E300_SERVER_I2C_PORT));
                const mboard_eeprom_t eeprom = eeprom_manager.get_mb_eeprom();
                new_addr["name"] = eeprom["name"];
                new_addr["serial"] = eeprom["serial"];
                new_addr["product"] = eeprom_manager.get_mb_type_string();
            } catch (...) {
                // set these values as empty string, so the device may still be found
                // and the filters below can still operate on the discovered device
                new_addr["name"] = "";
                new_addr["serial"] = "";
            }
            // filter the discovered device below by matching optional keys
            if ((not hint.has_key("name")   or hint["name"]   == new_addr["name"]) and
                (not hint.has_key("serial") or hint["serial"] == new_addr["serial"]))
            {
                e300_addrs.push_back(new_addr);
            }
        }
    }

    // finally search locally
    // if device node is not provided,
    // use the default one
    if (not hint.has_key("node")) {
        device_addr_t new_addr = hint;
        new_addr["node"] = "/dev/axi_fpga";
        return e300_find(new_addr);
    }

    // use the given node
    if (fs::exists(hint["node"])) {
        device_addr_t new_addr;
        new_addr["type"] = "e3x0";
        new_addr["node"] = fs::system_complete(fs::path(hint["node"])).string();

        try {
            e300_eeprom_manager eeprom_manager(i2c::make_i2cdev(E300_I2CDEV_DEVICE));
            const mboard_eeprom_t eeprom = eeprom_manager.get_mb_eeprom();
            new_addr["name"] = eeprom["name"];
            new_addr["serial"] = eeprom["serial"];
            new_addr["product"] = eeprom_manager.get_mb_type_string();
        } catch (...) {
            // set these values as empty string, so the device may still be found
            // and the filters below can still operate on the discovered device
            new_addr["name"] = "";
            new_addr["serial"] = "";
        }
        // filter the discovered device below by matching optional keys
        if ((not hint.has_key("name")   or hint["name"]   == new_addr["name"]) and
            (not hint.has_key("serial") or hint["serial"] == new_addr["serial"]))
        {
            e300_addrs.push_back(new_addr);
        }
    }

    return e300_addrs;
}


/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr e300_make(const device_addr_t &device_addr)
{
    UHD_LOGGER_DEBUG("E300")<< "e300_make with args " << device_addr.to_pp_string() ;
    if(device_addr.has_key("server"))
        throw uhd::runtime_error(
            str(boost::format("Please run the server executable \"%s\"")
                % "usrp_e3x0_network_mode"));
    else
        return device::sptr(new e300_impl(device_addr));
}

// Common code used by e300_impl and e300_image_loader
void get_e3x0_fpga_images(const uhd::device_addr_t &device_addr,
                          std::string &fpga_image,
                          std::string &idle_image){
    const uint16_t pid = boost::lexical_cast<uint16_t>(
            device_addr["product"]);

    //extract the FPGA path for the e300
    switch(e300_eeprom_manager::get_mb_type(pid)) {
    case e300_eeprom_manager::USRP_E310_SG1_MB:
        fpga_image = device_addr.cast<std::string>("fpga",
            find_image_path(E310_SG1_FPGA_FILE_NAME));
        idle_image = find_image_path(E3XX_SG1_FPGA_IDLE_FILE_NAME);
        break;
    case e300_eeprom_manager::USRP_E310_SG3_MB:
        fpga_image = device_addr.cast<std::string>("fpga",
            find_image_path(E310_SG3_FPGA_FILE_NAME));
        idle_image = find_image_path(E3XX_SG3_FPGA_IDLE_FILE_NAME);
        break;
    case e300_eeprom_manager::USRP_E300_MB:
        fpga_image = device_addr.cast<std::string>("fpga",
            find_image_path(E300_FPGA_FILE_NAME));
        idle_image = find_image_path(E3XX_SG1_FPGA_IDLE_FILE_NAME);
        break;
    case e300_eeprom_manager::UNKNOWN:
    default:
        UHD_LOGGER_WARNING("E300") << "Unknown motherboard type, loading e300 image."
                             ;
        fpga_image = device_addr.cast<std::string>("fpga",
            find_image_path(E300_FPGA_FILE_NAME));
        idle_image = find_image_path(E3XX_SG1_FPGA_IDLE_FILE_NAME);
        break;
    }
}

/***********************************************************************
 * Structors
 **********************************************************************/
e300_impl::e300_impl(const uhd::device_addr_t &device_addr)
    : _device_addr(device_addr)
    , _xport_path(device_addr.has_key("addr") ? ETH : AXI)
    , _sid_framer(0)
{
    _type = uhd::device::USRP;

    _async_md.reset(new async_md_type(1000/*messages deep*/));

    ////////////////////////////////////////////////////////////////////
    // load the fpga image
    ////////////////////////////////////////////////////////////////////
    if (_xport_path == AXI) {
        _do_not_reload = device_addr.has_key("no_reload_fpga");
        if (not _do_not_reload) {
            std::string fpga_image;

            // need to re-read product ID code because of conversion into string in find function
            e300_eeprom_manager eeprom_manager(i2c::make_i2cdev(E300_I2CDEV_DEVICE));
            const mboard_eeprom_t eeprom = eeprom_manager.get_mb_eeprom();
            device_addr_t device_addr_cp;
            device_addr_cp["product"] = eeprom["product"];

            get_e3x0_fpga_images(device_addr_cp,
                                 fpga_image,
                                 _idle_image);
            common::load_fpga_image(fpga_image);
        }
    }

    ////////////////////////////////////////////////////////////////////
    // setup fifo xports
    ////////////////////////////////////////////////////////////////////
    _ctrl_xport_params.recv_frame_size = e300::DEFAULT_CTRL_FRAME_SIZE;
    _ctrl_xport_params.num_recv_frames = e300::DEFAULT_CTRL_NUM_FRAMES;
    _ctrl_xport_params.send_frame_size = e300::DEFAULT_CTRL_FRAME_SIZE;
    _ctrl_xport_params.num_send_frames = e300::DEFAULT_CTRL_NUM_FRAMES;

    _data_xport_params.recv_frame_size = device_addr.cast<size_t>("recv_frame_size",
        e300::DEFAULT_RX_DATA_FRAME_SIZE);
    _data_xport_params.num_recv_frames = device_addr.cast<size_t>("num_recv_frames",
	e300::DEFAULT_RX_DATA_NUM_FRAMES);
    _data_xport_params.send_frame_size = device_addr.cast<size_t>("send_frame_size",
        e300::DEFAULT_TX_DATA_FRAME_SIZE);
    _data_xport_params.num_send_frames = device_addr.cast<size_t>("num_send_frames",
	e300::DEFAULT_TX_DATA_NUM_FRAMES);


    // until we figure out why this goes wrong we'll keep this hack around for
    // the ethernet case, in the AXI case we cannot go above one page
    if (_xport_path == ETH) {
        _data_xport_params.recv_frame_size =
            std::min(e300::MAX_NET_RX_DATA_FRAME_SIZE, _data_xport_params.recv_frame_size);
        _data_xport_params.send_frame_size =
            std::min(e300::MAX_NET_TX_DATA_FRAME_SIZE, _data_xport_params.send_frame_size);
    } else {
        _data_xport_params.recv_frame_size =
            std::min(e300::MAX_AXI_RX_DATA_FRAME_SIZE, _data_xport_params.recv_frame_size);
        _data_xport_params.send_frame_size =
            std::min(e300::MAX_AXI_TX_DATA_FRAME_SIZE, _data_xport_params.send_frame_size);
    }
    udp_zero_copy::buff_params dummy_buff_params_out;

    if (_xport_path == ETH) {
        zero_copy_if::sptr codec_xport =
            udp_zero_copy::make(device_addr["addr"], E300_SERVER_CODEC_PORT, _ctrl_xport_params, dummy_buff_params_out, device_addr);
        _codec_ctrl = e300_remote_codec_ctrl::make(codec_xport);
        zero_copy_if::sptr gregs_xport =
            udp_zero_copy::make(device_addr["addr"], E300_SERVER_GREGS_PORT, _ctrl_xport_params, dummy_buff_params_out, device_addr);
        _global_regs = global_regs::make(gregs_xport);

        zero_copy_if::sptr i2c_xport;
        i2c_xport = udp_zero_copy::make(device_addr["addr"], E300_SERVER_I2C_PORT, _ctrl_xport_params, dummy_buff_params_out, device_addr);
        _eeprom_manager = boost::make_shared<e300_eeprom_manager>(i2c::make_zc(i2c_xport));

        uhd::transport::zero_copy_xport_params sensor_xport_params;
        sensor_xport_params.recv_frame_size = 128;
        sensor_xport_params.num_recv_frames = 10;
        sensor_xport_params.send_frame_size = 128;
        sensor_xport_params.num_send_frames = 10;

        zero_copy_if::sptr sensors_xport;
        sensors_xport = udp_zero_copy::make(device_addr["addr"], E300_SERVER_SENSOR_PORT, sensor_xport_params, dummy_buff_params_out, device_addr);
        _sensor_manager = e300_sensor_manager::make_proxy(sensors_xport);

    } else {
        e300_fifo_config_t fifo_cfg;
        try {
            fifo_cfg = e300_read_sysfs();
        } catch (...) {
            throw uhd::runtime_error("Failed to get driver parameters from sysfs.");
        }
        _fifo_iface = e300_fifo_interface::make(fifo_cfg);
        _global_regs = global_regs::make(_fifo_iface->get_global_regs_base());

        ad9361_params::sptr client_settings = boost::make_shared<e300_ad9361_client_t>();
        _codec_ctrl = ad9361_ctrl::make_spi(client_settings, spi::make(E300_SPIDEV_DEVICE), 1);
        // This is horrible ... why do I have to sleep here?
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        _eeprom_manager = boost::make_shared<e300_eeprom_manager>(i2c::make_i2cdev(E300_I2CDEV_DEVICE));
        _sensor_manager = e300_sensor_manager::make_local(_global_regs);
    }
    _codec_mgr = ad936x_manager::make(_codec_ctrl, fpga::NUM_RADIOS);

#ifdef E300_GPSD
    UHD_LOGGER_INFO("E300") << "Detecting internal GPS ";
    try {
        if (_xport_path == AXI)
            _gps = gpsd_iface::make("localhost", 2947);
        else
            _gps = gpsd_iface::make(device_addr["addr"], 2947);
    } catch (std::exception &e) {
        UHD_LOGGER_ERROR("E300") << "An error occured making GPSDd interface: " << e.what();
    }

    if (_gps) {
        for (size_t i = 0; i < _GPS_TIMEOUT; i++)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            if (!_gps->gps_detected())
                std::cout << "." << std::flush;
            else {
                std::cout << ".... " << std::flush;
                break;
            }
        }
        UHD_LOGGER_INFO("E300") << "GPSDO " << (_gps->gps_detected() ? "found" : "not found");
    }
#endif

    // Verify we can talk to the e300 core control registers ...
    UHD_LOGGER_INFO("E300") << "Initializing core control...";
    this->_register_loopback_self_test(_global_regs);

    // Verify fpga compatibility version matches at least for the major
    if (_get_version(FPGA_MAJOR) != fpga::COMPAT_MAJOR) {
        throw uhd::runtime_error(str(boost::format(
            "Expected FPGA compatibility number %lu.x, but got %lu.%lu:\n"
            "The FPGA build is not compatible with the host code build.\n"
            "%s"
        ) % fpga::COMPAT_MAJOR
          % _get_version(FPGA_MAJOR) % _get_version(FPGA_MINOR)
          % print_utility_error("uhd_images_downloader.py")));
    }

    ////////////////////////////////////////////////////////////////////
    // Initialize the properties tree
    ////////////////////////////////////////////////////////////////////
    _tree = property_tree::make();
    _tree->create<std::string>("/name").set("E-Series Device");
    const fs_path mb_path = "/mboards/0";
    _tree->create<std::string>(mb_path / "name")
        .set(_eeprom_manager->get_mb_type_string());

    _tree->create<std::string>(mb_path / "codename").set("Troll");

    _tree->create<std::string>(mb_path / "fpga_version").set(
        str(boost::format("%u.%u")
            % _get_version(FPGA_MAJOR)
            % _get_version(FPGA_MINOR)));

    _tree->create<std::string>(mb_path / "fpga_version_hash").set(
        _get_version_hash());

    ////////////////////////////////////////////////////////////////////
    // and do the misc mboard sensors
    ////////////////////////////////////////////////////////////////////
    _tree->create<int>(mb_path / "sensors");
    for(const std::string &name:  _sensor_manager->get_sensors())
    {
        _tree->create<sensor_value_t>(mb_path / "sensors" / name)
            .set_publisher(boost::bind(&e300_sensor_manager::get_sensor, _sensor_manager, name));
    }
#ifdef E300_GPSD
    if (_gps) {
        for(const std::string &name:  _gps->get_sensors())
        {
            _tree->create<sensor_value_t>(mb_path / "sensors" / name)
                .set_publisher(boost::bind(&gpsd_iface::get_sensor, _gps, name));
        }
    }
#endif

    ////////////////////////////////////////////////////////////////////
    // setup the mboard eeprom
    ////////////////////////////////////////////////////////////////////
    _tree->create<mboard_eeprom_t>(mb_path / "eeprom")
        .set(_eeprom_manager->get_mb_eeprom())  // set first...
        .add_coerced_subscriber(boost::bind(
            &e300_eeprom_manager::write_mb_eeprom,
            _eeprom_manager, _1));

    ////////////////////////////////////////////////////////////////////
    // clocking
    ////////////////////////////////////////////////////////////////////
    _tree->create<double>(mb_path / "tick_rate")
        .set_coercer(boost::bind(&e300_impl::_set_tick_rate, this, _1))
        .set_publisher(boost::bind(&e300_impl::_get_tick_rate, this))
        .add_coerced_subscriber(boost::bind(&e300_impl::_update_tick_rate, this, _1));

    //default some chains on -- needed for setup purposes
    _codec_ctrl->set_active_chains(true, false, true, false);
    _codec_ctrl->set_clock_rate(50e6);

    ////////////////////////////////////////////////////////////////////
    // setup radios
    ////////////////////////////////////////////////////////////////////
    for(size_t instance = 0; instance < fpga::NUM_RADIOS; instance++)
        this->_setup_radio(instance);

    //now test each radio module's connection to the codec interface
    for (radio_perifs_t &perif : _radio_perifs) {
        _codec_mgr->loopback_self_test(
            [&perif](const uint32_t value){
                perif.ctrl->poke32(radio::sr_addr(radio::CODEC_IDLE), value);
            },
            [&perif](){
                return perif.ctrl->peek64(radio::RB64_CODEC_READBACK);
            }
        );
    }
    ////////////////////////////////////////////////////////////////////
    // internal gpios
    ////////////////////////////////////////////////////////////////////
    gpio_atr_3000::sptr fp_gpio = gpio_atr_3000::make(_radio_perifs[0].ctrl, radio::sr_addr(radio::FP_GPIO), radio::RB32_FP_GPIO);
    for(const auto& attr:  gpio_attr_map){
        switch (attr.first){
                case usrp::gpio_atr::GPIO_SRC:
                    _tree->create<std::vector<std::string>>(mb_path / "gpio" / "INT0" / attr.second)
                         .set(std::vector<std::string>(32, usrp::gpio_atr::default_attr_value_map.at(attr.first)))
                         .add_coerced_subscriber([this](const std::vector<std::string>&){
                            throw uhd::runtime_error("This device does not support setting the GPIO_SRC attribute.");
                         });
                    break;
                case usrp::gpio_atr::GPIO_CTRL:
                case usrp::gpio_atr::GPIO_DDR:
                    _tree->create<std::vector<std::string>>(mb_path / "gpio" / "INT0" / attr.second)
                         .set(std::vector<std::string>(32, usrp::gpio_atr::default_attr_value_map.at(attr.first)))
                         .add_coerced_subscriber([this, fp_gpio, attr](const std::vector<std::string> str_val){
                            uint32_t val = 0;
                            for(size_t i = 0 ; i < str_val.size() ; i++){
                                val += usrp::gpio_atr::gpio_attr_value_pair.at(attr.second).at(str_val[i])<<i;
                            }
                            fp_gpio->set_gpio_attr(attr.first, val);
                         });
                    break;
                case usrp::gpio_atr::GPIO_READBACK:
                    _tree->create<uint8_t>(mb_path / "gpio" / "INT0" / "READBACK")
                        .set_publisher([this, fp_gpio](){
                            return fp_gpio->read_gpio();
                         });
                    break;
                default:
                    _tree->create<uint32_t>(mb_path / "gpio" / "INT0" / attr.second)
                         .set(0)
                         .add_coerced_subscriber([this, fp_gpio, attr](const uint32_t val){
                             fp_gpio->set_gpio_attr(attr.first, val);
                         });
            }
    }


    ////////////////////////////////////////////////////////////////////
    // register the time keepers - only one can be the highlander
    ////////////////////////////////////////////////////////////////////
    _tree->create<time_spec_t>(mb_path / "time" / "now")
        .set_publisher(boost::bind(&time_core_3000::get_time_now, _radio_perifs[0].time64))
        .add_coerced_subscriber(boost::bind(&e300_impl::_set_time, this, _1))
        .set(0.0);
    //re-sync the times when the tick rate changes
    _tree->access<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&e300_impl::_sync_times, this));
    _tree->create<time_spec_t>(mb_path / "time" / "pps")
        .set_publisher(boost::bind(&time_core_3000::get_time_last_pps, _radio_perifs[0].time64))
        .add_coerced_subscriber(boost::bind(&time_core_3000::set_time_next_pps, _radio_perifs[0].time64, _1))
        .add_coerced_subscriber(boost::bind(&time_core_3000::set_time_next_pps, _radio_perifs[1].time64, _1));
    //setup time source props
    _tree->create<std::string>(mb_path / "time_source" / "value")
        .add_coerced_subscriber(boost::bind(&e300_impl::_update_time_source, this, _1))
        .set(e300::DEFAULT_TIME_SRC);
#ifdef E300_GPSD
    static const std::vector<std::string> time_sources = boost::assign::list_of("none")("internal")("external")("gpsdo");
#else
    static const std::vector<std::string> time_sources = boost::assign::list_of("none")("internal")("external");
#endif
    _tree->create<std::vector<std::string> >(mb_path / "time_source" / "options").set(time_sources);
    //setup reference source props
    _tree->create<std::string>(mb_path / "clock_source" / "value")
        .add_coerced_subscriber(boost::bind(&e300_impl::_update_clock_source, this, _1))
        .set(e300::DEFAULT_CLOCK_SRC);
    static const std::vector<std::string> clock_sources = boost::assign::list_of("internal"); //external,gpsdo not supported
    _tree->create<std::vector<std::string> >(mb_path / "clock_source" / "options").set(clock_sources);

    ////////////////////////////////////////////////////////////////////
    // dboard eeproms but not really
    ////////////////////////////////////////////////////////////////////
    dboard_eeprom_t db_eeprom;
    _tree->create<dboard_eeprom_t>(mb_path / "dboards" / "A" / "rx_eeprom")
        .set(_eeprom_manager->get_db_eeprom())
        .add_coerced_subscriber(boost::bind(
            &e300_eeprom_manager::write_db_eeprom,
            _eeprom_manager, _1));

    _tree->create<dboard_eeprom_t>(mb_path / "dboards" / "A" / "tx_eeprom")
        .set(_eeprom_manager->get_db_eeprom())
        .add_coerced_subscriber(boost::bind(
            &e300_eeprom_manager::write_db_eeprom,
            _eeprom_manager, _1));

    _tree->create<dboard_eeprom_t>(mb_path / "dboards" / "A" / "gdb_eeprom").set(db_eeprom);

    ////////////////////////////////////////////////////////////////////
    // create RF frontend interfacing
    ////////////////////////////////////////////////////////////////////
    {
        const fs_path codec_path = mb_path / ("rx_codecs") / "A";
        _tree->create<std::string>(codec_path / "name").set("E3x0 RX dual ADC");
        _tree->create<int>(codec_path / "gains"); //empty cuz gains are in frontend
    }
    {
        const fs_path codec_path = mb_path / ("tx_codecs") / "A";
        _tree->create<std::string>(codec_path / "name").set("E3x0 TX dual DAC");
        _tree->create<int>(codec_path / "gains"); //empty cuz gains are in frontend
    }

    ////////////////////////////////////////////////////////////////////
    // create frontend mapping
    ////////////////////////////////////////////////////////////////////

     std::vector<size_t> default_map(2, 0);
     default_map[0] = 0; // set A->0
     default_map[1] = 1; // set B->1, even if there's only A

    _tree->create<std::vector<size_t> >(mb_path / "rx_chan_dsp_mapping").set(default_map);
    _tree->create<std::vector<size_t> >(mb_path / "tx_chan_dsp_mapping").set(default_map);

    _tree->create<subdev_spec_t>(mb_path / "rx_subdev_spec")
        .set(subdev_spec_t())
        .add_coerced_subscriber(boost::bind(&e300_impl::_update_subdev_spec, this, "rx", _1));
    _tree->create<subdev_spec_t>(mb_path / "tx_subdev_spec")
        .set(subdev_spec_t())
        .add_coerced_subscriber(boost::bind(&e300_impl::_update_subdev_spec, this, "tx", _1));

    ////////////////////////////////////////////////////////////////////
    // do some post-init tasks
    ////////////////////////////////////////////////////////////////////

    // init the clock rate to something reasonable
    _tree->access<double>(mb_path / "tick_rate").set(
        device_addr.cast<double>("master_clock_rate", ad936x_manager::DEFAULT_TICK_RATE));

    // subdev spec contains full width of selections
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
}

uhd::sensor_value_t e300_impl::_get_fe_pll_lock(const bool is_tx)
{
    const uint32_t st =
        _global_regs->peek32(global_regs::RB32_CORE_PLL);
    const bool locked = is_tx ? ((st & 0x1) > 0) : ((st & 0x2) > 0);
    return sensor_value_t("LO", locked, "locked", "unlocked");
}

e300_impl::~e300_impl(void)
{
    if (_xport_path == AXI and not _do_not_reload)
        common::load_fpga_image(_idle_image);
}

void e300_impl::_enforce_tick_rate_limits(
        const size_t chan_count,
        const double tick_rate,
        const std::string &direction)
{
    const size_t max_chans = 2;
    if (chan_count > max_chans) {
        throw uhd::value_error(boost::str(
            boost::format("cannot not setup %d %s channels (maximum is %d)")
                % chan_count
                % direction
                % max_chans
        ));
    } else {
        const double max_tick_rate = ad9361_device_t::AD9361_MAX_CLOCK_RATE / ((chan_count <= 1) ? 1 : 2);
        if (tick_rate - max_tick_rate >= 1.0)
        {
            throw uhd::value_error(boost::str(
                boost::format("current master clock rate (%.6f MHz) exceeds maximum possible master clock rate (%.6f MHz) when using %d %s channels")
                    % (tick_rate/1e6)
                    % (max_tick_rate/1e6)
                    % chan_count
                    % direction
            ));
        }
        // Minimum rate restriction due to MMCM used in capture interface to AD9361.
        // Xilinx Artix-7 FPGA MMCM minimum input frequency is 10 MHz.
        const double min_tick_rate = uhd::usrp::e300::MIN_TICK_RATE / ((chan_count <= 1) ? 1 : 2);
        if (tick_rate - min_tick_rate < 0.0)
        {
            throw uhd::value_error(boost::str(
                boost::format("current master clock rate (%.6f MHz) set below minimum possible master clock rate (%.6f MHz)")
                    % (tick_rate/1e6)
                    % (min_tick_rate/1e6)
            ));
        }
    }
}

double e300_impl::_set_tick_rate(const double rate)
{
    UHD_LOGGER_INFO("E300") << "Asking for clock rate " << rate/1e6 << " MHz\n";
    _tick_rate = _codec_ctrl->set_clock_rate(rate);
    UHD_LOGGER_INFO("E300") << "Actually got clock rate " << _tick_rate/1e6 << " MHz\n";

    for(radio_perifs_t &perif:  _radio_perifs)
    {
        perif.time64->set_tick_rate(_tick_rate);
        perif.time64->self_test();
    }
    return _tick_rate;
}

void e300_impl::_register_loopback_self_test(wb_iface::sptr iface)
{
    bool test_fail = false;
    UHD_LOGGER_INFO("E300") << "Performing register loopback test... ";
    size_t hash = size_t(time(NULL));
    for (size_t i = 0; i < 100; i++)
    {
        boost::hash_combine(hash, i);
        iface->poke32(radio::sr_addr(radio::TEST), uint32_t(hash));
        test_fail = iface->peek32(radio::RB32_TEST) != uint32_t(hash);
        if (test_fail) break; //exit loop on any failure
    }
    UHD_LOGGER_INFO("E300") << "Register loopback test " << ((test_fail)? " failed" : "passed");
}

uint32_t e300_impl::_get_version(compat_t which)
{
    const uint16_t compat_num
        = _global_regs->peek32(global_regs::RB32_CORE_COMPAT);

    switch(which) {
    case FPGA_MINOR:
        return compat_num & 0xff;
    case FPGA_MAJOR:
        return (compat_num & 0xff00) >> 8;
    default:
        throw uhd::value_error("Requested unknown version.");
    };
}

std::string e300_impl::_get_version_hash(void)
{
    const uint32_t git_hash
        = _global_regs->peek32(global_regs::RB32_CORE_GITHASH);
    return str(boost::format("%7x%s")
        % (git_hash & 0x0FFFFFFF)
        % ((git_hash & 0xF000000) ? "-dirty" : ""));
}

uint32_t e300_impl::_allocate_sid(const sid_config_t &config)
{
    const uint32_t stream = (config.dst_prefix | (config.router_dst_there << 2)) & 0xff;

    const size_t sid_framer = _sid_framer++; //increment for next setup
    const uint32_t sid = 0
        | (E300_DEVICE_HERE << 24)
        | (sid_framer << 16)
        | (config.router_addr_there << 8)
        | (stream << 0)
    ;
    UHD_LOGGER_DEBUG("E300")<< std::hex
        << " sid 0x" << sid
        << " framer 0x" << sid_framer
        << " stream 0x" << stream
        << " router_dst_there 0x" << int(config.router_dst_there)
        << " router_addr_there 0x" << int(config.router_addr_there)
        << std::dec ;

    // Program the E300 to recognize it's own local address.
    _global_regs->poke32(global_regs::SR_CORE_XB_LOCAL, config.router_addr_there);

    // Program CAM entry for outgoing packets matching a E300 resource (e.g. Radio).
    // This type of packet matches the XB_LOCAL address and is looked up in the upper
    // half of the CAM
    _global_regs->poke32(XB_ADDR(256 + stream),
                         config.router_dst_there);

    // Program CAM entry for returning packets to us (for example GR host via zynq_fifo)
    // This type of packet does not match the XB_LOCAL address and is looked up in the lower half of the CAM
    _global_regs->poke32(XB_ADDR(E300_DEVICE_HERE),
                         config.router_dst_here);

    UHD_LOGGER_TRACE("E300") << std::hex
        << "done router config for sid 0x" << sid
        << std::dec ;

    return sid;
}

void e300_impl::_setup_dest_mapping(const uint32_t sid, const size_t which_stream)
{
    UHD_LOGGER_DEBUG("E300") << boost::format("Setting up dest map for 0x%lx to be stream %d")
                                     % (sid & 0xff) % which_stream ;
    _global_regs->poke32(DST_ADDR(sid & 0xff), which_stream);
}

void e300_impl::_update_time_source(const std::string &source)
{
    UHD_LOGGER_INFO("E300") << boost::format("Setting time source to %s") % source;
    if (source == "none" or source == "internal") {
        _misc.pps_sel = global_regs::PPS_INT;
#ifdef E300_GPSD
    } else if (source == "gpsdo") {
        _misc.pps_sel = global_regs::PPS_GPS;
#endif
    } else if (source == "external") {
        _misc.pps_sel = global_regs::PPS_EXT;
    } else {
        throw uhd::key_error("update_time_source: unknown source: " + source);
    }
    _update_gpio_state();
}

void e300_impl::_set_time(const uhd::time_spec_t& t)
{
    for(radio_perifs_t &perif:  _radio_perifs)
        perif.time64->set_time_sync(t);
    _misc.time_sync = 1;
    _update_gpio_state();
    _misc.time_sync = 0;
    _update_gpio_state();
}

void e300_impl::_sync_times()
{
    _set_time(_radio_perifs[0].time64->get_time_now());
}

size_t e300_impl::_get_axi_dma_channel(
    uint8_t destination,
    uint8_t prefix)
{
    static const uint32_t RADIO_GRP_SIZE = 4;
    static const uint32_t RADIO0_GRP     = 0;
    static const uint32_t RADIO1_GRP     = 1;

    uint32_t radio_grp = (destination == E300_XB_DST_R0) ? RADIO0_GRP : RADIO1_GRP;
    return ((radio_grp * RADIO_GRP_SIZE) + prefix);
}

uint16_t e300_impl::_get_udp_port(
        uint8_t destination,
        uint8_t prefix)
{
    if (destination == E300_XB_DST_R0) {
        if (prefix == E300_RADIO_DEST_PREFIX_CTRL)
            return boost::lexical_cast<uint16_t>(E300_SERVER_CTRL_PORT0);
        else if (prefix == E300_RADIO_DEST_PREFIX_TX)
            return boost::lexical_cast<uint16_t>(E300_SERVER_TX_PORT0);
        else if (prefix == E300_RADIO_DEST_PREFIX_RX)
            return boost::lexical_cast<uint16_t>(E300_SERVER_RX_PORT0);
    } else if (destination == E300_XB_DST_R1) {
        if (prefix == E300_RADIO_DEST_PREFIX_CTRL)
            return boost::lexical_cast<uint16_t>(E300_SERVER_CTRL_PORT1);
        else if (prefix == E300_RADIO_DEST_PREFIX_TX)
            return boost::lexical_cast<uint16_t>(E300_SERVER_TX_PORT1);
        else if (prefix == E300_RADIO_DEST_PREFIX_RX)
            return boost::lexical_cast<uint16_t>(E300_SERVER_RX_PORT1);
    }
    throw uhd::value_error(str(boost::format("No UDP port defined for combination: %u %u") % destination % prefix));
}

e300_impl::both_xports_t e300_impl::_make_transport(
    const uint8_t &destination,
    const uint8_t &prefix,
    const uhd::transport::zero_copy_xport_params &params,
    uint32_t &sid)
{
    both_xports_t xports;

    sid_config_t config;
    config.router_addr_there    = E300_DEVICE_THERE;
    config.dst_prefix           = prefix;
    config.router_dst_there     = destination;
    config.router_dst_here      = E300_XB_DST_AXI;
    sid = this->_allocate_sid(config);

    // in local mode
    if (_xport_path == AXI) {
        // lookup which dma channel we need
        // to use to create our transport
        const size_t stream = _get_axi_dma_channel(
            destination,
            prefix);

        xports.send =
            _fifo_iface->make_send_xport(stream, params);
        xports.recv =
            _fifo_iface->make_recv_xport(stream, params);

    // in network mode
    } else if (_xport_path == ETH) {
        // lookup which udp port we need
        // to use to create our transport
        const uint16_t port = _get_udp_port(
            destination,
            prefix);

        udp_zero_copy::buff_params dummy_buff_params_out;
        xports.send = udp_zero_copy::make(
            _device_addr["addr"],
            str(boost::format("%u") % port), params,
            dummy_buff_params_out,
            _device_addr);

        // use the same xport in both directions
        xports.recv = xports.send;
    }

    // configure the return path
    _setup_dest_mapping(sid, _get_axi_dma_channel(destination, prefix));

    return xports;
}

void e300_impl::_update_clock_source(const std::string &source)
{
    if (source != "internal") {
        throw uhd::value_error(boost::str(
            boost::format("Clock source option not supported: %s. The only value supported is \"internal\". " \
                          "To discipline the internal oscillator, set the appropriate time source.") % source
        ));
    }
}

void e300_impl::_update_antenna_sel(const size_t &which, const std::string &ant)
{
    if (ant != "TX/RX" and ant != "RX2")
        throw uhd::value_error("Unknown RX antenna option: " + ant);
    _radio_perifs[which].ant_rx2 = (ant == "RX2");
    this->_update_atrs();
}

void e300_impl::_update_fe_lo_freq(const std::string &fe, const double freq)
{
    if (fe[0] == 'R')
        _settings.rx_freq = freq;
    if (fe[0] == 'T')
        _settings.tx_freq = freq;
    this->_update_atrs();
    _update_bandsel(fe, freq);
}

void e300_impl::_setup_radio(const size_t dspno)
{
    radio_perifs_t &perif = _radio_perifs[dspno];
    const fs_path mb_path = "/mboards/0";
    std::string slot_name = (dspno == 0) ? "A" : "B";

    ////////////////////////////////////////////////////////////////////
    // crossbar config for ctrl xports
    ////////////////////////////////////////////////////////////////////

    // make a transport, grab a sid
    uint32_t ctrl_sid;
    both_xports_t ctrl_xports = _make_transport(
       dspno ? E300_XB_DST_R1 : E300_XB_DST_R0,
       E300_RADIO_DEST_PREFIX_CTRL,
       _ctrl_xport_params,
       ctrl_sid);

    this->_setup_dest_mapping(
        ctrl_sid,
        dspno ? E300_R1_CTRL_STREAM
              : E300_R0_CTRL_STREAM);

    ////////////////////////////////////////////////////////////////////
    // radio control
    ////////////////////////////////////////////////////////////////////
    perif.ctrl = radio_ctrl_core_3000::make(
        false/*lilE*/,
        ctrl_xports.send,
        ctrl_xports.recv,
        ctrl_sid,
        dspno ? "1" : "0");
    this->_register_loopback_self_test(perif.ctrl);

    ////////////////////////////////////////////////////////////////////
    // Set up peripherals
    ////////////////////////////////////////////////////////////////////
    perif.atr = gpio_atr_3000::make_write_only(perif.ctrl, radio::sr_addr(radio::GPIO));
    perif.atr->set_atr_mode(MODE_ATR, 0xFFFFFFFF);
    perif.rx_fe = rx_frontend_core_200::make(perif.ctrl, radio::sr_addr(radio::RX_FRONT));
    perif.rx_fe->set_dc_offset(rx_frontend_core_200::DEFAULT_DC_OFFSET_VALUE);
    perif.rx_fe->set_dc_offset_auto(rx_frontend_core_200::DEFAULT_DC_OFFSET_ENABLE);
    perif.rx_fe->set_iq_balance(rx_frontend_core_200::DEFAULT_IQ_BALANCE_VALUE);
    perif.tx_fe = tx_frontend_core_200::make(perif.ctrl, radio::sr_addr(radio::TX_FRONT));
    perif.tx_fe->set_dc_offset(tx_frontend_core_200::DEFAULT_DC_OFFSET_VALUE);
    perif.tx_fe->set_iq_balance(tx_frontend_core_200::DEFAULT_IQ_BALANCE_VALUE);
    perif.framer = rx_vita_core_3000::make(perif.ctrl, radio::sr_addr(radio::RX_CTRL));
    perif.ddc = rx_dsp_core_3000::make(perif.ctrl, radio::sr_addr(radio::RX_DSP));
    perif.ddc->set_link_rate(10e9/8); //whatever
    perif.ddc->set_freq(e300::DEFAULT_DDC_FREQ);
    perif.deframer = tx_vita_core_3000::make(perif.ctrl, radio::sr_addr(radio::TX_CTRL));
    perif.duc = tx_dsp_core_3000::make(perif.ctrl, radio::sr_addr(radio::TX_DSP));
    perif.duc->set_link_rate(10e9/8); //whatever
    perif.duc->set_freq(e300::DEFAULT_DUC_FREQ);

    ////////////////////////////////////////////////////////////////////
    // create time control objects
    ////////////////////////////////////////////////////////////////////
    time_core_3000::readback_bases_type time64_rb_bases;
    time64_rb_bases.rb_now = radio::RB64_TIME_NOW;
    time64_rb_bases.rb_pps = radio::RB64_TIME_PPS;
    perif.time64 = time_core_3000::make(perif.ctrl, radio::sr_addr(radio::TIME), time64_rb_bases);

    ////////////////////////////////////////////////////////////////////
    // front end corrections
    ////////////////////////////////////////////////////////////////////
    perif.rx_fe->populate_subtree(_tree->subtree(mb_path / "rx_frontends" / slot_name));
    perif.tx_fe->populate_subtree(_tree->subtree(mb_path / "tx_frontends" / slot_name));

    ////////////////////////////////////////////////////////////////////
    // connect rx dsp control objects
    ////////////////////////////////////////////////////////////////////
    _tree->access<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&rx_vita_core_3000::set_tick_rate, perif.framer, _1))
        .add_coerced_subscriber(boost::bind(&rx_dsp_core_3000::set_tick_rate, perif.ddc, _1));
    const fs_path rx_dsp_path = mb_path / "rx_dsps" / str(boost::format("%u") % dspno);
    perif.ddc->populate_subtree(_tree->subtree(rx_dsp_path));
    _tree->access<double>(rx_dsp_path / "rate" / "value")
        .add_coerced_subscriber(boost::bind(&e300_impl::_update_rx_samp_rate, this, dspno, _1))
    ;
    _tree->create<stream_cmd_t>(rx_dsp_path / "stream_cmd")
        .add_coerced_subscriber(boost::bind(&rx_vita_core_3000::issue_stream_command, perif.framer, _1));

    ////////////////////////////////////////////////////////////////////
    // create tx dsp control objects
    ////////////////////////////////////////////////////////////////////
    _tree->access<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&tx_dsp_core_3000::set_tick_rate, perif.duc, _1));
    const fs_path tx_dsp_path = mb_path / "tx_dsps" / str(boost::format("%u") % dspno);
    perif.duc->populate_subtree(_tree->subtree(tx_dsp_path));
    _tree->access<double>(tx_dsp_path / "rate" / "value")
        .add_coerced_subscriber(boost::bind(&e300_impl::_update_tx_samp_rate, this, dspno, _1))
    ;

    ////////////////////////////////////////////////////////////////////
    // create RF frontend interfacing
    ////////////////////////////////////////////////////////////////////
    static const std::vector<direction_t> dirs = boost::assign::list_of(RX_DIRECTION)(TX_DIRECTION);
    for(direction_t dir:  dirs) {
        const std::string x = (dir == RX_DIRECTION) ? "rx" : "tx";
        const std::string key = boost::to_upper_copy(x) + std::string(((dspno == FE0)? "1" : "2"));
        const fs_path rf_fe_path
            = mb_path / "dboards" / "A" / (x + "_frontends") / ((dspno == 0) ? "A" : "B");

        // This will connect all the AD936x-specific items
        _codec_mgr->populate_frontend_subtree(
            _tree->subtree(rf_fe_path), key, dir
        );

        // This will connect all the e300_impl-specific items
        _tree->create<sensor_value_t>(rf_fe_path / "sensors" / "lo_locked")
            .set_publisher(boost::bind(&e300_impl::_get_fe_pll_lock, this, dir == TX_DIRECTION))
        ;
        _tree->access<double>(rf_fe_path / "freq" / "value")
            .add_coerced_subscriber(boost::bind(&e300_impl::_update_fe_lo_freq, this, key, _1))
        ;

        // Antenna Setup
        if (dir == RX_DIRECTION) {
            static const std::vector<std::string> ants = boost::assign::list_of("TX/RX")("RX2");
            _tree->create<std::vector<std::string> >(rf_fe_path / "antenna" / "options").set(ants);
            _tree->create<std::string>(rf_fe_path / "antenna" / "value")
                .add_coerced_subscriber(boost::bind(&e300_impl::_update_antenna_sel, this, dspno, _1))
                .set("RX2");
        }
        else if (dir == TX_DIRECTION) {
            static const std::vector<std::string> ants(1, "TX/RX");
            _tree->create<std::vector<std::string> >(rf_fe_path / "antenna" / "options").set(ants);
            _tree->create<std::string>(rf_fe_path / "antenna" / "value").set("TX/RX");
        }
    }
}

void e300_impl::_update_enables(void)
{
    //extract settings from state variables
    const bool enb_tx1 = bool(_radio_perifs[FE0].tx_streamer.lock());
    const bool enb_rx1 = bool(_radio_perifs[FE0].rx_streamer.lock());
    const bool enb_tx2 = bool(_radio_perifs[FE1].tx_streamer.lock());
    const bool enb_rx2 = bool(_radio_perifs[FE1].rx_streamer.lock());
    const size_t num_rx = (enb_rx1 ? 1 : 0) + (enb_rx2 ? 1:0);
    const size_t num_tx = (enb_tx1 ? 1 : 0) + (enb_tx2 ? 1:0);
    const bool mimo = num_rx == 2 or num_tx == 2;

    //setup the active chains in the codec
    _codec_ctrl->set_active_chains(enb_tx1, enb_tx2, enb_rx1, enb_rx2);
    if ((num_rx + num_tx) == 0)
        _codec_ctrl->set_active_chains(
            true, false, true, false); // enable something

    //set_active_chains could cause a clock rate change - reset dcm
    _reset_codec_mmcm();

    //figure out if mimo is enabled based on new state
    _misc.mimo = (mimo)? 1 : 0;
    _update_gpio_state();

    //atrs change based on enables
    _update_atrs();
}

void e300_impl::_update_gpio_state(void)
{
    uint32_t misc_reg = 0
        | (_misc.pps_sel      << gpio_t::PPS_SEL)
        | (_misc.mimo         << gpio_t::MIMO)
        | (_misc.codec_arst   << gpio_t::CODEC_ARST)
        | (_misc.tx_bandsels  << gpio_t::TX_BANDSEL)
        | (_misc.rx_bandsel_a << gpio_t::RX_BANDSELA)
        | (_misc.rx_bandsel_b << gpio_t::RX_BANDSELB)
        | (_misc.rx_bandsel_c << gpio_t::RX_BANDSELC)
        | (_misc.time_sync    << gpio_t::TIME_SYNC);
    _global_regs->poke32(global_regs::SR_CORE_MISC, misc_reg);
}

void e300_impl::_reset_codec_mmcm(void)
{
    _misc.codec_arst = 1;
    _update_gpio_state();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    _misc.codec_arst = 0;
    _update_gpio_state();
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//////////////// ATR SETUP FOR FRONTEND CONTROL VIA GPIO ///////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void e300_impl::_update_bandsel(const std::string& which, double freq)
{
    if(which[0] == 'R') {
        if (freq < 450e6) {
            _misc.rx_bandsel_a  = 44; // 4 | (5 << 3)
            _misc.rx_bandsel_b  = 0;  // 0 | (0 << 2)
            _misc.rx_bandsel_c  = 6;  // 2 | (1 << 2)
        } else if (freq < 700e6) {
            _misc.rx_bandsel_a  = 26; // 2 | (3 << 3)
            _misc.rx_bandsel_b  = 0;  // 0 | (0 << 2)
            _misc.rx_bandsel_c  = 15; // 3 | (3 << 2)
        } else if (freq < 1200e6) {
            _misc.rx_bandsel_a  = 8; // 0 | (1 << 3)
            _misc.rx_bandsel_b  = 0; // 0 | (0 << 2)
            _misc.rx_bandsel_c  = 9; // 1 | (2 << 2)
        } else if (freq < 1800e6) {
            _misc.rx_bandsel_a  = 1; // 1 | (0 << 3)
            _misc.rx_bandsel_b  = 6; // 2 | (1 << 2)
            _misc.rx_bandsel_c  = 0; // 0 | (0 << 2)
        } else if (freq < 2350e6){
            _misc.rx_bandsel_a  = 19; // 3 | (2 << 3)
            _misc.rx_bandsel_b  = 15; // 3 | (3 << 2)
            _misc.rx_bandsel_c  = 0;  // 0 | (0 << 2)
        } else if (freq < 2600e6){
            _misc.rx_bandsel_a  = 37; // 5 | (4 << 3)
            _misc.rx_bandsel_b  = 9;  // 1 | (2 << 2)
            _misc.rx_bandsel_c  = 0;  // 0 | (0 << 2)
        } else {
            _misc.rx_bandsel_a  = 0;
            _misc.rx_bandsel_b  = 0;
            _misc.rx_bandsel_c  = 0;
        }
        _update_gpio_state();
    } else if(which[0] == 'T') {
        if (freq < 117.7e6)
            _misc.tx_bandsels = 7;
        else if (freq < 178.2e6)
            _misc.tx_bandsels = 6;
        else if (freq < 284.3e6)
            _misc.tx_bandsels = 5;
        else if (freq < 453.7e6)
            _misc.tx_bandsels = 4;
        else if (freq < 723.8e6)
            _misc.tx_bandsels = 3;
        else if (freq < 1154.9e6)
            _misc.tx_bandsels = 2;
        else if (freq < 1842.6e6)
            _misc.tx_bandsels = 1;
        else if (freq < 2940.0e6)
            _misc.tx_bandsels = 0;
        else
            _misc.tx_bandsels = 7;
        _update_gpio_state();
    } else {
        UHD_THROW_INVALID_CODE_PATH();
    }
}


void e300_impl::_update_atrs(void)
{
    for (size_t instance = 0; instance < fpga::NUM_RADIOS; instance++)
    {
        // if we're not ready, no point ...
        if (not _radio_perifs[instance].atr)
            return;

        radio_perifs_t &perif = _radio_perifs[instance];
        const bool enb_rx = bool(perif.rx_streamer.lock());
        const bool enb_tx = bool(perif.tx_streamer.lock());
        const bool rx_ant_rx2  = perif.ant_rx2;

        const bool rx_low_band = _settings.rx_freq < 2.6e9;
        const bool tx_low_band = _settings.tx_freq < 2940.0e6;

        // VCRX
        int vcrx_v1_rxing = 1;
        int vcrx_v2_rxing = 0;
        int vcrx_v1_txing = 1;
        int vcrx_v2_txing = 0;

        if (rx_low_band) {
            vcrx_v1_rxing = rx_ant_rx2 ? 0 : 1;
            vcrx_v2_rxing = rx_ant_rx2 ? 1 : 0;
            vcrx_v1_txing = 0;
            vcrx_v2_txing = 1;
        } else {
            vcrx_v1_rxing = rx_ant_rx2 ? 1 : 0;
            vcrx_v2_rxing = rx_ant_rx2 ? 0 : 1;
            vcrx_v1_txing = 1;
            vcrx_v2_txing = 0;
        }

        // VCTX
        int vctxrx_v1_rxing = 0;
        int vctxrx_v2_rxing = 1;
        int vctxrx_v1_txing = 0;
        int vctxrx_v2_txing = 1;

        if (tx_low_band) {
            vctxrx_v1_rxing = rx_ant_rx2 ? 1 : 0;
            vctxrx_v2_rxing = rx_ant_rx2 ? 0 : 1;
            vctxrx_v1_txing = 1;
            vctxrx_v2_txing = 0;
        } else {
            vctxrx_v1_rxing = rx_ant_rx2 ? 1 : 0;
            vctxrx_v2_rxing = rx_ant_rx2 ? 0 : 1;
            vctxrx_v1_txing = 1;
            vctxrx_v2_txing = 1;
        }
        //swapped for routing reasons, reswap it here
        if (instance == 1) {
            std::swap(vctxrx_v1_rxing, vctxrx_v2_rxing);
            std::swap(vctxrx_v1_txing, vctxrx_v2_txing);
        }

        int tx_enable_a = (!tx_low_band and enb_tx) ? 1 : 0;
        int tx_enable_b = (tx_low_band and  enb_tx) ? 1 : 0;

        //----------------- LEDS ----------------------------//
        const int led_rx2  = rx_ant_rx2  ? 1 : 0;
        const int led_txrx = !rx_ant_rx2 ? 1 : 0;
        const int led_tx   = 1;

        const int rx_leds = (led_rx2 << LED_RX_RX) | (led_txrx << LED_TXRX_RX);
        const int tx_leds = (led_tx << LED_TXRX_TX);
        const int xx_leds = tx_leds | (1 << LED_RX_RX); //forced to rx2

        const int rx_selects = 0
            | (vcrx_v1_rxing << VCRX_V1)
            | (vcrx_v2_rxing << VCRX_V2)
            | (vctxrx_v1_rxing << VCTXRX_V1)
            | (vctxrx_v2_rxing << VCTXRX_V2)
        ;
        const int tx_selects = 0
            | (vcrx_v1_txing << VCRX_V1)
            | (vcrx_v2_txing << VCRX_V2)
            | (vctxrx_v1_txing << VCTXRX_V1)
            | (vctxrx_v2_txing << VCTXRX_V2)
        ;
        const int tx_enables = 0
            | (tx_enable_a << TX_ENABLEA)
            | (tx_enable_b << TX_ENABLEB)
        ;

        //default selects
        int oo_reg = rx_selects;
        int rx_reg = rx_selects;
        int tx_reg = tx_selects;
        int fd_reg = tx_selects; //tx selects dominate in fd mode

        //add in leds and tx enables based on fe enable
        if (enb_rx)
            rx_reg |= rx_leds;
        if (enb_rx)
            fd_reg |= xx_leds;
        if (enb_tx)
            tx_reg |= tx_enables | tx_leds;
        if (enb_tx)
            fd_reg |= tx_enables | xx_leds;

        gpio_atr_3000::sptr atr = _radio_perifs[instance].atr;
        atr->set_atr_reg(ATR_REG_IDLE, oo_reg);
        atr->set_atr_reg(ATR_REG_RX_ONLY, rx_reg);
        atr->set_atr_reg(ATR_REG_TX_ONLY, tx_reg);
        atr->set_atr_reg(ATR_REG_FULL_DUPLEX, fd_reg);
    }
}

}}} // namespace

UHD_STATIC_BLOCK(register_e300_device)
{
    device::register_device(&uhd::usrp::e300::e300_find, &uhd::usrp::e300::e300_make, uhd::device::USRP);
}
