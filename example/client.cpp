// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "chat.hpp"
#include <iostream>
#include <pwd.h>
#include <tiny_ipc/client.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/local/basic_endpoint.hpp>
#include <boost/asio/completion_condition.hpp>

char chat_input_buffer[1024];

using namespace tiny_ipc::literals;

auto& connect_socket(boost::asio::local::stream_protocol::socket& sock, boost::asio::local::stream_protocol::endpoint& ep)
{
    sock.connect(ep);
    return sock;
}
/**
 * The purpose of this class is to show how one can wrap the interactions in a type so that
 * the interprocess functionalities seamlessly integrate with the rest of the code base.
 *
 * There is no need to do this, but it might help to avoid some of the boilerplate.
 * Alternative strategies is to provide domain specific free-standing functions that prefill
 * the compile time aspects of the communication like interface ids, protocol and method
 * name.
 */
struct chat_client
{
    boost::asio::io_context&                      m_io_ctx;
    boost::asio::local::stream_protocol::endpoint m_ep;
    boost::asio::local::stream_protocol::socket   m_sock;
    tiny_ipc::client                              m_client;
    std::string                                   m_name;

    chat_client(boost::asio::io_context& ctx, std::string const& endpoint, std::string const& user_name = "user")
        : m_io_ctx(ctx),
          m_ep(endpoint),
          m_sock(ctx),
          m_client(connect_socket(m_sock, m_ep),
                   [this](boost::system::error_code ec, tiny_ipc::client&)
                   {
                       std::cout << "Error " << ec << "\n";
                       m_io_ctx.stop();
                   }),
          m_name(user_name)
    {
        async_dispatch_messages<chat::chat_protocol>(                                      //
            m_client,                                                                      //
            tiny_ipc::signals_of(                                                          //
                "chat"_i, "1.0"_v,                                                         //
                "text_added"_s = [](std::string const& text) { std::puts(text.c_str()); }  //
                ));
    }

    template <tiny_ipc::c::method_name M, typename RH, typename... Ts>
    void execute(M m, RH&& rh, Ts&&... ts)
    {
        tiny_ipc::interface_id iface("chat"_i, "1.0"_v);
        execute_method<chat::chat_protocol>(iface, m, m_client, std::forward<RH>(rh), std::forward<Ts>(ts)...);
    }

    void connect_user()
    {
        execute(
            "connect"_m,
            [this](bool r)
            {
                if (!r) m_io_ctx.stop();
            },
            ::ucred{}, m_name);
    }

    void enter_chat_message(std::string_view const& buf)
    {
        execute(
            "send"_m, []() {}, buf);
    }
};

struct input_reader
{
    chat_client&                                                m_client;
    boost::asio::posix::stream_descriptor                       m_input_stream;
    std::function<void(boost::system::error_code, std::size_t)> m_read_handler;

    input_reader(boost::asio::io_context& ctx, chat_client& client)
        : m_client(client),
          m_input_stream(ctx, STDIN_FILENO),
          m_read_handler(
              [this](boost::system::error_code ec, std::size_t bytes_transfered)
              {
                  m_client.enter_chat_message(std::string_view(chat_input_buffer, bytes_transfered));
                  async_read(m_input_stream, boost::asio::buffer(chat_input_buffer), boost::asio::transfer_at_least(1), m_read_handler);
              })
    {
        async_read(m_input_stream, boost::asio::buffer(chat_input_buffer), boost::asio::transfer_at_least(1), m_read_handler);
    }
};

int main(int argc, char** argv)
{
    std::string user_name = getpwuid(getuid())->pw_name;
    std::cout << user_name << std::endl;
    if (argc == 1 || argc > 3) std::cout << "Usage: client SERVERSOCKET [USERNAME]" << std::endl;
    if (argc == 3) user_name = argv[2];

    boost::asio::io_context io_ctx;

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work = boost::asio::make_work_guard(io_ctx);

    chat_client  my_client(io_ctx, argv[1], user_name);
    input_reader input(io_ctx, my_client);

    my_client.connect_user();

    io_ctx.run();
}
