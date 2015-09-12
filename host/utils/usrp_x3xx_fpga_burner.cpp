//
// Copyright 2013-2015 Ettus Research LLC
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
#include <map>
#include <fstream>
#include <stdexcept>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include <boost/foreach.hpp>
#include <boost/asio.hpp>
#include <boost/program_options.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/assign.hpp>
#include <boost/cstdint.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string/erase.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>

#include <uhd/exception.hpp>
#include <uhd/transport/if_addrs.hpp>
#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/device.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/safe_main.hpp>
#include <uhd/utils/safe_call.hpp>

extern "C" {
#include "cdecode.h"
}

#define X300_FPGA_BIN_SIZE_BYTES 15877916
#define X300_FPGA_BIT_MAX_SIZE_BYTES 15878032
#define X300_FPGA_PROG_UDP_PORT 49157
#define X300_FLASH_SECTOR_SIZE 131072
#define X300_PACKET_SIZE_BYTES 256
#define X300_FPGA_SECTOR_START 32
#define X300_MAX_RESPONSE_BYTES 128
#define UDP_TIMEOUT 3
#define FPGA_LOAD_TIMEOUT 15

#define X300_FPGA_PROG_FLAGS_ACK     1
#define X300_FPGA_PROG_FLAGS_ERROR   2
#define X300_FPGA_PROG_FLAGS_INIT    4
#define X300_FPGA_PROG_FLAGS_CLEANUP 8
#define X300_FPGA_PROG_FLAGS_ERASE   16
#define X300_FPGA_PROG_FLAGS_VERIFY  32
#define X300_FPGA_PROG_CONFIGURE     64
#define X300_FPGA_PROG_CONFIG_STATUS 128

namespace fs = boost::filesystem;
namespace po = boost::program_options;

using namespace uhd;
using namespace uhd::transport;

static int num_ctrl_c = 0;
void sig_int_handler(int){
    num_ctrl_c++;
    if(num_ctrl_c == 1){
        std::cout << std::endl << "Are you sure you want to abort the image burning? If you do, your "
                                  "USRP-X series device will be bricked!" << std::endl
                               << "Press Ctrl+C again to abort the image burning procedure." << std::endl << std::endl;
    }
    else{
        std::cout << std::endl << "Aborting. Your USRP X-Series device will be bricked." << std::endl
                  << "http://files.ettus.com/manual/page_usrp_x3x0.html#x3x0_load_fpga_imgs_jtag" << std::endl
                  << "for details on restoring your device." << std::endl;
        exit(EXIT_FAILURE);
    }
}

typedef struct {
    boost::uint32_t flags;
    boost::uint32_t sector;
    boost::uint32_t index;
    boost::uint32_t size;
    boost::uint16_t data[128];
} x300_fpga_update_data_t;

boost::uint8_t x300_data_in_mem[udp_simple::mtu];
boost::uint8_t intermediary_packet_data[X300_PACKET_SIZE_BYTES];

boost::uint8_t bitswap(uint8_t b){
    b = ((b & 0xF0) >> 4) | ((b & 0x0F) << 4);
    b = ((b & 0xCC) >> 2) | ((b & 0x33) << 2);
    b = ((b & 0xAA) >> 1) | ((b & 0x55) << 1);

    return b;
}

void list_usrps(){
    device_addrs_t found_devices = device::find(device_addr_t("type=x300"), device::USRP);

    std::cout << "Available X3x0 devices:" << std::endl;
    BOOST_FOREACH(const device_addr_t &dev, found_devices){
        std::string dev_string;
        if(dev.has_key("addr")){
            dev_string = str(boost::format(" * %s (%s, addr: %s)")
                                           % dev["product"]
                                           % dev["fpga"]
                                           % dev["addr"]);
        }
        else{
            dev_string = str(boost::format(" * %s (%s, resource: %s)")
                                           % dev["product"]
                                           % dev["fpga"]
                                           % dev["resource"]);
        }
        std::cout << dev_string << std::endl;
    }
}

