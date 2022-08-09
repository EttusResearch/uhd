//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "x300_pcie_mgr.hpp"
#include "x300_claim.hpp"
#include "x300_lvbitx.hpp"
#include "x300_mb_eeprom.hpp"
#include "x300_mb_eeprom_iface.hpp"
#include "x300_mboard_type.hpp"
#include "x300_regs.hpp"
#include "x310_lvbitx.hpp"
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/rfnoc/device_id.hpp>
#include <uhdlib/transport/inline_io_service.hpp>
#include <uhdlib/transport/nirio_link.hpp>
#include <uhdlib/usrp/cores/i2c_core_100_wb32.hpp>
#include <unordered_map>
#include <mutex>

using namespace uhd;
using namespace uhd::transport;
using namespace uhd::usrp::x300;
using namespace uhd::niusrprio;

namespace {

constexpr uint32_t RADIO_DEST_PREFIX_TX = 0;

// The FIFO closest to the DMA controller is 1023 elements deep for RX and 1029 elements
// deep for TX where an element is 8 bytes. The buffers (number of frames * frame size)
// must be aligned to the memory page size.  For the control, we are getting lucky because
// 64 frames * 256 bytes each aligns with the typical page size of 4096 bytes.  Since most
// page sizes are 4096 bytes or some multiple of that, keep the number of frames * frame
// size aligned to it.
constexpr size_t PCIE_RX_DATA_FRAME_SIZE = 4096; // bytes
constexpr size_t PCIE_RX_DATA_NUM_FRAMES = 4096;
constexpr size_t PCIE_TX_DATA_FRAME_SIZE = 4096; // bytes
constexpr size_t PCIE_TX_DATA_NUM_FRAMES = 4096;
constexpr size_t PCIE_MSG_FRAME_SIZE     = 256; // bytes
constexpr size_t PCIE_MSG_NUM_FRAMES     = 64;
constexpr size_t PCIE_MAX_CHANNELS       = 6;
// constexpr size_t MAX_RATE_PCIE               = 800000000; // bytes/s


//! Get default send/recv num frames and frame size per link type
link_params_t get_default_link_params(const link_type_t link_type)
{
    link_params_t link_params;
    switch (link_type) {
        case link_type_t::CTRL:
            link_params.send_frame_size = PCIE_MSG_FRAME_SIZE;
            link_params.recv_frame_size = PCIE_MSG_FRAME_SIZE;
            link_params.num_send_frames = PCIE_MSG_NUM_FRAMES;
            link_params.num_recv_frames = PCIE_MSG_NUM_FRAMES;
            break;
        case link_type_t::TX_DATA:
            link_params.send_frame_size = PCIE_TX_DATA_FRAME_SIZE;
            link_params.recv_frame_size = PCIE_MSG_FRAME_SIZE;
            link_params.num_send_frames = PCIE_TX_DATA_NUM_FRAMES;
            link_params.num_recv_frames = PCIE_MSG_NUM_FRAMES;
            break;
        case link_type_t::RX_DATA:
            link_params.send_frame_size = PCIE_MSG_FRAME_SIZE;
            link_params.recv_frame_size = PCIE_RX_DATA_FRAME_SIZE;
            link_params.num_send_frames = PCIE_MSG_NUM_FRAMES;
            link_params.num_recv_frames = PCIE_RX_DATA_NUM_FRAMES;
            break;
        default:
            UHD_THROW_INVALID_CODE_PATH();
    }
    link_params.recv_buff_size =
        link_params.num_recv_frames * link_params.recv_frame_size;
    link_params.send_buff_size =
        link_params.num_send_frames * link_params.send_frame_size;
    return link_params;
}

} // namespace

uhd::wb_iface::sptr x300_make_ctrl_iface_pcie(
    uhd::niusrprio::niriok_proxy::sptr drv_proxy, bool enable_errors = true);

