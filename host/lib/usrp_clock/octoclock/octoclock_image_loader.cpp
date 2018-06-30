//
// Copyright 2015-2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "octoclock_impl.hpp"
#include "common.h"

#include <uhd/device.hpp>
#include <uhd/image_loader.hpp>
#include <uhd/transport/udp_simple.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/time_spec.hpp>
#include <uhd/utils/byteswap.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/static.hpp>
#include <uhdlib/utils/ihex.hpp>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <algorithm>
#include <iterator>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <stdint.h>

namespace fs = boost::filesystem;
using namespace uhd;
using namespace uhd::usrp_clock;
using namespace uhd::transport;

#define OCTOCLOCK_FIRMWARE_MAX_SIZE_BYTES (1024*120) // Last 8 MB are for bootloader
#define OCTOCLOCK_BLOCK_SIZE              256

/*
 * OctoClock burn session
 */
typedef struct {
    bool                        found;
    uhd::device_addr_t          dev_addr;
    std::string                 image_filepath;
    uint16_t             crc;
    uint16_t             num_blocks;
    uint32_t             sequence;
    udp_simple::sptr            ctrl_xport;
    udp_simple::sptr            fw_xport;
    uint8_t              data_in[udp_simple::mtu];
    uint32_t             starting_firmware_version;
    std::vector<uint8_t> image;
} octoclock_session_t;

static void octoclock_calculate_crc(octoclock_session_t &session){
    session.crc = 0xFFFF;
    for(size_t i = 0; i < session.image.size(); i++)
    {
        session.crc ^= session.image[i];
        for(uint8_t j = 0; j < 8; ++j){
            if(session.crc & 1) session.crc = (session.crc >> 1) ^ 0xA001;
            else session.crc = (session.crc >> 1);
        }
    }
}

static void octoclock_read_bin(octoclock_session_t &session)
{
    std::ifstream bin_file(session.image_filepath.c_str(), std::ios::in | std::ios::binary);
    if (not bin_file.is_open()) {
        throw uhd::io_error("Could not read image file.");
    }

    size_t filesize = fs::file_size(session.image_filepath);
    session.image.clear();
    session.image.resize(filesize);
    bin_file.read((char*)&session.image[0], filesize);
    if(size_t(bin_file.gcount()) != filesize) {
        throw uhd::io_error("Failed to read firmware image.");
    }

    bin_file.close();
}

static void octoclock_validate_firmware_image(octoclock_session_t &session){
    if(not fs::exists(session.image_filepath)){
        throw uhd::runtime_error(str(boost::format("Could not find image at path \"%s\"")
                                     % session.image_filepath));
    }

    std::string extension = fs::extension(session.image_filepath);
    if(extension == ".bin"){
        octoclock_read_bin(session);
    }
    else if(extension == ".hex"){
        ihex_reader hex_reader(session.image_filepath);
        session.image = hex_reader.to_vector(OCTOCLOCK_FIRMWARE_MAX_SIZE_BYTES);
    }
    else throw uhd::runtime_error(str(boost::format("Invalid extension \"%s\". Extension must be .hex or .bin.")
                                      % extension));

    if(session.image.size() > OCTOCLOCK_FIRMWARE_MAX_SIZE_BYTES){
        throw uhd::runtime_error(str(boost::format("The specified firmware image is too large: %d vs. %d")
                                     % session.image.size() % OCTOCLOCK_FIRMWARE_MAX_SIZE_BYTES));
    }

    session.num_blocks = (session.image.size() % OCTOCLOCK_BLOCK_SIZE)
                            ? ((session.image.size() / OCTOCLOCK_BLOCK_SIZE) + 1)
                            : (session.image.size() / OCTOCLOCK_BLOCK_SIZE);

    octoclock_calculate_crc(session);
}

