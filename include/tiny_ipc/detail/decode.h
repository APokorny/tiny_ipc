// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TINY_IPC_DETAIL_DECODE_H_INCLUDED
#define TINY_IPC_DETAIL_DECODE_H_INCLUDED

#include <tiny_ipc/detail/message_parser.h>
#include <tiny_ipc/detail/serialization_utilities.h>
#include <limits>
#include <string>

namespace tiny_ipc
{
// Overload this funcion as needed
template <typename T>
requires is_trivially_serializable_v<T> inline T decode_item(detail::message_parser& msg, type<T>)
{
    // alternatively if we guarantee alignment we could decode by casting - but then there is a dependency on ABI
    T    ret;
    auto data = msg.consume_message(sizeof(ret));
    std::memcpy(&ret, data.data(), data.size());
    return ret;
}

inline ucred decode_item(detail::message_parser& msg, type<ucred>)
{
    auto creds = msg.get_cred();
    if (creds)
        return *creds;
    else
        return ucred{std::numeric_limits<pid_t>::max(), 
          std::numeric_limits<uid_t>::max(), 
          std::numeric_limits<gid_t>::max()};
}

inline fd decode_item(detail::message_parser& msg, type<fd>) { return msg.consume_fd(); }

template <typename T>
inline std::vector<T> decode_item(detail::message_parser& msg, type<std::vector<T>>)
{
    auto           vec_size = decode_item(msg, type<uint16_t>{});
    std::vector<T> ret;
    ret.reserve(vec_size);
    for (int i = 0; i != vec_size; ++i) ret.push_back(decode_item(msg, type<T>{}));
    return ret;
}

inline std::string decode_item(detail::message_parser& msg, type<std::string>)
{
    auto length = decode_item(msg, type<uint16_t>{});
    return std::string(msg.consume_message(length).data(), length);
}

inline std::string_view decode_item(detail::message_parser& msg, type<std::string_view>)
{
    auto length = decode_item(msg, type<uint16_t>{});
    return std::string_view(msg.consume_message(length).data(), length);
}

inline char const* decode_item(detail::message_parser& msg, type<char const*>)
{
    auto length = decode_item(msg, type<uint16_t>{});
    return msg.consume_message(length).data();
}
namespace detail
{
namespace impl
{
template <typename... ListItems, typename F>
inline decltype(std::declval<F>()(std::declval<ListItems>()...)) decode_items(detail::message_parser& msg, kvasir::mpl::list<ListItems...>,
                                                                       F&&                     fun)
{
    return fun(decode_item(msg, type<ListItems>{})...);
}
}  // namespace impl

template <typename Signature, typename F>
inline auto decode(detail::message_parser& msg, F&& fun)
{
    using signature_list = typename impl::to_list<Signature>::type;
    return impl::decode_items(msg, signature_list{}, std::forward<F>(fun));
}
}  // namespace detail
}  // namespace tiny_ipc

#endif
