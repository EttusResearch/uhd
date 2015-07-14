/* USRP E310 Firmware Misc utility macros
 * Copyright (C) 2014 Ettus Research
 * This file is part of the USRP E310 Firmware
 * The USRP E310 Firmware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 * The USRP E310 Firmware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with the USRP E310 Firmware. If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * \file utils.h
 * \brief Misc utility macros
 */

#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

/**
 * \brief Returns the size of a (static) array
 *
 * \param x Array
 * \warning Do NOT use on pointers
 */
#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))


/**
 * \brief A ISO compliant version of the linux kernel's container_of macro.
 *        This allows implementing multiple interfaces in a struct.
 */
#ifdef __GNUC__
#define member_type(type, member) __typeof__ (((type *)0)->member)
#else
#define member_type(type, member) const void
#endif

#define container_of(ptr, type, member) ((type *)( \
    (char *)(member_type(type, member) *){ ptr } - offsetof(type, member)))

/**
 * \brief Convenience macro to make bitmasks more readable
 */
#define BIT(bit) (1 << (bit))

#endif /* UTILS_H */
