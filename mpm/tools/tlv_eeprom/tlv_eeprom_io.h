// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright 2019 Ettus Research, a National Instruments Brand
 */

#pragma once

#include "tlv_eeprom.h"

/**
 * tlv_eeprom_write_to_file - write an eeprom to a file
 * @e: eeprom to write
 * @path: destination to write to
 *
 * Writes the eeprom to the specified path
 *
 * Return: zero on success, else negative error code
 */
int tlv_eeprom_write_to_file(const struct tlv_eeprom *e, const char *path);

/**
 * tlv_eeprom_read_from_file - read an eeprom from file
 * @path: path to read from
 *
 * Reads from the path into an allocated eeprom object. No validation of the
 * data is performed, and caller is responsible for free'ing this.
 *
 * Return: eeprom, or NULL if error
 */
struct tlv_eeprom *tlv_eeprom_read_from_file(const char *path);

