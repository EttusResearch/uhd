//
// Copyright 2014-2015 Ettus Research LLC
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

#include <algorithm>
#include <csignal>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/assign.hpp>
#include <boost/cstdint.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>

#include <uhd/device.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/safe_main.hpp>

#include "../lib/usrp_clock/octoclock/common.h"
#include "../lib/utils/ihex.hpp"

#define MAX_FIRMWARE_SIZE 1024*120
#define BLOCK_SIZE 256
#define UDP_TIMEOUT 5

namespace fs = boost::filesystem;
namespace po = boost::program_options;

using namespace uhd;
using namespace uhd::transport;

static int num_ctrl_c = 0;
void sig_int_handler(int){
    num_ctrl_c++;
    if(num_ctrl_c == 1){
        std::cout << std::endl << "Are you sure you want to abort the image burning? If you do, your "
                                  "OctoClock device will be bricked!" << std::endl
                               << "Press Ctrl+C again to abort the image burning procedure." << std::endl << std::endl;
    }
    else{
        std::cout << std::endl << "Aborting. Your OctoClock device will be bricked." << std::endl
                               << "Refer to http://files.ettus.com/manual/page_octoclock.html#bootloader" << std::endl
                               << "for details on restoring your device." << std::endl;
        exit(EXIT_FAILURE);
    }
}

boost::uint8_t firmware_image[MAX_FIRMWARE_SIZE];
size_t firmware_size = 0;
boost::uint8_t octoclock_data[udp_simple::mtu];
octoclock_packet_t *pkt_in = reinterpret_cast<octoclock_packet_t *>(octoclock_data);
std::string firmware_path, actual_firmware_path;
size_t num_blocks = 0;
bool hex = true;

static uint16_t calculate_crc(boost::uint8_t* buffer, boost::uint16_t len){
    boost::uint16_t crc = 0xFFFF;

    for(size_t i = 0; i < len; i++){
        crc ^= buffer[i];
        for(boost::uint8_t j = 0; j < 8; ++j){
            if(crc & 1) crc = (crc >> 1) ^ 0xA001;
            else crc = (crc >> 1);
        }
    }

    return crc;
}

/*
 * Functions
 */
void list_octoclocks(){
    device_addrs_t found_octoclocks = device::find(uhd::device_addr_t(), device::CLOCK);

    std::cout << "Available OctoClock devices:" << std::endl;
    BOOST_FOREACH(const device_addr_t &oc, found_octoclocks){
        std::cout << " * " << oc["addr"] << std::endl;
    }
}

void print_image_loader_warning(const std::string &fw_path, const po::variables_map &vm){
    // Newline + indent
    #ifdef UHD_PLATFORM_WIN32
    const std::string nl = " ^\n    ";
    #else
    const std::string nl = " \\\n    ";
    #endif

    std::string uhd_image_loader = str(boost::format("uhd_image_loader --args=\"type=octoclock,addr=%s\""
                                                     "%s --fw-path=%s")
                                       % vm["addr"].as<std::string>() % nl % fw_path);

    std::cout << "************************************************************************************************" << std::endl
              << "WARNING: This utility will be removed in an upcoming version of UHD. In the future, use" << std::endl
              << "         this command:" << std::endl
              << std::endl
              << uhd_image_loader << std::endl
              << std::endl
              << "************************************************************************************************" << std::endl
              << std::endl;
}

/*
 * Manually find bootloader. This sends multiple packets in order to increase chances of getting
 * bootloader before it switches to the application.
 */
device_addrs_t bootloader_find(const std::string &ip_addr){
    udp_simple::sptr udp_transport = udp_simple::make_connected(ip_addr, BOOST_STRINGIZE(OCTOCLOCK_UDP_CTRL_PORT));

    octoclock_packet_t pkt_out;
    pkt_out.sequence = uhd::htonx<boost::uint32_t>(std::rand());
    pkt_out.code = OCTOCLOCK_QUERY_CMD;
    pkt_out.len = 0;
    size_t len = 0;

    device_addrs_t addrs;

    boost::system_time comm_timeout = boost::get_system_time() + boost::posix_time::milliseconds(3000);

    while(boost::get_system_time() < comm_timeout){
        UHD_OCTOCLOCK_SEND_AND_RECV(udp_transport, OCTOCLOCK_FW_COMPAT_NUM, OCTOCLOCK_QUERY_CMD, pkt_out, len, octoclock_data);
        if(UHD_OCTOCLOCK_PACKET_MATCHES(OCTOCLOCK_QUERY_ACK, pkt_out, pkt_in, len) and
           pkt_in->proto_ver == OCTOCLOCK_BOOTLOADER_PROTO_VER){
            addrs.push_back(device_addr_t());
            addrs[0]["type"] = "octoclock-bootloader";
            addrs[0]["addr"] = udp_transport->get_recv_addr();
            break;
        }
    }

    return addrs;
}

