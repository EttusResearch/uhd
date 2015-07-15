//
// Copyright 2012-2015 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <csignal>
#include <iostream>
#include <fstream>
#include <time.h>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/assign.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>

#include <uhd/exception.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/safe_call.hpp>

namespace fs = boost::filesystem;
namespace po = boost::program_options;
using namespace boost::algorithm;
using namespace uhd;
using namespace uhd::transport;

#define UDP_FW_UPDATE_PORT 49154
#define UDP_MAX_XFER_BYTES 1024
#define UDP_TIMEOUT 3
#define UDP_POLL_INTERVAL 0.10 //in seconds
#define USRP2_FW_PROTO_VERSION 7 //should be unused after r6
#define USRP2_UDP_UPDATE_PORT 49154
#define FLASH_DATA_PACKET_SIZE 256
#define FPGA_IMAGE_SIZE_BYTES 1572864
#define FW_IMAGE_SIZE_BYTES 31744

#define PROD_FPGA_IMAGE_LOCATION_ADDR 0x00180000
#define SAFE_FPGA_IMAGE_LOCATION_ADDR 0x00000000

#define PROD_FW_IMAGE_LOCATION_ADDR 0x00300000
#define SAFE_FW_IMAGE_LOCATION_ADDR 0x003F0000

typedef enum {
    UNKNOWN = ' ',

    USRP2_QUERY = 'a',
    USRP2_ACK = 'A',

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

    RESET_USRP_CMD = 's',
    RESET_USRP_ACK = 'S',

    GET_HW_REV_CMD = 'v',
    GET_HW_REV_ACK = 'V',

} usrp2_fw_update_id_t;

typedef struct {
    boost::uint32_t proto_ver;
    boost::uint32_t id;
    boost::uint32_t seq;
    union {
        boost::uint32_t ip_addr;
        boost::uint32_t hw_rev;
        struct {
            boost::uint32_t flash_addr;
            boost::uint32_t length;
            boost::uint8_t  data[256];
        } flash_args;
        struct {
            boost::uint32_t sector_size_bytes;
            boost::uint32_t memory_size_bytes;
        } flash_info_args;
    } data;
} usrp2_fw_update_data_t;

//Mapping revision numbers to filenames
uhd::dict<boost::uint32_t, std::string> filename_map = boost::assign::map_list_of
    (0,      "N2XX")
    (0xa,    "n200_r3")
    (0x100a, "n200_r4")
    (0x10a,  "n210_r3")
    (0x110a, "n210_r4")
;

boost::uint8_t usrp2_update_data_in_mem[udp_simple::mtu];
boost::uint8_t fpga_image[FPGA_IMAGE_SIZE_BYTES];
boost::uint8_t fw_image[FW_IMAGE_SIZE_BYTES];

/***********************************************************************
 * Signal handlers
 **********************************************************************/
static int num_ctrl_c = 0;
void sig_int_handler(int){
    num_ctrl_c++;
    if(num_ctrl_c == 1){
        std::cout << std::endl << "Are you sure you want to abort the image burning? If you do, your "
                                  "USRP-N Series unit will be bricked!" << std::endl
                               << "Press Ctrl+C again to abort the image burning procedure." << std::endl << std::endl;
    }
    else{
        std::cout << std::endl << "Aborting. Your USRP-N Series unit will be bricked." << std::endl
                  << "Refer to http://files.ettus.com/manual/page_usrp2.html#usrp2_loadflash_brick" << std::endl
                  << "for details on restoring your device." << std::endl;
        exit(EXIT_FAILURE);
    }
}

/***********************************************************************
 * List all connected USRP N2XX devices
 **********************************************************************/
