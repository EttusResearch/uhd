//
// Copyright 2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <cstring>
#include <iostream>
#include <fstream>

#include <boost/asio/ip/address_v4.hpp>
#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <boost/algorithm/string/erase.hpp>

#include <uhd/config.hpp>
#include <uhd/image_loader.hpp>
#include <uhd/exception.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/types/dict.hpp>

#include "fw_common.h"
#include "usrp2_iface.hpp"
#include "usrp2_impl.hpp"

typedef boost::asio::ip::address_v4 ip_v4;

namespace fs = boost::filesystem;
using namespace boost::algorithm;

using namespace uhd;
using namespace uhd::usrp;
using namespace uhd::transport;

/*
 * Constants
 */

#define N200_FLASH_DATA_PACKET_SIZE 256
#define N200_UDP_FW_UPDATE_PORT 49154
#define UDP_TIMEOUT 0.5

#define N200_FW_MAX_SIZE_BYTES 31744
#define N200_PROD_FW_IMAGE_ADDR  0x00300000
#define N200_SAFE_FW_IMAGE_ADDR  0x003F0000

#define N200_FPGA_MAX_SIZE_BYTES 1572864
#define N200_PROD_FPGA_IMAGE_ADDR  0x00180000
#define N200_SAFE_FPGA_IMAGE_ADDR  0x00000000

/*
 * Packet codes
 */
typedef enum {
    UNKNOWN = ' ',

    N200_QUERY = 'a',
    N200_ACK = 'A',

    GET_FLASH_INFO_CMD = 'f',
    GET_FLASH_INFO_ACK = 'F',

    ERASE_FLASH_CMD = 'e',
    ERASE_FLASH_ACK = 'E',

    CHECK_ERASING_DONE_CMD = 'd',
    DONE_ERASING_ACK = 'D',
    NOT_DONE_ERASING_ACK = 'B',

    WRITE_FLASH_CMD = 'w',
    WRITE_FLASH_ACK = 'W',

    READ_FLASH_CMD = 'r',
    READ_FLASH_ACK = 'R',

    RESET_CMD = 's',
    RESET_ACK = 'S',

    GET_HW_REV_CMD = 'v',
    GET_HW_REV_ACK = 'V',
} n200_fw_update_id_t;

/*
 * Mapping revision numbers to names
 */
static const uhd::dict<uint32_t, std::string> n200_filename_map = boost::assign::map_list_of
    (0,      "n2xx")    // Is an N-Series, but the EEPROM value is invalid
    (0xa,    "n200_r3")
    (0x100a, "n200_r4")
    (0x10a,  "n210_r3")
    (0x110a, "n210_r4")
;

/*
 * Packet structure
 */
typedef struct {
    uint32_t proto_ver;
    uint32_t id;
    uint32_t seq;
    union {
        uint32_t ip_addr;
        uint32_t hw_rev;
        struct {
            uint32_t flash_addr;
            uint32_t length;
            uint8_t  data[256];
        } flash_args;
        struct {
            uint32_t sector_size_bytes;
            uint32_t memory_size_bytes;
        } flash_info_args;
    } data;
} n200_fw_update_data_t;

/*
 * N-Series burn session
 */
typedef struct {
    bool               fw;
    bool               overwrite_safe;
    bool               reset;
    uhd::device_addr_t dev_addr;
    std::string        burn_type;
    std::string        filepath;
    uint8_t     data_in[udp_simple::mtu];
    uint32_t    size;
    uint32_t    max_size;
    uint32_t    flash_addr;
    udp_simple::sptr   xport;
} n200_session_t;

/***********************************************************************
 * uhd::image_loader functionality
 **********************************************************************/

