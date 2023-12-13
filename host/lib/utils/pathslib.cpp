//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/config.hpp>
#include <uhdlib/utils/paths.hpp>

#ifdef BOOST_MSVC
#    include <windows.h>
#elif defined(__OpenBSD__)
#    include <glob.h>
#else
#    include <wordexp.h>
#endif

std::string uhd::path_expandvars(const std::string& path)
{
    if (path.empty()) {
        return path;
    }
#ifdef BOOST_MSVC
    constexpr size_t max_pathlen = 4096;
    char result[max_pathlen];
    const size_t result_len =
        ExpandEnvironmentStrings(path.c_str(), &result[0], max_pathlen);
    if (result == 0) {
        return path;
    }
    return std::string(result, result + result_len);
#elif defined(__OpenBSD__)
    glob_t p;
    std::string return_value;
    memset(&p, 0, sizeof(p));
    if (glob(path.c_str(), GLOB_TILDE, NULL, &p) == 0 && p.gl_pathc > 0) {
        return_value = std::string(p.gl_pathv[0]);
    } else {
        return_value = path;
    }
    globfree(&p);
    return return_value;
#else
    wordexp_t p;
    std::string return_value;
    if (wordexp(path.c_str(), &p, 0) == 0 && p.we_wordc > 0) {
        return_value = std::string(p.we_wordv[0]);
    } else {
        return_value = path;
    }
    wordfree(&p);
    return return_value;
#endif
}
