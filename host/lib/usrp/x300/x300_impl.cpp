//
// Copyright 2013-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_impl.hpp"
#include "x300_claim.hpp"
#include "x300_eth_mgr.hpp"
#include "x300_mb_eeprom.hpp"
#include "x300_mb_eeprom_iface.hpp"
#include "x300_mboard_type.hpp"
#include "x300_pcie_mgr.hpp"
#include <uhd/transport/if_addrs.hpp>
#include <uhd/types/sid.hpp>
#include <uhd/usrp/subdev_spec.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/math.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/static.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/make_shared.hpp>
#include <chrono>
#include <fstream>
#include <thread>

uhd::uart_iface::sptr x300_make_uart_iface(uhd::wb_iface::sptr iface);

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace uhd::usrp::x300;
namespace asio = boost::asio;


/***********************************************************************
 * Discovery over the udp and pcie transport
 **********************************************************************/
device_addrs_t x300_find(const device_addr_t& hint_)
{
    // handle the multi-device discovery
    device_addrs_t hints = separate_device_addr(hint_);
    if (hints.size() > 1) {
        device_addrs_t found_devices;
        std::string error_msg;
        for (const device_addr_t& hint_i : hints) {
            device_addrs_t found_devices_i = x300_find(hint_i);
            if (found_devices_i.size() != 1)
                error_msg +=
                    str(boost::format(
                            "Could not resolve device hint \"%s\" to a single device.")
                        % hint_i.to_string());
            else
                found_devices.push_back(found_devices_i[0]);
        }
        if (found_devices.empty())
            return device_addrs_t();
        if (not error_msg.empty())
            throw uhd::value_error(error_msg);

        return device_addrs_t(1, combine_device_addrs(found_devices));
    }

    // initialize the hint for a single device case
    UHD_ASSERT_THROW(hints.size() <= 1);
    hints.resize(1); // in case it was empty
    device_addr_t hint = hints[0];
    device_addrs_t addrs;
    if (hint.has_key("type") and hint["type"] != "x300") {
        return addrs;
    }

    // use the address given
    if (hint.has_key("addr")) {
        device_addrs_t reply_addrs;
        try {
            reply_addrs = eth_manager::find(hint);
        } catch (const std::exception& ex) {
            UHD_LOGGER_ERROR("X300") << "X300 Network discovery error " << ex.what();
        } catch (...) {
            UHD_LOGGER_ERROR("X300") << "X300 Network discovery unknown error ";
        }
        return reply_addrs;
    }

    if (!hint.has_key("resource")) {
        // otherwise, no address was specified, send a broadcast on each interface
        for (const transport::if_addrs_t& if_addrs : transport::get_if_addrs()) {
            // avoid the loopback device
            if (if_addrs.inet == asio::ip::address_v4::loopback().to_string())
                continue;

            // create a new hint with this broadcast address
            device_addr_t new_hint = hint;
            new_hint["addr"]       = if_addrs.bcast;

            // call discover with the new hint and append results
            device_addrs_t new_addrs = x300_find(new_hint);
            // if we are looking for a serial, only add the one device with a matching
            // serial
            if (hint.has_key("serial")) {
                bool found_serial = false; // signal to break out of the interface loop
                for (device_addrs_t::iterator new_addr_it = new_addrs.begin();
                     new_addr_it != new_addrs.end();
                     new_addr_it++) {
                    if ((*new_addr_it)["serial"] == hint["serial"]) {
                        addrs.insert(addrs.begin(), *new_addr_it);
                        found_serial = true;
                        break;
                    }
                }
                if (found_serial)
                    break;
            } else {
                // Otherwise, add all devices we find
                addrs.insert(addrs.begin(), new_addrs.begin(), new_addrs.end());
            }
        }
    }

    device_addrs_t pcie_addrs = pcie_manager::find(hint, hint.has_key("resource"));
    if (not pcie_addrs.empty()) {
        addrs.insert(addrs.end(), pcie_addrs.begin(), pcie_addrs.end());
    }

    return addrs;
}

/***********************************************************************
 * Make
 **********************************************************************/
static device::sptr x300_make(const device_addr_t& device_addr)
{
    return device::sptr(new x300_impl(device_addr));
}

UHD_STATIC_BLOCK(register_x300_device)
{
    device::register_device(&x300_find, &x300_make, device::USRP);
}

