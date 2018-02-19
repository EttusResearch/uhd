//
// Copyright 2017 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef LIBERIO_HPP
#define LIBERIO_HPP

#include <string>
#include <vector>

#include <uhd/config.hpp>
#include <uhd/transport/zero_copy.hpp>
#include <uhd/types/device_addr.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd { namespace transport {

/*!
 * A zero copy transport interface to the liberio DMA library.
 */
class liberio_zero_copy : public virtual zero_copy_if {
public:
    typedef boost::shared_ptr<liberio_zero_copy> sptr;

    static sptr make(
        const std::string &tx_path,
        const std::string &rx_path,
        const zero_copy_xport_params &default_buff_args
    );
};

}}

#endif /* LIBERIO_HPP */
