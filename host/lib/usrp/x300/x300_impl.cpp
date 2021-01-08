//
// Copyright 2013-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_impl.hpp"
#include "../dboard/db_ubx.hpp"
#include "x300_claim.hpp"
#include "x300_eth_mgr.hpp"
#include "x300_mb_controller.hpp"
#include "x300_mb_eeprom.hpp"
#include "x300_mb_eeprom_iface.hpp"
#include "x300_mboard_type.hpp"
#include "x300_pcie_mgr.hpp"
#include <uhd/transport/if_addrs.hpp>
#include <uhd/usrp/dboard_eeprom.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/safe_call.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/rfnoc/device_id.hpp>
#include <chrono>
#include <fstream>
#include <thread>
#ifdef HAVE_DPDK
#    include <uhdlib/transport/dpdk/common.hpp>
#endif

uhd::uart_iface::sptr x300_make_uart_iface(uhd::wb_iface::sptr iface);

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::rfnoc;
using namespace uhd::usrp::x300;
namespace asio = boost::asio;

namespace uhd { namespace usrp { namespace x300 {

void init_prop_tree(
    const size_t mb_idx, uhd::rfnoc::x300_mb_controller* mbc, property_tree::sptr pt);

}}} // namespace uhd::usrp::x300


const uhd::rfnoc::chdr::chdr_packet_factory x300_impl::_pkt_factory(
    CHDR_W_64, ENDIANNESS_LITTLE);


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
 * Daughterboard detection before initialization in software
 **********************************************************************/