void list_usrps(){
    udp_simple::sptr udp_bc_transport;
    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);
    boost::uint32_t hw_rev;

    usrp2_fw_update_data_t usrp2_ack_pkt = usrp2_fw_update_data_t();
    usrp2_ack_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);
    usrp2_ack_pkt.id = htonx<boost::uint32_t>(USRP2_QUERY);

    std::cout << "Available USRP N2XX devices:" << std::endl;

    //Send UDP packets to all broadcast addresses
    BOOST_FOREACH(const if_addrs_t &if_addrs, get_if_addrs()){
        //Avoid the loopback device
        if(if_addrs.inet == boost::asio::ip::address_v4::loopback().to_string()) continue;
        udp_bc_transport = udp_simple::make_broadcast(if_addrs.bcast, BOOST_STRINGIZE(USRP2_UDP_UPDATE_PORT));
        udp_bc_transport->send(boost::asio::buffer(&usrp2_ack_pkt, sizeof(usrp2_ack_pkt)));

        size_t len = udp_bc_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
        if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == USRP2_ACK){
            usrp2_ack_pkt.id = htonx<boost::uint32_t>(GET_HW_REV_CMD);
            udp_bc_transport->send(boost::asio::buffer(&usrp2_ack_pkt, sizeof(usrp2_ack_pkt)));

            size_t len = udp_bc_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
            if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == GET_HW_REV_ACK){
                hw_rev = ntohl(update_data_in->data.hw_rev);
            }

            std::cout << boost::format(" * %s (%s)\n") % udp_bc_transport->get_recv_addr() % filename_map[hw_rev];
        }
    }
}

/***********************************************************************
 * Find USRP N2XX with specified IP address and return type
 **********************************************************************/
void print_image_loader_warning(const std::string &fw_path,
                                const std::string &fpga_path,
                                const po::variables_map &vm){

    // Newline + indent
    #ifdef UHD_PLATFORM_WIN32
    const std::string nl = " ^\n    ";
    #else
    const std::string nl = " \\\n    ";
    #endif

    std::string uhd_image_loader = str(boost::format("uhd_image_loader --args=\"type=usrp2,addr=%s")
                                       % vm["addr"].as<std::string>());
    if(vm.count("auto-reboot") > 0)
        uhd_image_loader += ",reset";
    if(vm.count("overwrite-safe") > 0)
        uhd_image_loader += ",overwrite-safe";
    if(vm.count("dont-check-rev") > 0)
        uhd_image_loader += ",dont-check-rev";

    uhd_image_loader += "\"";

    if(vm.count("no-fw") == 0){
        uhd_image_loader += str(boost::format("%s--fw-path=\"%s\"")
                                % nl % fw_path);
    }
    else{
        uhd_image_loader += str(boost::format("%s--no-fw")
                                % nl);
    }

    if(vm.count("no-fpga") == 0){
        uhd_image_loader += str(boost::format("%s--fpga-path=\"%s\"")
                                % nl % fpga_path);
    }
    else{
        uhd_image_loader += str(boost::format("%s--no-fpga")
                                % nl);
    }

    std::cout << "************************************************************************************************" << std::endl
              << "WARNING: This utility will be removed in an upcoming version of UHD. In the future, use" << std::endl
              << "         this command:" << std::endl
              << std::endl
              << uhd_image_loader << std::endl
              << std::endl
              << "************************************************************************************************" << std::endl
              << std::endl;
}

/***********************************************************************
 * Find USRP N2XX with specified IP address and return type
 **********************************************************************/
boost::uint32_t find_usrp(udp_simple::sptr udp_transport, bool check_rev){
    boost::uint32_t hw_rev;
    bool found_it = false;

    // If the user chooses to not care about the rev, simply check
    // for the presence of a USRP N2XX.
    boost::uint32_t cmd_id = (check_rev) ? GET_HW_REV_CMD
                                         : USRP2_QUERY;
    boost::uint32_t ack_id = (check_rev) ? GET_HW_REV_ACK
                                         : USRP2_ACK;

    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);
    usrp2_fw_update_data_t hw_info_pkt = usrp2_fw_update_data_t();
    hw_info_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);
    hw_info_pkt.id = htonx<boost::uint32_t>(cmd_id);
    udp_transport->send(boost::asio::buffer(&hw_info_pkt, sizeof(hw_info_pkt)));

    //Loop and receive until the timeout
    size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
    if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == ack_id){
        hw_rev = ntohl(update_data_in->data.hw_rev);
        if(filename_map.has_key(hw_rev)){
            std::cout << boost::format("Found %s.\n\n") % filename_map[hw_rev];
            found_it = true;
        }
        else{
            if(check_rev) throw std::runtime_error("Invalid revision found.");
            else{
                hw_rev = 0;
                std::cout << "Found USRP N2XX." << std::endl;
                found_it = true;
            }
        }
    }
    if(not found_it) throw std::runtime_error("No USRP N2XX found.");

    return hw_rev;
}