static void print_usrp2_error(const image_loader::image_loader_args_t &image_loader_args){
    #ifdef UHD_PLATFORM_WIN32
    std::string usrp2_card_burner_gui = "\"";
    const std::string nl = " ^\n    ";
    #else
    std::string usrp2_card_burner_gui = "sudo \"";
    const std::string nl = " \\\n    ";
    #endif

    usrp2_card_burner_gui += find_utility("usrp2_card_burner_gui.py");
    usrp2_card_burner_gui += "\"";

    if(image_loader_args.load_firmware){
        usrp2_card_burner_gui += str(boost::format("%s--fw=\"%s\"")
                                     % nl
                                     % ((image_loader_args.firmware_path == "")
                                             ? find_image_path("usrp2_fw.bin")
                                             : image_loader_args.firmware_path));
    }
    if(image_loader_args.load_fpga){
        usrp2_card_burner_gui += str(boost::format("%s--fpga=\"%s\"")
                                     % nl
                                     % ((image_loader_args.fpga_path == "")
                                             ? find_image_path("usrp2_fpga.bin")
                                             : image_loader_args.fpga_path));
    }

    throw uhd::runtime_error(str(boost::format("The specified device is a USRP2, which is not supported by this utility.\n"
                                               "Instead, plug the device's SD card into your machine and run this command:\n\n"
                                               "%s"
                                              ) % usrp2_card_burner_gui));
}

/*
 * Ethernet communication functions
 */
static UHD_INLINE size_t n200_send_and_recv(udp_simple::sptr xport,
                                            n200_fw_update_id_t pkt_code,
                                            n200_fw_update_data_t *pkt_out,
                                            uint8_t* data){
    pkt_out->proto_ver = htonx<uint32_t>(USRP2_FW_COMPAT_NUM);
    pkt_out->id = htonx<uint32_t>(pkt_code);
    xport->send(boost::asio::buffer(pkt_out, sizeof(*pkt_out)));
    return xport->recv(boost::asio::buffer(data, udp_simple::mtu), UDP_TIMEOUT);
}

static UHD_INLINE bool n200_response_matches(const n200_fw_update_data_t *pkt_in,
                                             n200_fw_update_id_t pkt_code,
                                             size_t len){
    return (len > offsetof(n200_fw_update_data_t, data) and
            ntohl(pkt_in->id) == (unsigned)pkt_code);
}

static uhd::device_addr_t n200_find(const image_loader::image_loader_args_t &image_loader_args){
    bool user_specified = image_loader_args.args.has_key("addr") or
                          image_loader_args.args.has_key("serial") or
                          image_loader_args.args.has_key("name");

    uhd::device_addrs_t found = usrp2_find(image_loader_args.args);

    if(found.size() > 0){
        uhd::device_addrs_t n200_found;
        udp_simple::sptr rev_xport;
        n200_fw_update_data_t pkt_out;
        uint8_t data_in[udp_simple::mtu];
        const n200_fw_update_data_t *pkt_in = reinterpret_cast<const n200_fw_update_data_t*>(data_in);
        size_t len = 0;

        /*
         * Filter out any USRP2 devices by sending a query over the
         * UDP update port. Only N-Series devices will respond to
         * this query. If the user supplied specific arguments that
         * led to a USRP2, throw an error.
         */
        for(const uhd::device_addr_t &dev:  found){
            rev_xport = udp_simple::make_connected(
                            dev.get("addr"),
                            BOOST_STRINGIZE(N200_UDP_FW_UPDATE_PORT)
                        );

            len = n200_send_and_recv(rev_xport, GET_HW_REV_CMD, &pkt_out, data_in);
            if(n200_response_matches(pkt_in, GET_HW_REV_ACK, len)){
                uint32_t rev = ntohl(pkt_in->data.hw_rev);
                std::string hw_rev = n200_filename_map.get(rev, "n2xx");

                n200_found.push_back(dev);
                n200_found[n200_found.size()-1]["hw_rev"] = hw_rev;
            }
            else if(len > offsetof(n200_fw_update_data_t, data) and ntohl(pkt_in->id) != GET_HW_REV_ACK){
                throw uhd::runtime_error(str(boost::format("Received invalid reply %d from device.")
                                             % ntohl(pkt_in->id)));
            }
            else if(user_specified){
                // At this point, we haven't received any response, so assume it's a USRP2
                print_usrp2_error(image_loader_args);
            }
        }

        // At this point, we should have a single N-Series device
        if(n200_found.size() == 1){
            return n200_found[0];
        }
        else if(n200_found.size() > 1){
            std::string err_msg = "Could not resolve given args to a single N-Series device.\n"
                                  "Applicable devices:\n";

            for(const uhd::device_addr_t &dev:  n200_found){
                err_msg += str(boost::format("* %s (addr=%s)\n")
                               % dev.get("hw_rev")
                               % dev.get("addr"));
            }

            err_msg += "\nSpecify one of these devices with the given args to load an image onto it.";

            throw uhd::runtime_error(err_msg);
        }
    }

    return uhd::device_addr_t();
}

