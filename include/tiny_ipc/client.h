#ifndef TINY_IPC_CLIENT_H_INCLUDED
#define TINY_IPC_CLIENT_H_INCLUDED
#include <tiny_ipc/proto_def.h>
#include <tiny_ipc/detail/protocol.h>
#include <tiny_ipc/detail/packet.h>
#include <boost/asio/local/stream_protocol.hpp>
#include <tiny_tuple/tuple.h>

namespace tiny_ipc
{
template <c::protocol P, c::signal_handler... Ts>
struct client
{
    // TODO maybe make the protocol configureable
    boost::asio::local::stream_protocol::socket& socket;
    tiny_tuple::tuple<Ts...>                     callbacks;
    client(boost::asio::local::stream_protocol::socket& s, Ts&&... ts) : socket{s}, callbacks{std::move(ts)...} {}
    enum state
    {
        start,
        end
    };

    template <c::method_name M, typename ResultHandler, typename... Cs>
    requires detail::is_in_protocol<P, M> && detail::is_convertible_signature<P, M, Cs...>  //
    void execute(ResultHandler && fun, Cs&&... params)
    {
        using signature = detail::get_signature<P, M>;
        packet new_msg;
        encode<signature>(new_msg, std::forward<Cs>(params)...);
        ::sendmsg(socket.native_handle(), new_msg.commit_to_header(), MSG_NOSIGNAL);
    }
    state           protocol_state{start};
    //std::function<> stuff;
};

}  // namespace tiny_ipc
#endif
