//
// Copyright 2010-2012,2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/utils/platform.hpp>
#include <uhd/config.hpp>
#include <boost/functional/hash.hpp>
#ifdef UHD_PLATFORM_WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace uhd {

    int32_t get_process_id() {
#ifdef UHD_PLATFORM_WIN32
    return int32_t(GetCurrentProcessId());
#else
    return int32_t(getpid());
#endif
    }

    uint32_t get_host_id() {
#ifdef UHD_PLATFORM_WIN32
        //extract volume serial number
        char szVolName[MAX_PATH+1], szFileSysName[MAX_PATH+1];
        DWORD dwSerialNumber, dwMaxComponentLen, dwFileSysFlags;
        GetVolumeInformation("C:\\", szVolName, MAX_PATH,
            &dwSerialNumber, &dwMaxComponentLen,
            &dwFileSysFlags, szFileSysName, sizeof(szFileSysName));

        return uint32_t(dwSerialNumber);
#else
        return uint32_t(gethostid());
#endif
    }

    uint32_t get_process_hash() {
        size_t hash = 0;
        boost::hash_combine(hash, uhd::get_process_id());
        boost::hash_combine(hash, uhd::get_host_id());
        return uint32_t(hash);
    }
}