static void x300_load_fw(wb_iface::sptr fw_reg_ctrl, const std::string& file_name)
{
    UHD_LOGGER_INFO("X300") << "Loading firmware " << file_name;

    // load file into memory
    std::ifstream fw_file(file_name.c_str());
    uint32_t fw_file_buff[X300_FW_NUM_BYTES / sizeof(uint32_t)];
    fw_file.read((char*)fw_file_buff, sizeof(fw_file_buff));
    fw_file.close();

    // Poke the fw words into the WB boot loader
    fw_reg_ctrl->poke32(SR_ADDR(BOOT_LDR_BASE, BL_ADDRESS), 0);
    for (size_t i = 0; i < X300_FW_NUM_BYTES; i += sizeof(uint32_t)) {
        //@TODO: FIXME: Since x300_ctrl_iface acks each write and traps exceptions, the
        // first try for the last word
        //              written will print an error because it triggers a FW reload and
        //              fails to reply.
        fw_reg_ctrl->poke32(SR_ADDR(BOOT_LDR_BASE, BL_DATA),
            uhd::byteswap(fw_file_buff[i / sizeof(uint32_t)]));
    }

    // Wait for fimrware to reboot. 3s is an upper bound
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    UHD_LOGGER_INFO("X300") << "Firmware loaded!";
}

x300_impl::x300_impl(const uhd::device_addr_t& dev_addr) : device3_impl(), _sid_framer(0)
{
    UHD_LOGGER_INFO("X300") << "X300 initialization sequence...";
    _tree->create<std::string>("/name").set("X-Series Device");

    const device_addrs_t device_args = separate_device_addr(dev_addr);
    _mb.resize(device_args.size());

    // Serialize the initialization process
    if (dev_addr.has_key("serialize_init") or device_args.size() == 1) {
        for (size_t i = 0; i < device_args.size(); i++) {
            this->setup_mb(i, device_args[i]);
        }
        return;
    }


    // Initialize groups of USRPs in parallel
    size_t total_usrps = device_args.size();
    size_t num_usrps   = 0;
    while (num_usrps < total_usrps) {
        size_t init_usrps = std::min(total_usrps - num_usrps, x300::MAX_INIT_THREADS);
        boost::thread_group setup_threads;
        for (size_t i = 0; i < init_usrps; i++) {
            const size_t index = num_usrps + i;
            setup_threads.create_thread([this, index, device_args]() {
                this->setup_mb(index, device_args[index]);
            });
        }
        setup_threads.join_all();
        num_usrps += init_usrps;
    }
}

