//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

namespace uhd { namespace mapper {

class gpio_port_mapper
{
public:
    /*! Converts from user-facing GPIO port numbering to internal representation working
     *  on int value. To be used in SPI core.
     */
    virtual uint32_t map_value(const uint32_t& value) = 0;

    /*! Converts internal GPIO port representation into user-facing port numbering working
     *  on int value. To be used in SPI core.
     */
    virtual uint32_t unmap_value(const uint32_t& value) = 0;
};

}} // namespace uhd::mapper
