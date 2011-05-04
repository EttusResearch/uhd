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

#include <uhd/utils/log.hpp>
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
#include <cctype>

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
    char lpBuffer[2048];
    if (GetTempPath(sizeof(lpBuffer), lpBuffer)) return lpBuffer;
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
class null_streambuf_class : public std::streambuf{
    int overflow(int c) { return c; }
};
UHD_SINGLETON_FCN(null_streambuf_class, null_streambuf);

class uhd_logger_stream_resource_class{
public:
    uhd_logger_stream_resource_class(void) : _null_stream(&null_streambuf()){
        const std::string log_path = (get_temp_path() / "uhd.log").string();
        _file_stream.open(log_path.c_str(), std::fstream::out | std::fstream::app);

        //set the default log level
        _log_level = uhd::_log::regularly;

        //allow override from macro definition
        #ifdef UHD_LOG_LEVEL
        _set_log_level(BOOST_STRINGIZE(UHD_LOG_LEVEL));
        #endif

        //allow override from environment variable
        const char * log_level_env = std::getenv("UHD_LOG_LEVEL");
        if (log_level_env != NULL) _set_log_level(log_level_env);

    }

    ~uhd_logger_stream_resource_class(void){
        _file_stream.close();
    }

    std::ostream &get(void){
        if (_verbosity >= _log_level) return _file_stream;
        return _null_stream;
    }

    void aquire(bool lock){
        if (lock) _mutex.lock();
        else _mutex.unlock();
    }

    void set_verbosity(uhd::_log::verbosity_t verbosity){
        _verbosity = verbosity;
    }

private:
    //! set the log level from a string that is either a digit or an enum name
    void _set_log_level(const std::string &log_level_str){
        const uhd::_log::verbosity_t log_level = uhd::_log::verbosity_t(log_level_str[0]-'0');
        if (std::isdigit(log_level_str[0]) and log_level >= uhd::_log::always and log_level <= uhd::_log::very_rarely){
            _log_level = log_level;
        }
        #define if_lls_equal(name) else if(log_level_str == #name) _log_level = uhd::_log::name
        if_lls_equal(always);
        if_lls_equal(often);
        if_lls_equal(regularly);
        if_lls_equal(rarely);
        if_lls_equal(very_rarely);
    }

    std::ofstream _file_stream;
    std::ostream _null_stream;
    boost::mutex _mutex;
    uhd::_log::verbosity_t _verbosity;
    uhd::_log::verbosity_t _log_level;
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
    uhd_logger_stream_resource().set_verbosity(verbosity);
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