/***********************************************************************
 * Custom filename validation functions
 **********************************************************************/

void validate_custom_fpga_file(std::string rev_str, std::string& fpga_path, bool check_rev){

    //Check for existence of file
    if(not fs::exists(fpga_path)) throw std::runtime_error(str(boost::format("No file at specified FPGA path: %s") % fpga_path));

    //If user cares about revision, use revision string to detect invalid image filename
    uhd::fs_path custom_fpga_path(fpga_path);
    if(custom_fpga_path.leaf().find(rev_str) == std::string::npos and check_rev){
        throw std::runtime_error(str(boost::format("Invalid FPGA image filename at path: %s\nFilename must contain '%s' to be considered valid for this model.")
            % fpga_path % rev_str));
    }
}

void validate_custom_fw_file(std::string rev_str, std::string& fw_path, bool check_rev){

    //Check for existence of file
    if(not fs::exists(fw_path)) throw std::runtime_error(str(boost::format("No file at specified firmware path: %s") % fw_path));

    //If user cares about revision, use revision string to detect invalid image filename
    uhd::fs_path custom_fw_path(fw_path);
    if(custom_fw_path.leaf().find(erase_tail_copy(rev_str,3)) == std::string::npos and check_rev){
        throw std::runtime_error(str(boost::format("Invalid firmware image filename at path: %s\nFilename must contain '%s' to be considered valid for this model.")
            % fw_path % erase_tail_copy(rev_str,3)));
    }
}

/***********************************************************************
 * Reading and validating image binaries
 **********************************************************************/

int read_fpga_image(std::string& fpga_path){

    //Check size of given image
    std::ifstream fpga_file(fpga_path.c_str(), std::ios::binary);
    fpga_file.seekg(0, std::ios::end);
    size_t fpga_image_size = size_t(fpga_file.tellg());
    if(fpga_image_size > FPGA_IMAGE_SIZE_BYTES){
        throw std::runtime_error(str(boost::format("FPGA image is too large. %d > %d")
                                     % fpga_image_size % FPGA_IMAGE_SIZE_BYTES));
    }

    //Check sequence of bytes in image before reading
    boost::uint8_t fpga_test_bytes[63];
    fpga_file.seekg(0, std::ios::beg);
    fpga_file.read((char*)fpga_test_bytes,63);
    bool is_good = false;
    for(int i = 0; i < 63; i++){
        if(fpga_test_bytes[i] == 255) continue;
        else if(fpga_test_bytes[i] == 170 and
                fpga_test_bytes[i+1] == 153){
            is_good = true;
            break;
        }
    }
    if(not is_good) throw std::runtime_error("Not a valid FPGA image.");

    //With image validated, read into utility
    fpga_file.seekg(0, std::ios::beg);
    fpga_file.read((char*)fpga_image,fpga_image_size);
    fpga_file.close();

    //Return image size
    return fpga_image_size;
}

int read_fw_image(std::string& fw_path){

    //Check size of given image
    std::ifstream fw_file(fw_path.c_str(), std::ios::binary);
    fw_file.seekg(0, std::ios::end);
    size_t fw_image_size = size_t(fw_file.tellg());
    if(fw_image_size > FW_IMAGE_SIZE_BYTES){
        throw std::runtime_error(str(boost::format("Firmware image is too large. %d > %d")
                                     % fw_image_size % FW_IMAGE_SIZE_BYTES));
    }

    //Check sequence of bytes in image before reading
    boost::uint8_t fw_test_bytes[4];
    fw_file.seekg(0, std::ios::beg);
    fw_file.read((char*)fw_test_bytes,4);
    for(int i = 0; i < 4; i++) if(fw_test_bytes[i] != 11) throw std::runtime_error("Not a valid firmware image.");

    //With image validated, read into utility
    fw_file.seekg(0, std::ios::beg);
    fw_file.read((char*)fw_image,fw_image_size);
    fw_file.close();

    return fw_image_size;
}

