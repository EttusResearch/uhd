//
// Copyright 2015 Ettus Research LLC
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

#ifndef INCLUDED_GPSD_IFACE_HPP
#define INCLUDED_GPSD_IFACE_HPP

#include <boost/cstdint.hpp>
#include <boost/shared_ptr.hpp>

#include <uhd/usrp/gps_ctrl.hpp>

namespace uhd { namespace usrp {

class gpsd_iface : public virtual uhd::gps_ctrl {
public:
    typedef boost::shared_ptr<gpsd_iface> sptr;
    static sptr make(const std::string &addr, boost::uint16_t port);
};

}};

#endif /* INCLUDED_GPSD_IFACE_HPP */