void read_firmware(){
    std::ifstream firmware_file(actual_firmware_path.c_str(), std::ios::binary);
    firmware_size = size_t(fs::file_size(actual_firmware_path));
    if(firmware_size > MAX_FIRMWARE_SIZE){
        firmware_file.close();
        throw uhd::runtime_error(str(boost::format("Firmware file too large: %d > %d")
                                     % firmware_size % (MAX_FIRMWARE_SIZE)));
    }
    firmware_file.read((char*)firmware_image, firmware_size);
    firmware_file.close();

    num_blocks = (firmware_size % BLOCK_SIZE) ? ((firmware_size / BLOCK_SIZE) + 1)
                                              : (firmware_size / BLOCK_SIZE);
}

void burn_firmware(udp_simple::sptr udp_transport){
    octoclock_packet_t pkt_out;
    pkt_out.sequence = uhd::htonx<boost::uint32_t>(std::rand());
    pkt_out.len = (boost::uint16_t)firmware_size;
    pkt_out.crc = calculate_crc(firmware_image, firmware_size);
    size_t len = 0, current_pos = 0;

    //Tell OctoClock not to jump to application, wait for us instead
    std::cout << "Telling OctoClock to prepare for firmware download..." << std::flush;
    UHD_OCTOCLOCK_SEND_AND_RECV(udp_transport, OCTOCLOCK_FW_COMPAT_NUM, PREPARE_FW_BURN_CMD, pkt_out, len, octoclock_data);
    if(UHD_OCTOCLOCK_PACKET_MATCHES(FW_BURN_READY_ACK, pkt_out, pkt_in, len)) std::cout << "ready." << std::endl;
    else{
        std::cout << std::endl;
        if(hex) fs::remove(actual_firmware_path);
        throw uhd::runtime_error("Could not get OctoClock in valid state for firmware download.");
    }

    std::cout << std::endl << "Burning firmware." << std::endl;
    pkt_out.code = FILE_TRANSFER_CMD;

    //Actual burning below
    size_t num_tries = 0;
    for(size_t i = 0; i < num_blocks; i++){
        num_tries = 0;
        pkt_out.sequence++;
        pkt_out.addr = i*BLOCK_SIZE;
        std::cout << "\r * Progress: " << int(double(i)/double(num_blocks)*100)
                  << "% (" << (i+1) << "/" << num_blocks << " blocks)" << std::flush;

        memset(pkt_out.data, 0, BLOCK_SIZE);
        memcpy((void*)(pkt_out.data), &firmware_image[i*BLOCK_SIZE], BLOCK_SIZE);

        bool success = false;
        while(num_tries <= 5){
            UHD_OCTOCLOCK_SEND_AND_RECV(udp_transport, OCTOCLOCK_FW_COMPAT_NUM, FILE_TRANSFER_CMD, pkt_out, len, octoclock_data);
            if(UHD_OCTOCLOCK_PACKET_MATCHES(FILE_TRANSFER_ACK, pkt_out, pkt_in, len)){
                success = true;
                break;
            }
            else{
                num_tries++;
                boost::this_thread::sleep(boost::posix_time::milliseconds(100));
            }
        }
        if(not success){
            std::cout << std::endl;
            if(hex) fs::remove(actual_firmware_path);
            throw uhd::runtime_error("Failed to burn firmware to OctoClock!");
        }

        current_pos += BLOCK_SIZE;
    }

    std::cout << "\r * Progress: 100% (" << num_blocks << "/" << num_blocks << " blocks)" << std::endl;
}