boost::uint32_t* get_flash_info(std::string& ip_addr){

    boost::uint32_t *flash_info = new boost::uint32_t[2];
    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);

    udp_simple::sptr udp_transport = udp_simple::make_connected(ip_addr, BOOST_STRINGIZE(USRP2_UDP_UPDATE_PORT));
    usrp2_fw_update_data_t get_flash_info_pkt = usrp2_fw_update_data_t();
    get_flash_info_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);
    get_flash_info_pkt.id = htonx<boost::uint32_t>(GET_FLASH_INFO_CMD);
    udp_transport->send(boost::asio::buffer(&get_flash_info_pkt, sizeof(get_flash_info_pkt)));

    //Loop and receive until the timeout
    size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
    if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == GET_FLASH_INFO_ACK){
        flash_info[0] = ntohl(update_data_in->data.flash_info_args.sector_size_bytes);
        flash_info[1] = ntohl(update_data_in->data.flash_info_args.memory_size_bytes);
    }
    else if(ntohl(update_data_in->id) != GET_FLASH_INFO_ACK){
        throw std::runtime_error(str(boost::format("Received invalid reply %d from device.\n")
                                     % ntohl(update_data_in->id)));
    }

    return flash_info;
}

/***********************************************************************
 * Image burning functions
 **********************************************************************/

void erase_image(udp_simple::sptr udp_transport, bool is_fw, boost::uint32_t memory_size, bool overwrite_safe){

    boost::uint32_t image_location_addr = is_fw ? overwrite_safe ? SAFE_FW_IMAGE_LOCATION_ADDR
                                                                 : PROD_FW_IMAGE_LOCATION_ADDR
                                                : overwrite_safe ? SAFE_FPGA_IMAGE_LOCATION_ADDR
                                                                 : PROD_FPGA_IMAGE_LOCATION_ADDR;
    boost::uint32_t image_size = is_fw ? FW_IMAGE_SIZE_BYTES
                                       : FPGA_IMAGE_SIZE_BYTES;

    //Making sure this won't attempt to erase past end of device
    if((image_location_addr+image_size) > memory_size) throw std::runtime_error("Cannot erase past end of device.");

    //UDP receive buffer
    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);

    //Setting up UDP packet
    usrp2_fw_update_data_t erase_pkt = usrp2_fw_update_data_t();
    erase_pkt.id = htonx<boost::uint32_t>(ERASE_FLASH_CMD);
    erase_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);
    erase_pkt.data.flash_args.flash_addr = htonx<boost::uint32_t>(image_location_addr);
    erase_pkt.data.flash_args.length = htonx<boost::uint32_t>(image_size);

    //Begin erasing
    udp_transport->send(boost::asio::buffer(&erase_pkt, sizeof(erase_pkt)));
    size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
    if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == ERASE_FLASH_ACK){
        if(is_fw) std::cout << "Erasing firmware image." << std::endl;
        else      std::cout << "Erasing FPGA image." << std::endl;
    }
    else if(ntohl(update_data_in->id) != ERASE_FLASH_ACK){
        throw std::runtime_error(str(boost::format("Received invalid reply %d from device.\n")
                                     % ntohl(update_data_in->id)));
    }

    //Check for erase completion
    erase_pkt.id = htonx<boost::uint32_t>(CHECK_ERASING_DONE_CMD);
    while(true){
        udp_transport->send(boost::asio::buffer(&erase_pkt, sizeof(erase_pkt)));
        size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
        if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == DONE_ERASING_ACK){
            std::cout << boost::format(" * Successfully erased %d bytes at %d.\n")
                         % image_size % image_location_addr;
            break;
        }
        else if(ntohl(update_data_in->id) != NOT_DONE_ERASING_ACK){
            throw std::runtime_error(str(boost::format("Received invalid reply %d from device.\n")
                                         % ntohl(update_data_in->id)));
        }
    }
}

