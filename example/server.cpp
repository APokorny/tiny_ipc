#include "chat.h"
#include <tiny_ipc/server_session.h>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/local/basic_endpoint.hpp>
#include <boost/asio/completion_condition.hpp>

int main(int argc, char** argv)
{
    boost::asio::io_context                     io_ctx;
    boost::asio::local::stream_protocol::socket local(io_ctx, argv[1]);
    using namespace tiny_ipc::literals;

    auto on_accept = [](boost::system::error_code ec, boost::asio::local::stream_protocol::socket other)
    {
        auto my_session = tiny_ipc::server_session(
            other, chat::chat, "text_added"_m = [](std::string const& text) { std::puts(text.c_str()); });
    };

    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work = boost::asio::make_work_guard(io_ctx);
    io_ctx.run();
}
