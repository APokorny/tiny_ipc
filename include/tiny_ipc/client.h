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

template <c::protocol P, c::signal_group... Ts>
requires(detail::are_in_protocol<P, typename std::decay_t<Ts>::id, typename std::decay_t<Ts>::signals>&&... &&
         true) void async_dispatch_messages(client& c, Ts&&... ts)
{
    auto default_handler = [&c, ts...]() { async_dispatch_messages<P>(c, ts...); };

    c.communicator.socket.async_wait(
        boost::asio::socket_base::wait_read,
        [&c, default_handler,
         interface_dispatcher =
             tiny_tuple::map<tiny_tuple::detail::item<typename std::decay_t<Ts>::id, decltype(std::decay_t<Ts>::dispatcher)>...>(
                 tiny_tuple::detail::item<typename std::decay_t<Ts>::id, decltype(std::decay_t<Ts>::dispatcher)>(
                     std::move(ts.dispatcher))...)](boost::system::error_code ec) mutable

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

                auto item_to_drop = std::remove_if(c.active_requests.begin(), c.active_requests.end(),
                                                   [id = header.id](auto const& item) { return item.id == id; });
                if (item_to_drop != c.active_requests.end())  // msg is a reply
                {
                    item_to_drop->payload_handler(msg);
                    c.active_requests.erase(item_to_drop, c.active_requests.end());
                }
                else  // msg is a signal
                {
                    detail::forward_item<P>(header.id.interface, header.id.id, interface_dispatcher,
                                            [&msg](auto& handler, auto const& signature)
                                            { detail::decode<std::decay_t<decltype(signature)>>(msg, handler); });
                }
            }
            default_handler();
        });
}
template <c::protocol P, c::interface_id I, c::method_name M, typename ResultHandler, typename... Cs>
requires detail::is_in_protocol<P, I, M>
void execute_method(I, M, client& client_instance, ResultHandler&& fun, Cs&&... params)
{
    using iface       = get_interface<P, I>;
    using signature   = detail::get_signature<iface, M>;
    using return_type = detail::just_return_type_t<signature>;
    auto cookie       = client_instance.gen_cookie();
    // todo get size hints for control and cred messages..
    if constexpr (!std::is_same_v<void, return_type>)
    {
        client_instance.active_requests.push_back({{iface::hash, id_of_item<iface, M>, cookie},
                                                   [handler = std::move(fun)](detail::message_parser& parser)
                                                   { handler(decode_item(parser, type<return_type>())); }});
    }
    packet new_msg(msg_header{{iface::hash, id_of_item<iface, M>, cookie}, 128, 0});
    detail::encode<signature>(new_msg, std::forward<Cs>(params)...);
    client_instance.communicator.send(new_msg);
}

}  // namespace tiny_ipc
#endif