void x300_impl::setup_mb(const size_t mb_i, const uhd::device_addr_t& dev_addr)
{
    const fs_path mb_path  = fs_path("/mboards") / mb_i;
    mboard_members_t& mb   = _mb[mb_i];
    mb.args.parse(dev_addr);
    mb.xport_path = dev_addr.has_key("resource") ? xport_path_t::NIRIO
                                                 : xport_path_t::ETH;
    for (const std::string& key : dev_addr.keys()) {
        if (key.find("recv") != std::string::npos)
            mb.recv_args[key] = dev_addr[key];
        if (key.find("send") != std::string::npos)
            mb.send_args[key] = dev_addr[key];
    }

    UHD_LOGGER_DEBUG("X300") << "Setting up basic communication...";
    if (mb.xport_path == xport_path_t::NIRIO) {
        mb.conn_mgr = std::make_shared<pcie_manager>(mb.args, _tree, mb_path);
    } else {
        mb.conn_mgr = std::make_shared<eth_manager>(mb.args, _tree, mb_path);
    }
    mb.zpu_ctrl = mb.conn_mgr->get_ctrl_iface();

    // Claim device
    if (not try_to_claim(mb.zpu_ctrl)) {
        throw uhd::runtime_error("Failed to claim device");
    }
    mb.claimer_task =
        uhd::task::make([&mb]() { claimer_loop(mb.zpu_ctrl); }, "x300_claimer");

    // extract the FW path for the X300
    // and live load fw over ethernet link
    if (mb.args.has_fw_file()) {
        const std::string x300_fw_image = find_image_path(mb.args.get_fw_file());
        x300_load_fw(mb.zpu_ctrl, x300_fw_image);
    }

    // check compat numbers
    // check fpga compat before fw compat because the fw is a subset of the fpga image
    this->check_fpga_compat(mb_path, mb);
    this->check_fw_compat(mb_path, mb);

    mb.fw_regmap = boost::make_shared<fw_regmap_t>();
    mb.fw_regmap->initialize(*mb.zpu_ctrl.get(), true);

    // store which FPGA image is loaded
    mb.loaded_fpga_image = get_fpga_option(mb.zpu_ctrl);

    // low speed perif access
    mb.zpu_spi = spi_core_3000::make(
        mb.zpu_ctrl, SR_ADDR(SET0_BASE, ZPU_SR_SPI), SR_ADDR(SET0_BASE, ZPU_RB_SPI));
    mb.zpu_i2c = i2c_core_100_wb32::make(mb.zpu_ctrl, I2C1_BASE);
    mb.zpu_i2c->set_clock_rate(x300::BUS_CLOCK_RATE / 2);

    ////////////////////////////////////////////////////////////////////
    // print network routes mapping
    ////////////////////////////////////////////////////////////////////
    /*
    const uint32_t routes_addr = mb.zpu_ctrl->peek32(SR_ADDR(X300_FW_SHMEM_BASE,
    X300_FW_SHMEM_ROUTE_MAP_ADDR)); const uint32_t routes_len =
    mb.zpu_ctrl->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_ROUTE_MAP_LEN));
    UHD_VAR(routes_len);
    for (size_t i = 0; i < routes_len; i+=1)
    {
        const uint32_t node_addr = mb.zpu_ctrl->peek32(SR_ADDR(routes_addr, i*2+0));
        const uint32_t nbor_addr = mb.zpu_ctrl->peek32(SR_ADDR(routes_addr, i*2+1));
        if (node_addr != 0 and nbor_addr != 0)
        {
            UHD_LOGGER_INFO("X300") << boost::format("%u: %s -> %s")
                % i
                % asio::ip::address_v4(node_addr).to_string()
                % asio::ip::address_v4(nbor_addr).to_string();
        }
    }
    */

    ////////////////////////////////////////////////////////////////////
    // setup the mboard eeprom
    ////////////////////////////////////////////////////////////////////
    UHD_LOGGER_DEBUG("X300") << "Loading values from EEPROM...";
    x300_mb_eeprom_iface::sptr eeprom16 =
        x300_mb_eeprom_iface::make(mb.zpu_ctrl, mb.zpu_i2c);
    if (mb.args.get_blank_eeprom()) {
        UHD_LOGGER_WARNING("X300") << "Obliterating the motherboard EEPROM...";
        eeprom16->write_eeprom(0x50, 0, byte_vector_t(256, 0xff));
    }

    const mboard_eeprom_t mb_eeprom = get_mb_eeprom(eeprom16);
    _tree
        ->create<mboard_eeprom_t>(mb_path / "eeprom")
        // Initialize the property with a current copy of the EEPROM contents
        .set(mb_eeprom)
        // Whenever this property is written, update the chip
        .add_coerced_subscriber([eeprom16](const mboard_eeprom_t& mb_eeprom) {
            set_mb_eeprom(eeprom16, mb_eeprom);
        });

    if (mb.args.get_recover_mb_eeprom()) {
        UHD_LOGGER_WARNING("X300")
            << "UHD is operating in EEPROM Recovery Mode which disables hardware version "
               "checks.\nOperating in this mode may cause hardware damage and unstable "
               "radio performance!";
        return;
    }

    ////////////////////////////////////////////////////////////////////
    // parse the product number
    ////////////////////////////////////////////////////////////////////
    const std::string product_name =
        map_mb_type_to_product_name(get_mb_type_from_eeprom(mb_eeprom), "X300?");
    if (product_name == "X300?") {
        if (not mb.args.get_recover_mb_eeprom()) {
            throw uhd::runtime_error(
                "Unrecognized product type.\n"
                "Either the software does not support this device in which "
                "case please update your driver software to the latest version "
                "and retry OR\n"
                "The product code in the EEPROM is corrupt and may require "
                "reprogramming.");
        }
    }
    _tree->create<std::string>(mb_path / "name").set(product_name);
    _tree->create<std::string>(mb_path / "codename").set("Yetti");

    ////////////////////////////////////////////////////////////////////
    // discover interfaces, frame sizes, and link rates
    ////////////////////////////////////////////////////////////////////
    if (mb.xport_path == xport_path_t::NIRIO) {
        std::dynamic_pointer_cast<pcie_manager>(mb.conn_mgr)->init_link();
    } else if (mb.xport_path == xport_path_t::ETH) {
        std::dynamic_pointer_cast<eth_manager>(mb.conn_mgr)
            ->init_link(mb_eeprom, mb.loaded_fpga_image);
    }

    ////////////////////////////////////////////////////////////////////
    // read hardware revision and compatibility number
    ////////////////////////////////////////////////////////////////////
    mb.hw_rev = get_and_check_hw_rev(mb_eeprom);

    ////////////////////////////////////////////////////////////////////
    // create clock control objects
    ////////////////////////////////////////////////////////////////////
    UHD_LOGGER_DEBUG("X300") << "Setting up RF frontend clocking...";

    // Initialize clock control registers. NOTE: This does not configure the LMK yet.
    mb.clock = x300_clock_ctrl::make(mb.zpu_spi,
        1 /*slaveno*/,
        mb.hw_rev,
        mb.args.get_master_clock_rate(),
        mb.args.get_dboard_clock_rate(),
        mb.args.get_system_ref_rate());
    mb.fw_regmap->ref_freq_reg.write(
        fw_regmap_t::ref_freq_reg_t::REF_FREQ, uint32_t(mb.args.get_system_ref_rate()));

    // Initialize clock source to use internal reference and generate
    // a valid radio clock. This may change after configuration is done.
    // This will configure the LMK and wait for lock
    update_clock_source(mb, mb.args.get_clock_source());

    ////////////////////////////////////////////////////////////////////
    // create clock properties
    ////////////////////////////////////////////////////////////////////
    _tree->create<double>(mb_path / "master_clock_rate").set_publisher([&mb]() {
        return mb.clock->get_master_clock_rate();
    });

    UHD_LOGGER_INFO("X300") << "Radio 1x clock: "
                            << (mb.clock->get_master_clock_rate() / 1e6) << " MHz";

    ////////////////////////////////////////////////////////////////////
    // Create the GPSDO control
    ////////////////////////////////////////////////////////////////////
    static constexpr uint32_t dont_look_for_gpsdo = 0x1234abcdul;

    // otherwise if not disabled, look for the internal GPSDO
    if (mb.zpu_ctrl->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_GPSDO_STATUS))
        != dont_look_for_gpsdo) {
        UHD_LOG_DEBUG("X300", "Detecting internal GPSDO....");
        try {
            // gps_ctrl will print its own log statements if a GPSDO was found
            mb.gps = gps_ctrl::make(x300_make_uart_iface(mb.zpu_ctrl));
        } catch (std::exception& e) {
            UHD_LOGGER_ERROR("X300")
                << "An error occurred making GPSDO control: " << e.what();
        }
        if (mb.gps and mb.gps->gps_detected()) {
            for (const std::string& name : mb.gps->get_sensors()) {
                _tree->create<sensor_value_t>(mb_path / "sensors" / name)
                    .set_publisher([&mb, name]() { return mb.gps->get_sensor(name); });
            }
        } else {
            mb.zpu_ctrl->poke32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_GPSDO_STATUS),
                dont_look_for_gpsdo);
        }
    }

    ////////////////////////////////////////////////////////////////////
    // setup time sources and properties
    ////////////////////////////////////////////////////////////////////
    _tree->create<std::string>(mb_path / "time_source" / "value")
        .set(mb.args.get_time_source())
        .add_coerced_subscriber([this, &mb](const std::string& time_source) {
            this->update_time_source(mb, time_source);
        });
    _tree->create<std::vector<std::string>>(mb_path / "time_source" / "options")
        .set(TIME_SOURCE_OPTIONS);

    // setup the time output, default to ON
    _tree->create<bool>(mb_path / "time_source" / "output")
        .add_coerced_subscriber([this, &mb](const bool time_output) {
            this->set_time_source_out(mb, time_output);
        })
        .set(true);

    ////////////////////////////////////////////////////////////////////
    // setup clock sources and properties
    ////////////////////////////////////////////////////////////////////
    _tree->create<std::string>(mb_path / "clock_source" / "value")
        .set(mb.args.get_clock_source())
        .add_coerced_subscriber([this, &mb](const std::string& clock_source) {
            this->update_clock_source(mb, clock_source);
        });
    _tree->create<std::vector<std::string>>(mb_path / "clock_source" / "options")
        .set(CLOCK_SOURCE_OPTIONS);

    // setup external reference options. default to 10 MHz input reference
    _tree->create<std::string>(mb_path / "clock_source" / "external");
    _tree
        ->create<std::vector<double>>(
            mb_path / "clock_source" / "external" / "freq" / "options")
        .set(x300::EXTERNAL_FREQ_OPTIONS);
    _tree->create<double>(mb_path / "clock_source" / "external" / "value")
        .set(mb.clock->get_sysref_clock_rate());
    // FIXME the external clock source settings need to be more robust

    // setup the clock output, default to ON
    _tree->create<bool>(mb_path / "clock_source" / "output")
        .add_coerced_subscriber(
            [&mb](const bool clock_output) { mb.clock->set_ref_out(clock_output); });

    // Initialize tick rate (must be done before setting time)
    // Note: The master tick rate can't be changed at runtime!
    const double master_clock_rate = mb.clock->get_master_clock_rate();
    _tree->create<double>(mb_path / "tick_rate")
        .set_coercer([master_clock_rate](const double rate) {
            // The contract of multi_usrp::set_master_clock_rate() is to coerce
            // and not throw, so we'll follow that behaviour here.
            if (!uhd::math::frequencies_are_equal(rate, master_clock_rate)) {
                UHD_LOGGER_WARNING("X300")
                    << "Cannot update master clock rate! X300 Series does not "
                       "allow changing the clock rate during runtime.";
            }
            return master_clock_rate;
        })
        .add_coerced_subscriber([this](const double) { this->update_tx_streamers(); })
        .add_coerced_subscriber([this](const double) { this->update_rx_streamers(); })
        .set(master_clock_rate);

    ////////////////////////////////////////////////////////////////////
    // and do the misc mboard sensors
    ////////////////////////////////////////////////////////////////////
    _tree->create<sensor_value_t>(mb_path / "sensors" / "ref_locked")
        .set_publisher([this, &mb]() { return this->get_ref_locked(mb); });

    //////////////// RFNOC /////////////////
    const size_t n_rfnoc_blocks = mb.zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_NUM_CE));
    enumerate_rfnoc_blocks(mb_i,
        n_rfnoc_blocks,
        x300::XB_DST_PCI + 1, /* base port */
        uhd::sid_t(x300::SRC_ADDR0, 0, x300::DST_ADDR + mb_i, 0),
        dev_addr);
    //////////////// RFNOC /////////////////

    // If we have a radio, we must configure its codec control:
    const std::string radio_blockid_hint = str(boost::format("%d/Radio") % mb_i);
    std::vector<rfnoc::block_id_t> radio_ids =
        find_blocks<rfnoc::x300_radio_ctrl_impl>(radio_blockid_hint);
    if (not radio_ids.empty()) {
        if (radio_ids.size() > 2) {
            UHD_LOGGER_WARNING("X300")
                << "Too many Radio Blocks found. Using only the first two.";
            radio_ids.resize(2);
        }

        for (const rfnoc::block_id_t& id : radio_ids) {
            rfnoc::x300_radio_ctrl_impl::sptr radio(
                get_block_ctrl<rfnoc::x300_radio_ctrl_impl>(id));
            mb.radios.push_back(radio);
            radio->setup_radio(mb.zpu_i2c,
                mb.clock,
                mb.args.get_ignore_cal_file(),
                mb.args.get_self_cal_adc_delay());
        }

        ////////////////////////////////////////////////////////////////////
        // ADC test and cal
        ////////////////////////////////////////////////////////////////////
        if (mb.args.get_self_cal_adc_delay()) {
            rfnoc::x300_radio_ctrl_impl::self_cal_adc_xfer_delay(mb.radios,
                mb.clock,
                [this, &mb](const double timeout) {
                    return this->wait_for_clk_locked(
                        mb, fw_regmap_t::clk_status_reg_t::LMK_LOCK, timeout);
                },
                true /* Apply ADC delay */);
        }
        if (mb.args.get_ext_adc_self_test()) {
            rfnoc::x300_radio_ctrl_impl::extended_adc_test(
                mb.radios, mb.args.get_ext_adc_self_test_duration());
        } else {
            for (size_t i = 0; i < mb.radios.size(); i++) {
                mb.radios.at(i)->self_test_adc();
            }
        }

        ////////////////////////////////////////////////////////////////////
        // Synchronize times (dboard initialization can desynchronize them)
        ////////////////////////////////////////////////////////////////////
        if (radio_ids.size() == 2) {
            this->sync_times(mb, mb.radios[0]->get_time_now());
        }

    } else {
        UHD_LOGGER_INFO("X300") << "No Radio Block found. Assuming radio-less operation.";
    } /* end of radio block(s) initialization */

    mb.initialization_done = true;
}