// We need a zpu xport registry to ensure synchronization between the static
// finder method and the instances of the x300_impl class.
typedef std::unordered_map<std::string, std::weak_ptr<uhd::wb_iface>>
    pcie_zpu_iface_registry_t;
UHD_SINGLETON_FCN(pcie_zpu_iface_registry_t, get_pcie_zpu_iface_registry)
static std::mutex pcie_zpu_iface_registry_mutex;


/******************************************************************************
 * Static methods
 *****************************************************************************/
x300_mboard_t pcie_manager::get_mb_type_from_pcie(
    const std::string& resource, const std::string& rpc_port)
{
    // Detect the PCIe product ID to distinguish between X300 and X310
    nirio_status status = NiRio_Status_Success;
    uint32_t pid;
    niriok_proxy::sptr discovery_proxy =
        niusrprio_session::create_kernel_proxy(resource, rpc_port);
    if (discovery_proxy) {
        nirio_status_chain(
            discovery_proxy->get_attribute(RIO_PRODUCT_NUMBER, pid), status);
        discovery_proxy->close();
        if (nirio_status_not_fatal(status)) {
            return map_pid_to_mb_type(pid);
        }
    }

    UHD_LOG_WARNING("X300", "NI-RIO Error -- unable to determine motherboard type!");
    return UNKNOWN;
}

/******************************************************************************
 * Find
 *****************************************************************************/
device_addrs_t pcie_manager::find(const device_addr_t& hint, bool explicit_query)
{
    std::string rpc_port_name(std::to_string(NIUSRPRIO_DEFAULT_RPC_PORT));
    if (hint.has_key("niusrpriorpc_port")) {
        rpc_port_name = hint["niusrpriorpc_port"];
    }

    device_addrs_t addrs;
    niusrprio_session::device_info_vtr dev_info_vtr;
    nirio_status status = niusrprio_session::enumerate(rpc_port_name, dev_info_vtr);
    if (explicit_query) {
        nirio_status_to_exception(
            status, "x300::pcie_manager::find: Error enumerating NI-RIO devices.");
    }

    for (niusrprio_session::device_info& dev_info : dev_info_vtr) {
        device_addr_t new_addr;
        new_addr["type"]     = "x300";
        new_addr["resource"] = dev_info.resource_name;
        std::string resource_d(dev_info.resource_name);
        boost::to_upper(resource_d);

        const std::string product_name =
            map_mb_type_to_product_name(get_mb_type_from_pcie(resource_d, rpc_port_name));
        if (product_name.empty()) {
            continue;
        } else {
            new_addr["product"] = product_name;
        }

        niriok_proxy::sptr kernel_proxy =
            niriok_proxy::make_and_open(dev_info.interface_path);

        // Attempt to read the name from the EEPROM and perform filtering.
        // This operation can throw due to compatibility mismatch.
        try {
            // This block could throw an exception if the user is switching to using UHD
            // after LabVIEW FPGA. In that case, skip reading the name and serial and pick
            // a default FPGA flavor. During make, a new image will be loaded and
            // everything will be OK

            wb_iface::sptr zpu_ctrl;

            // Hold on to the registry mutex as long as zpu_ctrl is alive
            // to prevent any use by different threads while enumerating
            std::lock_guard<std::mutex> lock(pcie_zpu_iface_registry_mutex);

            if (get_pcie_zpu_iface_registry().count(resource_d)) {
                zpu_ctrl = get_pcie_zpu_iface_registry()[resource_d].lock();
                if (!zpu_ctrl) {
                    get_pcie_zpu_iface_registry().erase(resource_d);
                }
            }

            // if the registry didn't have a key OR that key was an orphaned weak_ptr
            if (!zpu_ctrl) {
                zpu_ctrl = x300_make_ctrl_iface_pcie(
                    kernel_proxy, false /* suppress timeout errors */);
                // We don't put this zpu_ctrl in the registry because we need
                // a persistent niriok_proxy associated with the object
            }

            // Attempt to autodetect the FPGA type
            if (not hint.has_key("fpga")) {
                new_addr["fpga"] = get_fpga_option(zpu_ctrl);
            }

            i2c_core_100_wb32::sptr zpu_i2c =
                i2c_core_100_wb32::make(zpu_ctrl, I2C1_BASE);
            x300_mb_eeprom_iface::sptr eeprom_iface =
                x300_mb_eeprom_iface::make(zpu_ctrl, zpu_i2c);
            const mboard_eeprom_t mb_eeprom = get_mb_eeprom(eeprom_iface);
            if (mb_eeprom.size() == 0 or claim_status(zpu_ctrl) == CLAIMED_BY_OTHER) {
                // Skip device claimed by another process
                continue;
            }
            new_addr["name"]   = mb_eeprom["name"];
            new_addr["serial"] = mb_eeprom["serial"];
        } catch (const std::exception&) {
            // set these values as empty string so the device may still be found
            // and the filter's below can still operate on the discovered device
            if (not hint.has_key("fpga")) {
                new_addr["fpga"] = "HG";
            }
            new_addr["name"]   = "";
            new_addr["serial"] = "";
        }

        // filter the discovered device below by matching optional keys
        std::string resource_i = hint.has_key("resource") ? hint["resource"] : "";
        boost::to_upper(resource_i);

        if ((not hint.has_key("resource") or resource_i == resource_d)
            and (not hint.has_key("name") or hint["name"] == new_addr["name"])
            and (not hint.has_key("serial") or hint["serial"] == new_addr["serial"])
            and (not hint.has_key("product") or hint["product"] == new_addr["product"])) {
            addrs.push_back(new_addr);
        }
    }
    return addrs;
}


