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

#include <stddef.h>
#include <stdint.h>

/*
 * max_olen must be at least 8 bytes.  1KB is recommended.
 */
void
loader_parser(const unsigned char *input, size_t ilen,
	      unsigned char *output, size_t max_olen, size_t *actual_olen);

/*
 * Major kludge-master altert!
 * This function registers functions for setting caldiv eeprom stuff.
 * This way, the parser does not depend on the qpn apps at compile time.
 */
typedef void(*caldiv_eeprom_setter_t)(uint32_t);
void register_caldiv_eeprom_setters(
	caldiv_eeprom_setter_t set_rev,
	caldiv_eeprom_setter_t set_ser,
	caldiv_eeprom_setter_t set_mod);
