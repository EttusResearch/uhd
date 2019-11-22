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
#include <uhd/transport/nirio_zero_copy.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/usrp/cores/i2c_core_100_wb32.hpp>
#include <unordered_map>
#include <mutex>

namespace {

uint32_t extract_sid_from_pkt(void* pkt, size_t)
{
    return uhd::sid_t(uhd::wtohx(static_cast<const uint32_t*>(pkt)[1])).get_dst();
}

constexpr uint32_t RADIO_DEST_PREFIX_TX = 0;

// The FIFO closest to the DMA controller is 1023 elements deep for RX and 1029 elements
// deep for TX where an element is 8 bytes. The buffers (number of frames * frame size)
// must be aligned to the memory page size.  For the control, we are getting lucky because
// 64 frames * 256 bytes each aligns with the typical page size of 4096 bytes.  Since most
// page sizes are 4096 bytes or some multiple of that, keep the number of frames * frame
// size aligned to it.
constexpr size_t PCIE_RX_DATA_FRAME_SIZE     = 4096; // bytes
constexpr size_t PCIE_RX_DATA_NUM_FRAMES     = 4096;
constexpr size_t PCIE_TX_DATA_FRAME_SIZE     = 4096; // bytes
constexpr size_t PCIE_TX_DATA_NUM_FRAMES     = 4096;
constexpr size_t PCIE_MSG_FRAME_SIZE         = 256; // bytes
constexpr size_t PCIE_MSG_NUM_FRAMES         = 64;
constexpr size_t PCIE_MAX_MUXED_CTRL_XPORTS  = 32;
constexpr size_t PCIE_MAX_MUXED_ASYNC_XPORTS = 4;
constexpr size_t PCIE_MAX_CHANNELS           = 6;
constexpr size_t MAX_RATE_PCIE               = 800000000; // bytes/s

//! Default timeout value for receiving muxed control messages
constexpr double PCIE_DEFAULT_RECV_TIMEOUT_CTRL = 0.5; // seconds
//! Default timeout value for receiving muxed async messages
constexpr double PCIE_DEFAULT_RECV_TIMEOUT_ASYNC = 0.1; // seconds
}

uhd::wb_iface::sptr x300_make_ctrl_iface_pcie(
    uhd::niusrprio::niriok_proxy::sptr drv_proxy, bool enable_errors = true);

using namespace uhd;
using namespace uhd::transport;
using namespace uhd::usrp::x300;
using namespace uhd::niusrprio;

// We need a zpu xport registry to ensure synchronization between the static
// finder method and the instances of the x300_impl class.
typedef std::unordered_map<std::string, boost::weak_ptr<uhd::wb_iface>>
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
pcie_manager::pcie_manager(const x300_device_args_t& args,
    uhd::property_tree::sptr tree,
    const uhd::fs_path& root_path)
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

    tree->create<size_t>(root_path / "mtu/recv").set(PCIE_RX_DATA_FRAME_SIZE);
    tree->create<size_t>(root_path / "mtu/send").set(PCIE_TX_DATA_FRAME_SIZE);
    tree->create<double>(root_path / "link_max_rate").set(MAX_RATE_PCIE);
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
    get_pcie_zpu_iface_registry()[_resource] = boost::weak_ptr<wb_iface>(zpu_ctrl);
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
    const uhd::sid_t& tx_sid, const uhd::usrp::device3_impl::xport_type_t xport_type)
{
    constexpr uint32_t CTRL_CHANNEL       = 0;
    constexpr uint32_t ASYNC_MSG_CHANNEL  = 1;
    constexpr uint32_t FIRST_DATA_CHANNEL = 2;
    if (xport_type == uhd::usrp::device3_impl::CTRL) {
        return CTRL_CHANNEL;
    } else if (xport_type == uhd::usrp::device3_impl::ASYNC_MSG) {
        return ASYNC_MSG_CHANNEL;
    } else {
        // sid_t has no comparison defined, so we need to convert it uint32_t
        uint32_t raw_sid = tx_sid.get();

        if (_dma_chan_pool.count(raw_sid) == 0) {
            size_t channel = _dma_chan_pool.size() + FIRST_DATA_CHANNEL;
            if (channel > PCIE_MAX_CHANNELS) {
                throw uhd::runtime_error(
                    "Trying to allocate more DMA channels than are available");
            }
            _dma_chan_pool[raw_sid] = channel;
            UHD_LOGGER_DEBUG("X300")
                << "Assigning PCIe DMA channel " << _dma_chan_pool[raw_sid] << " to SID "
                << tx_sid.to_pp_string_hex();
        }

        return _dma_chan_pool[raw_sid];
    }
}

