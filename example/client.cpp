#include "chat.h"
#include <iostream>
#include <tiny_ipc/client.h>

int main(int argc, char** argv)
{
    boost::asio::io_context                     io_ctx;
    boost::asio::local::stream_protocol::socket local(io_ctx, argv[1]);
    using namespace tiny_ipc::literals;
    auto my_client = tiny_ipc::client(
        local, chat::chat, "text_added"_s = [](std::string const& text) { std::cout << text << std::endl; });

    my_client.execute(
        "connect"_m, [](bool result) {}, ::ucred{});
}
