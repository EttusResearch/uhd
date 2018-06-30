//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_TYPES_IO_TYPE_HPP
#define INCLUDED_UHD_TYPES_IO_TYPE_HPP

#include <uhd/config.hpp>

namespace uhd{

    /*!
     * The DEPRECATED Input/Output configuration struct:
     * Used to specify the IO type with device send/recv.
     *
     * Deprecated in favor of streamer interface.
     * Its still in this file for the sake of gr-uhd swig.
     */
    class UHD_API io_type_t{
    public:

        /*!
         * Built in IO types known to the system.
         */
        enum tid_t{
            //! Custom type (technically unsupported by implementation)
            CUSTOM_TYPE =     int('?'),
            //! Complex floating point (64-bit floats) range [-1.0, +1.0]
            COMPLEX_FLOAT64 = int('d'),
            //! Complex floating point (32-bit floats) range [-1.0, +1.0]
            COMPLEX_FLOAT32 = int('f'),
            //! Complex signed integer (16-bit integers) range [-32768, +32767]
            COMPLEX_INT16 =   int('s'),
            //! Complex signed integer (8-bit integers) range [-128, 127]
            COMPLEX_INT8 =    int('b')
        };

        /*!
         * The size of this io type in bytes.
         */
        const size_t size;

        /*!
         * The type id of this io type.
         * Good for using with switch statements.
         */
        const tid_t tid;

        /*!
         * Create an io type from a built-in type id.
         * \param tid a type id known to the system
         */
        io_type_t(tid_t tid);

        /*!
         * Create an io type from attributes.
         * The tid will be set to custom.
         * \param size the size in bytes
         */
        io_type_t(size_t size);

    };

} //namespace uhd

#endif /* INCLUDED_UHD_TYPES_IO_TYPE_HPP */
