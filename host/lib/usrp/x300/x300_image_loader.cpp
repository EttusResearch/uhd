//
// Copyright 2015-2017 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <fstream>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <uhd/config.hpp>
#include <uhd/device.hpp>
#include <uhd/image_loader.hpp>
#include <uhd/exception.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/transport/nirio/status.h>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/static.hpp>

#include "x300_impl.hpp"
#include "x300_fw_common.h"
#include "cdecode.h"

namespace fs = boost::filesystem;

using namespace boost::algorithm;
using namespace uhd;
using namespace uhd::transport;

/*
 * Constants
 */
#define X300_FPGA_BIN_SIZE_BYTES 15877916
#define X300_FPGA_BIT_SIZE_BYTES 15878032
#define X300_FPGA_PROG_UDP_PORT 49157
#define X300_FLASH_SECTOR_SIZE 131072
#define X300_PACKET_SIZE_BYTES 256
#define X300_FPGA_SECTOR_START 32
#define X300_MAX_RESPONSE_BYTES 128
#define UDP_TIMEOUT 3
#define FPGA_LOAD_TIMEOUT 15

/*
 * Bitstream header pattern
 */
static const uint8_t X300_FPGA_BIT_HEADER[] =
{
    0x00, 0x09, 0x0f, 0xf0, 0x0f, 0xf0, 0x0f, 0xf0,
    0x0f, 0xf0, 0x00, 0x00, 0x01, 0x61, 0x00
};

/*
 * Packet structure
 */
typedef struct {
    uint32_t flags;
    uint32_t sector;
    uint32_t index;
    uint32_t size;
    union {
        uint8_t  data8[X300_PACKET_SIZE_BYTES];
        uint16_t data16[X300_PACKET_SIZE_BYTES/2];
    };
} x300_fpga_update_data_t;

/*
 * X-Series burn session
 */
typedef struct {
    bool                             found;
    bool                             ethernet;
    bool                             configure; // Reload FPGA after burning to flash (Ethernet only)
    bool                             verify;    // Device will verify the download along the way (Ethernet only)
    bool                             download;  // Host will read the FPGA image on the device to a file
    bool                             lvbitx;
    uhd::device_addr_t               dev_addr;
    std::string                      ip_addr;
    std::string                      fpga_type;
    std::string                      resource;
    std::string                      filepath;
    std::string                      outpath;
    std::string                      rpc_port;
    udp_simple::sptr                 write_xport;
    udp_simple::sptr                 read_xport;
    uint32_t                         size;
    uint8_t                          data_in[udp_simple::mtu];
    std::vector<char>                bitstream; // .bin image extracted from .lvbitx file
} x300_session_t;

/*
 * Extract the .bin image from the given LVBITX file.
 */
static void extract_from_lvbitx(x300_session_t &session){
    boost::property_tree::ptree pt; 
    boost::property_tree::xml_parser::read_xml(session.filepath.c_str(), pt, 
                                               boost::property_tree::xml_parser::no_comments |
                                               boost::property_tree::xml_parser::trim_whitespace);
    const std::string encoded_bitstream(pt.get<std::string>("Bitfile.Bitstream"));
    std::vector<char> decoded_bitstream(encoded_bitstream.size());

    base64_decodestate decode_state;
    base64_init_decodestate(&decode_state);
    const size_t decoded_size = base64_decode_block(encoded_bitstream.c_str(),
                                encoded_bitstream.size(), &decoded_bitstream.front(), &decode_state);
    decoded_bitstream.resize(decoded_size);
    session.bitstream.swap(decoded_bitstream);

    session.size = session.bitstream.size();
}

/*
 * Validate X300 image and extract if LVBITX.
 */
