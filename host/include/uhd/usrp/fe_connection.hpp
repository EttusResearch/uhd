//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_USRP_FE_CONNECTION_HPP
#define INCLUDED_UHD_USRP_FE_CONNECTION_HPP

#include <uhd/config.hpp>
#include <boost/operators.hpp>
#include <string>

namespace uhd { namespace usrp {

    class UHD_API fe_connection_t : boost::equality_comparable<fe_connection_t> {
    public:
        /** Sampling mode.
         *  Represents the sampling architecture for the front-end
         */
        enum sampling_t {
            QUADRATURE, /**< Complex sampling (Complex input, Complex output). */
            HETERODYNE, /**< Heterodyne sampling (Real input, Complex output). Only one of the I and Q inputs is used. */
            REAL        /**< Real sampling (Real input, Real output). Only one of the I and Q inputs is used. */
        };

        /*!
         * Create a frontend connection class from individual settings.
         * \param sampling_mode can be { QUADRATURE, HETERODYNE, REAL }
         * \param iq_swapped indicates if the IQ channels are swapped (after inverion and heterodyne correction)
         * \param i_inverted indicates if the I channel is inverted (negated)
         * \param q_inverted indicates if the Q channel is inverted (negated)
         * \param if_freq the baseband sampling frequency.
         */
        fe_connection_t(
            sampling_t sampling_mode, bool iq_swapped,
            bool i_inverted, bool q_inverted, double if_freq = 0.0
        );

        /*!
         * Create a frontend connection class from a connection string
         * The connection string can be:
         * - in {I, Q}: Real mode sampling with no inversion.
         * - in {Ib, Qb}: Real mode sampling with inversion.
         * - in {IQ, QI}: Quadrature sampling with no inversion.
         * - in {IbQb, QbIb}: Quadrature sampling with inversion.
         * - in {II, QQ}: Heterodyne sampling with no inversion.
         * - in {IbIb, QbQb}: Heterodyne sampling with inversion.
         *
         * \param conn_str the connection string.
         * \param if_freq the baseband sampling frequency.
         */
        fe_connection_t(const std::string& conn_str, double if_freq = 0.0);

        /*!
         * Accessor for sampling mode
         */
        inline sampling_t get_sampling_mode() const {
            return _sampling_mode;
        }

        /*!
         * Accessor for IQ swap parameter
         */
        inline bool is_iq_swapped() const {
            return _iq_swapped;
        }

        /*!
         * Accessor for I inversion parameter
         */
        inline bool is_i_inverted() const {
            return _i_inverted;
        }

        /*!
         * Accessor for Q inversion parameter
         */
        inline bool is_q_inverted() const {
            return _q_inverted;
        }

        /*!
         * Accessor for IF frequency
         */
        inline double get_if_freq() const {
            return _if_freq;
        }

        /*!
         * Mutator for IF frequency
         */
        inline void set_if_freq(double freq) {
            _if_freq = freq;
        }

    private:
        sampling_t  _sampling_mode;
        bool        _iq_swapped;
        bool        _i_inverted;
        bool        _q_inverted;
        double      _if_freq;
    };

    /*!
     * Comparator operator overloaded for fe_connection_t.
     * The boost::equality_comparable provides the !=.
     * \param lhs the fe_connection_t to the left of the operator
     * \param rhs the fe_connection_t to the right of the operator
     * \return true when the fe connections are equal
     */
    UHD_API bool operator==(const fe_connection_t &lhs, const fe_connection_t &rhs);

}} //namespace

#endif /* INCLUDED_UHD_USRP_FE_CONNECTION_HPP */
