//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/exception.hpp>
#include <uhdlib/rfnoc/epid_allocator.hpp>

using namespace uhd;
using namespace uhd::rfnoc;

epid_allocator::epid_allocator(sep_id_t start_epid) : _next_epid(start_epid) {}


sep_id_t epid_allocator::allocate_epid(const sep_addr_t& addr)
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_epid_map.count(addr) == 0) {
        sep_id_t new_epid   = _next_epid++;
        _epid_map[addr]     = new_epid;
        _addr_map[new_epid] = addr;
        return new_epid;
    } else {
        return _epid_map.at(addr);
    }
}

sep_addr_t epid_allocator::lookup_epid(const sep_id_t& epid) const
{
    std::lock_guard<std::mutex> lock(_mutex);

    if (_addr_map.count(epid) > 0) {
        return _addr_map.at(epid);
    } else {
        throw uhd::lookup_error("The specified EPID has not been allocated");
    }
}

void epid_allocator::deallocate_epid(sep_id_t)
{
    std::lock_guard<std::mutex> lock(_mutex);
    // TODO: Nothing to do for deallocate.
    //      With the current counter approach we assume that we will not run out of EPIDs
}