x300_impl::~x300_impl(void)
{
    try {
        for (mboard_members_t& mb : _mb) {
            // kill the claimer task and unclaim the device
            mb.claimer_task.reset();
            if (mb.xport_path == xport_path_t::NIRIO) {
                std::dynamic_pointer_cast<pcie_manager>(mb.conn_mgr)
                    ->release_ctrl_iface([&mb]() { release(mb.zpu_ctrl); });
            } else {
                release(mb.zpu_ctrl);
            }
        }
    } catch (...) {
        UHD_SAFE_CALL(throw;)
    }
}

uhd::both_xports_t x300_impl::make_transport(const uhd::sid_t& address,
    const xport_type_t xport_type,
    const uhd::device_addr_t& args)
{
    const size_t mb_index = address.get_dst_addr() - x300::DST_ADDR;
    mboard_members_t& mb  = _mb[mb_index];
    both_xports_t xports;

    // Calculate MTU based on MTU in args and device limitations
    const size_t send_mtu = args.cast<size_t>("mtu",
        get_mtu(mb_index, uhd::TX_DIRECTION));
    const size_t recv_mtu = args.cast<size_t>("mtu",
        get_mtu(mb_index, uhd::RX_DIRECTION));

    if (mb.xport_path == xport_path_t::NIRIO) {
        xports.send_sid =
            this->allocate_sid(mb, address, x300::SRC_ADDR0, x300::XB_DST_PCI);
        xports.recv_sid = xports.send_sid.reversed();
        return std::dynamic_pointer_cast<pcie_manager>(mb.conn_mgr)
            ->make_transport(xports, xport_type, args, send_mtu, recv_mtu);
    } else if (mb.xport_path == xport_path_t::ETH) {
        xports = std::dynamic_pointer_cast<eth_manager>(mb.conn_mgr)
                     ->make_transport(xports,
                         xport_type,
                         args,
                         send_mtu,
                         recv_mtu,
                         [this, &mb, address](
                             const uint32_t src_addr, const uint32_t src_dst) {
                             return this->allocate_sid(mb, address, src_addr, src_dst);
                         });

        // reprogram the ethernet dispatcher's udp port (should be safe to always set)
        UHD_LOGGER_TRACE("X300")
            << "reprogram the ethernet dispatcher's udp port to " << X300_VITA_UDP_PORT;
        mb.zpu_ctrl->poke32(
            SR_ADDR(SET0_BASE, (ZPU_SR_ETHINT0 + 8 + 3)), X300_VITA_UDP_PORT);
        mb.zpu_ctrl->poke32(
            SR_ADDR(SET0_BASE, (ZPU_SR_ETHINT1 + 8 + 3)), X300_VITA_UDP_PORT);

        // Do a peek to an arbitrary address to guarantee that the
        // ethernet framer has been programmed before we return.
        mb.zpu_ctrl->peek32(0);

        return xports;
    }
    UHD_THROW_INVALID_CODE_PATH();
}


