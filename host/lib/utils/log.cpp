//
// Copyright 2012 Ettus Research LLC
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
#include <uhd/utils/msg.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/paths.hpp>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#ifdef BOOST_MSVC
//whoops! https://svn.boost.org/trac/boost/ticket/5287
//enjoy this useless dummy class instead
namespace boost{ namespace interprocess{
    struct file_lock{
        file_lock(const char * = NULL){}
        void lock(void){}
        void unlock(void){}
    };
}} //namespace
#else
#include <boost/interprocess/sync/file_lock.hpp>
#endif
#include <fstream>
#include <sstream>
#include <cctype>

namespace fs = boost::filesystem;
namespace pt = boost::posix_time;
namespace ip = boost::interprocess;

/***********************************************************************
 * Global resources for the logger
 **********************************************************************/
class log_resource_type{
public:
    uhd::_log::verbosity_t level;

    log_resource_type(void){

        //file lock pointer must be null
        _file_lock = NULL;

        //set the default log level
        level = uhd::_log::never;

        //allow override from macro definition
        #ifdef UHD_LOG_LEVEL
        _set_log_level(BOOST_STRINGIZE(UHD_LOG_LEVEL));
        #endif

        //allow override from environment variable
        const char * log_level_env = std::getenv("UHD_LOG_LEVEL");
        if (log_level_env != NULL) _set_log_level(log_level_env);
    }

    ~log_resource_type(void){
        boost::mutex::scoped_lock lock(_mutex);
        _file_stream.close();
        if (_file_lock != NULL) delete _file_lock;
    }

    void log_to_file(const std::string &log_msg){
        boost::mutex::scoped_lock lock(_mutex);
        if (_file_lock == NULL){
            const std::string log_path = (fs::path(uhd::get_tmp_path()) / "uhd.log").string();
            _file_stream.open(log_path.c_str(), std::fstream::out | std::fstream::app);
            _file_lock = new ip::file_lock(log_path.c_str());
        }
        _file_lock->lock();
        _file_stream << log_msg << std::flush;
        _file_lock->unlock();
    }

private:
    //! set the log level from a string that is either a digit or an enum name
    void _set_log_level(const std::string &log_level_str){
        const uhd::_log::verbosity_t log_level_num = uhd::_log::verbosity_t(log_level_str[0]-'0');
        if (std::isdigit(log_level_str[0]) and log_level_num >= uhd::_log::always and log_level_num <= uhd::_log::never){
            this->level = log_level_num;
            return;
        }
        #define if_lls_equal(name) else if(log_level_str == #name) this->level = uhd::_log::name
        if_lls_equal(always);
        if_lls_equal(often);
        if_lls_equal(regularly);
        if_lls_equal(rarely);
        if_lls_equal(very_rarely);
        if_lls_equal(never);
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
static std::string get_rel_file_path(const fs::path &file){
    fs::path abs_path = file.branch_path();
    fs::path rel_path = file.leaf();
    while (not abs_path.empty() and abs_path.leaf() != "host"){
        rel_path = abs_path.leaf() / rel_path;
        abs_path = abs_path.branch_path();
    }
    return rel_path.string();
}

struct uhd::_log::log::impl{
    std::ostringstream ss;
    verbosity_t verbosity;
};

uhd::_log::log::log(
    const verbosity_t verbosity,
    const std::string &file,
    const unsigned int line,
    const std::string &function
){
    _impl = UHD_PIMPL_MAKE(impl, ());
    _impl->verbosity = verbosity;
    const std::string time = pt::to_simple_string(pt::microsec_clock::local_time());
    const std::string header1 = str(boost::format("-- %s - level %d") % time % int(verbosity));
    const std::string header2 = str(boost::format("-- %s") % function).substr(0, 80);
    const std::string header3 = str(boost::format("-- %s:%u") % get_rel_file_path(file) % line);
    const std::string border = std::string(std::max(std::max(header1.size(), header2.size()), header3.size()), '-');
    _impl->ss
        << std::endl
        << border << std::endl
        << header1 << std::endl
        << header2 << std::endl
        << header3 << std::endl
        << border << std::endl
    ;
}

uhd::_log::log::~log(void){
    if (_impl->verbosity < log_rs().level) return;
    _impl->ss << std::endl;
    try{
        log_rs().log_to_file(_impl->ss.str());
    }
    catch(const std::exception &e){
        /*!
         * Critical behavior below.
         * The following steps must happen in order to avoid a lock-up condition.
         * This is because the message facility will call into the logging facility.
         * Therefore we must disable the logger (level = never) before messaging.
         */
        log_rs().level = never;
        UHD_MSG(error)
            << "Logging failed: " << e.what() << std::endl
            << "Logging has been disabled for this process" << std::endl
        ;
    }
}

std::ostream & uhd::_log::log::operator()(void){
    return _impl->ss;
}
