#ifndef TINY_IPC_SERVER_SESSION_H_INCLUDED
#define TINY_IPC_SERVER_SESSION_H_INCLUDED
#include <algorithm>
#include <tiny_ipc/proto_def.h>
#include <tiny_ipc/detail/protocol.h>
#include <tiny_ipc/detail/packet.h>
#include <tiny_ipc/detail/encode.h>
#include <tiny_ipc/detail/to_item.h>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <tiny_tuple/map.h>

namespace tiny_ipc
{
template <c::protocol P, c::method_handler... Ts>
struct server_session
{
    boost::asio::local::stream_protocol::socket& socket;  // accept on socket ...
    tiny_tuple::map<detail::to_item<Ts>...>      callbacks;
    server_session(boost::asio::local::stream_protocol::socket& s, const P, Ts&&... ts)
        : socket{s}, callbacks{detail::to_item<Ts>{std::move(ts.callable)}...}
    {
    }
};

}  // namespace tiny_ipc

#endif
