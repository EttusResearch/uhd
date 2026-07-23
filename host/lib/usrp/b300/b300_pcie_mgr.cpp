//
// Copyright 2025 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "b300_pcie_mgr.hpp"
#include "b300_regs.hpp"
#include <uhd/rfnoc/rfnoc_types.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/cast.hpp>
#include <uhd/utils/log.hpp>
#include <uhdlib/rfnoc/device_id.hpp>
#include <uhdlib/usrp/b300/b300_pcie_link.hpp>
#include <uhdlib/usrp/b300/b300_pcie_session.hpp>

using namespace uhd::transport;

namespace {
constexpr size_t DATA_FRAME_SIZE_LINUX   = 8192; // bytes, 1024 elements per frame
constexpr size_t DATA_NUM_FRAMES_LINUX   = 8192; // 64 MiB, 8,388,608 elements total
constexpr size_t DATA_FRAME_SIZE_WINDOWS = 8192; // bytes, 1024 elements per frame
constexpr size_t DATA_NUM_FRAMES_WINDOWS = 8192; // 64 MiB, 8,388,608 elements total
constexpr size_t MSG_FRAME_SIZE          = 256; // bytes - constant for all CHDR widths
constexpr size_t MSG_NUM_FRAMES          = 64; // 16 KiB, 2,048 elements total

//! Get default send/recv num frames and frame size per link type
link_params_t get_default_link_params(const link_type_t link_type)
{
    link_params_t link_params;

    // Use platform-specific constants for data frame parameters
#ifdef _WIN32
    const size_t data_frame_size = DATA_FRAME_SIZE_WINDOWS;
    const size_t data_num_frames = DATA_NUM_FRAMES_WINDOWS;
#else
    const size_t data_frame_size = DATA_FRAME_SIZE_LINUX;
    const size_t data_num_frames = DATA_NUM_FRAMES_LINUX;
#endif

    switch (link_type) {
        case link_type_t::CTRL:
            link_params.send_frame_size = MSG_FRAME_SIZE;
            link_params.recv_frame_size = MSG_FRAME_SIZE;
            link_params.num_send_frames = MSG_NUM_FRAMES;
            link_params.num_recv_frames = MSG_NUM_FRAMES;
            break;
        case link_type_t::TX_DATA:
            link_params.send_frame_size = data_frame_size;
            link_params.recv_frame_size = MSG_FRAME_SIZE;
            link_params.num_send_frames = data_num_frames;
            link_params.num_recv_frames = MSG_NUM_FRAMES;
            break;
        case link_type_t::RX_DATA:
            link_params.send_frame_size = MSG_FRAME_SIZE;
            link_params.recv_frame_size = data_frame_size;
            link_params.num_send_frames = MSG_NUM_FRAMES;
            link_params.num_recv_frames = data_num_frames;
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

namespace uhd { namespace usrp { namespace b300 {
/******************************************************************************
 * Structors
 *****************************************************************************/
b300_pcie_manager::b300_pcie_manager(const std::string& resource,
    uhd::property_tree::sptr tree,
    const uhd::fs_path& root_path)
    : _resource(resource)
{
    UHD_LOG_TRACE("B300", "Creating B300 PCIe manager for resource: " << resource);
    // Create the PCIe session
    try {
        _session = b300_pcie_session::make(resource);
        UHD_LOG_TRACE("B300", "Successfully created B300 PCIe session");
        test_pcie_registers();
    } catch (const std::exception& ex) {
        UHD_LOG_ERROR("B300", "Failed to create B300 PCIe session: " << ex.what());
        throw uhd::runtime_error(
            std::string("Failed to create B300 PCIe session: ") + ex.what());
    }
    _local_device_id = rfnoc::allocate_device_id();
    // Store tree and root_path if needed for future use
    _tree      = tree;
    _root_path = root_path;
}

/******************************************************************************
 * API
 *****************************************************************************/
size_t b300_pcie_manager::get_mtu(uhd::direction_t)
{
#ifdef _WIN32
    return DATA_FRAME_SIZE_WINDOWS;
#else
    return DATA_FRAME_SIZE_LINUX;
#endif
}

uint32_t b300_pcie_manager::allocate_pcie_dma_chan(
    const rfnoc::sep_id_t& remote_epid, const link_type_t link_type)
{
    // We have seen issues where an underlying DMA channel can get into a bad state
    // if it had previously been allocated for an Rx channel and then tries to be
    // allocated for a Tx channel. Hardcode which DMA channels are used for Rx
    // and which are used for Tx rather than dynamically allocating. Ctrl link is always
    // the first DMA channel, then have all Rx channels, then all Tx channels. Each DMA
    // channel corresponds to a pair of FIFOs (one for host-to-device and one for
    // device-to-host). Each FIFO is created during the link creation, the device to-host
    // FIFO uses the same instance number as the DMA channel and the host-to-device FIFO
    // instance number is offset by B300_TX_FIFO_OFFSET. So make sure the DMA channel
    // allocated never exceeds B300_TX_FIFO_OFFSET.
    constexpr uint32_t CTRL_CHANNEL       = 0;
    constexpr uint32_t NUM_DATA_CHANS_RX  = 2;
    constexpr uint32_t NUM_DATA_CHANS_TX  = 2;
    constexpr uint32_t FIRST_RX_DATA_CHAN = 1;
    constexpr uint32_t FIRST_TX_DATA_CHAN = FIRST_RX_DATA_CHAN + NUM_DATA_CHANS_RX;

    std::lock_guard<std::mutex> l(_dma_chan_mutex);
    uint32_t dma_chan = CTRL_CHANNEL;
    if (link_type == link_type_t::CTRL) {
        if (_dma_chan_pool.count(CTRL_CHANNEL)) {
            throw uhd::runtime_error("Cannot reallocate PCIe control channel!");
        }
    } else if (link_type == link_type_t::RX_DATA) {
        dma_chan = FIRST_RX_DATA_CHAN;
        while (_dma_chan_pool.count(dma_chan)) {
            dma_chan++;
        }
        if (dma_chan >= FIRST_RX_DATA_CHAN + NUM_DATA_CHANS_RX) {
            throw uhd::runtime_error(
                "Trying to allocate more RX DMA channels than are available!");
        }
    } else if (link_type == link_type_t::TX_DATA) {
        dma_chan = FIRST_TX_DATA_CHAN;
        while (_dma_chan_pool.count(dma_chan)) {
            dma_chan++;
        }
        if (dma_chan >= FIRST_TX_DATA_CHAN + NUM_DATA_CHANS_TX) {
            throw uhd::runtime_error(
                "Trying to allocate more TX DMA channels than are available!");
        }
    } else {
        throw uhd::runtime_error("Invalid link type for DMA channel allocation!");
    }

    _dma_chan_pool[dma_chan] = remote_epid;
    UHD_LOG_TRACE("B300",
        "Assigning DMA channel " << dma_chan << " to remote EPID " << remote_epid);
    return dma_chan;
}

both_links_t b300_pcie_manager::get_links(link_type_t link_type,
    const rfnoc::device_id_t local_device_id,
    const rfnoc::sep_id_t& /*local_epid*/,
    const rfnoc::sep_id_t& remote_epid,
    const device_addr_t& link_args,
    rfnoc::chdr_w_t chdr_w)
{
    if (local_device_id != _local_device_id) {
        throw uhd::runtime_error("Cannot create PCIe link through local device ID "
                                 + std::to_string(local_device_id)
                                 + ", no such device associated with this motherboard!");
    }

    const bool enable_fc = not link_args.has_key("enable_fc")
                           || uhd::cast::from_str<bool>(link_args.get("enable_fc"));

    const uint32_t dma_channel_num = allocate_pcie_dma_chan(remote_epid, link_type);
    link_params_t link_params      = get_default_link_params(link_type);

    size_t recv_buff_size, send_buff_size;
    std::function<void(uint32_t)> release_cb = [this](uint32_t channel) {
        UHD_LOG_DEBUG("B300", "Release channel assignment for DMA " << channel);
        std::lock_guard<std::mutex> l(_dma_chan_mutex);
        _dma_chan_pool.erase(channel);
    };

    // Create the PCIe link
    try {
        auto link = b300_pcie_link::make(_session,
            dma_channel_num,
            std::move(release_cb),
            link_params,
            link_args,
            recv_buff_size,
            send_buff_size,
            chdr_w);

        return std::make_tuple(link,
            send_buff_size,
            link,
            recv_buff_size,
            false /*not lossy*/,
            false /*don't swap endianness*/,
            enable_fc);
    } catch (const std::exception& ex) {
        UHD_LOG_ERROR("B300", "Failed to create PCIe link: " << ex.what());
        throw uhd::runtime_error(std::string("Failed to create PCIe link: ") + ex.what());
    }
}

// Helper to test reading and writing PCIe registers
void b300_pcie_manager::test_pcie_registers()
{
    UHD_LOG_TRACE("B300", "Testing PCIe register access...");
    try {
        // Read-only registers
        // Note that the signature values usually the ASCII for the device name
        // e.g. B310 is 0x42333130 (ASCII "B310")
        UHD_LOG_TRACE("B300",
            "PCI Signature Register: 0x" << std::hex
                                         << _session->peek32(PCIE_PCI_SIGNATURE_REG));

        UHD_LOG_TRACE("B300",
            "FPGA Counter: 0x" << std::hex << peek32(PCIE_FPGA_COUNTER_HI_REG)
                               << peek32(PCIE_FPGA_COUNTER_LO_REG));
        UHD_LOG_TRACE("B300", "FPGA Freq: 0x" << std::hex << peek32(PCIE_FPGA_FREQ_REG));
        // Also show the frequency in MHz with 4 decimal places
        UHD_LOG_TRACE("B300",
            "FPGA Frequency: " << std::fixed << std::setprecision(4)
                               << (static_cast<double>(peek32(PCIE_FPGA_FREQ_REG)) / 1e6)
                               << " MHz");
        UHD_LOG_TRACE(
            "B300", "MISC Status: 0x" << std::hex << peek32(PCIE_MISC_STATUS_REG));
        // User signature registers (read-only in practice)
        for (int i = 0; i < 4; ++i) {
            UHD_LOG_TRACE("B300",
                "USR_SIG" << i << ": 0x" << std::hex
                          << peek32(PCIE_USR_SIG0_REG + 4 * i));
        }
        // Read/write test for scratch registers
        uint32_t test_lo = 0xA5A5A5A5;
        uint32_t test_hi = 0x5A5A5A5A;
        poke32(PCIE_SCRATCH_LO_REG, test_lo);
        poke32(PCIE_SCRATCH_HI_REG, test_hi);
        uint32_t read_lo = peek32(PCIE_SCRATCH_LO_REG);
        uint32_t read_hi = peek32(PCIE_SCRATCH_HI_REG);
        UHD_LOG_TRACE(
            "B300", "Scratch LO: 0x" << std::hex << read_lo << ", HI: 0x" << read_hi);
        if (read_lo != test_lo || read_hi != test_hi) {
            UHD_LOG_WARNING("B300", "Scratch register R/W test failed!");
        } else {
            UHD_LOG_TRACE("B300", "Scratch register R/W test passed.");
        }
    } catch (const std::exception& ex) {
        UHD_LOG_ERROR("B300", "PCIe register test failed: " << ex.what());
        throw;
    }
}

}}} // namespace uhd::usrp::b300