device_addr_t find_usrp_with_ethernet(std::string ip_addr, bool output){
    if(output) std::cout << "Attempting to find X3x0 with IP address: " << ip_addr << std::endl;
    const device_addr_t dev = device_addr_t(str(boost::format("addr=%s") % ip_addr));
    device_addrs_t found_devices = device::find(dev, device::USRP);

    if(found_devices.size() < 1) {
        throw std::runtime_error("Could not find X3x0 with the specified address!");
    }
    else if(found_devices.size() > 1) {
        throw std::runtime_error("Found multiple X3x0 units with the specified address!");
    }
    else {
        if(output) std::cout << (boost::format("Found %s (%s).\n\n")
                                 % found_devices[0]["product"]
                                 % found_devices[0]["fpga"]);
    }
    return found_devices[0];
}

device_addr_t find_usrp_with_pcie(std::string resource, bool output){
    if(output) std::cout << "Attempting to find X3x0 with resource: " << resource << std::endl;
    const device_addr_t dev = device_addr_t(str(boost::format("resource=%s") % resource));
    device_addrs_t found_devices = device::find(dev, device::USRP);

    if(found_devices.size() < 1) {
        throw std::runtime_error("Could not find X3x0 with the specified resource!");
    }
    else {
        if(output) std::cout << (boost::format("Found %s (%s).\n\n")
                                 % found_devices[0]["product"]
                                 % found_devices[0]["fpga"]);
    }
    return found_devices[0];
}

std::string get_default_image_path(std::string model, std::string image_type){
    std::transform(model.begin(), model.end(), model.begin(), ::tolower);

    std::string image_name = str(boost::format("usrp_%s_fpga_%s.bit")
                                            % model.c_str() % image_type.c_str());

    return find_image_path(image_name);
}

void extract_from_lvbitx(std::string lvbitx_path, std::vector<char> &bitstream){
    boost::property_tree::ptree pt;
    boost::property_tree::xml_parser::read_xml(lvbitx_path.c_str(), pt,
                                               boost::property_tree::xml_parser::no_comments |
                                               boost::property_tree::xml_parser::trim_whitespace);
    std::string const encoded_bitstream(pt.get<std::string>("Bitfile.Bitstream"));
    std::vector<char> decoded_bitstream(encoded_bitstream.size());

    base64_decodestate decode_state;
    base64_init_decodestate(&decode_state);
    size_t const decoded_size = base64_decode_block(encoded_bitstream.c_str(),
                                encoded_bitstream.size(), &decoded_bitstream.front(), &decode_state);
    decoded_bitstream.resize(decoded_size);
    bitstream.swap(decoded_bitstream);
}