uhd::sid_t x300_impl::allocate_sid(mboard_members_t& mb,
    const uhd::sid_t& address,
    const uint32_t src_addr,
    const uint32_t src_dst)
{
    uhd::sid_t sid = address;
    sid.set_src_addr(src_addr);
    sid.set_src_endpoint(_sid_framer++); // increment for next setup

    // TODO Move all of this setup_mb()
    // Program the X300 to recognise it's own local address.
    mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_XB_LOCAL), address.get_dst_addr());
    // Program CAM entry for outgoing packets matching a X300 resource (for example a
    // Radio) This type of packet matches the XB_LOCAL address and is looked up in the
    // upper half of the CAM
    mb.zpu_ctrl->poke32(SR_ADDR(SETXB_BASE, 256 + address.get_dst_endpoint()),
        address.get_dst_xbarport());
    // Program CAM entry for returning packets to us (for example GR host via Eth0)
    // This type of packet does not match the XB_LOCAL address and is looked up in the
    // lower half of the CAM
    mb.zpu_ctrl->poke32(SR_ADDR(SETXB_BASE, 0 + src_addr), src_dst);

    UHD_LOGGER_TRACE("X300") << "done router config for sid " << sid;

    return sid;
}

/***********************************************************************
 * clock and time control logic
 **********************************************************************/