/******************************************************************************
 * Structors
 *****************************************************************************/
pcie_manager::pcie_manager(
    const x300_device_args_t& args, uhd::property_tree::sptr, const uhd::fs_path&)
    : _args(args), _resource(args.get_resource())
{
    nirio_status status = 0;

    const std::string rpc_port_name = args.get_niusrprio_rpc_port();
    UHD_LOG_INFO(
        "X300", "Connecting to niusrpriorpc at localhost:" << rpc_port_name << "...");

    // Instantiate the correct lvbitx object
    nifpga_lvbitx::sptr lvbitx;
    switch (get_mb_type_from_pcie(args.get_resource(), rpc_port_name)) {
        case USRP_X300_MB:
            lvbitx.reset(new x300_lvbitx(args.get_fpga_option()));
            break;
        case USRP_X310_MB:
        case USRP_X310_MB_NI_2974:
            lvbitx.reset(new x310_lvbitx(args.get_fpga_option()));
            break;
        default:
            nirio_status_to_exception(
                status, "Motherboard detection error. Please ensure that you \
                    have a valid USRP X3x0, NI USRP-294xR, NI USRP-295xR or NI USRP-2974 device and that all the device \
                    drivers have loaded successfully.");
    }
    // Load the lvbitx onto the device
    UHD_LOG_INFO("X300", "Using LVBITX bitfile " << lvbitx->get_bitfile_path());
    _rio_fpga_interface.reset(new niusrprio_session(args.get_resource(), rpc_port_name));
    nirio_status_chain(
        _rio_fpga_interface->open(lvbitx, args.get_download_fpga()), status);
    nirio_status_to_exception(status, "x300_impl: Could not initialize RIO session.");

    // Tell the quirks object which FIFOs carry TX stream data
    const uint32_t tx_data_fifos[2] = {RADIO_DEST_PREFIX_TX, RADIO_DEST_PREFIX_TX + 3};
    _rio_fpga_interface->get_kernel_proxy()->get_rio_quirks().register_tx_streams(
        tx_data_fifos, 2);

    _local_device_id = rfnoc::allocate_device_id();
}

/******************************************************************************
 * API
 *****************************************************************************/
