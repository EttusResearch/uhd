//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/exception.hpp>
#include <cstdint>
#include <string>

namespace uhd { namespace rfnoc {

/*! Describes the source of a particular resource (property or action)
 */
struct res_source_info
{
    /*! Source type
     */
    enum source_t {
        USER, ///< The user API sources this resource
        INPUT_EDGE, ///< An input edge sources this resource
        OUTPUT_EDGE, ///< An input edge sources this resource
        FRAMEWORK ///< This is a special resource, only accessed by the framework
    };

    // No default ctor: The source type must be specified
    res_source_info() = delete;

    res_source_info(source_t source_type, size_t instance_ = 0)
        : type(source_type), instance(instance_)
    {
        // nop
    }

    //! The type of source (user or edge)
    source_t type;

    //! The instance of the source. For resource that is sourced by a edge, it
    // corresponds to the port number
    size_t instance = 0;

    bool operator==(const res_source_info& rhs) const
    {
        return rhs.type == type && rhs.instance == instance;
    }

    bool operator!=(const res_source_info& rhs) const
    {
        return !(*this == rhs);
    }

    //! Returns a string representation of the source
    std::string to_string() const
    {
        const std::string type_repr = type == USER          ? "USER"
                                      : type == INPUT_EDGE  ? "INPUT_EDGE"
                                      : type == OUTPUT_EDGE ? "OUTPUT_EDGE"
                                                            : "INVALID";
        return type_repr + ":" + std::to_string(instance);
    }

    /*! Convenience function to invert the type value if it's an edge.
     *
     * - Will assert that the edge is actually either INPUT_EDGE or OUTPUT_EDGE
     * - Then, returns the opposite of what it was before
     *
     * \throws uhd::assertion_error if \p edge_direction is neither INPUT_EDGE
     *         nor OUTPUT_EDGE
     */
    static source_t invert_edge(const source_t edge_direction)
    {
        UHD_ASSERT_THROW(edge_direction == INPUT_EDGE || edge_direction == OUTPUT_EDGE);
        return edge_direction == INPUT_EDGE ? OUTPUT_EDGE : INPUT_EDGE;
    }
};

}} /* namespace uhd::rfnoc */

namespace std {
template <>
struct hash<uhd::rfnoc::res_source_info>
{
    size_t operator()(const uhd::rfnoc::res_source_info& src_info) const
    {
        const size_t hash_type = std::hash<size_t>{}(src_info.type);
        const size_t hash_inst = std::hash<size_t>{}(src_info.instance);
        return hash_type ^ (hash_inst << 1);
    }
};
} // namespace std
