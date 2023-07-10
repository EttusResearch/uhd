//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/types/filters.hpp>
#include <string>
#include <vector>

namespace uhd { namespace rfnoc { namespace detail {

// TODO: Support static filters
// TODO: User-defined (external?) filter
/*! Pure virtual mix-in class for RFNoC block controllers that have filters present.
 */
class filter_node
{
public:
    using sptr = std::shared_ptr<filter_node>;

    virtual ~filter_node() = default;

    virtual std::vector<std::string> get_rx_filter_names(const size_t chan) const = 0;
    virtual uhd::filter_info_base::sptr get_rx_filter(
        const std::string& name, const size_t chan) = 0;
    virtual void set_rx_filter(const std::string& name,
        uhd::filter_info_base::sptr filter,
        const size_t chan)                          = 0;

    virtual std::vector<std::string> get_tx_filter_names(const size_t chan) const = 0;
    virtual uhd::filter_info_base::sptr get_tx_filter(
        const std::string& name, const size_t chan) = 0;
    virtual void set_tx_filter(const std::string& name,
        uhd::filter_info_base::sptr filter,
        const size_t chan)                          = 0;
};

}}} /* namespace uhd::rfnoc::detail */