static void x300_validate_image(x300_session_t &session){
    if(not fs::exists(session.filepath)){
        throw uhd::runtime_error(str(boost::format("Could not find image at path \"%s\".")
                                     % session.filepath));
    }

    std::string extension = fs::extension(session.filepath);
    session.lvbitx = (extension == ".lvbitx");

    if(session.lvbitx){
        extract_from_lvbitx(session);
        if(session.size > X300_FPGA_BIN_SIZE_BYTES){
            throw uhd::runtime_error(str(boost::format("The specified FPGA image is too large: %d vs. %d")
                                         % session.size % X300_FPGA_BIN_SIZE_BYTES));
        }

        /*
         * PCIe burning just takes a filepath, even for a .lvbitx file,
         * so just extract it to validate the size.
         */
        if(!session.ethernet) session.bitstream.clear();
    }
    else if(extension == ".bin" or extension == ".bit"){
        uint32_t max_size = (extension == ".bin") ? X300_FPGA_BIN_SIZE_BYTES
                                                         : X300_FPGA_BIT_SIZE_BYTES;

        session.size = fs::file_size(session.filepath);
        if(session.size > max_size){
            throw uhd::runtime_error(str(boost::format("The specified FPGA image is too large: %d vs. %d")
                                         % session.size % max_size));
            return;
        }
    }
    else{
        throw uhd::runtime_error(str(boost::format("Invalid extension \"%s\". Extension must be .bin, .bit, or .lvbitx.")
                                     % extension));
    }
}

static void x300_setup_session(x300_session_t &session,
                               const device_addr_t &args,
                               const std::string &filepath,
                               const std::string &outpath){
    device_addrs_t devs = x300_find(args);
    if(devs.size() == 0){
        session.found = false;
        return;
    }
    else if(devs.size() > 1){
        std::string err_msg = "Could not resolve given args to a single X-Series device.\n"
                              "Applicable devices:\n";

        for(const uhd::device_addr_t &dev:  devs){
            std::string identifier = dev.has_key("addr") ? "addr"
                                                         : "resource";

            err_msg += str(boost::format(" * %s (%s=%s)\n")
                           % dev.get("product", "X3XX")
                           % identifier
                           % dev.get(identifier));
        }

        err_msg += "\nSpecify one of these devices with the given args to load an image onto it.";

        throw uhd::runtime_error(err_msg);
    }

    session.found = true;
    session.dev_addr = devs[0];
    session.ethernet = session.dev_addr.has_key("addr");
    if(session.ethernet){
        session.ip_addr = session.dev_addr["addr"];
        session.configure = args.has_key("configure");
        session.write_xport = udp_simple::make_connected(session.ip_addr,
                                                        BOOST_STRINGIZE(X300_FPGA_PROG_UDP_PORT));
        session.read_xport = udp_simple::make_connected(session.ip_addr,
                                                        BOOST_STRINGIZE(X300_FPGA_READ_UDP_PORT));
        session.verify = args.has_key("verify");
        session.download = args.has_key("download");
    }
    else{
        session.resource = session.dev_addr["resource"];
        session.rpc_port = args.get("rpc-port", "5444");
    }

    /*
     * The user can specify an FPGA type (1G, HGS, XGS), rather than a filename. If the user
     * does not specify one, this will default to the type currently on the device. If this
     * cannot be determined, then the user is forced to specify a filename.
     */
    session.fpga_type = args.get("fpga", session.dev_addr.get("fpga", ""));
    if(filepath == ""){
        if(!session.dev_addr.has_key("product") or session.fpga_type == ""){
            throw uhd::runtime_error("Found a device but could not auto-generate an image filename.");
        }
        else session.filepath = find_image_path(str(boost::format("usrp_%s_fpga_%s.bit")
                                                    % (to_lower_copy(session.dev_addr["product"]))
                                                    % session.fpga_type));
    }
    else session.filepath = filepath;

    /*
     * The user can specify an output image path, or UHD will use the
     * system temporary path by default
     */
    if(outpath == ""){
        if(!session.dev_addr.has_key("product") or session.fpga_type == ""){
            throw uhd::runtime_error("Found a device but could not auto-generate an image filename.");
        }
        std::string filename = str(boost::format("usrp_%s_fpga_%s")
                                   % (to_lower_copy(session.dev_addr["product"]))
                                   % session.fpga_type);

        session.outpath = get_tmp_path() + "/" + filename;
    } else {
        session.outpath = outpath;
    }

    // Validate image
    x300_validate_image(session);
}

