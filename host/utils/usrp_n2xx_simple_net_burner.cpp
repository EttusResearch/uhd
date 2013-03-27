//
// Copyright 2012-2013 Ettus Research LLC
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

#include <iostream>
#include <map>
#include <fstream>
#include <time.h>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/assign.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>

#include "usrp_simple_burner_utils.hpp"
#include <uhd/exception.hpp>
#include <uhd/property_tree.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/images.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/safe_call.hpp>

namespace po = boost::program_options;
using namespace boost::algorithm;
using namespace uhd;
using namespace uhd::transport;

//Mapping revision numbers to filenames
std::map<boost::uint32_t, std::string> filename_map = boost::assign::map_list_of
    (0xa,    "n200_r3")
    (0x100a, "n200_r4")
    (0x10a,  "n210_r3")
    (0x110a, "n210_r4")
;

//Images and image sizes, to be populated as necessary
boost::uint8_t fpga_image[FPGA_IMAGE_SIZE_BYTES];
boost::uint8_t fw_image[FW_IMAGE_SIZE_BYTES];
int fpga_image_size = 0;
int fw_image_size = 0;

//For non-standard images not covered by uhd::find_image_path()
bool does_image_exist(std::string image_filepath){

    std::ifstream ifile((char*)image_filepath.c_str());
    return ifile;
}

/***********************************************************************
 * Custom filename validation functions
 **********************************************************************/

void validate_custom_fpga_file(std::string rev_str, std::string fpga_path){

    //Check for existence of file
    if(!does_image_exist(fpga_path)) throw std::runtime_error(str(boost::format("No file at specified FPGA path: %s") % fpga_path));

    //Check to find rev_str in filename
    uhd::fs_path custom_fpga_path(fpga_path);
    if(custom_fpga_path.leaf().find("fw") != std::string::npos){
        throw std::runtime_error(str(boost::format("Invalid FPGA image filename at path: %s\nFilename indicates that this is a firmware image.")
            % fpga_path));
    }
    if(custom_fpga_path.leaf().find(rev_str) == std::string::npos){
        throw std::runtime_error(str(boost::format("Invalid FPGA image filename at path: %s\nFilename must contain '%s' to be considered valid for this model.")
            % fpga_path % rev_str));
    }
}

void validate_custom_fw_file(std::string rev_str, std::string fw_path){

    //Check for existence of file
    if(!does_image_exist(fw_path)) throw std::runtime_error(str(boost::format("No file at specified firmware path: %s") % fw_path));

    //Check to find truncated rev_str in filename
    uhd::fs_path custom_fw_path(fw_path);
    if(custom_fw_path.leaf().find("fpga") != std::string::npos){
        throw std::runtime_error(str(boost::format("Invalid firmware image filename at path: %s\nFilename indicates that this is an FPGA image.")
            % fw_path));
    }
    if(custom_fw_path.leaf().find(erase_tail_copy(rev_str,3)) == std::string::npos){
        throw std::runtime_error(str(boost::format("Invalid firmware image filename at path: %s\nFilename must contain '%s' to be considered valid for this model.")
            % fw_path % erase_tail_copy(rev_str,3)));
    }
}

/***********************************************************************
 * Grabbing and validating image binaries
 **********************************************************************/

