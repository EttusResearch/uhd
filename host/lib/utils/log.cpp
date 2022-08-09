//
// Copyright 2012,2014,2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <uhd/transport/bounded_buffer.hpp>
#include <uhd/utils/log.hpp>
#include <uhd/utils/log_add.hpp>
#include <uhd/utils/paths.hpp>
#include <uhd/utils/static.hpp>
#include <uhd/utils/thread.hpp>
#include <uhd/version.hpp>
#include <uhdlib/utils/isatty.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <atomic>
#include <cctype>
#include <fstream>
#include <memory>
#include <mutex>
#include <thread>
#ifdef HAVE_DPDK
#include <uhdlib/transport/dpdk/common.hpp>
#endif

namespace pt = boost::posix_time;

// Don't make these static const std::string -- we need their lifetime guaranteed!
#define PURPLE "\033[0;35m" // purple
#define BLUE "\033[1;34m" // blue
#define GREEN "\033[0;32m" // green
#define YELLOW "\033[0;33m" // yellow
#define BYELLOW "\033[1;33m" // yellow
#define RED "\033[0;31m" // red
#define BRED "\033[1;31m" // bright red
#define RED_ON_YELLOW "\033[0;31;43m" // bright red
#define RESET_COLORS "\033[0;39m" // reset colors

/***********************************************************************
 * Helpers
 **********************************************************************/
namespace {

#ifdef BOOST_MSVC
constexpr double READ_TIMEOUT = 0.5; // Waiting time to read from the queue
#endif

constexpr char LOG_THREAD_NAME[]          = "uhd_log";
constexpr char LOG_THREAD_NAME_FP[]       = "uhd_log_fastpath";
constexpr char LOG_THREAD_NAME_FP_DUMMY[] = "uhd_log_fp_dummy";

std::string verbosity_color(const uhd::log::severity_level& level)
{
    static const bool tty = uhd::is_a_tty(2); // is stderr a tty?
    if (!tty) {
        return "";
    }
    switch (level) {
        case (uhd::log::trace):
            return PURPLE;
        case (uhd::log::debug):
            return BLUE;
        case (uhd::log::info):
            return GREEN;
        case (uhd::log::warning):
            return BYELLOW;
        case (uhd::log::error):
            return BRED;
        case (uhd::log::fatal):
            return RED_ON_YELLOW;
        default:
            return RESET_COLORS;
    }
}

std::string verbosity_name(const uhd::log::severity_level& level)
{
    switch (level) {
        case (uhd::log::trace):
            return "TRACE";
        case (uhd::log::debug):
            return "DEBUG";
        case (uhd::log::info):
            return "INFO";
        case (uhd::log::warning):
            return "WARNING";
        case (uhd::log::error):
            return "ERROR";
        case (uhd::log::fatal):
            return "FATAL";
        default:
            return "-";
    }
}

//! get the relative file path from the host directory
inline std::string path_to_filename(std::string path)
{
    return path.substr(path.find_last_of("/\\") + 1);
}

} // namespace

namespace uhd { namespace log {

boost::optional<uhd::log::severity_level> parse_log_level_from_string(
    const std::string& log_level_str)
{
    if (std::isdigit(log_level_str[0])) {
        const uhd::log::severity_level log_level_num =
            uhd::log::severity_level(std::stoi(log_level_str));
        if (log_level_num >= uhd::log::trace && log_level_num <= uhd::log::fatal) {
            return log_level_num;
        } else {
            std::cerr << "[LOG] Failed to set log level to: " << log_level_str;
            return boost::none;
        }
    }

#define if_loglevel_equal(name) else if (log_level_str == #name) return uhd::log::name
    if_loglevel_equal(trace);
    if_loglevel_equal(debug);
    if_loglevel_equal(info);
    if_loglevel_equal(warning);
    if_loglevel_equal(error);
    if_loglevel_equal(fatal);
    if_loglevel_equal(off);
    return boost::none;
}

}}

/***********************************************************************
 * Logger backends
 **********************************************************************/
void console_log(const uhd::log::logging_info& log_info)
{
    std::ostringstream log_buffer;
    log_buffer
#ifdef UHD_LOG_CONSOLE_COLOR
        << verbosity_color(log_info.verbosity)
#endif
#ifdef UHD_LOG_CONSOLE_TIME
        << "[" << pt::to_simple_string(log_info.time) << "] "
#endif
#ifdef UHD_LOG_CONSOLE_THREAD
        << "[0x" << log_info.thread_id << "] "
#endif
#ifdef UHD_LOG_CONSOLE_SRC
        << "[" << path_to_filename(log_info.file) << ":" << log_info.line << "] "
#endif
        << "[" << verbosity_name(log_info.verbosity) << "] "
        << "[" << log_info.component << "] "
#ifdef UHD_LOG_CONSOLE_COLOR
        << verbosity_color(uhd::log::off) // This will reset colors
#endif
        << log_info.message << std::endl;
    std::clog << log_buffer.str();
}

