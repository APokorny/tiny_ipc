// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TINY_IPC_PACKET_H_INCLUDED
#define TINY_IPC_PACKET_H_INCLUDED

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <utility>
#include <span>
#include <cstring>
#include <vector>

namespace tiny_ipc
{
struct packet
{
    bool                           creds{false};
    std::vector<int>               fds;
    msghdr                         header;
    std::vector<iovec>             iovecs;
    std::vector<std::vector<char>> buffers;
    std::vector<char>              ctrl_buffer;

    packet()                    = default;
    packet(packet const& other) = delete;
    packet& operator=(packet const& other) = delete;
    void    add_fd(int fd) { fds.push_back(fd); }
    void    add_cred() noexcept { creds = true; }
    void    add_data(std::span<char const> const& data)
    {
        if (!buffers.empty() && (buffers.back().capacity() - buffers.back().size()) >= data.size())
        {
        auto& buf_back = buffers.back();
            buf_back.insert(buf_back.end(), data.begin(), data.end());
            iovecs.back().iov_len = buf_back.size();
        }
        else
        {
            buffers.emplace_back(data.size());
            std::memcpy(buffers.back().data(), data.data(), data.size());
            iovecs.push_back(iovec{buffers.back().data(), buffers.back().size()});
        }
    }

    std::span<char> reserve_data(std::size_t count)
    {
        if (!buffers.empty() && (buffers.back().capacity() - buffers.back().size()) >= count)
        {
        auto& buf_back = buffers.back();
            buf_back.resize(count);
            iovecs.back().iov_base = buf_back.data();
            iovecs.back().iov_len = buf_back.size();
            return std::span<char>(buf_back.data() + buf_back.size() - count, count);
        }
        else
        {
            buffers.emplace_back(count);
            iovecs.push_back(iovec{buffers.back().data(), buffers.back().size()});
            return std::span<char>(buffers.back().data(), buffers.back().size());
        }
    }

    msghdr* commit_to_header()
    {
      // todo handle empty iov
        header.msg_name    = nullptr;
        header.msg_namelen = 0;
        header.msg_iov     = iovecs.data();
        header.msg_iovlen  = iovecs.size();
        if (creds || fds.size())
        {
            ctrl_buffer.resize((creds ? (CMSG_SPACE(sizeof(::ucred))) : 0) +  //
                               (fds.size() ? (CMSG_SPACE(fds.size() * sizeof(int))) : 0));
            header.msg_control    = ctrl_buffer.data();
            header.msg_controllen = CMSG_ALIGN(ctrl_buffer.size());
            cmsghdr* first        = nullptr;
            if (creds)
            {
                first             = CMSG_FIRSTHDR(&header);
                first->cmsg_len   = CMSG_LEN(sizeof(ucred));
                first->cmsg_level = CMSG_LEN(sizeof(ucred));
                first->cmsg_type  = SCM_CREDENTIALS;
                ::ucred my_creds{.pid = getpid(), .uid = geteuid(), .gid = getegid()};
                std::memcpy(CMSG_DATA(first), &my_creds, sizeof(my_creds));
            }
            if (fds.size())
            {
                first             = first ? CMSG_NXTHDR(&header, first) : CMSG_FIRSTHDR(&header);
                first->cmsg_len   = CMSG_LEN(fds.size() * sizeof(int));
                first->cmsg_level = SOL_SOCKET;
                first->cmsg_type  = SCM_RIGHTS;
                std::memcpy(CMSG_DATA(first), fds.data(), fds.size() * sizeof(int));
            }
        }
        else
        {
            header.msg_control    = nullptr;
            header.msg_controllen = 0;
        }

        // construct
        return &header;
    }
};
}  // namespace tiny_ipc

#endif