/*
 * Ethernet communication functions
 */
static UHD_INLINE size_t x300_send_and_recv(udp_simple::sptr xport,
                                            uint32_t pkt_code,
                                            x300_fpga_update_data_t *pkt_out,
                                            uint8_t* data){
    pkt_out->flags = uhd::htonx<uint32_t>(pkt_code);
    xport->send(boost::asio::buffer(pkt_out, sizeof(*pkt_out)));
    return xport->recv(boost::asio::buffer(data, udp_simple::mtu), UDP_TIMEOUT);
}

static UHD_INLINE bool x300_recv_ok(const x300_fpga_update_data_t *pkt_in,
                                    size_t len){
    return (len > 0 and
            ((ntohl(pkt_in->flags) & X300_FPGA_PROG_FLAGS_ERROR) != X300_FPGA_PROG_FLAGS_ERROR));
}

// Image data needs to be bitswapped
static UHD_INLINE void x300_bitswap(uint8_t *num){
    *num = ((*num & 0xF0) >> 4) | ((*num & 0x0F) << 4);
    *num = ((*num & 0xCC) >> 2) | ((*num & 0x33) << 2);
    *num = ((*num & 0xAA) >> 1) | ((*num & 0x55) << 1);
}

static void x300_ethernet_load(x300_session_t &session){

    // UDP receive buffer
    x300_fpga_update_data_t pkt_out;
    const x300_fpga_update_data_t *pkt_in = reinterpret_cast<const x300_fpga_update_data_t*>(session.data_in);

    // Initialize write session
    uint32_t flags = X300_FPGA_PROG_FLAGS_ACK | X300_FPGA_PROG_FLAGS_INIT;
    size_t len = x300_send_and_recv(session.write_xport, flags, &pkt_out, session.data_in);
    if(x300_recv_ok(pkt_in, len)){
        std::cout << "-- Initializing FPGA loading..." << std::flush;
    }
    else if(len == 0){
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Timed out waiting for reply from device.");
    }
    else{
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Device reported an error during initialization.");
    }

    std::cout << "successful." << std::endl;
    if(session.verify){
        std::cout << "-- NOTE: Device is verifying the image it is receiving, increasing the loading time." << std::endl;
    }

    size_t current_pos = 0;
    size_t sectors = (session.size / X300_FLASH_SECTOR_SIZE);
    std::ifstream image(session.filepath.c_str(), std::ios::binary);

    // Each sector
    for(size_t i = 0; i < session.size; i += X300_FLASH_SECTOR_SIZE){

        // Print progress percentage at beginning of each sector
        std::cout << boost::format("\r-- Loading %s FPGA image: %d%% (%d/%d sectors)")
                     % session.fpga_type
                     % (int(double(i) / double(session.size) * 100.0))
                     % (i / X300_FLASH_SECTOR_SIZE)
                     % sectors
                 << std::flush;

        // Each packet
        for(size_t j = i; (j < session.size and j < (i+X300_FLASH_SECTOR_SIZE)); j += X300_PACKET_SIZE_BYTES){
            flags = X300_FPGA_PROG_FLAGS_ACK;
            if(j == i)         flags |= X300_FPGA_PROG_FLAGS_ERASE; // Erase at beginning of sector
            if(session.verify) flags |= X300_FPGA_PROG_FLAGS_VERIFY;

            // Set burn location
            pkt_out.sector = htonx<uint32_t>(X300_FPGA_SECTOR_START + (i/X300_FLASH_SECTOR_SIZE));
            pkt_out.index  = htonx<uint32_t>((j % X300_FLASH_SECTOR_SIZE) / 2);
            pkt_out.size   = htonx<uint32_t>(X300_PACKET_SIZE_BYTES / 2);

            // Read next piece of image
            memset(pkt_out.data8, 0, X300_PACKET_SIZE_BYTES);
            if(session.lvbitx){
                memcpy(pkt_out.data8, &session.bitstream[current_pos], X300_PACKET_SIZE_BYTES);
                current_pos += X300_PACKET_SIZE_BYTES;
            }
            else{
                image.read((char*)pkt_out.data8, X300_PACKET_SIZE_BYTES);
            }

            // Data must be bitswapped and byteswapped
            for(size_t k = 0; k < X300_PACKET_SIZE_BYTES; k++){
                x300_bitswap(&pkt_out.data8[k]);
            }
            for(size_t k = 0; k < (X300_PACKET_SIZE_BYTES/2); k++){
                pkt_out.data16[k] = htonx<uint16_t>(pkt_out.data16[k]);
            }

            len = x300_send_and_recv(session.write_xport, flags, &pkt_out, session.data_in);
            if(len == 0){
                if(!session.lvbitx) image.close();
                throw uhd::runtime_error("Timed out waiting for reply from device.");
            }
            else if((ntohl(pkt_in->flags) & X300_FPGA_PROG_FLAGS_ERROR)){
                if(!session.lvbitx) image.close();
                throw uhd::runtime_error("Device reported an error.");
            }
        }
    }
    if(!session.lvbitx){
        image.close();
    }

    std::cout << boost::format("\r-- Loading %s FPGA image: 100%% (%d/%d sectors)")
                 % session.fpga_type
                 % sectors
                 % sectors
             << std::endl;

    // Cleanup
    if(!session.lvbitx) image.close();
    flags = (X300_FPGA_PROG_FLAGS_CLEANUP | X300_FPGA_PROG_FLAGS_ACK);
    pkt_out.sector = pkt_out.index = pkt_out.size = 0;
    memset(pkt_out.data8, 0, X300_PACKET_SIZE_BYTES);
    std::cout << "-- Finalizing image load..." << std::flush;
    len = x300_send_and_recv(session.write_xport, flags, &pkt_out, session.data_in);
    if(len == 0){
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Timed out waiting for reply from device.");
    }
    else if((ntohl(pkt_in->flags) & X300_FPGA_PROG_FLAGS_ERROR)){
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Device reported an error during cleanup.");
    }
    else std::cout << "successful." << std::endl;

    // Save new FPGA image (if option set)
    if(session.configure){
        flags = (X300_FPGA_PROG_CONFIGURE | X300_FPGA_PROG_FLAGS_ACK);
        x300_send_and_recv(session.write_xport, flags, &pkt_out, session.data_in);
        std::cout << "-- Saving image onto device..." << std::flush;
        if(len == 0){
            std::cout << "failed." << std::endl;
            throw uhd::runtime_error("Timed out waiting for reply from device.");
        }
        else if((ntohl(pkt_in->flags) & X300_FPGA_PROG_FLAGS_ERROR)){
            std::cout << "failed." << std::endl;
            throw uhd::runtime_error("Device reported an error while saving the image.");
        }
        else std::cout << "successful." << std::endl;
    }
    std::cout << str(boost::format("Power-cycle the USRP %s to use the new image.") % session.dev_addr.get("product", "")) << std::endl;
}

