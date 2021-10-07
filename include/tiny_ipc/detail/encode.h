// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TINY_IPC_DETAIL_ENCODE_H_INCLUDED
#define TINY_IPC_DETAIL_ENCODE_H_INCLUDED

#include <tiny_ipc/detail/serialization_utilities.h>
#include <tiny_ipc/detail/packet.h>

namespace tiny_ipc
{
template <typename U, typename T>
requires(is_trivially_serializable_v<U>&& std::is_same_v<std::decay_t<U>, std::decay_t<T>>) void encode_item(packet& encoded_msg, type<U>,
                                                                                                             T&&     param)
{
    encoded_msg.add_data(std::span<char const>(static_cast<char const*>(static_cast<void const*>(&param)), sizeof(param)));
}

template <typename U, typename T>
requires(is_trivially_serializable_v<U> && !std::is_same_v<std::decay_t<U>, std::decay_t<T>>) void encode_item(packet& encoded_msg, type<U>,
                                                                                                               T&&     param)
{
    U temp = std::forward<T>(param);
    encoded_msg.add_data({&temp, sizeof(temp)});
}

template <typename T>
void encode_item(packet& encoded_msg, type<::ucred>, T&& param)
{
    encoded_msg.add_cred();
}

template <typename T, typename Container>
void encode_item(packet& encoded_msg, type<std::vector<T>>, Container&& param)
{
    encode_item(encoded_msg, type<uint16_t>{}, param.size());
    for (auto const& item : param) encode_item(encoded_msg, type<T>{}, item);
}

template <typename T, typename Container>
void encode_item(packet& encoded_msg, type<std::span<T>>, Container&& param)
{
    encode_item(encoded_msg, type<uint16_t>{}, param.size());
    for (auto const& item : param) encode_item(encoded_msg, type<T>{}, item);
}

template <typename T>
void encode_item(packet& encoded_msg, type<fd>, T&& param)
{
    encoded_msg.add_fd(std::forward<fd>(param));
}

void encode_item(packet& encoded_msg, type<std::string>, std::string const& param)
{
    auto     part   = encoded_msg.reserve_data(sizeof(uint16_t) + param.length());
    uint16_t length = param.length();
    mempcpy(part.data(), &length, sizeof(length));
    mempcpy(part.data() + sizeof(length), param.data(), length);
}

void encode_item(packet& encoded_msg, type<std::string>, char const* param)
{
    uint16_t length = strlen(param);
    auto     part   = encoded_msg.reserve_data(sizeof(uint16_t) + length);
    mempcpy(part.data(), &length, sizeof(length));
    mempcpy(part.data() + sizeof(length), param, length);
}

void encode_item(packet& encoded_msg, type<std::string>, std::string_view const& param)
{
    auto     part   = encoded_msg.reserve_data(sizeof(uint16_t) + param.length());
    uint16_t length = param.length();
    mempcpy(part.data(), &length, sizeof(length));
    mempcpy(part.data() + sizeof(length), param.data(), length);
}
namespace detail
{
namespace impl
{
template <typename T, typename U>
requires is_trivially_serializable_v<T> && std::is_same_v<T, std::decay_t<U>>
int internal_encode_item(packet& encoded_msg, type<T>, U&& param)
{
    encoded_msg.add_data({static_cast<char const*>(&param), sizeof(U)});
    return 0;
}

template <typename T, typename U>
requires(is_trivially_serializable_v<T> && (!std::is_same_v<T, std::decay_t<U>>)) int internal_encode_item(packet& encoded_msg, type<T>,
                                                                                                           U&&     param)
{
    T temp = param;
    encoded_msg.add_data({static_cast<char const*>(&temp), sizeof(U)});
    return 0;
}

template <typename U, typename T>
requires(!is_trivially_serializable_v<T>) int internal_encode_item(packet& encoded_msg, type<T>, U&& param)
{
    tiny_ipc::encode_item(encoded_msg, type<T>{}, std::forward<U>(param));
    return 0;
}
template <typename... ListItems, typename... Ts>
void encode_items(packet& encoded_msg, kvasir::mpl::list<ListItems...>, Ts&&... params)
{
    int ignore[] = {internal_encode_item(encoded_msg, type<ListItems>{}, std::forward<Ts>(params))...};
}
}  // namespace impl

template <typename Signature, typename... Ts>
void encode(packet& encoded_msg, Ts&&... params)
{
    using signature_list = typename detail::impl::to_list<Signature>::type;
    impl::encode_items(encoded_msg, signature_list{}, std::forward<Ts>(params)...);
}
}  // namespace detail
}  // namespace tiny_ipc

#endif
