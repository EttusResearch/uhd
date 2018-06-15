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
#include "e3xx_radio_ctrl_impl.hpp"


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
#include <boost/thread/thread.hpp> //sleep
#include <boost/asio.hpp>
#include <fstream>

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::usrp::gpio_atr;
using namespace uhd::transport;
namespace fs = boost::filesystem;
namespace asio = boost::asio;

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
        BOOST_FOREACH(const device_addr_t &hint_i, hints)
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
            BOOST_FOREACH(const if_addrs_t &if_addrs, get_if_addrs())
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

        BOOST_FOREACH(const std::string &ip_addr, ip_addrs)
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
    , _dma_chans_available(MAX_DMA_CHANNEL_PAIRS, ~size_t(0) /* all available at the beginning */)
{
    stream_options.rx_fc_request_freq = E300_RX_FC_REQUEST_FREQ;

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
            device_addr_t device_addr_cp(device_addr.to_string());
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

    ad9361_ctrl::sptr  codec_ctrl;
    if (_xport_path == ETH) {
        zero_copy_if::sptr codec_xport =
            udp_zero_copy::make(device_addr["addr"], E300_SERVER_CODEC_PORT, _ctrl_xport_params, dummy_buff_params_out, device_addr);
        codec_ctrl = e300_remote_codec_ctrl::make(codec_xport);
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
        codec_ctrl = ad9361_ctrl::make_spi(client_settings, spi::make(E300_SPIDEV_DEVICE), 1);
        // This is horrible ... why do I have to sleep here?
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
        _eeprom_manager = boost::make_shared<e300_eeprom_manager>(i2c::make_i2cdev(E300_I2CDEV_DEVICE));
        _sensor_manager = e300_sensor_manager::make_local(_global_regs);
    }

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
            boost::this_thread::sleep(boost::posix_time::seconds(1));
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
    UHD_LOGGER_INFO("E300") << "Initializing core control (global registers)..." << std::endl;
    this->_register_loopback_self_test(
        _global_regs,
        global_regs::SR_CORE_TEST,
        global_regs::RB32_CORE_TEST
    );

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

    // Clock reference source
    _tree->create<std::string>(mb_path / "clock_source" / "value")
        .add_coerced_subscriber(boost::bind(&e300_impl::_update_clock_source, this, _1))
        .set(e300::DEFAULT_CLOCK_SRC);
    static const std::vector<std::string> clock_sources =
        boost::assign::list_of("internal"); //external,gpsdo not supported
    _tree->create<std::vector<std::string> >(mb_path / "clock_source" / "options").set(clock_sources);

    ////////////////////////////////////////////////////////////////////
    // and do the misc mboard sensors
    ////////////////////////////////////////////////////////////////////
    _tree->create<int>(mb_path / "sensors");
    BOOST_FOREACH(const std::string &name, _sensor_manager->get_sensors())
    {
        _tree->create<sensor_value_t>(mb_path / "sensors" / name)
            .set_publisher(boost::bind(&e300_sensor_manager::get_sensor, _sensor_manager, name));
    }
#ifdef E300_GPSD
    if (_gps) {
        BOOST_FOREACH(const std::string &name, _gps->get_sensors())
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
    // Access to global regs
    ////////////////////////////////////////////////////////////////////
    _tree->create<uint32_t>(mb_path / "global_regs" / "misc")
        .add_coerced_subscriber(boost::bind(&global_regs::poke32, _global_regs, global_regs::SR_CORE_MISC, _1))
    ;
    _tree->create<uint32_t>(mb_path / "global_regs" / "pll")
        .set_publisher(boost::bind(&global_regs::peek32, _global_regs, global_regs::RB32_CORE_PLL))
    ;

    ////////////////////////////////////////////////////////////////////
    // clocking
    ////////////////////////////////////////////////////////////////////
    _tree->create<double>(mb_path / "tick_rate")
        .add_coerced_subscriber(boost::bind(&device3_impl::update_tx_streamers, this, _1))
        .add_coerced_subscriber(boost::bind(&device3_impl::update_rx_streamers, this, _1))
    ;

    //default some chains on -- needed for setup purposes
    UHD_LOGGER_DEBUG("E300") << "Initializing AD9361 using hard SPI core..." << std::flush;
    codec_ctrl->set_active_chains(true, false, true, false);
    codec_ctrl->set_clock_rate(50e6);
    UHD_LOGGER_DEBUG("E300") << "OK" << std::endl;

    ////////////////////////////////////////////////////////////////////
    // Set up RFNoC blocks
    ////////////////////////////////////////////////////////////////////
    const size_t n_rfnoc_blocks = _global_regs->peek32(global_regs::RB32_CORE_NUM_CE);
    enumerate_rfnoc_blocks(
        0, /* mboard index */
        n_rfnoc_blocks,
        E300_XB_DST_AXI + 1, /* base port, rfnoc blocks come after the AXI connect */
        uhd::sid_t(E300_DEVICE_HERE, 0, E300_DEVICE_THERE, 0),
        device_addr_t()
    );

    // If we have a radio, we must configure its codec control:
    std::vector<rfnoc::block_id_t> radio_ids = find_blocks<rfnoc::e3xx_radio_ctrl_impl>("Radio");
    if (radio_ids.size() > 0) {
        UHD_LOGGER_DEBUG("E300") << "Initializing Radio Block..." << std::endl;
        get_block_ctrl<rfnoc::e3xx_radio_ctrl_impl>(radio_ids[0])->setup_radio(codec_ctrl);
        if (radio_ids.size() != 1) {
            UHD_LOGGER_WARNING("E300") << "Too many Radio Blocks found. Using only " << radio_ids[0] << std::endl;
        }
    } else {
        UHD_LOGGER_DEBUG("E300") << "No Radio Block found. Assuming radio-less operation." << std::endl;
    }

    ////////////////////////////////////////////////////////////////////
    // do some post-init tasks
    ////////////////////////////////////////////////////////////////////
    // init the clock rate to something reasonable
    _tree->access<double>(mb_path / "tick_rate")
        .set(device_addr.cast<double>("master_clock_rate", ad936x_manager::DEFAULT_TICK_RATE));

    // subdev spec contains full width of selections
    subdev_spec_t rx_spec, tx_spec;
    BOOST_FOREACH(const std::string &fe, _tree->list(mb_path / "dboards" / "A" / "rx_frontends"))
    {
        rx_spec.push_back(subdev_spec_pair_t("A", fe));
    }
    BOOST_FOREACH(const std::string &fe, _tree->list(mb_path / "dboards" / "A" / "tx_frontends"))
    {
        tx_spec.push_back(subdev_spec_pair_t("A", fe));
    }
    _tree->create<subdev_spec_t>(mb_path / "rx_subdev_spec").set(rx_spec);
    _tree->create<subdev_spec_t>(mb_path / "tx_subdev_spec").set(tx_spec);
    UHD_LOGGER_DEBUG("E300") << "end of e300_impl()" << std::endl;
}

e300_impl::~e300_impl(void)
{
    if (_xport_path == AXI and not _do_not_reload)
        common::load_fpga_image(_idle_image);
}

void e300_impl::_register_loopback_self_test(wb_iface::sptr iface, uint32_t w_addr, uint32_t r_addr)
{
    bool test_fail = false;
    UHD_LOGGER_INFO("E300") << "Performing register loopback test... ";
    size_t hash = size_t(time(NULL));
    for (size_t i = 0; i < 100; i++)
    {
        boost::hash_combine(hash, i);
        iface->poke32(w_addr, uint32_t(hash));
        test_fail = iface->peek32(r_addr) != uint32_t(hash);
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
        % ((git_hash & 0xF0000000) ? "-dirty" : ""));
}


void e300_impl::_setup_dest_mapping(
    const uhd::sid_t &sid,
    const size_t which_stream)
{
    UHD_LOGGER_DEBUG("E300") << boost::format("[E300] Setting up dest map for host ep %lu to be stream %d")
                                     % sid.get_src_endpoint() % which_stream << std::endl;
    _global_regs->poke32(DST_ADDR(sid.get_src_endpoint()), which_stream);
}

size_t e300_impl::_get_axi_dma_channel_pair()
{
    if (_dma_chans_available.none()) {
        throw uhd::runtime_error("No more free DMA channels available.");
    }

    size_t first_free_pair = _dma_chans_available.find_first();
    _dma_chans_available.reset(first_free_pair);
    return first_free_pair;
}

uint16_t e300_impl::_get_udp_port(
        uint8_t destination,
        uint8_t prefix)
{
    if (destination == E300_XB_DST_RADIO) {
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

uhd::sid_t e300_impl::_allocate_sid(
    const uhd::sid_t &address)
{
    uhd::sid_t sid = address;
    sid.set_src_addr(E300_DEVICE_HERE);
    sid.set_src_endpoint(_sid_framer);

    // TODO: We don't have to do this everytime ...
    // Program the E300 to recognize it's own local address.
    _global_regs->poke32(global_regs::SR_CORE_XB_LOCAL, address.get_dst_addr());

    // Program CAM entry for outgoing packets matching a E300 resource
    // (e.g. Radio).
    // This type of packet matches the XB_LOCAL address and is looked up in
    // the upper half of the CAM
    _global_regs->poke32(XB_ADDR(256 + address.get_dst_endpoint()), address.get_dst_xbarport());

    // TODO: We don't have to do this everytime ...
    // Program CAM entry for returning packets to us
    // (for example host via zynq_fifo)
    // This type of packet does not match the XB_LOCAL address and is
    // looked up in the lower half of the CAM
    _global_regs->poke32(XB_ADDR(E300_DEVICE_HERE), E300_XB_DST_AXI);

    // increment for next setup
    _sid_framer++;

    return sid;
}

uhd::both_xports_t e300_impl::make_transport(
    const uhd::sid_t &address,
    const xport_type_t type,
    const uhd::device_addr_t &)
{
    uhd::both_xports_t xports;
    xports.endianness = ENDIANNESS_LITTLE;

    const uhd::transport::zero_copy_xport_params params =
        (type == CTRL) ? _ctrl_xport_params : _data_xport_params;

    xports.send_sid = _allocate_sid(address);
    xports.recv_sid = xports.send_sid.reversed();
    xports.recv_buff_size = params.recv_frame_size * params.num_recv_frames;
    xports.send_buff_size = params.send_frame_size * params.num_send_frames;

    if (_xport_path != AXI) {
        throw uhd::runtime_error("[E300] Currently only AXI transport supported with RFNOC");
    }

    const size_t chan_pair = _get_axi_dma_channel_pair();
    xports.send = _fifo_iface->make_send_xport(chan_pair, params);
    xports.recv = _fifo_iface->make_recv_xport(chan_pair, params);
    _setup_dest_mapping(xports.send_sid, chan_pair);

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

}}} // namespace

UHD_STATIC_BLOCK(register_e300_device)
{
    device::register_device(&uhd::usrp::e300::e300_find, &uhd::usrp::e300::e300_make, uhd::device::USRP);
}
