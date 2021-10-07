// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TINY_IPC_DETAIL_MESSAGE_COMM_H_INCLUDED
#define TINY_IPC_DETAIL_MESSAGE_COMM_H_INCLUDED

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <boost/asio/local/stream_protocol.hpp>
#include <tiny_ipc/detail/packet.h>
#include <tiny_ipc/detail/message_parser.h>

namespace tiny_ipc::detail
{
struct message_comm
{
    boost::asio::local::stream_protocol::socket& socket;
    msg_header                                   receive_header;
    std::vector<char>                            receive_payload;
    std::vector<char>                            receive_ctrl;
    message_comm(boost::asio::local::stream_protocol::socket& s) : socket(s)
    {
        int enable = 1;
        setsockopt(socket.native_handle(), AF_UNIX, SO_PASSCRED, &enable, sizeof(enable));
        setsockopt(socket.native_handle(), AF_UNIX, SO_PASSSEC, &enable, sizeof(enable));
    }

    inline detail::message_parser peek_and_receive() noexcept
    {
        iovec  single_vec{&receive_header, sizeof(msg_header)};
        msghdr received_message{nullptr, 0, &single_vec, 1, nullptr, 0, 0};
        ::recvmsg(socket.native_handle(), &received_message, MSG_PEEK | MSG_TRUNC | MSG_DONTWAIT);

        receive_payload.resize(sizeof(msg_header) + receive_header.payload);
        single_vec = iovec{receive_payload.data(), receive_payload.size()};

        if (receive_header.control != 0)
        {
            receive_ctrl.resize(receive_header.control);
            received_message.msg_control    = receive_ctrl.data();
            received_message.msg_controllen = receive_ctrl.size();
        }

        ::recvmsg(socket.native_handle(), &received_message, MSG_CMSG_CLOEXEC | MSG_DONTWAIT);
        return detail::message_parser(&received_message, {receive_payload.data(), receive_payload.size()});
    }
    inline void send(packet& message) noexcept { ::sendmsg(socket.native_handle(), message.commit_to_header(), MSG_NOSIGNAL); }
};

}  // namespace tiny_ipc::detail

#endif
