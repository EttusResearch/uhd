/*
 * valgrind helpers
 *
 * Copyright (c) 2016 National Instruments
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 */

#pragma once

#include "Common.h"

#ifdef ENABLE_VALGRIND
#    include <valgrind/memcheck.h>
#else
#    define VALGRIND_MAKE_MEM_NOACCESS(_qzz_addr, _qzz_len) \
        do {                                                \
            UNUSED(_qzz_addr);                              \
            UNUSED(_qzz_len);                               \
        } while (0)
#    define VALGRIND_MAKE_MEM_DEFINED(_qzz_addr, _qzz_len) \
        do {                                               \
            UNUSED(_qzz_addr);                             \
            UNUSED(_qzz_len);                              \
        } while (0)
#    define VALGRIND_MAKE_MEM_UNDEFINED(_qzz_addr, _qzz_len) \
        do {                                                 \
            UNUSED(_qzz_addr);                               \
            UNUSED(_qzz_len);                                \
        } while (0)
#endif