/*
 * Validate and read firmware image
 */
static void n200_validate_firmware_image(n200_session_t &session){
    if(not fs::exists(session.filepath)){
        throw uhd::runtime_error(str(boost::format("Could not find image at path \"%s\".")
                                     % session.filepath));
    }

    session.size     = fs::file_size(session.filepath);
    session.max_size = N200_FW_MAX_SIZE_BYTES;

    if(session.size > session.max_size){
        throw uhd::runtime_error(str(boost::format("The specified firmware image is too large: %d vs. %d")
                                     % session.size % session.max_size));
    }

    // File must have proper header
    std::ifstream image_file(session.filepath.c_str(), std::ios::binary);
    uint8_t test_bytes[4];
    image_file.seekg(0, std::ios::beg);
    image_file.read((char*)test_bytes,4);
    image_file.close();
    for(int i = 0; i < 4; i++) if(test_bytes[i] != 11){
        throw uhd::runtime_error(str(boost::format("The file at path \"%s\" is not a valid firmware image.")
                                     % session.filepath));
    }
}

/*
 * Validate and validate FPGA image
 */
static void n200_validate_fpga_image(n200_session_t &session){
    if(not fs::exists(session.filepath)){
        throw uhd::runtime_error(str(boost::format("Could not find image at path \"%s\".")
                                     % session.filepath));
    }

    session.size     = fs::file_size(session.filepath);
    session.max_size = N200_FPGA_MAX_SIZE_BYTES;

    if(session.size > session.max_size){
        throw uhd::runtime_error(str(boost::format("The specified FPGA image is too large: %d vs. %d")
                                     % session.size % session.max_size));
    }

    // File must have proper header
    std::ifstream image_file(session.filepath.c_str(), std::ios::binary);
    uint8_t test_bytes[63];
    image_file.seekg(0, std::ios::beg);
    image_file.read((char*)test_bytes, 63);
    bool is_good = false;
    for(int i = 0; i < 62; i++){
        if(test_bytes[i] == 255) continue;
        else if(test_bytes[i] == 170 and
                test_bytes[i+1] == 153){
            is_good = true;
            break;
        }
    }
    image_file.close();
    if(not is_good){
        throw uhd::runtime_error(str(boost::format("The file at path \"%s\" is not a valid FPGA image.")
                                     % session.filepath));
    }
}

/*
 * Set up a session for burning an N-Series image. This session info
 * will be passed into the erase, burn, and verify functions.
 */
