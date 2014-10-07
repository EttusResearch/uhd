/* -*- c -*- */
/*
 * Copyright 2009-2011 Ettus Research LLC
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
#ifndef INCLUDED_BANAL_H
#define INCLUDED_BANAL_H

#include <stdint.h>

#define dimof(x) (sizeof(x)/sizeof(x[0]))

//-------------- unsigned get_int 8, 16, 32, 64 --------------//

static inline uint8_t
get_uint8(const unsigned char *s)
{
  return s[0];
}

static inline uint16_t
get_uint16(const unsigned char *s)
{
  return (s[0] << 8) | s[1];
}

uint32_t
get_uint32(const unsigned char *s);

uint64_t
get_uint64(const unsigned char *s);

//--------------- signed get_int 8, 16, 32, 64 --------------//

static inline int8_t
get_int8(const unsigned char *s)
{
  return get_uint8(s);
}

static inline int16_t
get_int16(const unsigned char *s)
{
  return get_uint16(s);
}

static inline int32_t
get_int32(const unsigned char *s)
{
  return get_uint32(s);
}

static inline int64_t
get_int64(const unsigned char *s)
{
  return get_uint64(s);
}

#endif /* INCLUDED_BANAL_H */