void x300_impl::set_time_source_out(mboard_members_t& mb, const bool enb)
{
    mb.fw_regmap->clock_ctrl_reg.write(
        fw_regmap_t::clk_ctrl_reg_t::PPS_OUT_EN, enb ? 1 : 0);
}

void x300_impl::update_clock_source(mboard_members_t& mb, const std::string& source)
{
    // Optimize for the case when the current source is internal and we are trying
    // to set it to internal. This is the only case where we are guaranteed that
    // the clock has not gone away so we can skip setting the MUX and reseting the LMK.
    const bool reconfigure_clks = (mb.current_refclk_src != "internal")
                                  or (source != "internal");
    if (reconfigure_clks) {
        // Update the clock MUX on the motherboard to select the requested source
        if (source == "internal") {
            mb.fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::CLK_SOURCE,
                fw_regmap_t::clk_ctrl_reg_t::SRC_INTERNAL);
            mb.fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::TCXO_EN, 1);
        } else if (source == "external") {
            mb.fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::CLK_SOURCE,
                fw_regmap_t::clk_ctrl_reg_t::SRC_EXTERNAL);
            mb.fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::TCXO_EN, 0);
        } else if (source == "gpsdo") {
            mb.fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::CLK_SOURCE,
                fw_regmap_t::clk_ctrl_reg_t::SRC_GPSDO);
            mb.fw_regmap->clock_ctrl_reg.set(fw_regmap_t::clk_ctrl_reg_t::TCXO_EN, 0);
        } else {
            throw uhd::key_error("update_clock_source: unknown source: " + source);
        }
        mb.fw_regmap->clock_ctrl_reg.flush();

        // Reset the LMK to make sure it re-locks to the new reference
        mb.clock->reset_clocks();
    }

    // Wait for the LMK to lock (always, as a sanity check that the clock is useable)
    //* Currently the LMK can take as long as 30 seconds to lock to a reference but we
    // don't
    //* want to wait that long during initialization.
    // TODO: Need to verify timeout and settings to make sure lock can be achieved in
    // < 1.0 seconds
    double timeout = mb.initialization_done ? 30.0 : 1.0;

    // The programming code in x300_clock_ctrl is not compatible with revs <= 4 and may
    // lead to locking issues. So, disable the ref-locked check for older (unsupported)
    // boards.
    if (mb.hw_rev > 4) {
        if (not wait_for_clk_locked(
                mb, fw_regmap_t::clk_status_reg_t::LMK_LOCK, timeout)) {
            // failed to lock on reference
            if (mb.initialization_done) {
                throw uhd::runtime_error(
                    (boost::format("Reference Clock PLL failed to lock to %s source.")
                        % source)
                        .str());
            } else {
                // TODO: Re-enable this warning when we figure out a reliable lock time
                // UHD_LOGGER_WARNING("X300") << "Reference clock failed to lock to " +
                // source + " during device initialization.  " <<
                //    "Check for the lock before operation or ignore this warning if using
                //    another clock source." ;
            }
        }
    }

    if (reconfigure_clks) {
        // Reset the radio clock PLL in the FPGA
        mb.zpu_ctrl->poke32(
            SR_ADDR(SET0_BASE, ZPU_SR_SW_RST), ZPU_SR_SW_RST_RADIO_CLK_PLL);
        mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_SW_RST), 0);

        // Wait for radio clock PLL to lock
        if (not wait_for_clk_locked(
                mb, fw_regmap_t::clk_status_reg_t::RADIO_CLK_LOCK, 0.01)) {
            throw uhd::runtime_error(
                (boost::format("Reference Clock PLL in FPGA failed to lock to %s source.")
                    % source)
                    .str());
        }

        // Reset the IDELAYCTRL used to calibrate the data interface delays
        mb.zpu_ctrl->poke32(
            SR_ADDR(SET0_BASE, ZPU_SR_SW_RST), ZPU_SR_SW_RST_ADC_IDELAYCTRL);
        mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_SW_RST), 0);

        // Wait for the ADC IDELAYCTRL to be ready
        if (not wait_for_clk_locked(
                mb, fw_regmap_t::clk_status_reg_t::IDELAYCTRL_LOCK, 0.01)) {
            throw uhd::runtime_error(
                (boost::format(
                     "ADC Calibration Clock in FPGA failed to lock to %s source.")
                    % source)
                    .str());
        }

        // Reset ADCs and DACs
        for (rfnoc::x300_radio_ctrl_impl::sptr r : mb.radios) {
            r->reset_codec();
        }
    }

    // Update cache value
    mb.current_refclk_src = source;
}