int grab_fpga_image(std::string fpga_path){

    //Reading FPGA image from file
    std::ifstream to_read_fpga((char*)fpga_path.c_str(), std::ios::binary);
    to_read_fpga.seekg(0, std::ios::end);
    fpga_image_size = to_read_fpga.tellg();
    to_read_fpga.seekg(0, std::ios::beg);
    char fpga_read[FPGA_IMAGE_SIZE_BYTES];
    to_read_fpga.read(fpga_read,fpga_image_size);
    to_read_fpga.close();
    for(int i = 0; i < fpga_image_size; i++) fpga_image[i] = (boost::uint8_t)fpga_read[i];

    //Checking validity of image
    if(fpga_image_size > FPGA_IMAGE_SIZE_BYTES){
        throw std::runtime_error(str(boost::format("FPGA image is too large. %d > %d") % fpga_image_size % FPGA_IMAGE_SIZE_BYTES));
    }

    //Check sequence of bytes in image
    bool is_good = false;
    for(int i = 0; i < 63; i++){
        if((boost::uint8_t)fpga_image[i] == 255) continue;
        else if((boost::uint8_t)fpga_image[i] == 170 and
                (boost::uint8_t)fpga_image[i+1] == 153){
            is_good = true;
            break;
        }
    }

    if(!is_good) throw std::runtime_error("Not a valid FPGA image.");

    //Return image size
    return fpga_image_size;
}

int grab_fw_image(std::string fw_path){

    //Reading firmware image from file
    std::ifstream to_read_fw((char*)fw_path.c_str(), std::ios::binary);
    to_read_fw.seekg(0, std::ios::end);
    fw_image_size = to_read_fw.tellg();
    to_read_fw.seekg(0, std::ios::beg);
    char fw_read[FW_IMAGE_SIZE_BYTES];
    to_read_fw.read(fw_read,fw_image_size);
    to_read_fw.close();
    for(int i = 0; i < fw_image_size; i++) fw_image[i] = (boost::uint8_t)fw_read[i];

    //Checking validity of image
    if(fw_image_size > FW_IMAGE_SIZE_BYTES){
        throw std::runtime_error(str(boost::format("Firmware image is too large. %d > %d") % fw_image_size % FW_IMAGE_SIZE_BYTES));
    }

    //Check first four bytes of image
    for(int i = 0; i < 4; i++) if((boost::uint8_t)fw_image[i] != 11) throw std::runtime_error("Not a valid firmware image.");

    //Return image size
    return fw_image_size;
}

boost::uint32_t* get_flash_info(std::string ip_addr){

    boost::uint32_t *flash_info = new boost::uint32_t[2];
    boost::uint8_t usrp2_update_data_in_mem[udp_simple::mtu];
    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);

    udp_simple::sptr udp_transport = udp_simple::make_connected(ip_addr, BOOST_STRINGIZE(USRP2_UDP_UPDATE_PORT));
    usrp2_fw_update_data_t get_flash_info_pkt = usrp2_fw_update_data_t();
    get_flash_info_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);
    get_flash_info_pkt.id = htonx<boost::uint32_t>(USRP2_FW_UPDATE_ID_WATS_TEH_FLASH_INFO_LOL);
    udp_transport->send(boost::asio::buffer(&get_flash_info_pkt, sizeof(get_flash_info_pkt)));

    //Loop and receive until the timeout
    size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
    if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == USRP2_FW_UPDATE_ID_HERES_TEH_FLASH_INFO_OMG){
        flash_info[0] = ntohl(update_data_in->data.flash_info_args.sector_size_bytes);
        flash_info[1] = ntohl(update_data_in->data.flash_info_args.memory_size_bytes);
    }
    else if(ntohl(update_data_in->id) != USRP2_FW_UPDATE_ID_HERES_TEH_FLASH_INFO_OMG){
        throw std::runtime_error(str(boost::format("Received invalid reply %d from device.\n") % ntohl(update_data_in->id)));
    }
    
    return flash_info;
}

/***********************************************************************
 * Image burning functions
 **********************************************************************/

