//
// Copyright 2014 Ettus Research LLC
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

#include <uhd/transport/nirio/niusrprio_session.h>
#include <uhd/transport/nirio/niriok_proxy.h>
#include <uhd/transport/nirio/nifpga_lvbitx.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <iostream>
#include <fstream>
#include <streambuf>
#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>

using namespace uhd::niusrprio;
using namespace uhd::usrprio_rpc;

class dummy_lvbitx : public nifpga_lvbitx {
public:
    dummy_lvbitx(const std::string& fpga_lvbitx_path) : _fpga_lvbitx_path(fpga_lvbitx_path) {
        std::ifstream lvbitx_stream(_fpga_lvbitx_path.c_str());
        if (lvbitx_stream.is_open()) {
            std::string lvbitx_contents;
            lvbitx_stream.seekg(0, std::ios::end);
            lvbitx_contents.reserve(static_cast<size_t>(lvbitx_stream.tellg()));
            lvbitx_stream.seekg(0, std::ios::beg);
            lvbitx_contents.assign((std::istreambuf_iterator<char>(lvbitx_stream)), std::istreambuf_iterator<char>());
            try {
                boost::smatch md5_match;
                if (boost::regex_search(lvbitx_contents, md5_match, boost::regex("<BitstreamMD5>([a-zA-Z0-9]{32})<\\/BitstreamMD5>", boost::regex::icase))) {
                    _bitstream_checksum = std::string(md5_match[1].first, md5_match[1].second);
                }
                boost::to_upper(_bitstream_checksum);
            } catch (boost::exception&) {
                _bitstream_checksum = "";
            }
            try {
                boost::smatch sig_match;
                if (boost::regex_search(lvbitx_contents, sig_match, boost::regex("<SignatureRegister>([a-zA-Z0-9]{32})<\\/SignatureRegister>", boost::regex::icase))) {
                    _signature = std::string(sig_match[1].first, sig_match[1].second);
                }
                boost::to_upper(_signature);
            } catch (boost::exception&) {
                _signature = "";
            }
        }
    }
    ~dummy_lvbitx() {}

    virtual const char* get_bitfile_path() { return _fpga_lvbitx_path.c_str(); }
    virtual const char* get_signature() { return _signature.c_str(); }
    virtual const char* get_bitstream_checksum() { return _bitstream_checksum.c_str(); }

    virtual size_t get_input_fifo_count() { return 0; }
    virtual const char** get_input_fifo_names() { return NULL; }

    virtual size_t get_output_fifo_count() { return 0; }
    virtual const char** get_output_fifo_names() { return NULL; }

    virtual size_t get_control_count() { return 0; }
    virtual const char** get_control_names() { return NULL; }

    virtual size_t get_indicator_count() { return 0; }
    virtual const char** get_indicator_names() { return NULL; }

    virtual void init_register_info(nirio_register_info_vtr& vtr) { vtr.clear(); }
    virtual void init_fifo_info(nirio_fifo_info_vtr& vtr) { vtr.clear(); }

private:
    std::string _fpga_lvbitx_path;
    std::string _bitstream_checksum;
    std::string _signature;
};

