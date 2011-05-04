//
// Copyright 2011 Ettus Research LLC
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

#include <uhd/log.hpp>
#include <uhd/utils/static.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#ifdef BOOST_MSVC
#define USE_GET_TEMP_PATH
#include <Windows.h> //GetTempPath
#endif
#include <stdio.h> //P_tmpdir
#include <cstdlib> //getenv
#include <fstream>

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;

/***********************************************************************
 * Helper function to get the system's temporary path
 **********************************************************************/
static fs::path get_temp_path(void){
    const char *tmp_path = NULL;

    //try the official uhd temp path environment variable
    tmp_path = std::getenv("UHD_TEMP_PATH");
    if (tmp_path != NULL) return tmp_path;

    //try the windows function if available
    #ifdef USE_GET_TEMP_PATH
    LPTSTR lpBuffer[2048];
    if (GetTempPath(sizeof(lpBuffer)/sizeof(*lpBuffer), lpBuffer)) return lpBuffer;
    #endif

    //try windows environment variables
    tmp_path = std::getenv("TMP");
    if (tmp_path != NULL) return tmp_path;

    tmp_path = std::getenv("TEMP");
    if (tmp_path != NULL) return tmp_path;

    //try the stdio define if available
    #ifdef P_tmpdir
        return P_tmpdir;
    #endif

    //try unix environment variables
    tmp_path = std::getenv("TMPDIR");
    if (tmp_path != NULL) return tmp_path;

    //give up and use the unix default
    return "/tmp";
}

/***********************************************************************
 * The library's streamer resource (static initialization)
 **********************************************************************/
class uhd_logger_stream_resource_class{
public:
    uhd_logger_stream_resource_class(void){
        const std::string log_path = (get_temp_path() / "uhd.log").string();
        _streamer.open(log_path.c_str(), std::fstream::out | std::fstream::app);
    }

    ~uhd_logger_stream_resource_class(void){
        _streamer.close();
    }

    std::ostream &get(void){
        return _streamer;
    }

    void aquire(bool lock){
        if (lock) _mutex.lock();
        else _mutex.unlock();
    }

private:
    std::ofstream _streamer;
    boost::mutex _mutex;
};

UHD_SINGLETON_FCN(uhd_logger_stream_resource_class, uhd_logger_stream_resource);

/***********************************************************************
 * The logger function implementation
 **********************************************************************/
uhd::_log::log::log(
    const verbosity_t verbosity,
    const std::string &file,
    const unsigned int line,
    const std::string &function
){
    uhd_logger_stream_resource().aquire(true);
    const std::string time = pt::to_simple_string(pt::microsec_clock::local_time());
    const std::string header = str(boost::format(
        "-- %s - lvl %d - %s @ %s:%u"
    ) % time % int(verbosity) % function % fs::path(file).leaf() % line);
    uhd_logger_stream_resource().get()
        << std::endl
        << std::string(header.size(), '-') << std::endl
        << header << std::endl
        << std::string(header.size(), '-') << std::endl
    ;
}

uhd::_log::log::~log(void){
    uhd_logger_stream_resource().aquire(false);
}

std::ostream & uhd::_log::log::get(void){
    return uhd_logger_stream_resource().get();
}

UHD_STATIC_BLOCK(logger_begin){
    UHD_LOG << "Logger has started" << std::endl;
}