void write_image(udp_simple::sptr udp_transport, bool is_fw, boost::uint8_t* image,
                 boost::uint32_t memory_size, int image_size, bool overwrite_safe){

    boost::uint32_t begin_addr = is_fw ? overwrite_safe ? SAFE_FW_IMAGE_LOCATION_ADDR
                                                        : PROD_FW_IMAGE_LOCATION_ADDR
                                       : overwrite_safe ? SAFE_FPGA_IMAGE_LOCATION_ADDR
                                                        : PROD_FPGA_IMAGE_LOCATION_ADDR;
    boost::uint32_t current_addr = begin_addr;
    std::string type = is_fw ? "firmware" : "FPGA";

    //Making sure this won't attempt to write past end of device
    if(current_addr+image_size > memory_size) throw std::runtime_error("Cannot write past end of device.");

    //UDP receive buffer
    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);

    //Setting up UDP packet
    usrp2_fw_update_data_t write_pkt = usrp2_fw_update_data_t();
    write_pkt.id = htonx<boost::uint32_t>(WRITE_FLASH_CMD);
    write_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);
    write_pkt.data.flash_args.length = htonx<boost::uint32_t>(FLASH_DATA_PACKET_SIZE);

    for(int i = 0; i < ((image_size/FLASH_DATA_PACKET_SIZE)+1); i++){
        //Print progress
        std::cout << "\rWriting " << type << " image ("
                  << int((double(current_addr-begin_addr)/double(image_size))*100) << "%)." << std::flush;

        write_pkt.data.flash_args.flash_addr = htonx<boost::uint32_t>(current_addr);
        std::copy(image+(i*FLASH_DATA_PACKET_SIZE), image+((i+1)*FLASH_DATA_PACKET_SIZE), write_pkt.data.flash_args.data);

        udp_transport->send(boost::asio::buffer(&write_pkt, sizeof(write_pkt)));
        size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
        if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) != WRITE_FLASH_ACK){
            throw std::runtime_error(str(boost::format("Invalid reply %d from device.")
                                         % ntohl(update_data_in->id)));
        }

        current_addr += FLASH_DATA_PACKET_SIZE;
    }
    std::cout << std::flush << "\rWriting " << type << " image (100%)." << std::endl;
    std::cout << boost::format(" * Successfully wrote %d bytes.\n") % image_size;
}

void verify_image(udp_simple::sptr udp_transport, bool is_fw, boost::uint8_t* image,
                  boost::uint32_t memory_size, int image_size, bool overwrite_safe){

    int current_index = 0;
    boost::uint32_t begin_addr = is_fw ? overwrite_safe ? SAFE_FW_IMAGE_LOCATION_ADDR
                                                        : PROD_FW_IMAGE_LOCATION_ADDR
                                       : overwrite_safe ? SAFE_FPGA_IMAGE_LOCATION_ADDR
                                                        : PROD_FPGA_IMAGE_LOCATION_ADDR;
    boost::uint32_t current_addr = begin_addr;
    std::string type = is_fw ? "firmware" : "FPGA";

    //Array size needs to be known at runtime, this constant is guaranteed to be larger than any firmware or FPGA image
    boost::uint8_t from_usrp[FPGA_IMAGE_SIZE_BYTES];

    //Making sure this won't attempt to read past end of device
    if(current_addr+image_size > memory_size) throw std::runtime_error("Cannot read past end of device.");

    //UDP receive buffer
    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);

    //Setting up UDP packet
    usrp2_fw_update_data_t verify_pkt = usrp2_fw_update_data_t();
    verify_pkt.id = htonx<boost::uint32_t>(READ_FLASH_CMD);
    verify_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);
    verify_pkt.data.flash_args.length = htonx<boost::uint32_t>(FLASH_DATA_PACKET_SIZE);

    for(int i = 0; i < ((image_size/FLASH_DATA_PACKET_SIZE)+1); i++){
        //Print progress
        std::cout << "\rVerifying " << type << " image ("
                  << int((double(current_addr-begin_addr)/double(image_size))*100) << "%)." << std::flush;

        verify_pkt.data.flash_args.flash_addr = htonx<boost::uint32_t>(current_addr);

        udp_transport->send(boost::asio::buffer(&verify_pkt, sizeof(verify_pkt)));
        size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
        if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) != READ_FLASH_ACK){
            throw std::runtime_error(str(boost::format("Invalid reply %d from device.")
                                         % ntohl(update_data_in->id)));
        }
        for(int j = 0; j < FLASH_DATA_PACKET_SIZE; j++) from_usrp[current_index+j] = update_data_in->data.flash_args.data[j];

        current_addr += FLASH_DATA_PACKET_SIZE;
        current_index += FLASH_DATA_PACKET_SIZE;
    }
    for(int i = 0; i < image_size; i++) if(from_usrp[i] != image[i]) throw std::runtime_error("Image write failed.");

    std::cout << std::flush << "\rVerifying " << type << " image (100%)." << std::endl;
    std::cout << " * Successful." << std::endl;
}

