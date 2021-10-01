#ifndef TINY_IPC_CLIENT_H_INCLUDED
#define TINY_IPC_CLIENT_H_INCLUDED
#include <algorithm>
#include <ranges>
#include <tiny_ipc/proto_def.h>
#include <tiny_ipc/detail/protocol.h>
#include <tiny_ipc/detail/packet.h>
#include <tiny_ipc/detail/encode.h>
#include <tiny_ipc/detail/to_item.h>
#include <tiny_ipc/detail/forward_item.h>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <tiny_tuple/map.h>

namespace tiny_ipc
{
template <c::protocol P, c::signal_handler... Ts>
struct client
{
    // TODO maybe make the protocol configureable
    boost::asio::local::stream_protocol::socket& socket;
    tiny_tuple::map<detail::to_item<Ts>...>      callbacks;
    uint16_t                                     cookie_generator;

    union
    {
        msg_header value;
        char       data[sizeof(msg_header)];
    } header;
    std::vector<char>                                      payload;
    std::function<void(boost::system::error_code, size_t)> handle_header;
    std::function<void(boost::system::error_code, size_t)> handle_payload;
    struct active_request
    {
        msg_id                id;
        std::function<void()> payload_handler;
    };
    std::vector<active_request> active_requests;
    client(boost::asio::local::stream_protocol::socket& s, const P, Ts&&... ts)
        : socket{s},
          callbacks{detail::to_item<Ts>{std::move(ts.callable)}...},
          handle_header(
              [this](boost::system::error_code ec, std::size_t bytes_transfered)
              {
                  if (ec) {}
                  else
                  {
                      auto item_to_drop = std::ranges::remove(std::views::all(active_requests), header.value.id, &active_request::id);
                      if (item_to_drop.empty())
                      {
                          handle_payload = [this](boost::system::error_code ec, std::size_t)
                          {
                              if (ec) {}
                              else
                              {
                                  detail::forward_item<P>(header.value.id.id, callbacks, [](auto&) {});
                                  async_read(socket, boost::asio::buffer(header.data, sizeof(header.data)), handle_header);
                              }
                          };
                      }
                      else
                      {
                          handle_payload =
                              [this, handler = std::move(item_to_drop.begin()->payload_handler)](boost::system::error_code ec, std::size_t)
                          {
                              if (ec) {}
                              else
                              {
                                  handler();
                                  async_read(socket, boost::asio::buffer(header.data, sizeof(header.data)), handle_header);
                              }
                          };
                          active_requests.erase(item_to_drop.begin(), item_to_drop.end());
                      }
                      payload.resize(header.value.payload);
                      async_read(socket, boost::asio::buffer(payload.data(), payload.size()), handle_payload);
                  }
              }),
          handle_payload([](boost::system::error_code, std::size_t) {})
    {
        async_read(socket, boost::asio::buffer(header.data, sizeof(header.data)), handle_header);
    }

    template <c::method_name M, typename ResultHandler, typename... Cs>
    requires detail::is_in_protocol<P, M> && detail::is_convertible_signature<P, M, Cs...>  //
    void execute(M, ResultHandler&& fun, Cs&&... params)
    {
        using signature = detail::get_signature<P, M>;
        packet new_msg;
        detail::encode<signature>(new_msg, std::forward<Cs>(params)...);
        ::sendmsg(socket.native_handle(), new_msg.commit_to_header(), MSG_NOSIGNAL);
    }
};

}  // namespace tiny_ipc
#endif
