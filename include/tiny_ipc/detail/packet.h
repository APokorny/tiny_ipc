#ifndef TINY_IPC_PACKET_H_INCLUDED
#define TINY_IPC_PACKET_H_INCLUDED

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <utility>
#include <span>

namespace tiny_ipc
{
struct packet
{
    struct iovec           iov;
    std::optional<::ucred> creds;
    std::vector<int>       fds;
    msghdr                 header;
    cmsghdr                control_msg_header;
    /// ASIO Buffer sequence!
    packet();
    packet(packet const& pther) = delete;
    void add_fd(int fd) { fds.push_back(fd); }
    void add_cred() noexcept { creds = ::ucred{}; }
    void add_data(std::span<char const> const& data)
    {
        // extend iovec..
    }
    std::span<char> reserve_data(std::size_t count)
    {
        // extend iovec..
        return {};
    }
    msghdr* commit_to_header()
    {
        // construct
        return &header;
    }
};
}  // namespace tiny_ipc

#endif