static void octoclock_setup_session(octoclock_session_t &session,
                                    const uhd::device_addr_t &args,
                                    const std::string &filepath){

    // See if we can find an OctoClock with the given args
    device_addrs_t devs = octoclock_find(args);
    if(devs.size() == 0){
        session.found = false;
        return;
    }
    else if(devs.size() > 1){
        std::string err_msg = "Could not resolve given args to a single OctoClock device.\n"
                              "Applicable devices:\n";

        for(const uhd::device_addr_t &dev:  devs){
            std::string name = (dev["type"] == "octoclock") ? str(boost::format("OctoClock r%d")
                                                                  % dev.get("revision","4"))
                                                          : "OctoClock Bootloader";
            err_msg += str(boost::format(" * %s (addr=%s)\n")
                           % name
                           % dev.get("addr"));
        }

        err_msg += "\nSpecify one of these devices with the given args to load an image onto it.";

        throw uhd::runtime_error(err_msg);
    }

    session.dev_addr = devs[0];
    session.found = true;

    // If no filepath is given, use the default
    if(filepath == ""){
        session.image_filepath = find_image_path(str(boost::format("octoclock_r%s_fw.hex")
                                                     % session.dev_addr.get("revision","4")
                                                 ));
    }
    else session.image_filepath = filepath;

    octoclock_validate_firmware_image(session);

    session.ctrl_xport = udp_simple::make_connected(session.dev_addr["addr"],
                                                    BOOST_STRINGIZE(OCTOCLOCK_UDP_CTRL_PORT));
    session.fw_xport   = udp_simple::make_connected(session.dev_addr["addr"],
                                                    BOOST_STRINGIZE(OCTOCLOCK_UDP_FW_PORT));
    // To avoid replicating sequence numbers between sessions
    session.sequence   = uint32_t(std::rand());

    // Query OctoClock again to get compat number
    octoclock_packet_t pkt_out;
    const octoclock_packet_t* pkt_in = reinterpret_cast<const octoclock_packet_t*>(session.data_in);
    size_t len = 0;
    UHD_OCTOCLOCK_SEND_AND_RECV(session.ctrl_xport, OCTOCLOCK_FW_COMPAT_NUM, OCTOCLOCK_QUERY_CMD, pkt_out, len, session.data_in);
    if(UHD_OCTOCLOCK_PACKET_MATCHES(OCTOCLOCK_QUERY_ACK, pkt_out, pkt_in, len)){
        session.starting_firmware_version = uhd::htonx<uint32_t>(pkt_in->proto_ver);
    } else {
        throw uhd::runtime_error("Failed to communicate with OctoClock.");
    }
}

static void octoclock_reset_into_bootloader(octoclock_session_t &session){

    // Already in bootloader
    if(session.dev_addr["type"] == "octoclock-bootloader")
        return;

    // Force compat num to device's current, works around old firmware bug
    octoclock_packet_t pkt_out;
    pkt_out.sequence = uhd::htonx<uint32_t>(++session.sequence);
    pkt_out.proto_ver = uhd::htonx<uint32_t>(session.starting_firmware_version);
    pkt_out.code = RESET_CMD;

    std::cout << " -- Resetting into bootloader..." << std::flush;
    session.ctrl_xport->send(boost::asio::buffer(&pkt_out, sizeof(octoclock_packet_t))); \
    size_t len = session.ctrl_xport->recv(boost::asio::buffer(session.data_in), 2);\
    const octoclock_packet_t* pkt_in = reinterpret_cast<const octoclock_packet_t*>(session.data_in);

    if(UHD_OCTOCLOCK_PACKET_MATCHES(RESET_ACK, pkt_out, pkt_in, len)){
        // Make sure this device is now in its bootloader
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        uhd::device_addrs_t octoclocks = uhd::device::find(
                                             uhd::device_addr_t(str(boost::format("addr=%s")
                                                                    % session.dev_addr["addr"]
                                                               )));
        if(octoclocks.size() == 0){
            std::cout << "failed." << std::endl;
            throw uhd::runtime_error("Failed to reset OctoClock.");
        }
        else if(octoclocks[0]["type"] != "octoclock-bootloader"){
            std::cout << "failed." << std::endl;
            throw uhd::runtime_error("Failed to reset OctoClock.");
        }
        else{
            std::cout << "successful." << std::endl;
            session.dev_addr = octoclocks[0];
        }
    }
    else{
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Failed to reset OctoClock.");
    }
}

static void octoclock_burn(octoclock_session_t &session){

    // Make sure we're in the bootloader for this
    octoclock_reset_into_bootloader(session);

    octoclock_packet_t pkt_out;
    pkt_out.sequence = uhd::htonx<uint32_t>(++session.sequence);
    const octoclock_packet_t* pkt_in = reinterpret_cast<const octoclock_packet_t*>(session.data_in);

    // Tell OctoClock to prepare for burn
    pkt_out.len = htonx<uint16_t>(session.image.size());
    size_t len = 0;
    std::cout << " -- Preparing OctoClock for firmware load..." << std::flush;
    pkt_out.len = session.image.size();
    pkt_out.crc = session.crc;
    UHD_OCTOCLOCK_SEND_AND_RECV(session.fw_xport, OCTOCLOCK_FW_COMPAT_NUM, PREPARE_FW_BURN_CMD, pkt_out, len, session.data_in);
    if(UHD_OCTOCLOCK_PACKET_MATCHES(FW_BURN_READY_ACK, pkt_out, pkt_in, len)){
        std::cout << "successful." << std::endl;
    }
    else{
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Failed to prepare OctoClock for firmware load.");
    }

    // Start burning
    for(size_t i = 0; i < session.num_blocks; i++){
        pkt_out.sequence = uhd::htonx<uint32_t>(++session.sequence);
        pkt_out.addr = i * OCTOCLOCK_BLOCK_SIZE;

        std::cout << str(boost::format("\r -- Loading firmware: %d%% (%d/%d blocks)")
                         % int((double(i)/double(session.num_blocks))*100)
                         % i % session.num_blocks)
                  << std::flush;

        memset(pkt_out.data, 0, OCTOCLOCK_BLOCK_SIZE);
        memcpy((char*)pkt_out.data, &session.image[pkt_out.addr], OCTOCLOCK_BLOCK_SIZE);
        UHD_OCTOCLOCK_SEND_AND_RECV(session.fw_xport, OCTOCLOCK_FW_COMPAT_NUM, FILE_TRANSFER_CMD, pkt_out, len, session.data_in);
        if(not UHD_OCTOCLOCK_PACKET_MATCHES(FILE_TRANSFER_ACK, pkt_out, pkt_in, len)){
            std::cout << std::endl;
            throw uhd::runtime_error("Failed to load firmware.");
        }
    }

    std::cout << str(boost::format("\r -- Loading firmware: 100%% (%d/%d blocks)")
                     % session.num_blocks % session.num_blocks)
              << std::endl;
}

