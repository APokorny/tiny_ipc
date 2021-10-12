// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TINY_IPC_CLIENT_H_INCLUDED
#define TINY_IPC_CLIENT_H_INCLUDED
#include <algorithm>
#include <ranges>
#include <vector>
#include <tiny_ipc/proto_def.h>
#include <tiny_ipc/detail/protocol.h>
#include <tiny_ipc/detail/encode.h>
#include <tiny_ipc/detail/decode.h>
#include <tiny_ipc/detail/message_comm.h>
#include <tiny_ipc/detail/to_item.h>
#include <tiny_ipc/detail/forward_item.h>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <tiny_tuple/map.h>

namespace tiny_ipc
{
struct client
{
    detail::message_comm communicator;
    uint16_t             cookie_generator{0xE0F0};

    struct active_request
    {
        msg_id                                       id;
        std::function<void(detail::message_parser&)> payload_handler;
    };
    std::vector<active_request> active_requests;

    explicit client(boost::asio::local::stream_protocol::socket& s) : communicator{s} {}
    uint16_t gen_cookie() { return cookie_generator++; }
};

template <c::protocol P, c::signal_handler... Ts>
requires(detail::is_in_protocol<P, typename Ts::name>&&... && true) auto create_async_message_handler(client& c, Ts&&... ts)
{
    return [&c, callbacks = tiny_tuple::map<detail::to_item<Ts>...>{detail::to_item<Ts>{std::move(ts.callable)}...}](boost::system::error_code ec)mutable
    {
        if (ec) {}
        else
        {
            // Consider splitting message receival and consumption into two parts:
            // Firstly allow asynchronous message handling - i.e. by posting the the resulting invocation and ensuring that the
            // socket is used strictly synchronously or at least always form a single io_context wake. secondly to allow
            // handling multiple protocols within a server on a single socket. That way versioning of the protocol could be
            // achieved by always prefixing the messages with a protocol id, and or splitting up functionalities into multiple
            // modules might be nicer.
            auto       msg    = c.communicator.peek_and_receive();
            msg_header header = decode_item(msg, type<msg_header>{});

            auto item_to_drop = std::ranges::remove(std::views::all(c.active_requests), header.id, &client::active_request::id);
            if (!item_to_drop.empty())  // msg is a reply
            {
                item_to_drop.begin()->payload_handler(msg);
                c.active_requests.erase(item_to_drop.begin(), item_to_drop.end());
            }
            else  // msg is a signal
            {
                detail::forward_item<P>(header.id.id, callbacks,
                                        [&msg](auto& handler, auto const& signature)
                                        { detail::decode<std::decay_t<decltype(signature)>>(msg, handler); });
            }
        }
    };
}
template <c::protocol P, c::method_name M, typename ResultHandler, typename... Cs>
requires detail::is_in_protocol<P, M> && c::valid_invocation<P, M, Cs...>  //
void execute_method(M, client& client_instance, ResultHandler&& fun, Cs&&... params)
{
    using signature   = detail::get_signature<P, M>;
    using return_type = detail::just_return_type_t<signature>;
    auto cookie       = client_instance.gen_cookie();
    // todo get size hints for control and cred messages..
    if constexpr (!std::is_same_v<void, return_type>)
    {
        client_instance.active_requests.push_back({{id_of_item<P, M>, cookie}, [handler = std::move(fun)](detail::message_parser& parser) {
                                                       handler(decode_item(parser, type<return_type>()));
                                                   }});
    }
    packet new_msg(msg_header{{id_of_item<P, M>, cookie}, 128, 0});
    detail::encode<signature>(new_msg, std::forward<Cs>(params)...);
    client_instance.communicator.send(new_msg);
}

}  // namespace tiny_ipc
#endif
