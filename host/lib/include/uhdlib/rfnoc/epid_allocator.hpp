//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_LIBUHD_EPID_ALLOCATOR_HPP
#define INCLUDED_LIBUHD_EPID_ALLOCATOR_HPP

#include <uhdlib/rfnoc/rfnoc_common.hpp>
#include <map>
#include <memory>
#include <mutex>

namespace uhd { namespace rfnoc {

/*! A class that is responsible for allocating and keeping track of endpoint
 * IDs. There shall be one instance of this class per rfnoc_graph
 */
class epid_allocator
{
public:
    using sptr = std::shared_ptr<epid_allocator>;

    epid_allocator(sep_id_t start_epid = 1);
    epid_allocator(const epid_allocator& rhs) = delete;
    epid_allocator(epid_allocator&& rhs)      = delete;

    /*! \brief Allocate an EPID for the specified endpoint.
     *
     * \param addr The physical address (device, instance) of the stream endpoint
     * \return The allocated EPID
     */
    sep_id_t allocate_epid(const sep_addr_t& addr);

    /*! \brief Get a pre-allocated EPID. Throws an exception is not allocated
     *
     * \param addr The physical address (device, instance) of the stream endpoint
     * \return The allocated EPID
     */
    sep_id_t get_epid(const sep_addr_t& addr);

    /*! \brief Lookup an EPID and return the address associated with it.
     *
     * \param epid The allocated EPID
     * \return The physical address (device, instance) of the stream endpoint
     */
    sep_addr_t lookup_addr(const sep_id_t& epid) const;

    /*! \brief Deallocate the specified EPID.
     *
     * \param epid The EPID to deallocate
     */
    void deallocate_epid(sep_id_t epid);

private:
    std::map<sep_addr_t, sep_id_t> _epid_map;
    std::map<sep_id_t, sep_addr_t> _addr_map;
    sep_id_t _next_epid;
    mutable std::mutex _mutex;
};

}} /* namespace uhd::rfnoc */

#endif /* INCLUDED_LIBUHD_EPID_ALLOCATOR_HPP */
