//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_USRPRIO_RPC_COMMON_HPP
#define INCLUDED_USRPRIO_RPC_COMMON_HPP

#include <uhd/transport/nirio/rpc/rpc_common.hpp>

namespace uhd { namespace usrprio_rpc {

//Function IDs

static const func_id_t NIUSRPRIO_FUNC_BASE                  = 0x100;

static const func_id_t NIUSRPRIO_ENUMERATE                  = NIUSRPRIO_FUNC_BASE + 0;
static const func_id_t NIUSRPRIO_OPEN_SESSION               = NIUSRPRIO_FUNC_BASE + 1;
static const func_id_t NIUSRPRIO_CLOSE_SESSION              = NIUSRPRIO_FUNC_BASE + 2;
static const func_id_t NIUSRPRIO_RESET_SESSION              = NIUSRPRIO_FUNC_BASE + 3;
static const func_id_t NIUSRPRIO_DOWNLOAD_BITSTREAM_TO_FPGA = NIUSRPRIO_FUNC_BASE + 4;
static const func_id_t NIUSRPRIO_GET_INTERFACE_PATH         = NIUSRPRIO_FUNC_BASE + 5;
static const func_id_t NIUSRPRIO_DOWNLOAD_FPGA_TO_FLASH     = NIUSRPRIO_FUNC_BASE + 6;

//Function Args

struct usrprio_device_info {
    uint32_t interface_num;
    std::string     resource_name;
    std::string     pcie_serial_num;
    std::string     interface_path;

    template <typename Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        if (version || !version) {  //Suppress unused warning
            ar & interface_num;
            ar & resource_name;
            ar & pcie_serial_num;
            ar & interface_path;
        }
    }
};
typedef std::vector<usrprio_device_info> usrprio_device_info_vtr;

#define NIUSRPRIO_ENUMERATE_ARGS        \
    usrprio_device_info_vtr& device_info_vtr

#define NIUSRPRIO_OPEN_SESSION_ARGS         \
    const std::string& resource,            \
    const std::string& path,                \
    const std::string& signature,           \
    const uint16_t& download_fpga

#define NIUSRPRIO_CLOSE_SESSION_ARGS    \
    const std::string& resource

#define NIUSRPRIO_RESET_SESSION_ARGS    \
    const std::string& resource

#define NIUSRPRIO_DOWNLOAD_BITSTREAM_TO_FPGA_ARGS   \
    const std::string& resource

#define NIUSRPRIO_GET_INTERFACE_PATH_ARGS   \
    const std::string& resource,            \
    std::string& interface_path

#define NIUSRPRIO_DOWNLOAD_FPGA_TO_FLASH_ARGS   \
    const std::string& resource,                \
    const std::string& bitstream_path
}}

#endif /* INCLUDED_USRPRIO_RPC_COMMON_HPP */