void verify_firmware(udp_simple::sptr udp_transport){
    octoclock_packet_t pkt_out;
    pkt_out.proto_ver = OCTOCLOCK_FW_COMPAT_NUM;
    pkt_out.sequence = uhd::htonx<boost::uint32_t>(std::rand());
    size_t len = 0, current_pos = 0;

    for(size_t i = 0; i < num_blocks; i++){
        pkt_out.sequence++;
        pkt_out.addr = i*BLOCK_SIZE;
        std::cout << "\r * Progress: " << int(double(i)/double(num_blocks)*100)
                  << "% (" << (i+1) << "/" << num_blocks << " blocks)" << std::flush;

        UHD_OCTOCLOCK_SEND_AND_RECV(udp_transport, OCTOCLOCK_FW_COMPAT_NUM, READ_FW_CMD, pkt_out, len, octoclock_data);
        if(UHD_OCTOCLOCK_PACKET_MATCHES(READ_FW_ACK, pkt_out, pkt_in, len)){
            if(memcmp((void*)(pkt_in->data), &firmware_image[i*BLOCK_SIZE],
                      std::min(int(firmware_size-current_pos), BLOCK_SIZE))){
                std::cout << std::endl;
                if(hex) fs::remove(actual_firmware_path);
                throw uhd::runtime_error("Failed to verify OctoClock firmware!");
            }
        }
        else{
            std::cout << std::endl;
            if(hex) fs::remove(actual_firmware_path);
            throw uhd::runtime_error("Failed to verify OctoClock firmware!");
        }
    }

    std::cout << "\r * Progress: 100% (" << num_blocks << "/" << num_blocks << " blocks)" << std::endl;
}

bool reset_octoclock(const std::string &ip_addr){
    udp_simple::sptr udp_transport = udp_simple::make_connected(ip_addr, BOOST_STRINGIZE(OCTOCLOCK_UDP_CTRL_PORT));

    octoclock_packet_t pkt_out;
    pkt_out.sequence = uhd::htonx<boost::uint32_t>(std::rand());
    size_t len;

    UHD_OCTOCLOCK_SEND_AND_RECV(udp_transport, OCTOCLOCK_FW_COMPAT_NUM, RESET_CMD, pkt_out, len, octoclock_data);
    if(not UHD_OCTOCLOCK_PACKET_MATCHES(RESET_ACK, pkt_out, pkt_in, len)){
        std::cout << std::endl;
        if(hex) fs::remove(actual_firmware_path);
        throw uhd::runtime_error("Failed to place device in state to receive firmware.");
    }

    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    return (bootloader_find(ip_addr).size() == 1);
}

void finalize(udp_simple::sptr udp_transport){
    octoclock_packet_t pkt_out;
    pkt_out.len = 0;
    pkt_out.sequence = uhd::htonx<boost::uint32_t>(std::rand());
    size_t len = 0;

    UHD_OCTOCLOCK_SEND_AND_RECV(udp_transport, OCTOCLOCK_FW_COMPAT_NUM, FINALIZE_BURNING_CMD, pkt_out, len, octoclock_data);
    if(not UHD_OCTOCLOCK_PACKET_MATCHES(FINALIZE_BURNING_ACK, pkt_out, pkt_in, len)){
        std::cout << std::endl;
        if(hex) fs::remove(actual_firmware_path);
        std::cout << "no ACK. Bootloader may not have loaded application." << std::endl;
    }
}

void octoclock_convert_ihex(const std::string &hex_path, const std::string &bin_path){
    ihex_reader hex_reader(hex_path);
    hex_reader.to_bin_file(bin_path);
}

