//
// Copyright 2010-2012,2014 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

    boost::int32_t get_process_id() {
#ifdef UHD_PLATFORM_WIN32
    return boost::int32_t(GetCurrentProcessId());
#else
    return boost::int32_t(getpid());
#endif
    }

    boost::uint32_t get_host_id() {
#ifdef UHD_PLATFORM_WIN32
        //extract volume serial number
        char szVolName[MAX_PATH+1], szFileSysName[MAX_PATH+1];
        DWORD dwSerialNumber, dwMaxComponentLen, dwFileSysFlags;
        GetVolumeInformation("C:\\", szVolName, MAX_PATH,
            &dwSerialNumber, &dwMaxComponentLen,
            &dwFileSysFlags, szFileSysName, sizeof(szFileSysName));

        return boost::uint32_t(dwSerialNumber);
#else
        return boost::uint32_t(gethostid());
#endif
    }

    boost::uint32_t get_process_hash() {
        size_t hash = 0;
        boost::hash_combine(hash, uhd::get_process_id());
        boost::hash_combine(hash, uhd::get_host_id());
        return boost::uint32_t(hash);
    }
}