void erase_image(udp_simple::sptr udp_transport, bool is_fw, boost::uint32_t memory_size){

    //Making sure this won't attempt to erase past end of device
    if(is_fw){
        if(PROD_FW_IMAGE_LOCATION_ADDR+FW_IMAGE_SIZE_BYTES > memory_size) throw std::runtime_error("Cannot erase past end of device.");
    }
    else{
        if(PROD_FPGA_IMAGE_LOCATION_ADDR+FPGA_IMAGE_SIZE_BYTES > memory_size) throw std::runtime_error("Cannot erase past end of device.");
    }

    //Setting up UDP transport
    boost::uint8_t usrp2_update_data_in_mem[udp_simple::mtu];
    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);

    //Setting up UDP packet
    usrp2_fw_update_data_t erase_pkt = usrp2_fw_update_data_t();
    erase_pkt.id = htonx<boost::uint32_t>(USRP2_FW_UPDATE_ID_ERASE_TEH_FLASHES_LOL);
    erase_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);
    if(is_fw){
        erase_pkt.data.flash_args.flash_addr = htonx<boost::uint32_t>(PROD_FW_IMAGE_LOCATION_ADDR);
        erase_pkt.data.flash_args.length = htonx<boost::uint32_t>(FW_IMAGE_SIZE_BYTES);
    }
    else{
        erase_pkt.data.flash_args.flash_addr = htonx<boost::uint32_t>(PROD_FPGA_IMAGE_LOCATION_ADDR);
        erase_pkt.data.flash_args.length = htonx<boost::uint32_t>(FPGA_IMAGE_SIZE_BYTES);
    }

    //Begin erasing
    udp_transport->send(boost::asio::buffer(&erase_pkt, sizeof(erase_pkt)));
    size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
    if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == USRP2_FW_UPDATE_ID_ERASING_TEH_FLASHES_OMG){
        if(is_fw) std::cout << "Erasing firmware image." << std::endl;
        else      std::cout << "Erasing FPGA image." << std::endl;
    }
    else if(ntohl(update_data_in->id) != USRP2_FW_UPDATE_ID_ERASING_TEH_FLASHES_OMG){
        throw std::runtime_error(str(boost::format("Received invalid reply %d from device.\n") % ntohl(update_data_in->id)));
    }

    //Check for erase completion
    erase_pkt.id = htonx<boost::uint32_t>(USRP2_FW_UPDATE_ID_R_U_DONE_ERASING_LOL);
    while(true){
        udp_transport->send(boost::asio::buffer(&erase_pkt, sizeof(erase_pkt)));
        size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
        if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == USRP2_FW_UPDATE_ID_IM_DONE_ERASING_OMG){
            if(is_fw) std::cout << boost::format(" * Successfully erased %d bytes at %d.\n") % FW_IMAGE_SIZE_BYTES % PROD_FW_IMAGE_LOCATION_ADDR;
            else std::cout << boost::format(" * Successfully erased %d bytes at %d.\n") % FPGA_IMAGE_SIZE_BYTES % PROD_FPGA_IMAGE_LOCATION_ADDR;
            break;
        }
        else if(ntohl(update_data_in->id) != USRP2_FW_UPDATE_ID_NOPE_NOT_DONE_ERASING_OMG){
            throw std::runtime_error(str(boost::format("Received invalid reply %d from device.\n") % ntohl(update_data_in->id)));
        }
    }
}

void write_image(udp_simple::sptr udp_transport, bool is_fw, boost::uint8_t* image, boost::uint32_t memory_size, int image_size){

    boost::uint32_t current_addr;
    if(is_fw) current_addr = PROD_FW_IMAGE_LOCATION_ADDR;
    else current_addr = PROD_FPGA_IMAGE_LOCATION_ADDR;

    //Making sure this won't attempt to write past end of device
    if(current_addr+image_size > memory_size) throw std::runtime_error("Cannot write past end of device.");

    //Setting up UDP transport
    boost::uint8_t usrp2_update_data_in_mem[udp_simple::mtu];
    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);

    //Setting up UDP packet
    usrp2_fw_update_data_t write_pkt = usrp2_fw_update_data_t();
    write_pkt.id = htonx<boost::uint32_t>(USRP2_FW_UPDATE_ID_WRITE_TEH_FLASHES_LOL);
    write_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);
    write_pkt.data.flash_args.length = htonx<boost::uint32_t>(FLASH_DATA_PACKET_SIZE);

    //Write image
    if(is_fw) std::cout << "Writing firmware image." << std::endl;
    else std::cout << "Writing FPGA image." << std::endl;

    for(int i = 0; i < ((image_size/FLASH_DATA_PACKET_SIZE)+1); i++){
        write_pkt.data.flash_args.flash_addr = htonx<boost::uint32_t>(current_addr);
        std::copy(image+(i*FLASH_DATA_PACKET_SIZE), image+((i+1)*FLASH_DATA_PACKET_SIZE), write_pkt.data.flash_args.data);

        udp_transport->send(boost::asio::buffer(&write_pkt, sizeof(write_pkt)));
        size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
        if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) != USRP2_FW_UPDATE_ID_WROTE_TEH_FLASHES_OMG){
            throw std::runtime_error(str(boost::format("Invalid reply %d from device.") % ntohl(update_data_in->id)));
        }

        current_addr += FLASH_DATA_PACKET_SIZE;
    }
    std::cout << boost::format(" * Successfully wrote %d bytes.\n") % image_size;
}

