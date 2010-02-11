/* -*- c++ -*- */
/*
 * Copyright 2009 Ettus Research LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <banal.h>

uint32_t
get_uint32(const unsigned char *s)
{
  return (s[0] << 24) | (s[1] << 16) | (s[2] << 8) | s[3];
}

uint64_t
get_uint64(const unsigned char *s)
{
  return (((uint64_t)get_uint32(s)) << 32) | get_uint32(s+4);
}

uint32_t
divide_uint64(uint64_t dividend, uint32_t divisor)
{
    uint32_t result = 0;
    uint64_t dividend_ = 0;
    for(int i = 31; i >= 0; i--){
        //approximate the divisor with the ith result bit set
        uint64_t tmp = dividend_;
        tmp += (uint64_t)divisor << i;
        //set the ith result bit if the approximation is less
        if (tmp <= dividend){
            dividend_ = tmp;
            result |= 1 << i;
        }
    }
    return result;
}