void x300_impl::update_time_source(mboard_members_t& mb, const std::string& source)
{
    if (source == "internal") {
        mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::PPS_SELECT,
            fw_regmap_t::clk_ctrl_reg_t::SRC_INTERNAL);
    } else if (source == "external") {
        mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::PPS_SELECT,
            fw_regmap_t::clk_ctrl_reg_t::SRC_EXTERNAL);
    } else if (source == "gpsdo") {
        mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::PPS_SELECT,
            fw_regmap_t::clk_ctrl_reg_t::SRC_GPSDO);
    } else {
        throw uhd::key_error("update_time_source: unknown source: " + source);
    }

    /* TODO - Implement intelligent PPS detection
    //check for valid pps
    if (!is_pps_present(mb)) {
        throw uhd::runtime_error((boost::format("The %d PPS was not detected.  Please
    check the PPS source and try again.") % source).str());
    }
    */
}

void x300_impl::sync_times(mboard_members_t& mb, const uhd::time_spec_t& t)
{
    std::vector<rfnoc::block_id_t> radio_ids =
        find_blocks<rfnoc::x300_radio_ctrl_impl>("Radio");
    for (const rfnoc::block_id_t& id : radio_ids) {
        get_block_ctrl<rfnoc::x300_radio_ctrl_impl>(id)->set_time_sync(t);
    }

    mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::TIME_SYNC, 0);
    mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::TIME_SYNC, 1);
    mb.fw_regmap->clock_ctrl_reg.write(fw_regmap_t::clk_ctrl_reg_t::TIME_SYNC, 0);
}