muxed_zero_copy_if::sptr pcie_manager::make_muxed_pcie_msg_xport(
    uint32_t dma_channel_num, size_t max_muxed_ports, const double recv_timeout_s)
{
    zero_copy_xport_params buff_args;
    buff_args.send_frame_size = PCIE_MSG_FRAME_SIZE;
    buff_args.recv_frame_size = PCIE_MSG_FRAME_SIZE;
    buff_args.num_send_frames = PCIE_MSG_NUM_FRAMES;
    buff_args.num_recv_frames = PCIE_MSG_NUM_FRAMES;

    zero_copy_if::sptr base_xport = nirio_zero_copy::make(
        _rio_fpga_interface, dma_channel_num, buff_args, uhd::device_addr_t());
    return muxed_zero_copy_if::make(
        base_xport, extract_sid_from_pkt, max_muxed_ports, recv_timeout_s);
}

both_xports_t pcie_manager::make_transport(both_xports_t xports,
    const uhd::usrp::device3_impl::xport_type_t xport_type,
    const uhd::device_addr_t& args,
    const size_t send_mtu,
    const size_t recv_mtu)
{
    zero_copy_xport_params default_buff_args;
    xports.endianness              = ENDIANNESS_LITTLE;
    xports.lossless                = true;
    const uint32_t dma_channel_num = allocate_pcie_dma_chan(xports.send_sid, xport_type);
    if (xport_type == uhd::usrp::device3_impl::CTRL) {
        // Transport for control stream
        if (not _ctrl_dma_xport) {
            const double recv_timeout = PCIE_DEFAULT_RECV_TIMEOUT_CTRL;
            // One underlying DMA channel will handle
            // all control traffic
            _ctrl_dma_xport = make_muxed_pcie_msg_xport(
                dma_channel_num, PCIE_MAX_MUXED_CTRL_XPORTS, recv_timeout);
        }
        // Create a virtual control transport
        xports.recv = _ctrl_dma_xport->make_stream(xports.recv_sid.get_dst());
    } else if (xport_type == uhd::usrp::device3_impl::ASYNC_MSG) {
        // Transport for async message stream
        if (not _async_msg_dma_xport) {
            const double recv_timeout = PCIE_DEFAULT_RECV_TIMEOUT_ASYNC;
            // One underlying DMA channel will handle
            // all async message traffic
            _async_msg_dma_xport = make_muxed_pcie_msg_xport(
                dma_channel_num, PCIE_MAX_MUXED_ASYNC_XPORTS, recv_timeout);
        }
        // Create a virtual async message transport
        xports.recv = _async_msg_dma_xport->make_stream(xports.recv_sid.get_dst());
    } else if (xport_type == uhd::usrp::device3_impl::TX_DATA) {
        default_buff_args.send_frame_size = args.cast<size_t>(
            "send_frame_size", std::min(send_mtu, PCIE_TX_DATA_FRAME_SIZE));
        default_buff_args.num_send_frames =
            args.cast<size_t>("num_send_frames", PCIE_TX_DATA_NUM_FRAMES);
        default_buff_args.send_buff_size  = args.cast<size_t>("send_buff_size", 0);
        default_buff_args.recv_frame_size = PCIE_MSG_FRAME_SIZE;
        default_buff_args.num_recv_frames = PCIE_MSG_NUM_FRAMES;
        xports.recv                       = nirio_zero_copy::make(
            _rio_fpga_interface, dma_channel_num, default_buff_args);
    } else if (xport_type == uhd::usrp::device3_impl::RX_DATA) {
        default_buff_args.send_frame_size = PCIE_MSG_FRAME_SIZE;
        default_buff_args.num_send_frames = PCIE_MSG_NUM_FRAMES;
        default_buff_args.recv_frame_size = args.cast<size_t>(
            "recv_frame_size", std::min(recv_mtu, PCIE_RX_DATA_FRAME_SIZE));
        default_buff_args.num_recv_frames =
            args.cast<size_t>("num_recv_frames", PCIE_RX_DATA_NUM_FRAMES);
        default_buff_args.recv_buff_size = args.cast<size_t>("recv_buff_size", 0);
        xports.recv                      = nirio_zero_copy::make(
            _rio_fpga_interface, dma_channel_num, default_buff_args);
    }

    xports.send = xports.recv;

    // Router config word is:
    // - Upper 16 bits: Destination address (e.g. 0.0)
    // - Lower 16 bits: DMA channel
    uint32_t router_config_word = (xports.recv_sid.get_dst() << 16) | dma_channel_num;
    _rio_fpga_interface->get_kernel_proxy()->poke(PCIE_ROUTER_REG(0), router_config_word);

    // For the nirio transport, buffer size is depends on the frame size and num
    // frames
    xports.recv_buff_size =
        xports.recv->get_num_recv_frames() * xports.recv->get_recv_frame_size();
    xports.send_buff_size =
        xports.send->get_num_send_frames() * xports.send->get_send_frame_size();

    return xports;
}