static void x300_ethernet_read(x300_session_t &session){

    // UDP receive buffer
    x300_fpga_update_data_t pkt_out;
    memset(pkt_out.data8, 0, X300_PACKET_SIZE_BYTES);

    x300_fpga_update_data_t *pkt_in = reinterpret_cast<x300_fpga_update_data_t*>(session.data_in);

    // Initialize read session
    uint32_t flags = X300_FPGA_READ_FLAGS_ACK | X300_FPGA_READ_FLAGS_INIT;
    size_t len = x300_send_and_recv(session.read_xport, flags, &pkt_out, session.data_in);
    if(x300_recv_ok(pkt_in, len)){
        std::cout << "-- Initializing FPGA reading..." << std::flush;
    }
    else if(len == 0){
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Timed out waiting for reply from device.");
    }
    else{
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Device reported an error during initialization.");
    }

    std::cout << "successful." << std::endl;

    // Read the first packet
    // Acknowledge receipt of the FPGA image data
    flags = X300_FPGA_READ_FLAGS_ACK;

    // Set the initial burn location
    pkt_out.sector = htonx<uint32_t>(X300_FPGA_SECTOR_START);
    pkt_out.index  = 0;
    pkt_out.size   = htonx<uint32_t>(X300_PACKET_SIZE_BYTES / 2);

    len = x300_send_and_recv(session.read_xport, flags, &pkt_out, session.data_in);
    if(len == 0){
        throw uhd::runtime_error("Timed out waiting for reply from device.");
    }
    else if((ntohl(pkt_in->flags) & X300_FPGA_READ_FLAGS_ERROR)){
        throw uhd::runtime_error("Device reported an error.");
    }

    // Data must be bitswapped and byteswapped
    for(size_t k = 0; k < X300_PACKET_SIZE_BYTES; k++){
        x300_bitswap(&pkt_in->data8[k]);
    }
    for(size_t k = 0; k < (X300_PACKET_SIZE_BYTES/2); k++){
        pkt_in->data16[k] = htonx<uint16_t>(pkt_in->data16[k]);
    }

    // Assume the largest size first
    size_t image_size = X300_FPGA_BIT_SIZE_BYTES;
    size_t sectors = (image_size / X300_FLASH_SECTOR_SIZE);
    std::string extension(".bit");

    // Check for the beginning header sequence to determine
    // the total amount of data (.bit vs .bin) on the flash
    // The .bit file format includes header information not part of a .bin
    for (size_t i = 0; i < sizeof(X300_FPGA_BIT_HEADER); i++)
    {
        if (pkt_in->data8[i] != X300_FPGA_BIT_HEADER[i])
        {
            std::cout << "-- No *.bit header detected, FPGA image is a raw stream (*.bin)!" << std::endl;
            image_size = X300_FPGA_BIN_SIZE_BYTES;
            sectors = (image_size / X300_FLASH_SECTOR_SIZE);
            extension = std::string(".bin");
            break;
        }
    }

    session.outpath += extension;
    std::ofstream image(session.outpath.c_str(), std::ios::binary);
    std::cout << boost::format("-- Output FPGA file: %s\n")
                 % session.outpath;

    // Write the first packet
    image.write((char*)pkt_in->data8, X300_PACKET_SIZE_BYTES);

    // Each sector
    size_t pkt_count = X300_PACKET_SIZE_BYTES;
    for(size_t i = 0; i < image_size; i += X300_FLASH_SECTOR_SIZE){

        // Once we determine the image size, print the progress percentage
        std::cout << boost::format("\r-- Reading %s FPGA image: %d%% (%d/%d sectors)")
                     % session.fpga_type
                     % (int(double(i) / double(image_size) * 100.0))
                     % (i / X300_FLASH_SECTOR_SIZE)
                     % sectors
                  << std::flush;

        // Each packet
        while (pkt_count < image_size and pkt_count < (i + X300_FLASH_SECTOR_SIZE))
        {
            // Set burn location
            pkt_out.sector = htonx<uint32_t>(X300_FPGA_SECTOR_START + (i/X300_FLASH_SECTOR_SIZE));
            pkt_out.index  = htonx<uint32_t>((pkt_count % X300_FLASH_SECTOR_SIZE) / 2);

            len = x300_send_and_recv(session.read_xport, flags, &pkt_out, session.data_in);
            if(len == 0){
                image.close();
                throw uhd::runtime_error("Timed out waiting for reply from device.");
            }
            else if((ntohl(pkt_in->flags) & X300_FPGA_READ_FLAGS_ERROR)){
                image.close();
                throw uhd::runtime_error("Device reported an error.");
            }

            // Data must be bitswapped and byteswapped
            for(size_t k = 0; k < X300_PACKET_SIZE_BYTES; k++){
                x300_bitswap(&pkt_in->data8[k]);
            }
            for(size_t k = 0; k < (X300_PACKET_SIZE_BYTES/2); k++){
                pkt_in->data16[k] = htonx<uint16_t>(pkt_in->data16[k]);
            }

            // Calculate the number of bytes to write
            // If this is the last packet, get rid of the extra zero padding
            // due to packet size
            size_t nbytes = X300_PACKET_SIZE_BYTES;
            if (pkt_count > (image_size - X300_PACKET_SIZE_BYTES))
            {
                nbytes = (image_size - pkt_count);
            }

            // Write the incoming piece of the image to a file
            image.write((char*)pkt_in->data8, nbytes);

            // Increment the data count
            pkt_count += X300_PACKET_SIZE_BYTES;
        }

        pkt_count = i + X300_FLASH_SECTOR_SIZE;
    }

    std::cout << boost::format("\r-- Reading %s FPGA image: 100%% (%d/%d sectors)")
                 % session.fpga_type
                 % sectors
                 % sectors
              << std::endl;

    // Cleanup
    image.close();
    flags = (X300_FPGA_READ_FLAGS_CLEANUP | X300_FPGA_READ_FLAGS_ACK);
    pkt_out.sector = pkt_out.index = pkt_out.size = 0;
    memset(pkt_out.data8, 0, X300_PACKET_SIZE_BYTES);
    std::cout << "-- Finalizing image read for verification..." << std::flush;
    len = x300_send_and_recv(session.read_xport, flags, &pkt_out, session.data_in);
    if(len == 0){
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Timed out waiting for reply from device.");
    }
    else if((ntohl(pkt_in->flags) & X300_FPGA_READ_FLAGS_ERROR)){
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Device reported an error during cleanup.");
    }
    else std::cout << "successful image read." << std::endl;
}