void reset_usrp(udp_simple::sptr udp_transport){

    //Set up UDP transport
    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);

    //Set up UDP packet
    usrp2_fw_update_data_t reset_pkt = usrp2_fw_update_data_t();
    reset_pkt.id = htonx<boost::uint32_t>(RESET_USRP_CMD);
    reset_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);

    //Reset USRP
    udp_transport->send(boost::asio::buffer(&reset_pkt, sizeof(reset_pkt)));
    size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
    if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == RESET_USRP_ACK){
        throw std::runtime_error("USRP reset failed."); //There should be no response to this UDP packet
    }
    else std::cout << std::endl << "Resetting USRP." << std::endl;
}

int UHD_SAFE_MAIN(int argc, char *argv[]){

    //Establish user options
    std::string fw_path;
    std::string ip_addr;
    std::string fpga_path;

    po::options_description desc("Allowed options:");
    desc.add_options()
        ("help", "Display this help message.")
        ("addr", po::value<std::string>(&ip_addr)->default_value("192.168.10.2"), "Specify an IP address.")
        ("fw", po::value<std::string>(&fw_path), "Specify a filepath for a custom firmware image.")
        ("fpga", po::value<std::string>(&fpga_path), "Specify a filepath for a custom FPGA image.")
        ("no-fw", "Do not burn a firmware image.")
        ("no-fpga", "Do not burn an FPGA image.")
        ("overwrite-safe", "Overwrite safe images (not recommended).")
        ("dont-check-rev", "Don't verify images are for correct model before burning.")
        ("auto-reboot", "Automatically reboot N2XX without prompting.")
        ("list", "List available N2XX USRP devices.")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //Print help message
    if(vm.count("help") > 0){
        std::cout << boost::format("N2XX Simple Net Burner\n");
        std::cout << boost::format("Automatically detects and burns standard firmware and FPGA images onto USRP N2XX devices.\n");
        std::cout << boost::format("Can optionally take user input for custom images.\n\n");
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    //List options
    if(vm.count("list")){
        list_usrps();
        return EXIT_SUCCESS;
    }

    //Store user options
    bool burn_fpga = (vm.count("no-fpga") == 0);
    bool burn_fw = (vm.count("no-fw") == 0);
    bool use_custom_fpga = (vm.count("fpga") > 0);
    bool use_custom_fw = (vm.count("fw") > 0);
    bool auto_reboot = (vm.count("auto-reboot") > 0);
    bool check_rev = (vm.count("dont-check-rev") == 0);
    bool overwrite_safe = (vm.count("overwrite-safe") > 0);
    int fpga_image_size = 0;
    int fw_image_size = 0;

    //Process options and detect invalid option combinations
    if(not burn_fpga && not burn_fw){
        std::cout << "No images will be burned." << std::endl;
        return EXIT_FAILURE;
    }
    if(not check_rev){
        //Without knowing a revision, the utility cannot automatically generate a filepath, so the user
        //must specify one. The user must also burn both types of images for consistency.
        if(not (burn_fpga and burn_fw))
            throw std::runtime_error("If the --dont-check-rev option is used, both FPGA and firmware images need to be burned.");
        if(not (use_custom_fpga and use_custom_fw))
            throw std::runtime_error("If the --dont-check-rev option is used, the user must specify image filepaths.");
    }
    if(overwrite_safe){
        //If the user specifies overwriting safe images, both image types must be burned for consistency.
        if(not (burn_fpga and burn_fw))
            throw std::runtime_error("If the --overwrite-safe option is used, both FPGA and firmware images need to be burned.");

        std::cout << "Are you REALLY sure you want to overwrite the safe images?" << std::endl;
        std::cout << "This is ALMOST ALWAYS a terrible idea." << std::endl;
        std::cout << "Type \"yes\" to continue, or anything else to quit: " << std::flush;
        std::string safe_response;
        std::getline(std::cin, safe_response);
        if(safe_response != "yes"){
            std::cout << "Exiting." << std::endl;
            return EXIT_SUCCESS;
        }
        else std::cout << std::endl; //Formatting
    }

    //Find USRP and establish connection
    std::cout << boost::format("Searching for USRP N2XX with IP address %s.\n") % ip_addr;
    udp_simple::sptr udp_transport = udp_simple::make_connected(ip_addr, BOOST_STRINGIZE(USRP2_UDP_UPDATE_PORT));
    boost::uint32_t hw_rev = find_usrp(udp_transport, check_rev);

    //Check validity of file locations and binaries before attempting burn
    std::cout << "Searching for specified images." << std::endl << std::endl;
    if(burn_fpga){
        if(use_custom_fpga){
            //Expand tilde usage if applicable
            #ifndef UHD_PLATFORM_WIN32
                if(fpga_path.find("~/") == 0) fpga_path.replace(0,1,getenv("HOME"));
            #endif
            validate_custom_fpga_file(filename_map[hw_rev], fpga_path, check_rev);
        }
        else{
            std::string default_fpga_filename = str(boost::format("usrp_%s_fpga.bin") % filename_map[hw_rev]);
            fpga_path = find_image_path(default_fpga_filename);
        }

        fpga_image_size = read_fpga_image(fpga_path);
    }
    if(burn_fw){
        if(use_custom_fw){
            //Expand tilde usage if applicable
            #ifndef UHD_PLATFORM_WIN32
                if(fw_path.find("~/") == 0) fw_path.replace(0,1,getenv("HOME"));
            #endif
            validate_custom_fw_file(filename_map[hw_rev], fw_path, check_rev);
        }
        else{
            std::string default_fw_filename = str(boost::format("usrp_%s_fw.bin") % erase_tail_copy(filename_map[hw_rev],3));
            fw_path = find_image_path(default_fw_filename);
        }

        fw_image_size = read_fw_image(fw_path);
    }

    print_image_loader_warning(fw_path, fpga_path, vm);

    std::cout << "Will burn the following images:" << std::endl;
    if(burn_fw) std::cout << boost::format(" * Firmware: %s\n") % fw_path;
    if(burn_fpga) std::cout << boost::format(" * FPGA:     %s\n") % fpga_path;
    std::cout << std::endl;

    boost::uint32_t* flash_info = get_flash_info(ip_addr);
    std::cout << boost::format("Querying %s for flash information.\n") % filename_map[hw_rev];
    std::cout << boost::format(" * Flash size:  %3.2f\n") % flash_info[1];
    std::cout << boost::format(" * Sector size: %3.2f\n\n") % flash_info[0];

    //Burning images
    std::signal(SIGINT, &sig_int_handler);
    if(burn_fpga){
        erase_image(udp_transport, false, flash_info[1], overwrite_safe);
        write_image(udp_transport, false, fpga_image, flash_info[1], fpga_image_size, overwrite_safe);
        verify_image(udp_transport, false, fpga_image, flash_info[1], fpga_image_size, overwrite_safe);
    }
    if(burn_fpga and burn_fw) std::cout << std::endl; //Formatting
    if(burn_fw){
        erase_image(udp_transport, true, flash_info[1], overwrite_safe);
        write_image(udp_transport, true, fw_image, flash_info[1], fw_image_size, overwrite_safe);
        verify_image(udp_transport, true, fw_image, flash_info[1], fw_image_size, overwrite_safe);
    }

    delete(flash_info);

    //Reset USRP N2XX
    bool reset = false;
    if(auto_reboot) reset = true;
    else{
        std::string user_response = "foo";
        while(user_response != "y" and user_response != "" and user_response != "n"){
            std::cout << std::endl << "Image burning successful. Reset USRP (Y/n)? ";
            std::getline(std::cin, user_response);
            std::transform(user_response.begin(), user_response.end(), user_response.begin(), ::tolower);
            reset = (user_response == "" or user_response == "y");
        }
        std::cout << std::endl; //Formatting
    }
    if(reset) reset_usrp(udp_transport);

    return EXIT_SUCCESS;
}