static void n200_setup_session(n200_session_t &session,
                               const image_loader::image_loader_args_t &image_loader_args,
                               bool fw){


    session.fw = fw;
    session.reset = image_loader_args.args.has_key("reset");

    /*
     * If no filepath is given, attempt to determine the default image by
     * querying the device for its revision. If the device has a corrupt
     * EEPROM or is otherwise unable to provide its revision, this is
     * impossible, and the user must manually provide a firmware file.
     */
    if((session.fw and image_loader_args.firmware_path == "") or
       image_loader_args.fpga_path == ""){
        if(session.dev_addr["hw_rev"] == "n2xx"){
            throw uhd::runtime_error("This device's revision cannot be determined. "
                                     "You must manually specify a filepath.");
        }
        else{
            session.filepath = session.fw ? find_image_path(str(boost::format("usrp_%s_fw.bin")
                                                                % erase_tail_copy(session.dev_addr["hw_rev"],3)))
                                          : find_image_path(str(boost::format("usrp_%s_fpga.bin")
                                                                % session.dev_addr["hw_rev"]));
        }
    }
    else{
        session.filepath = session.fw ? image_loader_args.firmware_path
                                      : image_loader_args.fpga_path;
    }
    if(session.fw) n200_validate_firmware_image(session);
    else           n200_validate_fpga_image(session);

    session.overwrite_safe = image_loader_args.args.has_key("overwrite-safe");
    if(session.overwrite_safe){
        session.flash_addr = session.fw ? N200_SAFE_FW_IMAGE_ADDR
                                        : N200_SAFE_FPGA_IMAGE_ADDR;
        session.burn_type = session.fw ? "firmware safe"
                                       : "FPGA safe";
    }
    else{
        session.flash_addr = session.fw ? N200_PROD_FW_IMAGE_ADDR
                                        : N200_PROD_FPGA_IMAGE_ADDR;
        session.burn_type = session.fw ? "firmware"
                                       : "FPGA";
    }

    session.xport = udp_simple::make_connected(session.dev_addr["addr"],
                                               BOOST_STRINGIZE(N200_UDP_FW_UPDATE_PORT));
}

static void n200_erase_image(n200_session_t &session){

    // UDP receive buffer
    n200_fw_update_data_t pkt_out;
    const n200_fw_update_data_t *pkt_in = reinterpret_cast<const n200_fw_update_data_t*>(session.data_in);

    // Setting up UDP packet
    pkt_out.data.flash_args.flash_addr = htonx<uint32_t>(session.flash_addr);
    pkt_out.data.flash_args.length = htonx<uint32_t>(session.size);

    // Begin erasing
    size_t len = n200_send_and_recv(session.xport, ERASE_FLASH_CMD, &pkt_out, session.data_in);
    if(n200_response_matches(pkt_in, ERASE_FLASH_ACK, len)){
        std::cout << boost::format("-- Erasing %s image...") % session.burn_type << std::flush;
    }
    else if(len < offsetof(n200_fw_update_data_t, data)){
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Timed out waiting for reply from device.");
    }
    else if(ntohl(pkt_in->id) != ERASE_FLASH_ACK){
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error(str(boost::format("Received invalid reply %d from device.\n")
                                     % ntohl(pkt_in->id)));
    }
    else{
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Did not receive response from device.");
    }

    // Check for erase completion
    while(true){
        len = n200_send_and_recv(session.xport, CHECK_ERASING_DONE_CMD, &pkt_out, session.data_in);
        if(n200_response_matches(pkt_in, DONE_ERASING_ACK, len)){
            std::cout << "successful." << std::endl;
            break;
        }
        else if(len < offsetof(n200_fw_update_data_t, data)){
            std::cout << "failed." << std::endl;
            throw uhd::runtime_error("Timed out waiting for reply from device.");
        }
        else if(ntohl(pkt_in->id) != NOT_DONE_ERASING_ACK){
            std::cout << "failed." << std::endl;
            throw uhd::runtime_error(str(boost::format("Received invalid reply %d from device.\n")
                                         % ntohl(pkt_in->id)));
        }
    }
}