bool x300_impl::wait_for_clk_locked(mboard_members_t& mb, uint32_t which, double timeout)
{
    const auto timeout_time = std::chrono::steady_clock::now()
                              + std::chrono::milliseconds(int64_t(timeout * 1000));
    do {
        if (mb.fw_regmap->clock_status_reg.read(which) == 1) {
            return true;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } while (std::chrono::steady_clock::now() < timeout_time);

    // Check one last time
    return (mb.fw_regmap->clock_status_reg.read(which) == 1);
}

sensor_value_t x300_impl::get_ref_locked(mboard_members_t& mb)
{
    mb.fw_regmap->clock_status_reg.refresh();
    const bool lock =
        (mb.fw_regmap->clock_status_reg.get(fw_regmap_t::clk_status_reg_t::LMK_LOCK) == 1)
        && (mb.fw_regmap->clock_status_reg.get(
                fw_regmap_t::clk_status_reg_t::RADIO_CLK_LOCK)
               == 1)
        && (mb.fw_regmap->clock_status_reg.get(
                fw_regmap_t::clk_status_reg_t::IDELAYCTRL_LOCK)
               == 1);
    return sensor_value_t("Ref", lock, "locked", "unlocked");
}

bool x300_impl::is_pps_present(mboard_members_t& mb)
{
    // The ZPU_RB_CLK_STATUS_PPS_DETECT bit toggles with each rising edge of the PPS.
    // We monitor it for up to 1.5 seconds looking for it to toggle.
    uint32_t pps_detect =
        mb.fw_regmap->clock_status_reg.read(fw_regmap_t::clk_status_reg_t::PPS_DETECT);
    for (int i = 0; i < 15; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (pps_detect
            != mb.fw_regmap->clock_status_reg.read(
                   fw_regmap_t::clk_status_reg_t::PPS_DETECT))
            return true;
    }
    return false;
}

/***********************************************************************
 * Frame size detection
 **********************************************************************/
size_t x300_impl::get_mtu(const size_t mb_index, const uhd::direction_t dir)
{
    auto& mb = _mb.at(mb_index);
    return mb.conn_mgr->get_mtu(dir);
}

/***********************************************************************
 * compat checks
 **********************************************************************/
void x300_impl::check_fw_compat(const fs_path& mb_path, const mboard_members_t& members)
{
    auto iface = members.zpu_ctrl;
    const uint32_t compat_num =
        iface->peek32(SR_ADDR(X300_FW_SHMEM_BASE, X300_FW_SHMEM_COMPAT_NUM));
    const uint32_t compat_major = (compat_num >> 16);
    const uint32_t compat_minor = (compat_num & 0xffff);

    if (compat_major != X300_FW_COMPAT_MAJOR) {
        const std::string image_loader_path =
            (fs::path(uhd::get_pkg_path()) / "bin" / "uhd_image_loader").string();
        const std::string image_loader_cmd = str(
            boost::format("\"%s\" --args=\"type=x300,%s=%s\"") % image_loader_path
            % (members.xport_path == xport_path_t::ETH ? "addr" : "resource")
            % (members.xport_path == xport_path_t::ETH ? members.args.get_first_addr()
                                                       : members.args.get_resource()));

        throw uhd::runtime_error(
            str(boost::format(
                    "Expected firmware compatibility number %d, but got %d:\n"
                    "The FPGA/firmware image on your device is not compatible with this "
                    "host code build.\n"
                    "Download the appropriate FPGA images for this version of UHD.\n"
                    "%s\n\n"
                    "Then burn a new image to the on-board flash storage of your\n"
                    "USRP X3xx device using the image loader utility. "
                    "Use this command:\n\n%s\n\n"
                    "For more information, refer to the UHD manual:\n\n"
                    " http://files.ettus.com/manual/page_usrp_x3x0.html#x3x0_flash")
                % int(X300_FW_COMPAT_MAJOR) % compat_major
                % print_utility_error("uhd_images_downloader.py") % image_loader_cmd));
    }
    _tree->create<std::string>(mb_path / "fw_version")
        .set(str(boost::format("%u.%u") % compat_major % compat_minor));
}

void x300_impl::check_fpga_compat(const fs_path& mb_path, const mboard_members_t& members)
{
    uint32_t compat_num = members.zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_COMPAT_NUM));
    uint32_t compat_major = (compat_num >> 16);
    uint32_t compat_minor = (compat_num & 0xffff);

    if (compat_major != X300_FPGA_COMPAT_MAJOR) {
        std::string image_loader_path =
            (fs::path(uhd::get_pkg_path()) / "bin" / "uhd_image_loader").string();
        std::string image_loader_cmd = str(
            boost::format("\"%s\" --args=\"type=x300,%s=%s\"") % image_loader_path
            % (members.xport_path == xport_path_t::ETH ? "addr" : "resource")
            % (members.xport_path == xport_path_t::ETH ? members.args.get_first_addr()
                                                       : members.args.get_resource()));

        throw uhd::runtime_error(
            str(boost::format(
                    "Expected FPGA compatibility number %d, but got %d:\n"
                    "The FPGA image on your device is not compatible with this host code "
                    "build.\n"
                    "Download the appropriate FPGA images for this version of UHD.\n"
                    "%s\n\n"
                    "Then burn a new image to the on-board flash storage of your\n"
                    "USRP X3xx device using the image loader utility. Use this "
                    "command:\n\n%s\n\n"
                    "For more information, refer to the UHD manual:\n\n"
                    " http://files.ettus.com/manual/page_usrp_x3x0.html#x3x0_flash")
                % int(X300_FPGA_COMPAT_MAJOR) % compat_major
                % print_utility_error("uhd_images_downloader.py") % image_loader_cmd));
    }
    _tree->create<std::string>(mb_path / "fpga_version")
        .set(str(boost::format("%u.%u") % compat_major % compat_minor));

    const uint32_t git_hash =
        members.zpu_ctrl->peek32(SR_ADDR(SET0_BASE, ZPU_RB_GIT_HASH));
    const std::string git_hash_str = str(boost::format("%07x%s") % (git_hash & 0x0FFFFFFF)
                                         % ((git_hash & 0xF0000000) ? "-dirty" : ""));
    _tree->create<std::string>(mb_path / "fpga_version_hash").set(git_hash_str);
    UHD_LOG_DEBUG("X300",
        "Using FPGA version: " << compat_major << "." << compat_minor
                               << " git hash: " << git_hash_str);
}

