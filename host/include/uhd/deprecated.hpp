//----------------------------------------------------------------------
//-- deprecated interfaces below, to be removed when the API is changed
//----------------------------------------------------------------------

//
// Copyright 2010 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_OTW_TYPE_HPP
#define INCLUDED_UHD_TYPES_OTW_TYPE_HPP

#include <uhd/config.hpp>
#include <cstddef>

namespace uhd{

    /*!
     * Description for over-the-wire integers:
     * The DSP units in the FPGA deal with signed 16-bit integers.
     * The width and shift define the translation between OTW and DSP,
     * defined by the following relation: otw_int = dsp_int >> shift
     *
     * Note: possible combinations of width, shift, and byteorder
     * depend on the internals of the FPGA. Not all are supported!
     */
    struct UHD_API otw_type_t{

        /*!
         * Width of an over-the-wire integer in bits.
         */
        size_t width; //in bits

        /*!
         * Shift of an over-the-wire integer in bits.
         * otw_int = dsp_int >> shift
         * dsp_int = otw_int << shift
         */
        size_t shift; //in bits

        /*!
         * Constants for byte order (borrowed from numpy's dtype)
         */
        enum /*bo_t*/ {
            BO_NATIVE         = int('='),
            BO_LITTLE_ENDIAN  = int('<'),
            BO_BIG_ENDIAN     = int('>'),
            BO_NOT_APPLICABLE = int('|')
        } byteorder;

        /*!
         * Get the sample size of this otw type.
         * \return the size of a sample in bytes
         */
        size_t get_sample_size(void) const;

        otw_type_t(void);
    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_OTW_TYPE_HPP */

#include <uhd/types/io_type.hpp> //wish it was in here
#include <uhd/types/clock_config.hpp> //wish it was in here