static void x300_pcie_load(x300_session_t &session){

    std::cout << boost::format("\r-- Loading %s FPGA image (this will take 5-10 minutes)...")
                 % session.fpga_type
             << std::flush;

    nirio_status status = NiRio_Status_Success;
    niusrprio::niusrprio_session fpga_session(session.resource, session.rpc_port);
    nirio_status_chain(fpga_session.download_bitstream_to_flash(session.filepath), status);

    if(nirio_status_fatal(status)){
        std::cout << "failed." << std::endl;
        niusrprio::nirio_status_to_exception(status, "NI-RIO reported the following error:");
    }
    else std::cout << "successful." << std::endl;
    std::cout << str(boost::format("Power-cycle the USRP %s to use the new image.") % session.dev_addr.get("product", "")) << std::endl;
}

static bool x300_image_loader(const image_loader::image_loader_args_t &image_loader_args){
    // See if any X3x0 with the given args is found
    device_addrs_t devs = x300_find(image_loader_args.args);

    if (devs.size() == 0) return false;

    x300_session_t session;
    x300_setup_session(session,
                       image_loader_args.args,
                       image_loader_args.fpga_path,
                       image_loader_args.out_path);

    if(!session.found) return false;

    std::cout << boost::format("Unit: USRP %s (%s, %s)\nFPGA Image: %s\n")
                 % session.dev_addr["product"]
                 % session.dev_addr["serial"]
                 % session.dev_addr[session.ethernet ? "addr" : "resource"]
                 % session.filepath;

    // Download the FPGA image to a file
    if(image_loader_args.download) {
        std::cout << "Attempting to download the FPGA image ..." << std::endl;
        x300_ethernet_read(session);
    }

    if (not image_loader_args.load_fpga) return true;

    if (session.ethernet) x300_ethernet_load(session);
    else                  x300_pcie_load(session);

    return true;
}

UHD_STATIC_BLOCK(register_x300_image_loader){
    std::string recovery_instructions = "Aborting. Your USRP X-Series device will likely be unusable. Visit\n"
                                        "http://files.ettus.com/manual/page_usrp_x3x0.html#x3x0_load_fpga_imgs_jtag\n"
                                        "for details on restoring your device.";

    image_loader::register_image_loader("x300", x300_image_loader, recovery_instructions);
}