static void octoclock_verify(octoclock_session_t &session){

    octoclock_packet_t pkt_out;
    pkt_out.sequence = uhd::htonx<uint32_t>(++session.sequence);
    const octoclock_packet_t* pkt_in = reinterpret_cast<const octoclock_packet_t*>(session.data_in);
    size_t len = 0;

    uint8_t image_part[OCTOCLOCK_BLOCK_SIZE];
    uint16_t cmp_len = 0;
    for(size_t i = 0; i < session.num_blocks; i++){
        pkt_out.sequence = uhd::htonx<uint32_t>(++session.sequence);
        pkt_out.addr = i * OCTOCLOCK_BLOCK_SIZE;

        std::cout << str(boost::format("\r -- Verifying firmware load: %d%% (%d/%d blocks)")
                         % int((double(i)/double(session.num_blocks))*100)
                         % i % session.num_blocks)
                  << std::flush;

        memset(image_part, 0, OCTOCLOCK_BLOCK_SIZE);
        memcpy((char*)image_part, &session.image[pkt_out.addr], OCTOCLOCK_BLOCK_SIZE);
        cmp_len = std::min<size_t>(OCTOCLOCK_BLOCK_SIZE, session.image.size() - size_t(pkt_out.addr));

        UHD_OCTOCLOCK_SEND_AND_RECV(session.fw_xport, OCTOCLOCK_FW_COMPAT_NUM, READ_FW_CMD, pkt_out, len, session.data_in);
        if(UHD_OCTOCLOCK_PACKET_MATCHES(READ_FW_ACK, pkt_out, pkt_in, len)){
            if(memcmp(pkt_in->data, image_part, cmp_len)){
                std::cout << std::endl;
                throw uhd::runtime_error("Failed to verify OctoClock firmware.");
            }
        }
        else{
            std::cout << std::endl;
            throw uhd::runtime_error("Failed to verify OctoClock firmware.");
        }
    }

    std::cout << str(boost::format("\r -- Verifying firmware load: 100%% (%d/%d blocks)")
                     % session.num_blocks % session.num_blocks)
              << std::endl;
}

static void octoclock_finalize(octoclock_session_t &session){

    octoclock_packet_t pkt_out;
    pkt_out.sequence = uhd::htonx<uint32_t>(++session.sequence);
    const octoclock_packet_t* pkt_in = reinterpret_cast<const octoclock_packet_t*>(session.data_in);
    size_t len = 0;

    std::cout << " -- Finalizing firmware load..." << std::flush;
    UHD_OCTOCLOCK_SEND_AND_RECV(session.fw_xport, OCTOCLOCK_FW_COMPAT_NUM, FINALIZE_BURNING_CMD, pkt_out, len, session.data_in);
    if(UHD_OCTOCLOCK_PACKET_MATCHES(FINALIZE_BURNING_ACK, pkt_out, pkt_in, len)){
        std::cout << "successful." << std::endl;
    }
    else{
        std::cout << "failed." << std::endl;
        throw uhd::runtime_error("Failed to finalize OctoClock firmware load.");
    }
}

bool octoclock_image_loader(const image_loader::image_loader_args_t &image_loader_args){
    octoclock_session_t session;
    octoclock_setup_session(session,
                            image_loader_args.args,
                            image_loader_args.firmware_path
                           );
    if(!session.found or !image_loader_args.load_firmware) return false;

    std::cout << boost::format("Unit: OctoClock (%s)")
                 % session.dev_addr["addr"]
              << std::endl;
    std::cout << "Firmware: " << session.image_filepath << std::endl;

    octoclock_burn(session);
    octoclock_verify(session);
    octoclock_finalize(session);

    return true;
}

UHD_STATIC_BLOCK(register_octoclock_image_loader){
    std::string recovery_instructions = "Aborting. Your OctoClock firmware is now corrupt. The bootloader\n"
                                        "is functional, but the device will not have functional clock distribution.\n"
                                        "Run this utility again to restore functionality or refer to:\n\n"
                                        "http://files.ettus.com/manual/page_octoclock.html\n\n"
                                        "for alternative setups.";

    image_loader::register_image_loader("octoclock",
                                        octoclock_image_loader,
                                        recovery_instructions);
}