void print_image_loader_warning(const std::string &fpga_path, const po::variables_map &vm){

    // Newline + indent
    #ifdef UHD_PLATFORM_WIN32
    const std::string nl = " ^\n    ";
    #else
    const std::string nl = " \\\n    ";
    #endif

    // Generate equivalent uhd_image_loader command
    std::string uhd_image_loader = "uhd_image_loader --args=\"type=x300";

    if(vm.count("addr") > 0){
        uhd_image_loader += str(boost::format(",addr=%s")
                                % vm["addr"].as<std::string>());

        if(vm.count("configure") > 0){
            uhd_image_loader += ",configure";
        }

        if(vm.count("verify") > 0){
            uhd_image_loader += ",verify";
        }
    }
    else{
        uhd_image_loader += str(boost::format(",resource=%s")
                                % vm["resource"].as<std::string>());

        /*
         * Since we have a default value, vm.count("rpc-port") will
         * always be > 0, so only add the option if a different port
         * is given.
         */
        if(vm["rpc-port"].as<std::string>() != "5444"){
            uhd_image_loader += str(boost::format(",rpc-port=%s")
                                    % vm["rpc-port"].as<std::string>());
        }
    }

    if(vm.count("type") > 0){
        uhd_image_loader += str(boost::format(",fpga=%s")
                                % vm["type"].as<std::string>());
    }

    uhd_image_loader += "\"";

    /*
     * The --type option overrides any given path, so only add an FPGA path
     * if there was no --type argument.
     */
    if(vm.count("type") == 0){
        uhd_image_loader += str(boost::format("%s--fpga-path=\"%s\"")
                                % nl % fpga_path);
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

void ethernet_burn(udp_simple::sptr udp_transport, std::string fpga_path, bool verify){
    boost::uint32_t max_size;
    std::vector<char> bitstream;

    if(fs::extension(fpga_path) == ".bit") max_size = X300_FPGA_BIT_MAX_SIZE_BYTES;
    else max_size = X300_FPGA_BIN_SIZE_BYTES; //Use for both .bin and .lvbitx

    bool is_lvbitx = (fs::extension(fpga_path) == ".lvbitx");

    size_t fpga_image_size;
    FILE* file;
    if((file = fopen(fpga_path.c_str(), "rb"))){
        fseek(file, 0, SEEK_END);
        if(is_lvbitx){
            extract_from_lvbitx(fpga_path, bitstream);
            fpga_image_size = bitstream.size();
        }
        else fpga_image_size = ftell(file);
        if(fpga_image_size > max_size){
            fclose(file);
            throw std::runtime_error(str(boost::format("FPGA size is too large (%d > %d).")
                                         % fpga_image_size % max_size));
        }
        rewind(file);
    }
    else{
        throw std::runtime_error(str(boost::format("Could not find FPGA image at location: %s")
                                     % fpga_path.c_str()));
    }

    const x300_fpga_update_data_t *update_data_in = reinterpret_cast<const x300_fpga_update_data_t *>(x300_data_in_mem);

    x300_fpga_update_data_t ack_packet;
    ack_packet.flags = htonx<boost::uint32_t>(X300_FPGA_PROG_FLAGS_ACK | X300_FPGA_PROG_FLAGS_INIT);
    ack_packet.sector = 0;
    ack_packet.size = 0;
    ack_packet.index = 0;
    memset(ack_packet.data, 0, sizeof(ack_packet.data));
    udp_transport->send(boost::asio::buffer(&ack_packet, sizeof(ack_packet)));

    udp_transport->recv(boost::asio::buffer(x300_data_in_mem), UDP_TIMEOUT);
    if((ntohl(update_data_in->flags) & X300_FPGA_PROG_FLAGS_ERROR) != X300_FPGA_PROG_FLAGS_ERROR){
        std::cout << "Burning image: " << fpga_path << std::endl;
        if(verify) std::cout << "NOTE: Verifying image. Burning will take much longer." << std::endl;
        std::cout << std::endl;
    }
    else{
        throw std::runtime_error("Failed to start image burning! Did you specify the correct IP address? If so, power-cycle the device and try again.");
    }

    size_t current_pos = 0;
    size_t sectors = fpga_image_size / X300_FLASH_SECTOR_SIZE;

    //Each sector
    for(size_t i = 0; i < fpga_image_size; i += X300_FLASH_SECTOR_SIZE){

        //Print progress percentage at beginning of each sector
        std::cout   << "\rProgress: " << int(double(i)/double(fpga_image_size)*100)
                    << "% (" << (i / X300_FLASH_SECTOR_SIZE) << "/"
                    << sectors << " sectors)" << std::flush;

        //Each packet
        for(size_t j = i; (j < fpga_image_size and j < (i+X300_FLASH_SECTOR_SIZE)); j += X300_PACKET_SIZE_BYTES){
            x300_fpga_update_data_t send_packet;

            send_packet.flags = X300_FPGA_PROG_FLAGS_ACK;
            if(verify) send_packet.flags |= X300_FPGA_PROG_FLAGS_VERIFY;
            if(j == i) send_packet.flags |= X300_FPGA_PROG_FLAGS_ERASE; //Erase the sector before writing
            send_packet.flags = htonx<boost::uint32_t>(send_packet.flags);

            send_packet.sector = htonx<boost::uint32_t>(X300_FPGA_SECTOR_START + (i/X300_FLASH_SECTOR_SIZE));
            send_packet.index = htonx<boost::uint32_t>((j % X300_FLASH_SECTOR_SIZE) / 2);
            send_packet.size = htonx<boost::uint32_t>(X300_PACKET_SIZE_BYTES / 2);
            memset(intermediary_packet_data,0,X300_PACKET_SIZE_BYTES);
            memset(send_packet.data,0,X300_PACKET_SIZE_BYTES);
            if(!is_lvbitx) current_pos = ftell(file);

            if(current_pos + X300_PACKET_SIZE_BYTES > fpga_image_size){
                if(is_lvbitx){
                    memcpy(intermediary_packet_data, (&bitstream[current_pos]), (bitstream.size()-current_pos+1));
                }
                else{
                    size_t len = fread(intermediary_packet_data, sizeof(boost::uint8_t), (fpga_image_size-current_pos), file);
                    if(len != (fpga_image_size-current_pos)){
                        throw std::runtime_error("Error reading from file!");
                    }
                }
            }
            else{
                if(is_lvbitx){
                    memcpy(intermediary_packet_data, (&bitstream[current_pos]), X300_PACKET_SIZE_BYTES);
                    current_pos += X300_PACKET_SIZE_BYTES;
                }
                else{
                    size_t len = fread(intermediary_packet_data, sizeof(boost::uint8_t), X300_PACKET_SIZE_BYTES, file);
                    if(len != X300_PACKET_SIZE_BYTES){
                        throw std::runtime_error("Error reading from file!");
                    }
                }
            }

            for(size_t k = 0; k < X300_PACKET_SIZE_BYTES; k++){
                intermediary_packet_data[k] = bitswap(intermediary_packet_data[k]);
            }

            memcpy(send_packet.data, intermediary_packet_data, X300_PACKET_SIZE_BYTES);

            for(size_t k = 0; k < (X300_PACKET_SIZE_BYTES/2); k++){
                send_packet.data[k] = htonx<boost::uint16_t>(send_packet.data[k]);
            }

            udp_transport->send(boost::asio::buffer(&send_packet, sizeof(send_packet)));

            if (udp_transport->recv(boost::asio::buffer(x300_data_in_mem), UDP_TIMEOUT) == 0)
                throw std::runtime_error("Timed out waiting for ACK!");

            const x300_fpga_update_data_t *update_data_in = reinterpret_cast<const x300_fpga_update_data_t *>(x300_data_in_mem);

            if((ntohl(update_data_in->flags) & X300_FPGA_PROG_FLAGS_ERROR) == X300_FPGA_PROG_FLAGS_ERROR){
                throw std::runtime_error("Transfer or data verification failed!");
            }
        }
    }
    fclose(file);

    //Send clean-up signal
    x300_fpga_update_data_t cleanup_packet;
    cleanup_packet.flags = htonx<boost::uint32_t>(X300_FPGA_PROG_FLAGS_ACK | X300_FPGA_PROG_FLAGS_CLEANUP);
    cleanup_packet.sector = 0;
    cleanup_packet.size = 0;
    cleanup_packet.index = 0;
    memset(cleanup_packet.data, 0, sizeof(cleanup_packet.data));
    udp_transport->send(boost::asio::buffer(&cleanup_packet, sizeof(cleanup_packet)));

    if (udp_transport->recv(boost::asio::buffer(x300_data_in_mem), UDP_TIMEOUT) == 0)
            throw std::runtime_error("Timed out waiting for ACK!");
    const x300_fpga_update_data_t *cleanup_data_in = reinterpret_cast<const x300_fpga_update_data_t *>(x300_data_in_mem);

    if((ntohl(cleanup_data_in->flags) & X300_FPGA_PROG_FLAGS_ERROR) == X300_FPGA_PROG_FLAGS_ERROR){
        throw std::runtime_error("Transfer or data verification failed!");
    }

    std::cout   << "\rProgress: " << "100% (" << sectors << "/" << sectors << " sectors)" << std::endl;
}

void pcie_burn(std::string resource, std::string rpc_port, std::string fpga_path)
{
    std::cout << "Burning image: " << fpga_path << std::endl;
    std::cout << "This will take 3-10 minutes." << std::endl;

    nirio_status status = NiRio_Status_Success;

    uhd::niusrprio::niusrprio_session fpga_session(resource, rpc_port);
    nirio_status_chain(fpga_session.download_bitstream_to_flash(fpga_path), status);

    if(nirio_status_fatal(status)) throw std::runtime_error("Failed to burn FPGA image!");
}

bool configure_fpga(udp_simple::sptr udp_transport, std::string ip_addr){
    x300_fpga_update_data_t configure_packet;
    configure_packet.flags = htonx<boost::uint32_t>(X300_FPGA_PROG_CONFIGURE | X300_FPGA_PROG_FLAGS_ACK);
    configure_packet.sector = 0;
    configure_packet.size = 0;
    configure_packet.index = 0;
    memset(configure_packet.data, 0, sizeof(configure_packet.data));
    udp_transport->send(boost::asio::buffer(&configure_packet, sizeof(configure_packet)));

    udp_transport->recv(boost::asio::buffer(x300_data_in_mem), UDP_TIMEOUT);
    const x300_fpga_update_data_t *configure_data_in = reinterpret_cast<const x300_fpga_update_data_t *>(x300_data_in_mem);
    bool successful = false;

    if((ntohl(configure_data_in->flags) & X300_FPGA_PROG_FLAGS_ERROR) == X300_FPGA_PROG_FLAGS_ERROR){
        throw std::runtime_error("Transfer or data verification failed!");
    }
    else{
        std::cout << std::endl << "Waiting for X3x0 to configure FPGA image and reload." << std::endl;
        boost::this_thread::sleep(boost::posix_time::milliseconds(5000));

        x300_fpga_update_data_t config_status_packet;
        configure_packet.flags = htonx<boost::uint32_t>(X300_FPGA_PROG_CONFIG_STATUS);
        config_status_packet.sector = 0;
        config_status_packet.size = 0;
        config_status_packet.index = 0;
        memset(config_status_packet.data, 0, sizeof(config_status_packet.data));
        for(int i = 0; i < 5; i++){
            udp_transport->send(boost::asio::buffer(&config_status_packet, sizeof(config_status_packet)));
            udp_transport->recv(boost::asio::buffer(x300_data_in_mem), 1);
            const x300_fpga_update_data_t *config_status_data_in = reinterpret_cast<const x300_fpga_update_data_t *>(x300_data_in_mem);

            if((ntohl(config_status_data_in->flags) & X300_FPGA_PROG_FLAGS_ERROR) != X300_FPGA_PROG_FLAGS_ERROR
               and udp_transport->get_recv_addr() == ip_addr){
                successful = true;
                break;
            }
            successful = false; //If it worked, the break would skip this
        }
    }
    return successful;
}

int UHD_SAFE_MAIN(int argc, char *argv[]){
    memset(intermediary_packet_data, 0, X300_PACKET_SIZE_BYTES);
    std::string ip_addr, resource, fpga_path, image_type, rpc_port;

    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Display this help message.")
        ("addr", po::value<std::string>(&ip_addr), "Specify an IP address.")
        ("resource", po::value<std::string>(&resource), "Specify an NI-RIO resource.")
        ("rpc-port", po::value<std::string>(&rpc_port)->default_value("5444"), "Specify a port to communicate with the RPC server.")
        ("type", po::value<std::string>(&image_type), "Specify an image type (1G, HGS, XGS), leave blank for current type.")
        ("fpga-path", po::value<std::string>(&fpga_path), "Specify an FPGA path (overrides --type option).")
        ("configure", "Initialize FPGA with image currently burned to flash (Ethernet only).")
        ("verify", "Verify data downloaded to flash (Ethernet only, download will take much longer)")
        ("list", "List all available X3x0 devices.")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //Print help message
    if(vm.count("help")){
        std::cout << "USRP X3x0 FPGA Burner" << std::endl << std::endl;

        std::cout << "Burns an FPGA image onto a USRP X300/X310. To burn the image" << std::endl
                  << "over Ethernet, specify an IP address with the --addr option," << std::endl
                  << "or to burn over PCIe, specify an NI-RIO resource (ex. RIO0)" << std::endl
                  << "with the --resource option." << std::endl << std::endl;

        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    //List all available devices
    if(vm.count("list")){
        list_usrps();
        return EXIT_SUCCESS;
    }

    /*
     * The user must specify whether to burn the image over Ethernet or PCI-e.
     */
    if(not (vm.count("addr") xor vm.count("resource"))){
        throw std::runtime_error("You must specify addr OR resource!");
    }

    /*
     * With settings validated, find X3x0 with specified arguments.
     */
    device_addr_t dev = (vm.count("addr")) ? find_usrp_with_ethernet(ip_addr, true)
                                           : find_usrp_with_pcie(resource, true);

    /*
     * If custom FPGA path is given, ignore specified type and let FPGA
     * figure it out.
     */
    if(vm.count("fpga-path")){
        //Expand tilde usage if applicable
        #ifndef UHD_PLATFORM_WIN32
            if(fpga_path.find("~/") == 0) fpga_path.replace(0,1,getenv("HOME"));
        #endif
    }
    else{
        if(vm.count("type")){
            //Make sure the specified type is 1G, HGS, or XGS
            if((image_type != "1G") and (image_type != "HGS") and (image_type != "XGS")){
                throw std::runtime_error("--type must be 1G, HGS, or XGS!");
            }
            else fpga_path = get_default_image_path(dev["product"], image_type);
        }
        else{
            //Use default image of currently present FPGA type
            fpga_path = get_default_image_path(dev["product"], dev["fpga"]);
        }
    }

    /*
     * Check validity of image through extension
     */
    std::string ext = fs::extension(fpga_path.c_str());
    if(ext != ".bin" and ext != ".bit" and ext != ".lvbitx"){
        throw std::runtime_error("The image filename must end in .bin, .bit, or .lvbitx.");
    }

    print_image_loader_warning(fpga_path, vm);

    std::signal(SIGINT, &sig_int_handler);
    if(vm.count("addr")){
        udp_simple::sptr udp_transport = udp_simple::make_connected(ip_addr, BOOST_STRINGIZE(X300_FPGA_PROG_UDP_PORT));

        ethernet_burn(udp_transport, fpga_path, (vm.count("verify") > 0));

        if(vm.count("configure")){
            if(configure_fpga(udp_transport, ip_addr)) std::cout << "Successfully configured FPGA!" << std::endl;
            else throw std::runtime_error("FPGA configuring failed!");
        }
    }
    else pcie_burn(resource, rpc_port, fpga_path);

    /*
     * Attempt to find USRP after burning
     */
    std::cout << std::endl << "Attempting to find device..." << std::flush;
    boost::this_thread::sleep(boost::posix_time::milliseconds(2000)); //Sometimes needed for Ethernet to reconnect
    device_addr_t found_usrp = (vm.count("addr")) ? find_usrp_with_ethernet(ip_addr, false)
                                                  : find_usrp_with_pcie(resource, false);
    std::cout << "found!" << std::endl; //If unsuccessful, runtime error would occur in find functions
    std::cout << "Successfully burned FPGA image!" << std::endl << std::endl;

    if(vm.count("addr")) std::cout << str(boost::format("Power-cycle the USRP %s to use the new image.") % found_usrp["product"]) << std::endl;
    else std::cout << str(boost::format("Power-cycle the USRP %s and reboot your machine to use the new image.") % found_usrp["product"]) << std::endl;

    return EXIT_SUCCESS;
}