int UHD_SAFE_MAIN(UHD_UNUSED(int argc), UHD_UNUSED(char *argv[])){

    std::string ip_addr;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Display this help message.")
        ("addr", po::value<std::string>(&ip_addr), "Specify an IP address.")
        ("fw-path", po::value<std::string>(&firmware_path), "Specify a custom firmware path.")
        ("list", "List all available OctoClock devices.")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //Print help message
    if(vm.count("help")){
        std::cout << "OctoClock Firmware Burner" << std::endl << std::endl;

        std::cout << "Burns a firmware image file onto an OctoClock device. Specify" << std::endl
                  << "the address of the OctoClock with the --addr option. To burn" << std::endl
                  << "a custom firmware image, use the --fw-path option. Otherwise, the" << std::endl
                  << "utility will use the default image. To list all available" << std::endl
                  << "OctoClock devices without burning firmware, use the --list" << std::endl
                  << "option." << std::endl << std::endl;

        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    //List all available devices
    if(vm.count("list")){
        list_octoclocks();
        return EXIT_SUCCESS;
    }

    if(not (vm.count("addr"))){
        throw uhd::runtime_error("You must specify an address with the --addr option!");
    }
    udp_simple::sptr udp_transport = udp_simple::make_connected(ip_addr, BOOST_STRINGIZE(OCTOCLOCK_UDP_FW_PORT));

    //If custom path given, make sure it exists
    if(vm.count("fw-path")){
        //Expand tilde usage if applicable
        #ifndef UHD_PLATFORM_WIN32
        if(firmware_path.find("~/") == 0) firmware_path.replace(0,1,getenv("HOME"));
        #endif

        if(not fs::exists(firmware_path)){
            throw uhd::runtime_error(str(boost::format("This filepath does not exist: %s") % firmware_path));
        }
    }
    else firmware_path = find_image_path("octoclock_r4_fw.hex");

    //If Intel hex file detected, convert to binary
    std::string ext = fs::extension(firmware_path);
    if(ext == ".hex"){
        std::cout << "Found firmware at path: " << firmware_path << std::endl;

        //Write firmware .bin file to temporary directory
        fs::path temp_bin = fs::path(fs::path(get_tmp_path()) / str(boost::format("octoclock_fw_%d.bin")
                                                          % time_spec_t::get_system_time().get_full_secs()));
        octoclock_convert_ihex(firmware_path, temp_bin.string());

        actual_firmware_path = temp_bin.string();
    }
    else if(ext == ".bin"){
        hex = false;
        actual_firmware_path = firmware_path;
        std::cout << "Found firmware at path: " << firmware_path << std::endl;
    }
    else throw uhd::runtime_error("The firmware file has in improper extension (must be .hex or .bin).");

    std::cout << std::endl << boost::format("Searching for OctoClock with IP address %s...") % ip_addr << std::flush;
    device_addrs_t octoclocks = device::find(str(boost::format("addr=%s") % ip_addr), device::CLOCK);
    if(octoclocks.size() == 1){
        if(octoclocks[0]["type"] == "octoclock"){
            std::cout << "found. Resetting..." << std::flush;
            if(reset_octoclock(ip_addr)) std::cout << "successful." << std::endl;
            else{
                std::cout << "failed." << std::endl;
                if(hex) fs::remove(actual_firmware_path);
                throw uhd::runtime_error("Failed to reset OctoClock device into its bootloader.");
            }
        }
        else std::cout << "found." << std::endl;
    }
    else{
        std::cout << "failed." << std::endl;
        if(hex) fs::remove(actual_firmware_path);
        throw uhd::runtime_error("Could not find OctoClock with given IP address!");
    }

    read_firmware();

    print_image_loader_warning(firmware_path, vm);

    std::signal(SIGINT, &sig_int_handler);

    burn_firmware(udp_transport);
    std::cout << "Verifying firmware." << std::endl;
    verify_firmware(udp_transport);
    std::cout << std::endl << "Telling OctoClock bootloader to load application..." << std::flush;
    finalize(udp_transport);
    std::cout << "done." << std::endl;

    std::cout << "Waiting for OctoClock to reinitialize..." << std::flush;
    boost::this_thread::sleep(boost::posix_time::milliseconds(500));
    octoclocks = device::find(str(boost::format("addr=%s") % ip_addr), device::CLOCK);
    if(octoclocks.size() == 1){
        if(octoclocks[0]["type"] == "octoclock-bootloader"){
            std::cout << std::endl;
            if(hex) fs::remove(actual_firmware_path);
            throw uhd::runtime_error("Firmware did not load properly.");
        }
        else{
            std::cout << "found." << std::endl << std::endl
                      << "Successfully burned firmware." << std::endl << std::endl;
        }
    }
    else{
        std::cout << std::endl;
        if(hex) fs::remove(actual_firmware_path);
        throw uhd::runtime_error("Failed to reinitialize OctoClock.");
    }
    if(hex) fs::remove(actual_firmware_path);

    return EXIT_SUCCESS;
}