/*! Helper class to implement file logging
 *
 * The class holds references to the file stream object, and handles closing
 * and cleanup.
 */
class file_logger_backend
{
public:
    file_logger_backend(const std::string& file_path)
    {
        _file_stream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        if (!file_path.empty()) {
            try {
                _file_stream.open(
                    file_path.c_str(), std::fstream::out | std::fstream::app);
            } catch (const std::ofstream::failure& fail) {
                std::cerr << "Error opening log file: " << fail.what() << std::endl;
            }
        }
    }

    void log(const uhd::log::logging_info& log_info)
    {
        if (_file_stream.is_open()) {
            _file_stream << pt::to_simple_string(log_info.time) << ","
                         << "0x" << log_info.thread_id << ","
                         << path_to_filename(log_info.file) << ":" << log_info.line << ","
                         << log_info.verbosity << "," << log_info.component << ","
                         << log_info.message << std::endl;
            ;
        }
    }


    ~file_logger_backend()
    {
        if (_file_stream.is_open()) {
            _file_stream.close();
        }
    }

private:
    std::ofstream _file_stream;
};

/***********************************************************************
 * Global resources for the logger
 **********************************************************************/

#define UHD_CONSOLE_LOGGER_KEY "console"
#define UHD_FILE_LOGGER_KEY "file"

class log_resource
{
public:
    uhd::log::severity_level global_level;

    log_resource(void)
        : global_level(uhd::log::off)
        , _exit(false)
        ,
#ifndef UHD_LOG_FASTPATH_DISABLE
        _fastpath_queue(10)
        ,
#endif
        _log_queue(10)
    {
        // allow override from macro definition
#ifdef UHD_LOG_MIN_LEVEL
        this->global_level =
            _get_log_level(BOOST_STRINGIZE(UHD_LOG_MIN_LEVEL), this->global_level);
#endif
        // allow override from environment variables
        const char* log_level_env = std::getenv("UHD_LOG_LEVEL");
        if (log_level_env != NULL && log_level_env[0] != '\0') {
            this->global_level = _get_log_level(log_level_env, this->global_level);
        }

        // Setup default loggers (console and file)
        _setup_console_logging();
        _setup_file_logging();

        // On boot, we print the current UHD version info:
        {
            std::ostringstream sys_info;
            sys_info << BOOST_PLATFORM << "; " << BOOST_COMPILER << "; "
                     << "Boost_" << BOOST_VERSION << "; "
#ifdef HAVE_DPDK
                     << "DPDK_" << RTE_VER_YEAR << "." << RTE_VER_MONTH << "; "
#endif
                     << uhd::get_component() << "_" << uhd::get_version_string();
            _publish_log_msg(sys_info.str(), uhd::log::info, "UHD");
        }

        // Launch log message consumer
        _pop_task =
            std::make_shared<std::thread>(std::thread([this]() { this->pop_task(); }));
        uhd::set_thread_name(_pop_task.get(), LOG_THREAD_NAME);

        // Fastpath message consumer
#ifndef UHD_LOG_FASTPATH_DISABLE
        // allow override from environment variables
        const bool enable_fastpath = []() {
            const char* disable_fastpath_env = std::getenv("UHD_LOG_FASTPATH_DISABLE");
            if (disable_fastpath_env != NULL && disable_fastpath_env[0] != '\0') {
                return false;
            }
            return true;
        }();

        if (enable_fastpath) {
            _pop_fastpath_task = std::make_shared<std::thread>(
                std::thread([this]() { this->pop_fastpath_task(); }));
            uhd::set_thread_name(_pop_fastpath_task.get(), LOG_THREAD_NAME_FP);
        } else {
            _pop_fastpath_task = std::make_shared<std::thread>(
                std::thread([this]() { this->pop_fastpath_dummy_task(); }));
            uhd::set_thread_name(_pop_fastpath_task.get(), LOG_THREAD_NAME_FP_DUMMY);
            _publish_log_msg("Fastpath logging disabled at runtime.");
        }
#else
        {
            _publish_log_msg("Fastpath logging disabled at compile time.");
        }
#endif
    }

    ~log_resource(void)
    {
        _exit = true;

#ifndef BOOST_MSVC // push a final message is required, since the pop_with_wait() function
                   // will be used.
        // We push a final message to kick the pop task out of it's wait state.
        // This wouldn't be necessary if pop_with_wait() could fail. Should
        // that ever get fixed, we can remove this.
        auto final_message    = uhd::log::logging_info(pt::microsec_clock::local_time(),
            uhd::log::trace,
            __FILE__,
            __LINE__,
            "LOGGING",
            std::this_thread::get_id());
        final_message.message = "";
        push(final_message);
#    ifndef UHD_LOG_FASTPATH_DISABLE
        push_fastpath("");
#    endif
#endif // BOOST_MSVC

        _pop_task->join();
        {
            std::lock_guard<std::mutex> l(_logmap_mutex);
            _loggers.clear();
        }
        _pop_task.reset();
#ifndef UHD_LOG_FASTPATH_DISABLE
        _pop_fastpath_task->join();
        _pop_fastpath_task.reset();
#endif
    }

    void push(const uhd::log::logging_info& log_info)
    {
        static const double PUSH_TIMEOUT = 0.25; // seconds
        _log_queue.push_with_timed_wait(log_info, PUSH_TIMEOUT);
    }

#ifndef UHD_LOG_FASTPATH_DISABLE
    void push_fastpath(const std::string& message)
    {
        // Never wait. If the buffer is full, we just don't see the message.
        // Too bad.
        _fastpath_queue.push_with_haste(message);
    }
#endif

    void _handle_log_info(const uhd::log::logging_info& log_info)
    {
        if (log_info.message.empty()) {
            return;
        }
        std::lock_guard<std::mutex> l(_logmap_mutex);
        for (const auto& logger_pair : _loggers) {
            const auto& logger = logger_pair.second;
            if (log_info.verbosity < logger.first) {
                continue;
            }
            logger.second(log_info);
        }
    }

    void pop_task()
    {
        uhd::log::logging_info log_info;
        log_info.message = "";

        // For the lifetime of this thread, we run the following loop:
        while (!_exit) {
#ifdef BOOST_MSVC
            // Some versions of MSVC will hang if threads are being joined after main has
            // completed, so we need to guarantee a timeout here
            if (_log_queue.pop_with_timed_wait(log_info, READ_TIMEOUT)) {
                _handle_log_info(log_info);
            }
#else
            _log_queue.pop_with_wait(log_info); // Blocking call
            _handle_log_info(log_info);
#endif // BOOST_MSVC
        }

        // Exit procedure: Clear the queue
        while (_log_queue.pop_with_haste(log_info)) {
            _handle_log_info(log_info);
        }

        // Terminate this thread.
    }

    void pop_fastpath_task()
    {
#ifndef UHD_LOG_FASTPATH_DISABLE
        std::string msg;
        while (!_exit) {
#    ifdef BOOST_MSVC
            // Some versions of MSVC will hang if threads are being joined after main has
            // completed, so we need to guarantee a timeout here
            if (_fastpath_queue.pop_with_timed_wait(msg, READ_TIMEOUT)) {
                std::cerr << msg << std::flush;
            }
#    else
            _fastpath_queue.pop_with_wait(msg);
            std::cerr << msg << std::flush;
#    endif // BOOST_MSVC
        }

        // Exit procedure: Clear the queue
        while (_fastpath_queue.pop_with_haste(msg)) {
            std::cerr << msg << std::flush;
        }
#endif
    }

    void pop_fastpath_dummy_task()
    {
#ifndef UHD_LOG_FASTPATH_DISABLE
        std::string msg;
        while (!_exit) {
#    ifdef BOOST_MSVC
            // Some versions of MSVC will hang if threads are being joined after main has
            // completed, so we need to guarantee a timeout here
            _fastpath_queue.pop_with_timed_wait(msg, READ_TIMEOUT);
#    else
            _fastpath_queue.pop_with_wait(msg);
#    endif // BOOST_MSVC
        }

        // Exit procedure: Clear the queue
        while (_fastpath_queue.pop_with_haste(msg))
            ;
#endif
    }

    void add_logger(const std::string& key, uhd::log::log_fn_t logger_fn)
    {
        std::lock_guard<std::mutex> l(_logmap_mutex);
        _loggers[key] = level_logfn_pair{global_level, logger_fn};
    }

    void set_log_level(const std::string& key, const uhd::log::severity_level level)
    {
        std::lock_guard<std::mutex> l(_logmap_mutex);
        _loggers[key].first = level;
    }

private:
    std::shared_ptr<std::thread> _pop_task;
#ifndef UHD_LOG_FASTPATH_DISABLE
    std::shared_ptr<std::thread> _pop_fastpath_task;
#endif
    uhd::log::severity_level _get_log_level(
        const std::string& log_level_str, const uhd::log::severity_level& previous_level)
    {
        boost::optional<uhd::log::severity_level> parsed_level =
            uhd::log::parse_log_level_from_string(log_level_str);
        if (parsed_level) {
            return *parsed_level;
        } else {
            return previous_level;
        }
    }

    void _setup_console_logging()
    {
#ifndef UHD_LOG_CONSOLE_DISABLE
        uhd::log::severity_level console_level = uhd::log::trace;
#    ifdef UHD_LOG_CONSOLE_LEVEL
        console_level =
            _get_log_level(BOOST_STRINGIZE(UHD_LOG_CONSOLE_LEVEL), console_level);
#    endif
        const char* log_console_level_env = std::getenv("UHD_LOG_CONSOLE_LEVEL");
        if (log_console_level_env != NULL && log_console_level_env[0] != '\0') {
            console_level = _get_log_level(log_console_level_env, console_level);
        }
        _loggers[UHD_CONSOLE_LOGGER_KEY] = level_logfn_pair{console_level, &console_log};
#endif
    }

    void _setup_file_logging()
    {
        uhd::log::severity_level file_level = uhd::log::trace;
        std::string log_file_target;
#if defined(UHD_LOG_FILE_LEVEL)
        file_level = _get_log_level(BOOST_STRINGIZE(UHD_LOG_FILE_LEVEL), file_level);
#endif
#if defined(UHD_LOG_FILE)
        log_file_target = BOOST_STRINGIZE(UHD_LOG_FILE);
#endif
        const char* log_file_level_env = std::getenv("UHD_LOG_FILE_LEVEL");
        if (log_file_level_env != NULL && log_file_level_env[0] != '\0') {
            file_level = _get_log_level(log_file_level_env, file_level);
        }
        const char* log_file_env = std::getenv("UHD_LOG_FILE");
        if ((log_file_env != NULL) && (log_file_env[0] != '\0')) {
            log_file_target = std::string(log_file_env);
        }
        if (!log_file_target.empty()) {
            auto F = std::make_shared<file_logger_backend>(log_file_target);
            _loggers[UHD_FILE_LOGGER_KEY] = level_logfn_pair{file_level,
                [F](const uhd::log::logging_info& log_info) { F->log(log_info); }};
        }
    }

    void _publish_log_msg(const std::string& msg,
        const uhd::log::severity_level level = uhd::log::info,
        const std::string& component         = "LOGGING")
    {
        if (level < global_level) {
            return;
        }
        auto log_msg    = uhd::log::logging_info(pt::microsec_clock::local_time(),
            level,
            __FILE__,
            __LINE__,
            component,
            std::this_thread::get_id());
        log_msg.message = msg;
        _log_queue.push_with_timed_wait(log_msg, 0.25);
    }

    std::mutex _logmap_mutex;
    std::atomic<bool> _exit;
    using level_logfn_pair = std::pair<uhd::log::severity_level, uhd::log::log_fn_t>;
    std::map<std::string, level_logfn_pair> _loggers;
#ifndef UHD_LOG_FASTPATH_DISABLE
    uhd::transport::bounded_buffer<std::string> _fastpath_queue;
#endif
    uhd::transport::bounded_buffer<uhd::log::logging_info> _log_queue;
};

UHD_SINGLETON_FCN(log_resource, log_rs);

/***********************************************************************
 * The logger object implementation
 **********************************************************************/
uhd::_log::log::log(const uhd::log::severity_level verbosity,
    const std::string& file,
    const unsigned int line,
    const std::string& component,
    const std::thread::id thread_id)
    : _log_it(verbosity >= log_rs().global_level)
{
    if (_log_it) {
        this->_log_info = uhd::log::logging_info(pt::microsec_clock::local_time(),
            verbosity,
            file,
            line,
            component,
            thread_id);
    }
}

uhd::_log::log::~log(void)
{
    if (_log_it) {
        this->_log_info.message = _ss.str();
        try {
            log_rs().push(this->_log_info);
        } catch (...) {
        }
    }
}

#ifndef UHD_LOG_FASTPATH_DISABLE
void uhd::_log::log_fastpath(const std::string& msg)
{
    log_rs().push_fastpath(msg);
}
#else
void uhd::_log::log_fastpath(const std::string&)
{
    // nop
}
#endif

/***********************************************************************
 * Public API calls
 **********************************************************************/
void uhd::log::add_logger(const std::string& key, log_fn_t logger_fn)
{
    log_rs().add_logger(key, logger_fn);
}

void uhd::log::set_log_level(uhd::log::severity_level level)
{
    log_rs().global_level = level;
}

void uhd::log::set_logger_level(const std::string& key, uhd::log::severity_level level)
{
    log_rs().set_log_level(key, level);
}

void uhd::log::set_console_level(uhd::log::severity_level level)
{
    set_logger_level(UHD_CONSOLE_LOGGER_KEY, level);
}

void uhd::log::set_file_level(uhd::log::severity_level level)
{
    set_logger_level(UHD_FILE_LOGGER_KEY, level);
}
