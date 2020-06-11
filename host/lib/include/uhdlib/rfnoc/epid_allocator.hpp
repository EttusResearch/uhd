//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/rfnoc/rfnoc_types.hpp>
#include <uhdlib/rfnoc/mgmt_portal.hpp>
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
     * Does not initialize the specified endpoint (ideal for SW endpoints).
     *
     * \param addr The physical address (device, instance) of the stream endpoint
     * \return The allocated EPID
     */
    sep_id_t allocate_epid(const sep_addr_t& addr);

    /*! \brief Allocate an EPID for the specified endpoint.
     * Also initialize the specified endpoint.
     *
     * \param addr The physical address (device, instance) of the stream endpoint
     * \param mgmt_portal The management portal to use for initializing the SEP/EPID
     * \param chdr_ctrl_xport The ctrl xport to use for initializing the SEP/EPID
     * \return The allocated EPID
     */
    sep_id_t allocate_epid(
        const sep_addr_t& addr, mgmt::mgmt_portal& mgmt_portal, chdr_ctrl_xport& xport);

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
