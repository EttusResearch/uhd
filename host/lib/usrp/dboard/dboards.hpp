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

#ifndef INCLUDED_LOCAL_DBOARDS_HPP
#define INCLUDED_LOCAL_DBOARDS_HPP

#include <uhd/usrp/dboard_base.hpp>

using namespace uhd::usrp;

/***********************************************************************
 * The basic boards:
 **********************************************************************/
class basic_rx : public rx_dboard_base{
public:
    static dboard_base::sptr make(ctor_args_t const& args){
        return dboard_base::sptr(new basic_rx(args));
    }
    basic_rx(ctor_args_t const& args);
    ~basic_rx(void);

    void rx_get(const wax::obj &key, wax::obj &val);
    void rx_set(const wax::obj &key, const wax::obj &val);
};

class basic_tx : public tx_dboard_base{
public:
    static dboard_base::sptr make(ctor_args_t const& args){
        return dboard_base::sptr(new basic_tx(args));
    }
    basic_tx(ctor_args_t const& args);
    ~basic_tx(void);

    void tx_get(const wax::obj &key, wax::obj &val);
    void tx_set(const wax::obj &key, const wax::obj &val);

};

#endif /* INCLUDED_LOCAL_DBOARDS_HPP */
