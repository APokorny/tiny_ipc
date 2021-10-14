// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "chat.h"
#include <tiny_ipc/server_session.h>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/local/basic_endpoint.hpp>
#include <boost/asio/completion_condition.hpp>

using namespace tiny_ipc::literals;
struct chat_server
{
    boost::asio::io_context&                                                io_ctx;
    boost::asio::local::basic_endpoint<boost::asio::local::stream_protocol> local;
    boost::asio::local::stream_protocol::acceptor                           acceptor;
    struct session_handler
    {
        boost::asio::local::stream_protocol::socket socket;
        tiny_ipc::server_session                    session;
        explicit session_handler(boost::asio::local::stream_protocol::socket&& s) : socket{std::move(s)}, session(socket) {}
    };
    std::vector<session_handler> sessions;

    chat_server(boost::asio::io_context& ctx, std::string const& path) : io_ctx(ctx), local(path.c_str()), acceptor(io_ctx, local) {}
    void async_read(session_handler& c)
    {
        tiny_ipc::async_dispatch_messages<chat::chat_protocol>(  //
            c.session, tiny_ipc::methods_of(
                           "chat"_i, "1.0"_v,
                           "connect"_m = [this](::ucred cred) -> bool
                           {
                               std::cout << "new user connected:" << cred.uid << " G:" << cred.gid << " P:" << cred.pid << std::endl;
                               return true;
                           },
                           "send"_m =
                               [this](std::string const& text)
                           {
                               std::cout << "<CHAT>:";
                               std::puts(text.c_str());
                           }  //
                           ));
    }
    void accept_connections()
    {
        acceptor.async_accept(local,
                              [this](boost::system::error_code ec, boost::asio::local::stream_protocol::socket other)
                              {
                                  sessions.emplace_back(std::move(other));

                                  async_read(sessions.back());
                                  accept_connections();
                              });
    }
    void start() { accept_connections(); }
};
int main(int argc, char** argv)
{
    boost::asio::io_context io_ctx;
    ::unlink(argv[1]);
    chat_server server(io_ctx, argv[1]);
    server.start();
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work = boost::asio::make_work_guard(io_ctx);
    std::cout << "herer\n ";
    io_ctx.run();
    std::cout << "there \n ";
}