wb_iface::sptr pcie_manager::get_ctrl_iface()
{
    std::lock_guard<std::mutex> lock(pcie_zpu_iface_registry_mutex);
    if (get_pcie_zpu_iface_registry().count(_resource)) {
        throw uhd::assertion_error(
            "Someone else has a ZPU transport to the device open. Internal error!");
    }
    auto zpu_ctrl = x300_make_ctrl_iface_pcie(_rio_fpga_interface->get_kernel_proxy());
    get_pcie_zpu_iface_registry()[_resource] = std::weak_ptr<wb_iface>(zpu_ctrl);
    return zpu_ctrl;
}

void pcie_manager::init_link() {}

size_t pcie_manager::get_mtu(uhd::direction_t dir)
{
    return dir == uhd::RX_DIRECTION ? PCIE_RX_DATA_FRAME_SIZE : PCIE_TX_DATA_FRAME_SIZE;
}

void pcie_manager::release_ctrl_iface(std::function<void(void)>&& release_fn)
{
    std::lock_guard<std::mutex> lock(pcie_zpu_iface_registry_mutex);
    release_fn();
    // If the process is killed, the entire registry will disappear so we
    // don't need to worry about unclean shutdowns here.
    if (get_pcie_zpu_iface_registry().count(_resource)) {
        get_pcie_zpu_iface_registry().erase(_resource);
    }
}

uint32_t pcie_manager::allocate_pcie_dma_chan(
    const rfnoc::sep_id_t& remote_epid, const link_type_t link_type)
{
    constexpr uint32_t CTRL_CHANNEL       = 0;
    constexpr uint32_t FIRST_DATA_CHANNEL = 1;

    std::lock_guard<std::mutex> l(_dma_chan_mutex);
    uint32_t dma_chan = CTRL_CHANNEL;
    if (link_type == link_type_t::CTRL) {
        if (_dma_chan_pool.count(CTRL_CHANNEL)) {
            throw uhd::runtime_error("[X300] Cannot reallocate PCIe control channel!");
        }
    } else {
        dma_chan = FIRST_DATA_CHANNEL;
        while (_dma_chan_pool.count(dma_chan)) {
            dma_chan++;
        }
        if (dma_chan >= PCIE_MAX_CHANNELS) {
            throw uhd::runtime_error(
                "Trying to allocate more DMA channels than are available");
        }
    }

    _dma_chan_pool[dma_chan] = remote_epid;
    UHD_LOG_DEBUG("X300",
        "Assigning DMA channel " << dma_chan << " to remote EPID " << remote_epid);
    return dma_chan;
}

both_links_t pcie_manager::get_links(link_type_t link_type,
    const rfnoc::device_id_t local_device_id,
    const rfnoc::sep_id_t& /*local_epid*/,
    const rfnoc::sep_id_t& remote_epid,
    const device_addr_t& link_args)
{
    if (local_device_id != _local_device_id) {
        throw uhd::runtime_error(
            std::string("[X300] Cannot create NI-RIO link through local device ID ")
            + std::to_string(local_device_id)
            + ", no such device associated with this motherboard!");
    }

    const bool enable_fc = not link_args.has_key("enable_fc")
                           || uhd::cast::from_str<bool>(link_args.get("enable_fc"));

    const uint32_t dma_channel_num = allocate_pcie_dma_chan(remote_epid, link_type);
    // Note: The nirio_link object's factory has a lot of code for sanity
    // checking the link params, and merging the link_args with the default
    // link_params, so we use that.
    link_params_t link_params = get_default_link_params(link_type);

    // PCIe: Lossless, and little endian
    size_t recv_buff_size, send_buff_size;
    auto link = nirio_link::make(_rio_fpga_interface,
        dma_channel_num,
        link_params,
        link_args,
        recv_buff_size,
        send_buff_size);

    return std::make_tuple(link,
        send_buff_size,
        link,
        recv_buff_size,
        false /*not lossy*/,
        false,
        enable_fc);
}
