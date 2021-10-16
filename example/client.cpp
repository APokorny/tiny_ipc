// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#include "chat.h"
#include <iostream>
#include <tiny_ipc/client.h>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/local/basic_endpoint.hpp>
#include <boost/asio/completion_condition.hpp>

char chat_input_buffer[1024];

using namespace tiny_ipc::literals;

int main(int argc, char** argv)
{
    boost::asio::io_context io_ctx;
    using namespace tiny_ipc::literals;
    boost::asio::local::stream_protocol::endpoint ep(argv[1]);
    boost::asio::local::stream_protocol::socket   local(io_ctx);

    // connect to the server:
    local.connect(ep);

    std::cout << "connected " << std::endl;
    auto my_client =
        tiny_ipc::client(local, [](boost::system::error_code ec, tiny_ipc::client&) { std::cout << "Error " << ec << std::endl; });

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work = boost::asio::make_work_guard(io_ctx);
    // connecting with credentials:
    execute_method<chat::chat_protocol>(
        tiny_ipc::interface_id("chat"_i, "1.0"_v), "connect"_m, my_client,
        [](bool result) { std::cout << "Connected :" << result << "\n"; }, ::ucred{});

    boost::asio::posix::stream_descriptor input_stream(io_ctx, STDIN_FILENO);
    // assume that you already defined your read_handler ...
    std::function<void(boost::system::error_code, std::size_t)> read_handler;

    read_handler = [&read_handler, &my_client, &input_stream](boost::system::error_code ec, std::size_t bytes_transfered)
    {
        std::cout << "READ SOMETHING " << bytes_transfered << " " << std::string_view(chat_input_buffer, bytes_transfered) << std::endl;
        execute_method<chat::chat_protocol>(
            tiny_ipc::interface_id("chat"_i, "1.0"_v), "send"_m, my_client, []() {}, std::string_view(chat_input_buffer, bytes_transfered));
        async_read(input_stream, boost::asio::buffer(chat_input_buffer), boost::asio::transfer_at_least(1), read_handler);
    };

    async_read(input_stream, boost::asio::buffer(chat_input_buffer), boost::asio::transfer_at_least(1), read_handler);

    std::cout << "setup system signal" << std::endl;
    async_dispatch_messages<chat::chat_protocol>(my_client, tiny_ipc::signals_of(
                                                                "chat"_i, "1.0"_v,
                                                                "text_added"_s =
                                                                    [](std::string const& text)
                                                                {
                                                                    std::cout << "received SOMETHING " << text << std::endl;
                                                                    std::puts(text.c_str());
                                                                }));

    io_ctx.run();
}