void verify_image(udp_simple::sptr udp_transport, bool is_fw, boost::uint8_t* image, boost::uint32_t memory_size, int image_size){

    int current_index = 0;
    boost::uint32_t current_addr;
    if(is_fw) current_addr = PROD_FW_IMAGE_LOCATION_ADDR;
    else current_addr = PROD_FPGA_IMAGE_LOCATION_ADDR;

    //Array size needs to be known at runtime, this constant is guaranteed to be larger than any firmware or FPGA image
    boost::uint8_t from_usrp[FPGA_IMAGE_SIZE_BYTES];

    //Making sure this won't attempt to read past end of device
    if(current_addr+image_size > memory_size) throw std::runtime_error("Cannot read past end of device.");

    //Setting up UDP transport
    boost::uint8_t usrp2_update_data_in_mem[udp_simple::mtu];
    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);

    //Setting up UDP packet
    usrp2_fw_update_data_t verify_pkt = usrp2_fw_update_data_t();
    verify_pkt.id = htonx<boost::uint32_t>(USRP2_FW_UPDATE_ID_READ_TEH_FLASHES_LOL);
    verify_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);
    verify_pkt.data.flash_args.length = htonx<boost::uint32_t>(FLASH_DATA_PACKET_SIZE);

    //Verify image
    if(is_fw) std::cout << "Verifying firmware image." << std::endl;
    else std::cout << "Verifying FPGA image." << std::endl;

    for(int i = 0; i < ((image_size/FLASH_DATA_PACKET_SIZE)+1); i++){
        verify_pkt.data.flash_args.flash_addr = htonx<boost::uint32_t>(current_addr);

        udp_transport->send(boost::asio::buffer(&verify_pkt, sizeof(verify_pkt)));
        size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
        if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) != USRP2_FW_UPDATE_ID_KK_READ_TEH_FLASHES_OMG){
            throw std::runtime_error(str(boost::format("Invalid reply %d from device.") % ntohl(update_data_in->id)));
        }
        for(int j = 0; j < FLASH_DATA_PACKET_SIZE; j++) from_usrp[current_index+j] = update_data_in->data.flash_args.data[j];

        current_addr += FLASH_DATA_PACKET_SIZE;
        current_index += FLASH_DATA_PACKET_SIZE;
    }
    for(int i = 0; i < image_size; i++) if(from_usrp[i] != image[i]) throw std::runtime_error("Image write failed.");

    std::cout << " * Successful." << std::endl;
}

