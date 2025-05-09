#include "rpc/detail/server_session.h"

#include "rpc/server.h"
#include "rpc/this_handler.h"
#include "rpc/this_server.h"
#include "rpc/this_session.h"
#include "rpc/config.h"

#include "rpc/detail/log.h"

// If we take all supported versions of clang, gcc, and MSVC, we get some versions
// that require lambda captures for constexpr, and some compilers throw a warnings
// when you use a lambdas capture.
// To make everyone "happy", we turn off those warnings for clang.
#pragma GCC diagnostic push
#ifdef __clang__
#  pragma GCC diagnostic ignored "-Wunused-lambda-capture"
#endif

namespace rpc {
namespace detail {

static constexpr std::size_t default_buffer_size = rpc::constants::DEFAULT_BUFFER_SIZE;

server_session::server_session(server *srv, boost::asio::io_context *io,
                               boost::asio::ip::tcp::socket socket,
                               std::shared_ptr<dispatcher> disp,
                               bool suppress_exceptions)
    : async_writer(io, std::move(socket)),
      parent_(srv),
      io_(io),
      read_strand_(io->get_executor()),
      disp_(disp),
      pac_(),
      suppress_exceptions_(suppress_exceptions) {
    pac_.reserve_buffer(default_buffer_size); // TODO: make this configurable
                                              // [sztomi 2016-01-13]
}

void server_session::start() { do_read(); }

void server_session::close() {
    LOG_INFO("Closing session.");
    exit_ = true;
    boost::asio::post(write_strand_, [this]() {
        try {
            socket_.close();
        } catch (const boost::system::system_error&) {
            LOG_WARN("Error during closing socket.");
        }
    });
}

void server_session::do_read() {
    auto self(shared_from_this());
    constexpr std::size_t max_read_bytes = default_buffer_size;
    socket_.async_read_some(boost::asio::buffer(pac_.buffer(), default_buffer_size),
        // I don't think max_read_bytes needs to be captured explicitly
        // (since it's constexpr), but MSVC insists.
        boost::asio::bind_executor(read_strand_, [this, self, max_read_bytes](boost::system::error_code ec,
                                                                              std::size_t length) {
            if (!ec) {
                pac_.buffer_consumed(length);
                RPCLIB_MSGPACK::unpacked result;
                while (pac_.next(&result) && !exit_) {
                    auto msg = result.get();
                    output_buf_.clear();

                    // any worker thread can take this call
                    auto z = std::shared_ptr<RPCLIB_MSGPACK::zone>(result.zone().release());
                    boost::asio::post(io_->get_executor(), [
                        this, msg, z
                    ]() {
                        this_handler().clear();
                        this_session().clear();
                        this_server().cancel_stop();

                        auto resp = disp_->dispatch(msg, suppress_exceptions_);

                        // There are various things that decide what to send
                        // as a response. They have a precedence.

                        // First, if the response is disabled, that wins
                        // So You Get Nothing, You Lose! Good Day Sir!
                        if (!this_handler().resp_enabled_) {
                            return;
                        }

                        // Second, if there is an error set, we send that
                        // and only third, if there is a special response, we
                        // use it
                        if (!this_handler().error_.get().is_nil()) {
                            LOG_WARN("There was an error set in the handler");
                            resp.capture_error(this_handler().error_);
                        } else if (!this_handler().resp_.get().is_nil()) {
                            LOG_WARN("There was a special result set in the "
                                     "handler");
                            resp.capture_result(this_handler().resp_);
                        }

                        if (!resp.is_empty()) {
#ifdef _MSC_VER
                            // doesn't compile otherwise.
                            boost::asio::post(write_strand_,
                                [=]() { write(resp.get_data()); });
#else
                            boost::asio::post(write_strand_,
                                [this, resp, z]() { write(resp.get_data()); });
#endif
                        }

                        if (this_session().exit_) {
                            LOG_WARN("Session exit requested from a handler.");
                            // posting through the strand so this comes after
                            // the previous write
                            boost::asio::post(write_strand_, [this]() { exit_ = true; });
                        }

                        if (this_server().stopping_) {
                            LOG_WARN("Server exit requested from a handler.");
                            // posting through the strand so this comes after
                            // the previous write
                            boost::asio::post(write_strand_,
                                [this]() { parent_->close_sessions(); });
                        }
                    });
                }

                if (!exit_) {
                    // resizing strategy: if the remaining buffer size is
                    // less than the maximum bytes requested from asio,
                    // then request max_read_bytes. This prompts the unpacker
                    // to resize its buffer doubling its size
                    // (https://github.com/msgpack/msgpack-c/issues/567#issuecomment-280810018)
                    if (pac_.buffer_capacity() < max_read_bytes) {
                        LOG_TRACE("Reserving extra buffer: {}", max_read_bytes);
                        pac_.reserve_buffer(max_read_bytes);
                    }
                    do_read();
                }
            }
        }));
    if (exit_) {
        socket_.close();
    }
}

} /* detail */
} /* rpc */

#pragma GCC diagnostic pop
