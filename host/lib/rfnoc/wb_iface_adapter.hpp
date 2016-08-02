//
// Copyright 2016 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef INCLUDED_RFNOC_WB_IFACE_ADAPTER_HPP
#define INCLUDED_RFNOC_WB_IFACE_ADAPTER_HPP

#include <uhd/config.hpp>
#include <uhd/types/wb_iface.hpp>
#include <boost/function.hpp>

namespace uhd {
    namespace rfnoc {

class UHD_API wb_iface_adapter : public uhd::timed_wb_iface
{
public:
    typedef boost::shared_ptr<wb_iface_adapter> sptr;
    typedef boost::function<void(wb_addr_type, boost::uint32_t)> poke32_type;
    typedef boost::function<boost::uint32_t(wb_addr_type)> peek32_type;
    typedef boost::function<boost::uint64_t(wb_addr_type)> peek64_type;
    typedef boost::function<time_spec_t(void)> gettime_type;
    typedef boost::function<void(const time_spec_t&)> settime_type;

    wb_iface_adapter(
        const poke32_type &,
        const peek32_type &,
        const peek64_type &,
        const gettime_type &,
        const settime_type &
    );

    wb_iface_adapter(
        const poke32_type &,
        const peek32_type &,
        const peek64_type &
    );

    virtual ~wb_iface_adapter(void) {};

    virtual void poke32(const wb_addr_type addr, const boost::uint32_t data);
    virtual boost::uint32_t peek32(const wb_addr_type addr);
    virtual boost::uint64_t peek64(const wb_addr_type addr);
    virtual time_spec_t get_time(void);
    virtual void set_time(const time_spec_t& t);

private:
    const poke32_type   poke32_functor;
    const peek32_type   peek32_functor;
    const peek64_type   peek64_functor;
    const gettime_type  gettime_functor;
    const settime_type  settime_functor;
};

}} // namespace uhd::rfnoc

#endif /* INCLUDED_RFNOC_WB_IFACE_ADAPTER_HPP */