void reset_usrp(udp_simple::sptr udp_transport){

    //Set up UDP transport
    boost::uint8_t usrp2_update_data_in_mem[udp_simple::mtu];
    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);

    //Set up UDP packet
    usrp2_fw_update_data_t reset_pkt = usrp2_fw_update_data_t();
    reset_pkt.id = htonx<boost::uint32_t>(USRP2_FW_UPDATE_ID_RESET_MAH_COMPUTORZ_LOL);
    reset_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);

    //Reset USRP
    udp_transport->send(boost::asio::buffer(&reset_pkt, sizeof(reset_pkt)));
    size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
    if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == USRP2_FW_UPDATE_ID_RESETTIN_TEH_COMPUTORZ_OMG){
        throw std::runtime_error("USRP reset failed."); //There should be no response to this UDP packet
    }
    else std::cout << "Resetting USRP." << std::endl;
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
        ("no_fw", "Do not burn a firmware image.")
        ("no_fpga", "Do not burn an FPGA image.")
        ("auto_reboot", "Automatically reboot N2XX without prompting.")
        ("list", "List available N2XX USRP devices.")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //Apply options
    if(vm.count("help") > 0){
        std::cout << boost::format("N2XX Simple Net Burner\n");
        std::cout << boost::format("Automatically detects and burns standard firmware and FPGA images onto USRP N2XX devices.\n");
        std::cout << boost::format("Can optionally take user input for custom images.\n\n");
        std::cout << desc << std::endl;
        return EXIT_FAILURE;
    }

    bool burn_fpga = (vm.count("no_fpga") == 0);
    bool burn_fw = (vm.count("no_fw") == 0);
    bool use_custom_fpga = (vm.count("fpga") > 0);
    bool use_custom_fw = (vm.count("fw") > 0);
    bool list_usrps = (vm.count("list") > 0);
    bool auto_reboot = (vm.count("auto_reboot") > 0);

    if(!burn_fpga && !burn_fw){
        std::cout << "No images will be burned." << std::endl;
        return EXIT_FAILURE;
    }

    if(!burn_fw && use_custom_fw)     std::cout << boost::format("Conflicting firmware options presented. Will not burn a firmware image.\n\n");
    if(!burn_fpga && use_custom_fpga) std::cout << boost::format("Conflicting FPGA options presented. Will not burn an FPGA image.\n\n");

    //Variables not from options
    boost::uint32_t hw_rev;
    bool found_it = false;
    boost::uint8_t usrp2_update_data_in_mem[udp_simple::mtu];
    const usrp2_fw_update_data_t *update_data_in = reinterpret_cast<const usrp2_fw_update_data_t *>(usrp2_update_data_in_mem);

    //List option
    if(list_usrps){
        udp_simple::sptr udp_bc_transport;
        usrp2_fw_update_data_t usrp2_ack_pkt = usrp2_fw_update_data_t();
        usrp2_ack_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);
        usrp2_ack_pkt.id = htonx<boost::uint32_t>(USRP2_FW_UPDATE_ID_OHAI_LOL);

        std::cout << "Available USRP N2XX devices:" << std::endl;

        //Send UDP packets to all broadcast addresses
        BOOST_FOREACH(const if_addrs_t &if_addrs, get_if_addrs()){
            //Avoid the loopback device
            if(if_addrs.inet == boost::asio::ip::address_v4::loopback().to_string()) continue;
            udp_bc_transport = udp_simple::make_broadcast(if_addrs.bcast, BOOST_STRINGIZE(USRP2_UDP_UPDATE_PORT));
            udp_bc_transport->send(boost::asio::buffer(&usrp2_ack_pkt, sizeof(usrp2_ack_pkt)));

            size_t len = udp_bc_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
            if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == USRP2_FW_UPDATE_ID_OHAI_OMG){
                usrp2_ack_pkt.id = htonx<boost::uint32_t>(USRP2_FW_UPDATE_ID_I_CAN_HAS_HW_REV_LOL);
                udp_bc_transport->send(boost::asio::buffer(&usrp2_ack_pkt, sizeof(usrp2_ack_pkt)));

                size_t len = udp_bc_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
                if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == USRP2_FW_UPDATE_ID_HERES_TEH_HW_REV_OMG){
                    hw_rev = ntohl(update_data_in->data.hw_rev);
                }

                std::cout << boost::format(" * %s (%s)\n") % udp_bc_transport->get_recv_addr() % filename_map[hw_rev];
            }
        
        }
        return EXIT_FAILURE;
    }
    std::cout << boost::format("Searching for USRP N2XX with IP address %s.\n") % ip_addr;

    //Address specified
    udp_simple::sptr udp_transport = udp_simple::make_connected(ip_addr, BOOST_STRINGIZE(USRP2_UDP_UPDATE_PORT));
    usrp2_fw_update_data_t hw_info_pkt = usrp2_fw_update_data_t();
    hw_info_pkt.proto_ver = htonx<boost::uint32_t>(USRP2_FW_PROTO_VERSION);
    hw_info_pkt.id = htonx<boost::uint32_t>(USRP2_FW_UPDATE_ID_I_CAN_HAS_HW_REV_LOL);
    udp_transport->send(boost::asio::buffer(&hw_info_pkt, sizeof(hw_info_pkt)));

    //Loop and receive until the timeout
    size_t len = udp_transport->recv(boost::asio::buffer(usrp2_update_data_in_mem), UDP_TIMEOUT);
    if(len > offsetof(usrp2_fw_update_data_t, data) and ntohl(update_data_in->id) == USRP2_FW_UPDATE_ID_HERES_TEH_HW_REV_OMG){
        hw_rev = ntohl(update_data_in->data.hw_rev);
        if(filename_map.find(hw_rev) != filename_map.end()){
            std::cout << boost::format("Found %s.\n\n") % filename_map[hw_rev];
            found_it = true;
        }
        else throw std::runtime_error("Invalid revision found.");
    }
    if(!found_it) throw std::runtime_error("No USRP N2XX found.");

    //Determining default image filenames for validation
    std::string default_fw_filename = str(boost::format("usrp_%s_fw.bin") % erase_tail_copy(filename_map[hw_rev],3));
    std::string default_fpga_filename = str(boost::format("usrp_%s_fpga.bin") % filename_map[hw_rev]);
    std::string default_fw_filepath = "";
    std::string default_fpga_filepath = "";

    //Check validity of file locations and binaries before attempting burn
    std::cout << "Searching for specified images." << std::endl << std::endl;
    if(burn_fpga){
        if(!use_custom_fpga) fpga_path = find_image_path(default_fpga_filename);
        else validate_custom_fpga_file(filename_map[hw_rev], fpga_path);

        grab_fpga_image(fpga_path);
    }
    if(burn_fw){
        if(!use_custom_fw) fw_path = find_image_path(default_fw_filename);
        else validate_custom_fw_file(filename_map[hw_rev], fw_path);

        grab_fw_image(fw_path);
    }

    std::cout << "Will burn the following images:" << std::endl;
    if(burn_fw) std::cout << boost::format(" * Firmware: %s\n") % fw_path;
    if(burn_fpga) std::cout << boost::format(" * FPGA:     %s\n") % fpga_path;
    std::cout << std::endl;

    boost::uint32_t* flash_info = get_flash_info(ip_addr);
    std::cout << boost::format("Querying %s for flash information.\n") % filename_map[hw_rev];
    std::cout << boost::format(" * Flash size:  %3.2f\n") % flash_info[1];
    std::cout << boost::format(" * Sector size: %3.2f\n\n") % flash_info[0];

    //Burning images

    if(burn_fpga){
        erase_image(udp_transport, false, flash_info[1]);
        write_image(udp_transport, false, fpga_image, flash_info[1], fpga_image_size);
        verify_image(udp_transport, false, fpga_image, flash_info[1], fpga_image_size);
    }
    if(burn_fpga and burn_fw) std::cout << std::endl; //Formatting
    if(burn_fw){
        erase_image(udp_transport, true, flash_info[1]);
        write_image(udp_transport, true, fw_image, flash_info[1], fw_image_size);
        verify_image(udp_transport, true, fw_image, flash_info[1], fw_image_size);
    }

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
    else return EXIT_SUCCESS;

    return EXIT_SUCCESS;
}
