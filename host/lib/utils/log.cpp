//
// Copyright 2012,2014,2016 Ettus Research LLC
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
#include <uhd/utils/paths.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread/locks.hpp>
#include <boost/interprocess/sync/file_lock.hpp>
#include <fstream>
#include <cctype>

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace ip = boost::interprocess;

/***********************************************************************
 * Global resources for the logger
 **********************************************************************/
class log_resource_type{
public:
    uhd::log::severity_level level;
    uhd::log::severity_level file_level;
    uhd::log::severity_level console_level;

    log_resource_type(void): level(uhd::log::info), file_level(uhd::log::info), console_level(uhd::log::info){

        //file lock pointer must be null
        _file_lock = NULL;

        //default to no file logging
        this->file_logging = false;

        //set the default log level
        this->level = uhd::log::off;
        this->file_level = uhd::log::off;
        this->console_level = uhd::log::off;

        //allow override from macro definition
#ifdef UHD_LOG_MIN_LEVEL
        this->level = _get_log_level(BOOST_STRINGIZE(UHD_LOG_MIN_LEVEL), this->level);
#endif
#if defined(UHD_LOG_FILE_LEVEL) && defined(UHD_LOG_FILE_PATH)
        this->file_level = _get_log_level(BOOST_STRINGIZE(UHD_LOG_FILE_LEVEL), this->file_level);
#endif
#ifdef UHD_LOG_CONSOLE_LEVEL
        this->console_level = _get_log_level(BOOST_STRINGIZE(UHD_LOG_CONSOLE_LEVEL), this->console_level);
#endif
#ifdef UHD_LOG_FILE
        this->log_file_target = BOOST_STRINGIZE(UHD_LOG_FILE);
        this->file_logging = true;
#endif

        //allow override from environment variables
        const char * log_level_env = std::getenv("UHD_LOG_LEVEL");
        if (log_level_env != NULL && log_level_env[0] != '\0') this->level = _get_log_level(log_level_env, this->level);

        const char * log_file_level_env = std::getenv("UHD_LOG_FILE_LEVEL");
        if (log_file_level_env != NULL && log_file_level_env[0] != '\0') this->file_level = _get_log_level(log_file_level_env, this->file_level);

        const char * log_console_level_env = std::getenv("UHD_LOG_CONSOLE_LEVEL");
        if (log_console_level_env != NULL && log_console_level_env[0] != '\0') this->console_level = _get_log_level(log_console_level_env, this->console_level);

        const char* log_file_env = std::getenv("UHD_LOG_FILE");
        if ((log_file_env != NULL) && (log_file_env[0] != '\0')) {
            this->log_file_target = log_file_env;
            this->file_logging = true;
        }
    }

    ~log_resource_type(void){
        if (this->file_logging){
            boost::lock_guard<boost::mutex> lock(_mutex);
            _file_stream.close();
            if (_file_lock != NULL) delete _file_lock;
        }
    }

    void log_to_file(const std::string &log_msg){
        if ( this->file_logging ){
            boost::lock_guard<boost::mutex> lock(_mutex);
            if (_file_lock == NULL){
                _file_stream.open(this->log_file_target.c_str(), std::fstream::out | std::fstream::app);
                _file_lock = new ip::file_lock(this->log_file_target.c_str());
            }
            _file_lock->lock();
            _file_stream << log_msg << std::flush;
            _file_lock->unlock();
        }
    }

private:
    //! set the log level from a string that is either a digit or an enum name
    bool file_logging;
    std::string log_file_target;
    uhd::log::severity_level _get_log_level(const std::string &log_level_str,
                                            const uhd::log::severity_level &previous_level){
        if (std::isdigit(log_level_str[0])) {
            const uhd::log::severity_level log_level_num =
                uhd::log::severity_level(std::stoi(log_level_str));
            if (log_level_num >= uhd::log::trace and
                log_level_num <= uhd::log::fatal) {
                return log_level_num;
            }else{
                return previous_level;
            }
        }

#define if_loglevel_equal(name)                                 \
        else if (log_level_str == #name) return uhd::log::name
        if_loglevel_equal(trace);
        if_loglevel_equal(debug);
        if_loglevel_equal(info);
        if_loglevel_equal(warning);
        if_loglevel_equal(error);
        if_loglevel_equal(fatal);
        if_loglevel_equal(off);
        return previous_level;
    }

    //file stream and lock:
    std::ofstream _file_stream;
    ip::file_lock *_file_lock;
    boost::mutex _mutex;
};

UHD_SINGLETON_FCN(log_resource_type, log_rs);

/***********************************************************************
 * The logger object implementation
 **********************************************************************/
//! get the relative file path from the host directory

inline std::string path_to_filename(std::string path)
{
    return path.substr(path.find_last_of("/\\") + 1);
}

uhd::_log::log::log(
    const uhd::log::severity_level verbosity,
    const std::string &file,
    const unsigned int line,
    const std::string &component,
    const boost::thread::id id
    )
{
    _log_it = (verbosity >= log_rs().level);
    _log_file =(verbosity >= log_rs().file_level);
    _log_console = (verbosity >= log_rs().console_level);
    if (_log_it)
    {
        if (_log_file){
        const std::string time = pt::to_simple_string(pt::microsec_clock::local_time());
        _file
            << time << ","
            << "0x" << id << ","
            << path_to_filename(file) << ":" << line << ","
            << verbosity << ","
            << component << ","
        ;
        }
#ifndef UHD_LOG_CONSOLE_DISABLE
        if (_log_console){
#ifdef UHD_LOG_CONSOLE_TIME
            const std::string time = pt::to_simple_string(pt::microsec_clock::local_time());
#endif
            _console
#ifdef UHD_LOG_CONSOLE_TIME
                << "[" << time << "] "
#endif
#ifdef UHD_LOG_CONSOLE_THREAD
                << "[0x" << id << "] "
#endif
#ifdef UHD_LOG_CONSOLE_SRC
                << "[" << path_to_filename(file) << ":" << line << "] "
#endif
                << "[" << verbosity << "] "
                << "[" << component << "] "
                ;
        }
#endif
    }
}

uhd::_log::log::~log(void)
{
    if (not _log_it)
        return;
#ifndef UHD_LOG_CONSOLE_DISABLE
    if ( _log_console ){
        std::clog << _console.str() << _ss.str() << std::endl;
        }
#endif
    if ( _log_file){
        _file << _ss.str() << std::endl;
        try{
            log_rs().log_to_file(_file.str());
        }
        catch(const std::exception &e){
            /*!
             * Critical behavior below.
             * The following steps must happen in order to avoid a lock-up condition.
             * This is because the message facility will call into the logging facility.
             * Therefore we must disable the logger (level = never) before messaging.
             */
            log_rs().level = uhd::log::off;
            std::cerr
                << "Logging failed: " << e.what() << std::endl
                << "Logging has been disabled for this process" << std::endl
                ;
        }
       
    }
}

void
uhd::_log::log::set_log_level(uhd::log::severity_level level){
    log_rs().level = level;
}

void
uhd::_log::log::set_console_level(uhd::log::severity_level level){
    log_rs().console_level = level;
}

void
uhd::_log::log::set_file_level(uhd::log::severity_level level){
    log_rs().file_level = level;
}