int main(int argc, char *argv[])
{
    nirio_status status = NiRio_Status_Success;

    //Setup the program options
    uint32_t interface_num, peek_addr, poke_addr, poke_data;
    std::string rpc_port, fpga_lvbitx_path, flash_path, peek_tokens_str, poke_tokens_str;

    namespace po = boost::program_options;
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "help message")
        ("interface", po::value<uint32_t>(&interface_num)->default_value(0), "The interface number to communicate with.")
        ("port", po::value<std::string>(&rpc_port)->default_value("5444"), "Port to communicate with RPC server.")
        ("fpga", po::value<std::string>(&fpga_lvbitx_path)->default_value(""), "The absolute path to the LVBITX file to download to the FPGA.")
        ("flash", po::value<std::string>(&flash_path)->default_value(""), "The path to the image to download to the flash OR 'erase' to erase the FPGA image from flash.")
        ("peek", po::value<std::string>(&peek_tokens_str)->default_value(""), "Peek32.")
        ("poke", po::value<std::string>(&poke_tokens_str)->default_value(""), "Poke32.")
        ("stats", "Dump interface and DMA stats.")
    ;
    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    //Print the help message
    if (vm.count("help")){
        std::cout << boost::format("USRP-NIRIO-Programmer\n\n %s") % desc << std::endl;
        return ~0;
    }

    std::string resource_name = boost::str(boost::format("RIO%u") % interface_num);

    //Download LVBITX image
    if (fpga_lvbitx_path != "")
    {
        printf("Downloading image %s to FPGA as %s...", fpga_lvbitx_path.c_str(), resource_name.c_str());
        fflush(stdout);
        uhd::niusrprio::niusrprio_session fpga_session(resource_name, rpc_port);
        uhd::niusrprio::nifpga_lvbitx::sptr lvbitx(new dummy_lvbitx(fpga_lvbitx_path));
        nirio_status_chain(fpga_session.open(lvbitx, true), status);
        //Download BIN to flash or erase
        if (flash_path != "erase") {
            if (flash_path != "") {
                printf("Writing FPGA image %s to flash...", flash_path.c_str());
                fflush(stdout);
                nirio_status_chain(fpga_session.download_bitstream_to_flash(flash_path), status);
                printf("DONE\n");
            }
        } else {
            printf("Erasing FPGA image from flash...");
            fflush(stdout);
            nirio_status_chain(fpga_session.download_bitstream_to_flash(""), status);
            printf("DONE\n");
        }
        fpga_session.close();
        printf("DONE\n");
    }

    fflush(stdout);
    usrprio_rpc_client temp_rpc_client("localhost", rpc_port);
    std::string interface_path;
    nirio_status_chain(temp_rpc_client.niusrprio_get_interface_path(resource_name, interface_path), status);
    if (interface_path.empty()) {
        printf("ERROR: Could not open a proxy to interface %u. If it exists, try downloading an LVBITX to the FPGA first.\n", interface_num);
        exit(EXIT_FAILURE);
    }

    niriok_proxy::sptr dev_proxy = niriok_proxy::make_and_open(interface_path);

    if (poke_tokens_str != ""){
        std::stringstream ss;
        std::vector<std::string> poke_tokens;
        boost::split(poke_tokens, poke_tokens_str, boost::is_any_of(":"));
        ss.clear();
        ss << std::hex << poke_tokens[1];
        ss >> poke_addr;
        ss.clear();
        ss << std::hex << poke_tokens[2];
        ss >> poke_data;

        niriok_scoped_addr_space(dev_proxy, poke_tokens[0]=="c"?BUS_INTERFACE:FPGA, status);
        if (poke_tokens[0]=="z") {
            nirio_status_chain(dev_proxy->poke(poke_addr, (uint32_t)0x70000 + poke_addr), status);
        } else {
            nirio_status_chain(dev_proxy->poke(poke_addr, poke_data), status);
        }
        printf("[POKE] %s:0x%x <= 0x%x (%u)\n", poke_tokens[0]=="c"?"Chinch":(poke_tokens[0]=="z"?"ZPU":"FPGA"), poke_addr, poke_data, poke_data);
    }

    if (peek_tokens_str != ""){
        std::stringstream ss;
        std::vector<std::string> peek_tokens;
        boost::split(peek_tokens, peek_tokens_str, boost::is_any_of(":"));
        ss.clear();
        ss << std::hex << peek_tokens[1];
        ss >> peek_addr;

        niriok_scoped_addr_space(dev_proxy, peek_tokens[0]=="c"?BUS_INTERFACE:FPGA, status);
        uint32_t reg_val = 0;
        if (peek_tokens[0]=="z") {
            nirio_status_chain(dev_proxy->poke((uint32_t)0x60000 + peek_addr, (uint32_t)0), status);
            do {
                nirio_status_chain(dev_proxy->peek((uint32_t)0x60000 + peek_addr, reg_val), status);
            } while (reg_val != 0);
            nirio_status_chain(dev_proxy->peek((uint32_t)0x70000 + peek_addr, reg_val), status);
        } else {
            nirio_status_chain(dev_proxy->peek(peek_addr, reg_val), status);
        }

        printf("[PEEK] %s:0x%x = 0x%x (%u)\n", peek_tokens[0]=="c"?"Chinch":(peek_tokens[0]=="z"?"ZPU":"FPGA"), peek_addr, reg_val, reg_val);
    }

    //Display attributes
    if (vm.count("stats")){
        printf("[Interface %u]\n", interface_num);
        uint32_t attr_val = 0;
        nirio_status_chain(dev_proxy->get_attribute(RIO_IS_FPGA_PROGRAMMED, attr_val), status);
        printf("* Is FPGA Programmed? = %s\n", (attr_val==1)?"YES":"NO");

        std::string signature;
        for (int i = 0; i < 4; i++) {
            nirio_status_chain(dev_proxy->peek(0x3FFF4, attr_val), status);
            signature += boost::str(boost::format("%08x") % attr_val);
        }
        printf("* FPGA Signature = %s\n", signature.c_str());

        std::string checksum;
        for (int i = 0; i < 4; i++) {
            nirio_status_chain(dev_proxy->peek(0x40030 + (i * 4), attr_val), status);
            checksum += boost::str(boost::format("%08x") % attr_val);
        }
        printf("* FPGA Bitstream Checksum = %s\n", checksum.c_str());

        uint32_t reg_val = 0;
        nirio_status_chain(dev_proxy->set_attribute(RIO_ADDRESS_SPACE, BUS_INTERFACE), status);
        nirio_status_chain(dev_proxy->peek(0, reg_val), status);
        printf("* Chinch Signature = %x\n", reg_val);
        nirio_status_chain(dev_proxy->set_attribute(RIO_ADDRESS_SPACE, FPGA), status);
        nirio_status_chain(dev_proxy->peek(0, reg_val), status);
        printf("* PCIe FPGA Signature = %x\n", reg_val);

        printf("\n[DMA Stream Stats]\n");

        nirio_status_chain(dev_proxy->set_attribute(RIO_ADDRESS_SPACE, FPGA), status);

        printf("------------------------------------------------------------------------------------------------");
        printf("\nChannel =>       |");
        for (uint32_t i = 0; i < 6; i++) {
            printf("%11u |", i);
        }
        printf("\n------------------------------------------------------------------------------------------------");
        printf("\nTX Status        |");
        for (uint32_t i = 0; i < 6; i++) {
            nirio_status_chain(dev_proxy->peek(0x40200 + (i * 16), reg_val), status);
            printf("%s |", reg_val==0 ? "       Good" : "      Error");
        }
        printf("\nRX Status        |");
        for (uint32_t i = 0; i < 6; i++) {
            nirio_status_chain(dev_proxy->peek(0x40400 + (i * 16), reg_val), status);
            printf("%s |", reg_val==0 ? "       Good" : "      Error");
        }
        printf("\nTX Frm Size      |");
        for (uint32_t i = 0; i < 6; i++) {
            nirio_status_chain(dev_proxy->peek(0x40204 + (i * 16), reg_val), status);
            printf("%11u |", reg_val);
        }
        printf("\nRX Frm Size      |");
        for (uint32_t i = 0; i < 6; i++) {
            nirio_status_chain(dev_proxy->peek(0x40404 + (i * 16), reg_val), status);
            printf("%11u |", reg_val);
        }
        printf("\nTX Pkt Count     |");
        for (uint32_t i = 0; i < 6; i++) {
            nirio_status_chain(dev_proxy->peek(0x4020C + (i * 16), reg_val), status);
            printf("%11u |", reg_val);
        }
        printf("\nTX Samp Count    |");
        for (uint32_t i = 0; i < 6; i++) {
            nirio_status_chain(dev_proxy->peek(0x40208 + (i * 16), reg_val), status);
            printf("%11u |", reg_val);
        }
        printf("\nRX Pkt Count     |");
        for (uint32_t i = 0; i < 6; i++) {
            nirio_status_chain(dev_proxy->peek(0x4040C + (i * 16), reg_val), status);
            printf("%11u |", reg_val);
        }
        printf("\nRX Samp Count    |");
        for (uint32_t i = 0; i < 6; i++) {
            nirio_status_chain(dev_proxy->peek(0x40408 + (i * 16), reg_val), status);
            printf("%11u |", reg_val);
        }
        printf("\n------------------------------------------------------------------------------------------------\n");
    }

    exit(EXIT_SUCCESS);
}


