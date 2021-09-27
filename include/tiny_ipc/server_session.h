#ifndef TINY_IPC_SERVER_SESSION_H_INCLUDED
#define TINY_IPC_SERVER_SESSION_H_INCLUDED
#include <tiny_ipc/proto_def.h>
#include <tiny_ipc/detail/protocol.h>
#include <boost/asio/local/stream_protocol.hpp>
#include <tiny_tuple/tuple.h>

namespace tiny_ipc
{
template <c::protocol P, c::method_handler... Ts>
struct server_session
{
    boost::asio::local::stream_protocol::socket& socket;  // accept on socket ...
    tiny_tuple::tuple<Ts...>                     callbacks;
    server_session(boost::asio::local::stream_protocol::socket & s, Ts && ... ts) : socket{s}, callbacks{std::move(ts)...} {}
};

// <MSG_ID == 0> <PAYLOAD_SIZE> <VERSION_STR>
// <MSG_ID> <PAYLOAD_SIZE> ...

}  // namespace tiny_ipc

#endif
