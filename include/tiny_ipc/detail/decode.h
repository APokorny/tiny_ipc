#ifndef TINY_IPC_DETAIL_DECODE_H_INCLUDED
#define TINY_IPC_DETAIL_DECODE_H_INCLUDED

#include <tiny_ipc/detail/serialization_utilities.h>

namespace tiny_ipc
{
template <typename U, typename T>
requires(is_trivially_serializable_v<U>&& std::is_same_v<std::decay_t<U>, std::decay_t<T>>) void encode_item(packet& encoded_msg, type<U>,
                                                                                                             T&&     param)
{
    encoded_msg.add_data({&param, sizeof(param)});
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
template <typename T>
requires is_trivially_serializable_v<T> T internal_encode_item(packet& encoded_msg, type<T>)
{
    T ret;
    // memcpy
    return ret;
}

std::string internal_decode_item(packet& decoded_msg, type<std::string>)
{
    std::string str;
    return str;
}

std::string_view internal_decode_item(packet& decoded_msg, type<std::string_view>)
{
    std::string_view str;
    return str
}

template <typename T>
requires(!is_trivially_serializable_v<T>) T internal_decode_item(packet& decoded_msg, type<T>)
{
    return tiny_ipc::decode_item(decoded_msg, type<T>{});
}
template <typename... ListItems, typename F>
void decode_items(packet& decoded_msg, kvasir::mpl::list<ListItems...>, F&& fun)
{
    fun(internal_decode_item(decoded_msg, type<ListItems>{})...);
}
}  // namespace impl

template <typename Signature, typename F>
void decode(packet& decoded_msg, F&& fun)
{
    using signature_list = typename impl::to_list<Signature>::type;
    impl::decode_items(decoded_msg, signature_list{}, std::forward<F>(fun));
}
}  // namespace detail
}  // namespace tiny_ipc

#endif
