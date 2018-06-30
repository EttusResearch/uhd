#pragma once

#ifndef ASYNC_WRITER_H_HQIRH28I
#define ASYNC_WRITER_H_HQIRH28I

#include <boost/asio.hpp>
#include "rpc/msgpack.hpp"
#include <condition_variable>
#include <deque>
#include <memory>
#include <thread>

namespace rpc {

class client;

namespace detail {

//! \brief Common logic for classes that have a write queue with async writing.
class async_writer : public std::enable_shared_from_this<async_writer> {
public:
    async_writer(boost::asio::io_service *io,
                 boost::asio::ip::tcp::socket socket)
        : socket_(std::move(socket)), write_strand_(*io), exit_(false) {}

    void do_write() {
        if (exit_) {
            return;
        }
        auto self(shared_from_this());
        auto &item = write_queue_.front();
        // the data in item remains valid until the handler is called
        // since it will still be in the queue physically until then.
        boost::asio::async_write(
            socket_, boost::asio::buffer(item.data(), item.size()),
            write_strand_.wrap(
                [this, self](boost::system::error_code ec, std::size_t transferred) {
                    (void)transferred;
                    if (!ec) {
                        write_queue_.pop_front();
                        if (write_queue_.size() > 0) {
                            if (!exit_) {
                                do_write();
                            }
                        }
                    } else {
                        LOG_ERROR("Error while writing to socket: {}", ec);
                    }

                    if (exit_) {
                        LOG_INFO("Closing socket");
                        socket_.shutdown(
                            boost::asio::ip::tcp::socket::shutdown_both);
                        socket_.close();
                    }
                }));
    }

    void write(RPCLIB_MSGPACK::sbuffer &&data) {
        write_queue_.push_back(std::move(data));
        if (write_queue_.size() > 1) {
            return; // there is an ongoing write chain so don't start another
        }

        do_write();
    }

    friend class rpc::client;

protected:
    boost::asio::ip::tcp::socket socket_;
    boost::asio::io_service::strand write_strand_;
    std::atomic_bool exit_{false};
    bool exited_ = false;
    std::mutex m_exit_;
    std::condition_variable cv_exit_;

private:
    std::deque<RPCLIB_MSGPACK::sbuffer> write_queue_;
    RPCLIB_CREATE_LOG_CHANNEL(async_writer)
};

} /* detail */
} /* rpc  */

#endif /* end of include guard: ASYNC_WRITER_H_HQIRH28I */
