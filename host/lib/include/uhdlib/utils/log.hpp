//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <uhd/utils/log.hpp>
#include <uhdlib/utils/prefs.hpp>

#define UHD_LOG_GUIDE(message)          \
    if (uhd::prefs::is_guided_mode()) { \
        UHD_LOG_INFO("Guide", message); \
    }

#define UHD_LOG_IF_GUIDED(statement)    \
    if (uhd::prefs::is_guided_mode()) { \
        statement;                      \
    }