static void n200_write_image(n200_session_t &session){

    // UDP receive buffer
    n200_fw_update_data_t pkt_out;
    const n200_fw_update_data_t *pkt_in = reinterpret_cast<const n200_fw_update_data_t*>(session.data_in);
    size_t len = 0;

    // Write image
    std::ifstream image(session.filepath.c_str(), std::ios::binary);
    uint32_t current_addr = session.flash_addr;
    pkt_out.data.flash_args.length = htonx<uint32_t>(N200_FLASH_DATA_PACKET_SIZE);
    for(size_t i = 0; i < ((session.size/N200_FLASH_DATA_PACKET_SIZE)+1); i++){
        pkt_out.data.flash_args.flash_addr = htonx<uint32_t>(current_addr);
        memset(pkt_out.data.flash_args.data, 0x0, N200_FLASH_DATA_PACKET_SIZE);
        image.read((char*)pkt_out.data.flash_args.data, N200_FLASH_DATA_PACKET_SIZE);

        len = n200_send_and_recv(session.xport, WRITE_FLASH_CMD, &pkt_out, session.data_in);
        if(n200_response_matches(pkt_in, WRITE_FLASH_ACK, len)){
            std::cout << boost::format("\r-- Writing %s image (%d%%)")
                             % session.burn_type
                             % int((double(current_addr-session.flash_addr)/double(session.size))*100)
                      << std::flush;
        }
        else if(len < offsetof(n200_fw_update_data_t, data)){
            image.close();
            std::cout << boost::format("\r--Writing %s image..failed at %d%%.")
                             % session.burn_type
                             % int((double(current_addr-session.flash_addr)/double(session.size))*100)
                      << std::endl;
            throw uhd::runtime_error("Timed out waiting for reply from device.");
        }
        else if(ntohl(pkt_in->id) != WRITE_FLASH_ACK){
            image.close();
            std::cout << boost::format("\r--Writing %s image..failed at %d%%.")
                             % session.burn_type
                             % int((double(current_addr-session.flash_addr)/double(session.size))*100)
                      << std::endl;
            throw uhd::runtime_error(str(boost::format("Received invalid reply %d from device.\n")
                                             % ntohl(pkt_in->id)));
        }

        current_addr += N200_FLASH_DATA_PACKET_SIZE;
    }
    std::cout << boost::format("\r-- Writing %s image...successful.")
                     % session.burn_type
              << std::endl;

    image.close();
}

static void n200_verify_image(n200_session_t &session){

    // UDP receive buffer
    n200_fw_update_data_t pkt_out;
    const n200_fw_update_data_t *pkt_in = reinterpret_cast<const n200_fw_update_data_t*>(session.data_in);
    size_t len = 0;

    // Read and verify image
    std::ifstream image(session.filepath.c_str(), std::ios::binary);
    uint8_t image_part[N200_FLASH_DATA_PACKET_SIZE];
    uint32_t current_addr = session.flash_addr;
    pkt_out.data.flash_args.length = htonx<uint32_t>(N200_FLASH_DATA_PACKET_SIZE);
    uint16_t cmp_len = 0;
    for(size_t i = 0; i < ((session.size/N200_FLASH_DATA_PACKET_SIZE)+1); i++){
        memset(image_part, 0x0, N200_FLASH_DATA_PACKET_SIZE);
        memset((void*)pkt_in->data.flash_args.data, 0x0, N200_FLASH_DATA_PACKET_SIZE);

        pkt_out.data.flash_args.flash_addr = htonx<uint32_t>(current_addr);
        image.read((char*)image_part, N200_FLASH_DATA_PACKET_SIZE);
        cmp_len = image.gcount();

        len = n200_send_and_recv(session.xport, READ_FLASH_CMD, &pkt_out, session.data_in);
        if(n200_response_matches(pkt_in, READ_FLASH_ACK, len)){
            std::cout << boost::format("\r-- Verifying %s image (%d%%)")
                             % session.burn_type
                             % int((double(current_addr-session.flash_addr)/double(session.size))*100)
                      << std::flush;

            if(memcmp(image_part, pkt_in->data.flash_args.data, cmp_len)){
                std::cout << boost::format("\r-- Verifying %s image...failed at %d%%.")
                                 % session.burn_type
                                 % int((double(current_addr-session.flash_addr)/double(session.size))*100)
                          << std::endl;
                throw uhd::runtime_error(str(boost::format("Failed to verify %s image.")
                                                 % session.burn_type));
            }
        }
        else if(len < offsetof(n200_fw_update_data_t, data)){
            image.close();
            std::cout << boost::format("\r-- Verifying %s image...failed at %d%%.")
                             % session.burn_type
                             % int((double(current_addr-session.flash_addr)/double(session.size))*100)
                      << std::endl;
            throw uhd::runtime_error("Timed out waiting for reply from device.");
        }
        else if(ntohl(pkt_in->id) != READ_FLASH_ACK){
            image.close();
            std::cout << boost::format("\r-- Verifying %s image...failed at %d%%.")
                             % session.burn_type
                             % int((double(current_addr-session.flash_addr)/double(session.size))*100)
                      << std::endl;
            throw uhd::runtime_error(str(boost::format("Received invalid reply %d from device.\n")
                                         % ntohl(pkt_in->id)));
        }

        current_addr += N200_FLASH_DATA_PACKET_SIZE;
    }
    std::cout << boost::format("\r-- Verifying %s image...successful.") % session.burn_type
              << std::endl;

    image.close();
}

