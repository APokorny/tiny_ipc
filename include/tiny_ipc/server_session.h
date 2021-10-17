// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TINY_IPC_SERVER_SESSION_H_INCLUDED
#define TINY_IPC_SERVER_SESSION_H_INCLUDED
#include <algorithm>
#include <ranges>
#include <type_traits>
#include <iostream>
#include <vector>
#include <ranges>
#include <tiny_ipc/proto_def.h>
#include <tiny_ipc/detail/protocol.h>
#include <tiny_ipc/detail/encode.h>
#include <tiny_ipc/detail/decode.h>
#include <tiny_ipc/detail/message_comm.h>
#include <tiny_ipc/detail/to_item.h>
#include <tiny_ipc/detail/forward_item.h>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/basic_socket.hpp>
#include <boost/asio/read.hpp>
#include <tiny_tuple/map.h>

namespace tiny_ipc
{
struct server_session;

namespace concepts
{
template <typename F>
concept session_error_handler = requires
{
    std::declval<F>()(std::declval<boost::system::error_code>(), std::declval<server_session&>());
};
}  // namespace concepts

struct server_session
{
    detail::message_comm communicator;
    template <c::session_error_handler H>
    explicit server_session(boost::asio::local::stream_protocol::socket& s, H on_error) : communicator{s}
    {
        communicator.socket.async_wait(boost::asio::socket_base::wait_error,
                                       [this, on_error](boost::system::error_code ec) mutable
                                       {
                                           boost::system::error_code e;
                                           communicator.socket.cancel(e);
                                           communicator.socket.close(e);
                                           on_error(ec, *this);
                                       });
    }

    void close()
    {
        communicator.socket.cancel();
        communicator.socket.close();
    }
};

template <c::protocol P, c::method_group... Ts>
requires(detail::are_in_protocol<P, typename std::decay_t<Ts>::id, typename std::decay_t<Ts>::methods>&&... &&
         true) void async_dispatch_messages(server_session& s, Ts&&... ts)
{
    auto default_handler = [&s, ts...]() mutable { async_dispatch_messages<P>(s, ts...); };
    s.communicator.socket.async_wait(
        boost::asio::socket_base::wait_read,
        [&s, default_handler,
         interface_dispatcher =
             tiny_tuple::map<tiny_tuple::detail::item<typename std::decay_t<Ts>::id, decltype(std::decay_t<Ts>::dispatcher)>...>(
                 tiny_tuple::detail::item<typename std::decay_t<Ts>::id, decltype(std::decay_t<Ts>::dispatcher)>(
                     std::move(ts.dispatcher))...)](boost::system::error_code ec) mutable
        {
            if (ec) {}
            else
            {
                // Consider splitting message receival and consumption into two parts:
                // Firstly allow asynchronous message handling - i.e. by posting the the resulting invocation and
                // ensuring that the socket is used strictly synchronously or at least always form a single io_context
                // wake. secondly to allow handling multiple protocols within a server on a single socket. That way
                // versioning of the protocol could be achieved by always prefixing the messages with a protocol id,
                // and or splitting up functionalities into multiple modules might be nicer.
                auto msg = s.communicator.peek_and_receive();

                msg_header header = decode_item(msg, type<msg_header>{});
                detail::forward_item<P>(  //
                    header.id.interface, header.id.id, interface_dispatcher,
                    [&s, &header, &msg](auto& handler, auto const& signature)
                    {
                        using reply_type = detail::just_return_type_t<std::decay_t<decltype(signature)>>;
                        if constexpr (std::is_same_v<void, reply_type>) { detail::decode<std::decay_t<decltype(signature)>>(msg, handler); }
                        else
                        {
                            reply_type reply_value = detail::decode<std::decay_t<decltype(signature)>>(msg, handler);
                            packet     new_msg(msg_header{{header.id.interface, header.id.id, header.id.cookie}, 128, 0});
                            encode_item(new_msg, type<reply_type>{}, reply_value);
                            s.communicator.send(new_msg);
                        }
                    });
                default_handler();
            }
        });
}

template <c::protocol P, c::interface_id I, c::signal_name S, typename... Cs>
requires detail::is_in_protocol<P, I, S>
void send_signal(I, S, server_session& session, Cs&&... params)
{
    using iface     = get_interface<P, I>;
    using signature = detail::get_signature<iface, S>;
    packet new_msg(msg_header{{I::hash, id_of_item<iface, S>, 0}, 128, 0});
    detail::encode<signature>(new_msg, std::forward<Cs>(params)...);
    session.communicator.send(new_msg);
}

template <c::protocol P, c::interface_id I, c::signal_name S, typename... Cs>
requires detail::is_in_protocol<P, I, S>
auto dispatch_signal(I, S, Cs&&... params)
{
    using iface     = get_interface<P, I>;
    using signature = detail::get_signature<iface, S>;
    packet new_msg(msg_header{{I::hash, id_of_item<iface, S>, 0}, 128, 0});
    detail::encode<signature>(new_msg, std::forward<Cs>(params)...);
    new_msg.commit_to_header();
    return [msg_to_dispatch = std::move(new_msg)](server_session& session) { session.communicator.send(&msg_to_dispatch.header); };
}

}  // namespace tiny_ipc

#endif
