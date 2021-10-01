#include "chat.h"
#include <iostream>
#include <tiny_ipc/client.h>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/local/basic_endpoint.hpp>
#include <boost/asio/completion_condition.hpp>

char buffer[1024];

int main(int argc, char** argv)
{
    boost::asio::io_context io_ctx;
    using namespace tiny_ipc::literals;
    boost::asio::local::stream_protocol::endpoint ep(argv[1]);
    boost::asio::local::stream_protocol::socket   local(io_ctx);
    local.connect(ep);

    auto my_client = tiny_ipc::client(
        local, chat::chat, "text_added"_s = [](std::string const& text) { std::puts(text.c_str()); });

    my_client.execute(
        "connect"_m, [](bool result) {}, ::ucred{});

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work = boost::asio::make_work_guard(io_ctx);

    boost::asio::posix::stream_descriptor input_stream(io_ctx, STDIN_FILENO);
    // assume that you already defined your read_handler ...
    std::function<void(boost::system::error_code, std::size_t)> read_handler;

    read_handler = [&read_handler, &my_client, &input_stream](boost::system::error_code ec, std::size_t bytes_transfered)
    {
        my_client.execute(
            "send"_m, []() {}, std::string_view(buffer, bytes_transfered));
        // async_read(input_stream, buffer(buf), boost::asio::transfer_at_least, read_handler);
    };

    // async_read(input_stream, buffer(buf), boost::asio::transfer_at_least, read_handler);

    io_ctx.run();
}
