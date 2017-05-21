//
// Copyright 2017 Ettus Research (National Instruments)
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

#ifndef INCLUDED_MPMD_IMPL_HPP
#define INCLUDED_MPMD_IMPL_HPP
#include "../../utils/rpc.hpp"
#include "../device3/device3_impl.hpp"
#include <uhd/stream.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/tasks.hpp>
#include <map>

static const size_t MPMD_RX_SW_BUFF_SIZE_ETH        = 0x2000000;//32MiB    For an ~8k frame size any size >32MiB is just wasted buffer space
static const size_t MPMD_RX_SW_BUFF_SIZE_ETH_MACOS  = 0x100000; //1Mib

static const size_t MPM_DISCOVERY_PORT = 49600;
static const size_t MPM_RPC_PORT = 49601;
static const char MPM_DISCOVERY_CMD[] = "MPM-DISC";
static const char MPM_ECHO_CMD[] = "MPM-ECHO";
static const size_t MPMD_10GE_DATA_FRAME_MAX_SIZE = 8000; // CHDR packet size in bytes

struct frame_size_t
{
    size_t recv_frame_size;
    size_t send_frame_size;
};

class mpmd_mboard_impl
{
  public:
    using uptr = std::unique_ptr<mpmd_mboard_impl>;
    using dev_info = std::map<std::string, std::string>;
    mpmd_mboard_impl(const std::string& addr);
    ~mpmd_mboard_impl();
    static uptr make(const std::string& addr);

    bool initialization_done = false;
    uhd::dict<std::string, std::string> device_info;
    uhd::dict<std::string, std::string> recv_args;
    uhd::dict<std::string, std::string> send_args;
    std::map<std::string, std::string> data_interfaces;
    std::string loaded_fpga_image;
    std::string xport_path;
    uhd::rpc_client rpc;
    uhd::sid_t allocate_sid(const uint16_t port,
                            const uhd::sid_t address,
                            const uint32_t xbar_src_addr,
                            const uint32_t xbar_src_dst);

  private:
    bool claim();
    std::string generate_token() const;
    std::string _rpc_token;
    uhd::task::sptr _claimer_task;
};

class mpmd_impl : public uhd::usrp::device3_impl
{
  public:
    mpmd_impl(const uhd::device_addr_t& device_addr);
    ~mpmd_impl();

    mpmd_mboard_impl::uptr setup_mb(const size_t mb_i,
                                    const uhd::device_addr_t& dev_addr);
    uhd::both_xports_t make_transport(const uhd::sid_t&,
                                      uhd::usrp::device3_impl::xport_type_t,
                                      const uhd::device_addr_t&);

  private:
    uhd::device_addr_t get_rx_hints(size_t mb_index);

    uhd::device_addr_t _device_addr;
    std::vector<mpmd_mboard_impl::uptr> _mb;
    size_t _sid_framer;
};
uhd::device_addrs_t mpmd_find(const uhd::device_addr_t& hint_);
#endif /* INCLUDED_MPMD_IMPL_HPP */
// vim: sw=4 expandtab:
