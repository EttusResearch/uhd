//
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#pragma once

#include <boost/noncopyable.hpp>
#include <boost/circular_buffer.hpp>
#include <mutex>
#include <memory>
#include <tuple>
#include <functional>

namespace mpm { namespace types {

    //! Log levels, designed to match the ones in mpmlog.py
    enum class log_level_t {
        NONE      = 0, //!< Use this when there's actually no message
        TRACE     = 1,
        DEBUG     = 10,
        INFO      = 20,
        WARNING   = 30,
        ERROR     = 40,
        CRITICAL  = 50
    };

    struct log_message {
        log_level_t log_level;
        std::string component;
        std::string message;

        log_message(
            const log_level_t log_level_,
            const std::string& component_,
            const std::string& message_
        ) : log_level(log_level_)
          , component(component_)
          , message(message_)
        {
            // nop
        }
    };

    class log_buf : public boost::noncopyable
    {
    public:
        using sptr = std::shared_ptr<log_buf>;

        static const size_t BUFSIZE = 20;

        log_buf() : _buf(BUFSIZE) {}
        ~log_buf() {}

        static sptr make();

        static sptr make_singleton();

        /*! Post a message to the ring buffer
         *
         * \param log_level Log level
         * \param component The component where the message is originating from
         * \param message The actual log message
         */
        void post(
            const log_level_t log_level,
            const std::string &component,
            const std::string &message
        );

        //! Use this to set a callback that gets called when a new message was
        //  posted.
        void set_notify_callback(
            std::function<void(void)> callback
        );

        std::tuple<log_level_t, std::string, std::string> pop();

    private:
        std::mutex _buf_lock;
        boost::circular_buffer<log_message> _buf;
        std::function<void(void)> _notify_callback;
    };

}} /* namespace mpm::types */

