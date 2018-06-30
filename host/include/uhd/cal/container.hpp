//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_UHD_CAL_CONTAINER_HPP
#define INCLUDED_UHD_CAL_CONTAINER_HPP

#include <uhd/config.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/shared_ptr.hpp>

namespace uhd {
namespace cal {

class base_container {
public:
    typedef std::map<std::string, std::string> metadata_t;
    typedef boost::shared_ptr<base_container> sptr;
};

/*!
 * An interface for creating and managing a generic calibration
 * data container.
 *
 * These containers are used to represent N dimensional data structures
 * in order to accommodate a mapping from multi-variable input to a scalar
 * value (e.g. gain, frequency, temperature -> power level [dBm]).
 *
 * The container only supports inputs of the same type to be mapped.
 *
 */
template<typename in_type, typename out_type>
class UHD_API cal_container : public base_container {
public:
    typedef std::map<in_type, out_type> container_t;

    /*!
     * Get the mapping from an input to an output
     * from the calibration container.
     *
     * \param args input values
     * \returns the output of the mapping (a scalar value)
     * \throws uhd::assertion_error if the dimensions of the input args
     *         are incorrect for this container
     */
    virtual out_type get(const in_type &args) = 0;

    /*!
     * Add a data point to the container.
     * This function records a mapping R^n -> R between an input vector
     * and output scalar.
     *
     * \param output the output of the data point mapping
     * \param args input values
     */
    virtual void add(const out_type output, const in_type &args) = 0;

    /*!
     * Associate some metadata with the container.
     *
     * \param data a map of metadata (string -> string).
     */
    virtual void add_metadata(const metadata_t &data) = 0;

    /*!
     * Retrieve metadata from the container.
     *
     * \returns map of metadata.
     */
    virtual const metadata_t &get_metadata() = 0;

public:
    typedef boost::archive::text_iarchive iarchive_type;
    typedef boost::archive::text_oarchive oarchive_type;

protected:
    friend class boost::serialization::access;

    virtual void serialize(iarchive_type & ar, const unsigned int) = 0;
    virtual void serialize(oarchive_type & ar, const unsigned int) = 0;
};

} // namespace cal
} // namespace uhd

#endif /* INCLUDED_UHD_CAL_CONTAINER_HPP */
