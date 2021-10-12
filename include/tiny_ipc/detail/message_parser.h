// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TINY_IPC_DETAIL_MESSAGE_PARSER_H
#define TINY_IPC_DETAIL_MESSAGE_PARSER_H

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <span>
#include <cstring>
#include <vector>
#include <optional>
#include <tiny_ipc/fd.h>

namespace tiny_ipc::detail
{
struct message_parser
{
    msghdr*              hdr;
    std::span<char>      message_payload;
    std::vector<fd>      fds;
    std::optional<ucred> credentials;
    message_parser(msghdr* header, std::span<char> const& payload) : hdr(header), message_payload(payload)
    {
        for (cmsghdr* control_header = CMSG_FIRSTHDR(hdr); control_header; control_header = CMSG_NXTHDR(hdr, control_header))
        {
            auto data = std::span<char>(reinterpret_cast<char*>(CMSG_DATA(control_header)), control_header->cmsg_len);
            if (control_header->cmsg_type == SCM_RIGHTS)
            {
                fds.reserve(control_header->cmsg_len / sizeof(int));
                while (data.size() >= sizeof(int))
                {
                    int file_desc;
                    std::memcpy(&file_desc, data.data(), sizeof(file_desc));
                    data = data.last(data.size() - sizeof(int));
                    fds.emplace_back(file_desc);
                }
            }
            else if (control_header->cmsg_type == SCM_CREDENTIALS)
            {
                ucred temp_creds;
                std::memcpy(&temp_creds, data.data(), sizeof(temp_creds));
                credentials = temp_creds;
            }
        }
    }

    std::span<char> consume_message(std::size_t size)
    {
        auto ret        = message_payload.first(size);
        message_payload = message_payload.last(message_payload.size() - size);
        return ret;
    }

    std::optional<::ucred> get_cred() { return credentials; }
    fd                     consume_fd()
    {
        if (fds.size() > 0)
        {
            fd ret = fds.front();
            fds.erase(fds.begin());
            return ret;
        }
        else
            return fd{};
    }
};
}  // namespace tiny_ipc::detail
#endif