static std::vector<dboard_id_t> get_dboard_ids(uhd::i2c_iface& zpu_i2c)
{
    std::vector<dboard_id_t> dboard_ids;
    // Read dboard ids from the EEPROM
    constexpr size_t BASE_ADDR      = 0x50;
    constexpr size_t RX_EEPROM_ADDR = 0x5;
    constexpr size_t TX_EEPROM_ADDR = 0x4;
    static const std::vector<size_t> DB_OFFSETS{0x0, 0x2};
    static const std::vector<size_t> EEPROM_ADDRS{RX_EEPROM_ADDR, TX_EEPROM_ADDR};
    for (size_t eeprom_addr : EEPROM_ADDRS) {
        for (size_t db_offset : DB_OFFSETS) {
            const size_t addr = eeprom_addr + db_offset;
            // Load EEPROM
            std::unordered_map<size_t, usrp::dboard_eeprom_t> db_eeproms;
            db_eeproms[addr].load(zpu_i2c, BASE_ADDR | addr);
            uint16_t dboard_id = db_eeproms[addr].id.to_uint16();
            dboard_ids.push_back(static_cast<dboard_id_t>(dboard_id));
        }
    }
    return dboard_ids;
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

x300_impl::x300_impl(const uhd::device_addr_t& dev_addr) : rfnoc_device()
{
    UHD_LOGGER_INFO("X300") << "X300 initialization sequence...";

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
    const fs_path mb_path = fs_path("/mboards") / mb_i;
    mboard_members_t& mb  = _mb[mb_i];
    mb.args.parse(dev_addr);
    mb.xport_path = dev_addr.has_key("resource") ? xport_path_t::NIRIO
                                                 : xport_path_t::ETH;
    for (const std::string& key : dev_addr.keys()) {
        if (key.find("recv") != std::string::npos)
            mb.recv_args[key] = dev_addr[key];
        if (key.find("send") != std::string::npos)
            mb.send_args[key] = dev_addr[key];
    }

    mb.device_id = allocate_device_id();
    UHD_LOG_DEBUG(
        "X300", "Motherboard " << mb_i << " has remote device ID: " << mb.device_id);

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

    // store which FPGA image is loaded
    mb.loaded_fpga_image = get_fpga_option(mb.zpu_ctrl);

    // low speed perif access
    mb.zpu_spi = spi_core_3000::make(
        mb.zpu_ctrl, SR_ADDR(SET0_BASE, ZPU_SR_SPI), SR_ADDR(SET0_BASE, ZPU_RB_SPI));
    mb.zpu_i2c = i2c_core_100_wb32::make(mb.zpu_ctrl, I2C1_BASE);
    mb.zpu_i2c->set_clock_rate(x300::BUS_CLOCK_RATE / 2);

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

    // The default daughterboard clock rate may have to be overridden. This is due to the
    // limitation on X300 devices where both daughterboards must use the same clock rate.
    // The daughterboards that require specific clock rates are UBX and TwinRX. TwinRX
    // requires a clock rate of 100 MHz for the best RF performance. UBX daughterboards
    // require a clock rate of no more than the max pfd frequency to maintain phase
    // synchronization. If there is no UBX, the default daughterboard clock rate is half
    // of the master clock rate for X300.
    const double x300_dboard_clock_rate = [dev_addr, mb]() -> double {
        // Do not override use-specified dboard clock rates
        if (dev_addr.has_key("dboard_clock_rate")) {
            return mb.args.get_dboard_clock_rate();
        }
        const double mcr         = mb.args.get_master_clock_rate();
        double dboard_clock_rate = mb.args.get_dboard_clock_rate();
        // Check for UBX daughterboards
        std::vector<dboard_id_t> dboard_ids = get_dboard_ids(*mb.zpu_i2c);
        for (dboard_id_t dboard_id : dboard_ids) {
            if (std::find(
                    dboard::ubx::ubx_ids.begin(), dboard::ubx::ubx_ids.end(), dboard_id)
                != dboard::ubx::ubx_ids.end()) {
                double ubx_clock_rate = mcr;
                for (int i = 2; ubx_clock_rate > dboard::ubx::get_max_pfd_freq(dboard_id);
                     i++) {
                    ubx_clock_rate = mcr / i;
                }
                dboard_clock_rate = std::min(dboard_clock_rate, ubx_clock_rate);
            }
        }
        return dboard_clock_rate;
    }();

    // Initialize clock control registers.
    // NOTE: This does not configure the LMK yet.
    mb.clock = x300_clock_ctrl::make(mb.zpu_spi,
        1 /*slaveno*/,
        mb.hw_rev,
        mb.args.get_master_clock_rate(),
        x300_dboard_clock_rate,
        mb.args.get_system_ref_rate());

    ////////////////////////////////////////////////////////////////////
    // create motherboard controller
    ////////////////////////////////////////////////////////////////////
    // Now we have all the peripherals, create the MB controller. It will also
    // initialize the clock source, and the time source.
    auto mb_ctrl = std::make_shared<x300_mb_controller>(
        mb.hw_rev, product_name, mb.zpu_i2c, mb.zpu_ctrl, mb.clock, mb_eeprom, mb.args);

    register_mb_controller(mb_i, mb_ctrl);
    // Clock should be up now!
    UHD_LOGGER_INFO("X300") << "Radio 1x clock: "
                            << (mb.clock->get_master_clock_rate() / 1e6) << " MHz";

    ////////////////////////////////////////////////////////////////////
    // setup properties
    ////////////////////////////////////////////////////////////////////
    init_prop_tree(mb_i, mb_ctrl.get(), _tree);

    ////////////////////////////////////////////////////////////////////
    // RFNoC Stuff
    ////////////////////////////////////////////////////////////////////
    // Set the remote device ID
    mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, ZPU_SR_XB_LOCAL), mb.device_id);
    // Configure the CHDR port number in the dispatcher
    mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, (ZPU_SR_ETHINT0 + 8 + 3)), X300_VITA_UDP_PORT);
    mb.zpu_ctrl->poke32(SR_ADDR(SET0_BASE, (ZPU_SR_ETHINT1 + 8 + 3)), X300_VITA_UDP_PORT);
    // Peek to finish transaction
    mb.zpu_ctrl->peek32(0);

    { // Need to lock access to _mb_ifaces, so we can run setup_mb() in
      // parallel
        std::lock_guard<std::mutex> l(_mb_iface_mutex);
        _mb_ifaces.insert({mb_i,
            x300_mb_iface(mb.conn_mgr, mb.clock->get_master_clock_rate(), mb.device_id)});
        UHD_LOG_DEBUG("X300", "Motherboard " << mb_i << " has local device IDs: ");
        for (const auto local_dev_id : _mb_ifaces.at(mb_i).get_local_device_ids()) {
            UHD_LOG_DEBUG("X300", "* " << local_dev_id);
        }
    } // End of locked section

    mb_ctrl->set_initialization_done();
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