static void n200_reset(n200_session_t &session){

    // UDP receive buffer
    n200_fw_update_data_t pkt_out;

    // There should be no response
    std::cout << "-- Resetting device..." << std::flush;
    size_t len = n200_send_and_recv(session.xport, RESET_CMD, &pkt_out, session.data_in);
    if(len > 0){
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Failed to reset N200.");
    }
    std::cout << "successful." << std::endl;
}

// n210_r4 -> N210 r4
static std::string nice_name(const std::string &fw_rev){
    std::string ret = fw_rev;
    ret[0] = ::toupper(ret[0]);

    size_t pos = 0;
    if((pos = fw_rev.find("_")) != std::string::npos){
        ret[pos] = ' ';
    }

    return ret;
}

static bool n200_image_loader(const image_loader::image_loader_args_t &image_loader_args){
    if(!image_loader_args.load_firmware and !image_loader_args.load_fpga){
        return false;
    }

    // See if any N2x0 with the given args is found
    // This will throw if specific args lead to a USRP2
    n200_session_t session;
    session.dev_addr = n200_find(image_loader_args);
    if(session.dev_addr.size() == 0){
        return false;
    }

    std::cout << boost::format("Unit: USRP %s (%s, %s)")
                 % nice_name(session.dev_addr.get("hw_rev"))
                 % session.dev_addr.get("serial")
                 % session.dev_addr.get("addr")
             << std::endl;

    if(image_loader_args.load_firmware){
        n200_setup_session(session,
                           image_loader_args,
                           true
                          );

        std::cout << "Firmware image: " << session.filepath << std::endl;

        n200_erase_image(session);
        n200_write_image(session);
        n200_verify_image(session);
        if(session.reset and !image_loader_args.load_fpga){
            n200_reset(session);
        }
    }
    if(image_loader_args.load_fpga){
        n200_setup_session(session,
                           image_loader_args,
                           false
                          );

        std::cout << "FPGA image: " << session.filepath << std::endl;

        n200_erase_image(session);
        n200_write_image(session);
        n200_verify_image(session);
        if(session.reset){
            n200_reset(session);
        }
    }

    return true;
}

UHD_STATIC_BLOCK(register_n200_image_loader){
    std::string recovery_instructions = "Aborting. Your USRP-N Series unit will likely be unusable.\n"
                                        "Refer to http://files.ettus.com/manual/page_usrp2.html#usrp2_loadflash_brick\n"
                                        "for details on restoring your device.";

    image_loader::register_image_loader("usrp2", n200_image_loader, recovery_instructions);
}
