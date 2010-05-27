//
// Copyright 2010 Ettus Research LLC
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

#ifndef INCLUDED_CODEC_CTRL_HPP
#define INCLUDED_CODEC_CTRL_HPP

#include "usrp2_iface.hpp"
#include <boost/shared_ptr.hpp>
#include <boost/utility.hpp>

class codec_ctrl : boost::noncopyable{
public:
    typedef boost::shared_ptr<codec_ctrl> sptr;

    /*!
     * Make a codec control for the DAC and ADC.
     * \param _iface a pointer to the usrp2 interface object
     * \return a new codec control object
     */
    static sptr make(usrp2_iface::sptr iface);

};

#endif /* INCLUDED_CODEC_CTRL_HPP */
