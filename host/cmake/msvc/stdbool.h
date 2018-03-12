//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#ifndef INCLUDED_MSC_STDBOOL_H
#define INCLUDED_MSC_STDBOOL_H

#ifndef _MSC_VER
#error "Use this header only with Microsoft Visual C++ compilers!"
#endif

#ifndef __cplusplus

#define bool int
#define true 1
#define false 0

#endif

#endif /* INCLUDED_MSC_STDBOOL_H */
