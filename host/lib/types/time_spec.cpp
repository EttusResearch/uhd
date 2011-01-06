//
// Copyright 2010-2011 Ettus Research LLC
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

#include <uhd/types/time_spec.hpp>
#include <boost/math/special_functions/round.hpp>

using namespace uhd;
time_spec_t::time_spec_t(double secs):
    _full_secs(0),
    _frac_secs(secs)
{
    /* NOP */
}

time_spec_t::time_spec_t(time_t full_secs, double frac_secs):
    _full_secs(full_secs),
    _frac_secs(frac_secs)
{
    /* NOP */
}

time_spec_t::time_spec_t(time_t full_secs, long tick_count, double tick_rate):
    _full_secs(full_secs),
    _frac_secs(double(tick_count)/tick_rate)
{
    /* NOP */
}

long time_spec_t::get_tick_count(double tick_rate) const{
    return boost::math::iround(this->get_frac_secs()*tick_rate);
}

double time_spec_t::get_real_secs(void) const{
    return this->_full_secs + this->_frac_secs;
}

time_t time_spec_t::get_full_secs(void) const{
    double intpart;
    std::modf(this->_frac_secs, &intpart);
    return this->_full_secs + time_t(intpart);
}

double time_spec_t::get_frac_secs(void) const{
    return std::fmod(this->_frac_secs, 1.0);
}

time_spec_t &time_spec_t::operator+=(const time_spec_t &rhs){
    this->_full_secs += rhs.get_full_secs();
    this->_frac_secs += rhs.get_frac_secs();
    return *this;
}

time_spec_t &time_spec_t::operator-=(const time_spec_t &rhs){
    this->_full_secs -= rhs.get_full_secs();
    this->_frac_secs -= rhs.get_frac_secs();
    return *this;
}

bool uhd::operator==(const time_spec_t &lhs, const time_spec_t &rhs){
    return
        lhs.get_full_secs() == rhs.get_full_secs() and
        lhs.get_frac_secs() == rhs.get_frac_secs()
    ;
}

bool uhd::operator<(const time_spec_t &lhs, const time_spec_t &rhs){
    return (
        (lhs.get_full_secs() < rhs.get_full_secs()) or (
        (lhs.get_full_secs() == rhs.get_full_secs()) and
        (lhs.get_frac_secs() < rhs.get_frac_secs())
    ));
}
