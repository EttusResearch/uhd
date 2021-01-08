//
// Copyright 2017 Ettus Research, National Instruments Company
// Copyright 2019 Ettus Research, National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include "mpmd_link_if_mgr.hpp"
#include "mpmd_impl.hpp"
#include "mpmd_link_if_ctrl_base.hpp"
#include "mpmd_link_if_ctrl_udp.hpp"

uhd::dict<std::string, std::string> uhd::mpmd::xport::filter_args(
    const uhd::device_addr_t& args, const std::string& prefix)
{
    uhd::dict<std::string, std::string> filtered_args;
    for (const std::string& key : args.keys()) {
        if (key.find(prefix) != std::string::npos) {
            filtered_args[key] = args[key];
        }
    }

    return filtered_args;
}

using namespace uhd::mpmd::xport;

class mpmd_link_if_mgr_impl : public mpmd_link_if_mgr
{
public:
    mpmd_link_if_mgr_impl(const uhd::device_addr_t& mb_args) : _mb_args(mb_args) {}

    /**************************************************************************
     * API (see mpmd_link_if_mgr.hpp)
     *************************************************************************/
    bool connect(const std::string& link_type,
        const xport_info_list_t& xport_info,
        const uhd::rfnoc::chdr_w_t chdr_w) override
    {
        auto link_if_ctrl = make_link_if_ctrl(link_type, xport_info, chdr_w);
        if (!link_if_ctrl) {
            UHD_LOG_WARNING(
                "MPMD::XPORT", "Unable to create xport ctrl for link type " << link_type);
            return false;
        }
        if (link_if_ctrl->get_num_links() == 0) {
            UHD_LOG_TRACE("MPMD::XPORT",
                "Link type " << link_type
                             << " has no valid links in this configuration.");
            return false;
        }
        const size_t xport_idx = _link_if_ctrls.size();
        for (size_t link_idx = 0; link_idx < link_if_ctrl->get_num_links(); link_idx++) {
            _link_link_if_ctrl_map.push_back(std::make_pair(xport_idx, link_idx));
        }
        _link_if_ctrls.push_back(std::move(link_if_ctrl));
        return true;
    }

    size_t get_num_links() override
    {
        return _link_link_if_ctrl_map.size();
    }

    uhd::transport::both_links_t get_link(const size_t link_idx,
        const uhd::transport::link_type_t link_type,
        const uhd::device_addr_t& link_args) override
    {
        const size_t link_if_ctrl_idx = _link_link_if_ctrl_map.at(link_idx).first;
        const size_t xport_link_idx   = _link_link_if_ctrl_map.at(link_idx).second;
        return _link_if_ctrls.at(link_if_ctrl_idx)
            ->get_link(xport_link_idx, link_type, link_args);
    }

    size_t get_mtu(const size_t link_idx, const uhd::direction_t dir) const override
    {
        return _link_if_ctrls.at(_link_link_if_ctrl_map.at(link_idx).first)->get_mtu(dir);
    }

    const uhd::rfnoc::chdr::chdr_packet_factory& get_packet_factory(
        const size_t link_idx) const override
    {
        const size_t link_if_ctrl_idx = _link_link_if_ctrl_map.at(link_idx).first;
        return _link_if_ctrls.at(link_if_ctrl_idx)->get_packet_factory();
    }

private:
    /**************************************************************************
     * Private methods / helpers
     *************************************************************************/
    mpmd_link_if_ctrl_base::uptr make_link_if_ctrl(const std::string& link_type,
        const xport_info_list_t& xport_info,
        const uhd::rfnoc::chdr_w_t chdr_w)
    {
        // Here, we hard-code the list of available transport types
        if (link_type == "udp") {
            return std::make_unique<mpmd_link_if_ctrl_udp>(_mb_args, xport_info, chdr_w);
        }
        UHD_LOG_WARNING("MPMD", "Cannot instantiate transport medium " << link_type);
        return nullptr;
    }

    /**************************************************************************
     * Private attributes
     *************************************************************************/
    //! Cache available xport manager implementations
    //
    // Should only every be populated by connect()
    std::vector<mpmd_link_if_ctrl_base::uptr> _link_if_ctrls;
    // Maps link index to link_if_ctrl index. To look up the xport ctrl for link
    // number L, do something like this:
    // auto& link_if_ctrl = _link_if_ctrls.at(_link_link_if_ctrl_map.at(L).first);
    std::vector<std::pair<size_t, size_t>> _link_link_if_ctrl_map;

    //! Motherboard args, can contain things like 'recv_buff_size'
    const uhd::device_addr_t _mb_args;
};

mpmd_link_if_mgr::uptr mpmd_link_if_mgr::make(const uhd::device_addr_t& mb_args)
{
    return std::make_unique<mpmd_link_if_mgr_impl>(mb_args);
}
