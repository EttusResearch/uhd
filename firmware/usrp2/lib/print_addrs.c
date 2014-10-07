/* -*- c++ -*- */
/*
 * Copyright 2007 Free Software Foundation, Inc.
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
#include "nonstdio.h"

void
print_mac_addr(const void *addr)
{
  uint8_t *p = (uint8_t *)addr;
  for(size_t i = 0; i < 6; i++){
    if(i) putchar(':');
    puthex8(p[i]);
  }
}

void print_ip_addr(const void *addr){
    uint8_t *p = (uint8_t *)addr;
    printf("%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
}
