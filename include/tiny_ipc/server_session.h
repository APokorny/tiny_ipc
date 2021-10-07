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
struct server_session
{
    detail::message_comm communicator;
    explicit server_session(boost::asio::local::stream_protocol::socket& s) : communicator{s} {}
};

template <c::protocol P, c::method_handler... Ts>
requires(detail::is_in_protocol<P, typename Ts::name>&&... && true) auto create_async_message_handler(server_session& server,
                                                                                                      Ts&&... ts)
{
    return [&server, callbacks = tiny_tuple::map<detail::to_item<Ts>...>{detail::to_item<Ts>{std::move(ts.callable)}...}](boost::system::error_code ec) mutable
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
            auto msg = server.communicator.peek_and_receive();

            msg_header header = decode_item(msg, type<msg_header>{});
            detail::forward_item<P>(  //
                header.id.id, callbacks,
                [&server, &header, &msg](auto& handler, auto const& signature)
                {
                    using reply_type = detail::just_return_type_t<std::decay_t<decltype(signature)>>;
                    if constexpr (std::is_same_v<void, reply_type>) { detail::decode<std::decay_t<decltype(signature)>>(msg, handler); }
                    else
                    {
                        reply_type reply_value = detail::decode<std::decay_t<decltype(signature)>>(msg, handler);
                        packet     new_msg(msg_header{{header.id.id, header.id.cookie}, 128, 0});
                        encode_item(new_msg, type<reply_type>{}, reply_value);
                        server.communicator.send(new_msg);
                    }
                });
        }
    };
}

template <c::protocol P, c::signal_name S, typename... Cs>
requires detail::is_in_protocol<P, S> && detail::is_convertible_signature<P, S, Cs...>
void send_signal(server_session& session, Cs&&... params)
{
    using signature = detail::get_signature<P, S>;
    packet new_msg(msg_header{{id_of_item<P, S>, 0}, 128, 0});
    detail::encode<signature>(new_msg, std::forward<Cs>(params)...);
    session.communicator.send(new_msg);
}

}  // namespace tiny_ipc

#endif
