//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

template <typename data_t>
uhd::rfnoc::property_t<data_t>::property_t(
    const std::string& id, data_t&& data, const uhd::rfnoc::res_source_info& source_info)
    : uhd::rfnoc::property_base_t(id, source_info)
    , _data(std::forward<data_t>(data))
    , _valid(true)
{
    // nop
}

template <typename data_t>
uhd::rfnoc::property_t<data_t>::property_t(const std::string& id,
    const data_t& data,
    const uhd::rfnoc::res_source_info& source_info)
    : uhd::rfnoc::property_base_t(id, source_info), _data(data), _valid(true)
{
    // nop
}

template <typename data_t>
uhd::rfnoc::property_t<data_t>::property_t(
    const std::string& id, const uhd::rfnoc::res_source_info& source_info)
    : uhd::rfnoc::property_base_t(id, source_info), _valid(false)
{
    // nop
}
