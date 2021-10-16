// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "chat.h"
#include <algorithm>
#include <boost/system/error_code.hpp>
#include <tiny_ipc/server_session.h>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/local/basic_endpoint.hpp>
#include <boost/asio/completion_condition.hpp>
#include <boost/asio/signal_set.hpp>

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
        template <tiny_ipc::concepts::session_error_handler H>
        explicit session_handler(boost::asio::local::stream_protocol::socket&& s, H&& h)
            : socket{std::move(s)}, session(socket, std::forward<H>(h))
        {
        }
    };
    std::vector<std::shared_ptr<session_handler>> sessions;

    void clear_sessions()
    {
        acceptor.close();
        for (auto& s : sessions) s->session.close();
        sessions.clear();
    }
    chat_server(boost::asio::io_context& ctx, std::string const& path) : io_ctx(ctx), local(path.c_str()), acceptor(io_ctx, local) {}
    void async_read(std::shared_ptr<session_handler> const& c)
    {
        tiny_ipc::async_dispatch_messages<chat::chat_protocol>(  //
            c->session,                                          //

            tiny_ipc::methods_of(
                "chat"_i, "1.0"_v,
                "connect"_m = [this, c](::ucred cred) -> bool
                {
                    std::cout << "new user connected:" << cred.uid << " G:" << cred.gid << " P:" << cred.pid << std::endl;
                    return true;
                },
                "send"_m =
                    [this, c](std::string const& text)
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
                                  if (ec) { std::cout << "shutting down \n"; }
                                  else
                                  {
                                      std::cout << "async connections\n";
                                      sessions.emplace_back(std::make_shared<session_handler>(
                                          std::move(other),
                                          [this](boost::system::error_code ec, tiny_ipc::server_session& s) mutable
                                          {
                                              sessions.erase(                                     //
                                                  std::remove_if(begin(sessions), end(sessions),  //
                                                                 [&s](auto const& item) { return (&s == &item->session); }),
                                                  end(sessions));
                                              std::cout << "error " << ec << "\n";
                                          }));

                                      async_read(sessions.back());
                                      accept_connections();
                                  }
                              });
    }
    void start() { accept_connections(); }
};
int main(int argc, char** argv)
{
    boost::asio::io_context io_ctx;
    boost::asio::signal_set signals(io_ctx);
    signals.add(SIGHUP);
    signals.add(SIGQUIT);
    signals.add(SIGINT);
    signals.add(SIGTERM);
    ::unlink(argv[1]);
    chat_server server(io_ctx, argv[1]);
    server.start();
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work = boost::asio::make_work_guard(io_ctx);
    std::cout << "herer\n ";
    signals.async_wait(
        [&](boost::system::error_code ec, int sig)
        {
            std::cout << "signal " << sig << "received shutting down\n";
            work.reset();
            server.clear_sessions();
        });
    io_ctx.run();
    std::cout << "there \n ";
}
