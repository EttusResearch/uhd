/* -*- c++ -*- */
/*
 * Copyright 2024 Ettus Research, an NI Brand.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_RFNOC_GAIN_RFNOC_GAIN_H
#define INCLUDED_RFNOC_GAIN_RFNOC_GAIN_H

#include <gnuradio/rfnoc_gain/api.h>
#include <gnuradio/uhd/rfnoc_block.h>
#include <cstdint>

namespace gr {
namespace rfnoc_gain {

/*! Block controller for the RFNoC "gain" block.
 *
 * This RFNoC block is a very simply block that multiplies the incoming sample
 * value with the integer gain value. It is used as an example for how to write
 * RFNoC blocks.
 *
 * This GNU Radio block is a C++ controller of the RFNoC gain block, which
 * allows using gain block in both C++ and Python GNU Radio applications
 * (including using it in GRC). In many cases, a C++ GNU Radio block is overkill
 * for such a simple block, and the better approach would be to use the generic
 * RFNoC block instead. There are also examples of this, much simpler approach
 * in the examples subdirectory of this GNU Radio OOT module.
 *
 * \ingroup rfnoc_gain
 *
 */
class RFNOC_GAIN_API rfnoc_gain : virtual public gr::uhd::rfnoc_block
{
public:
    typedef std::shared_ptr<rfnoc_gain> sptr;

    /*!
     * \param graph Reference to the rfnoc_graph object this block is attached to
     * \param gain The digital gain that will be applied (e.g., \p gain == 2
     *             will double the signal)
     * \param block_args Additional block arguments
     * \param device_select Device Selection
     * \param instance Instance Selection
     */
    static sptr make(uhd::rfnoc_graph::sptr graph,
                     const uint32_t gain,
                     const ::uhd::device_addr_t& block_args,
                     const int device_select,
                     const int instance);

    /*! Set the gain value
     */
    virtual void set_gain(const uint32_t gain) = 0;

    /*! Read back the current gain value
     */
    virtual uint32_t get_gain() = 0;
};

} // namespace rfnoc_gain
} // namespace gr

#endif /* INCLUDED_RFNOC_GAIN_RFNOC_GAIN_H */
