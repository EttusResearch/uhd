//
// Copyright 2010 Ettus Research LLC
//
/*
 * Copyright 2008,2009 Free Software Foundation, Inc.
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


#include <memory_map.h>
#include <i2c.h>
#include <string.h>
#include <stdio.h>
#include <db.h>
#include <hal_io.h>
#include <nonstdio.h>


typedef enum { UDBE_OK, UDBE_NO_EEPROM, UDBE_INVALID_EEPROM } usrp_dbeeprom_status_t;

static usrp_dbeeprom_status_t
read_raw_dboard_eeprom (unsigned char *buf, int i2c_addr)
{
  if (!eeprom_read (i2c_addr, 0, buf, DB_EEPROM_CLEN))
    return UDBE_NO_EEPROM;

  if (buf[DB_EEPROM_MAGIC] != DB_EEPROM_MAGIC_VALUE)
    return UDBE_INVALID_EEPROM;

  int sum = 0;
  unsigned int i;
  for (i = 0; i < DB_EEPROM_CLEN; i++)
    sum += buf[i];

  if ((sum & 0xff) != 0)
    return UDBE_INVALID_EEPROM;

  return UDBE_OK;
}


/*
 * Return DBID, -1 <none> or -2 <invalid eeprom contents>
 */
int
read_dboard_eeprom(int i2c_addr)
{
  unsigned char buf[DB_EEPROM_CLEN];

  usrp_dbeeprom_status_t s = read_raw_dboard_eeprom (buf, i2c_addr);

  //printf("\nread_raw_dboard_eeprom: %d\n", s);

  switch (s){
  case UDBE_OK:
    return (buf[DB_EEPROM_ID_MSB] << 8) | buf[DB_EEPROM_ID_LSB];

  case UDBE_NO_EEPROM:
  default:
    return -1;

  case UDBE_INVALID_EEPROM:
    return -2;
  }
}
