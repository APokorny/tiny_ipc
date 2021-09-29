#ifndef TINY_IPC_DETAIL_ENCODE_H_INCLUDED
#define TINY_IPC_DETAIL_ENCODE_H_INCLUDED

#include <kvasir/mpl/types/list.hpp>
#include <type_traits>
#include <tiny_ipc/fd.h>

namespace tiny_ipc
{
template <typename T>
struct type
{
};

// Specialize is_trivially_serializable as needed:
// then overload:
// namespace tiny_ipc { void encode_item(packet&, type<YourType>, YourType const&); }
template <typename T>
struct is_trivially_serializable : std::is_trivially_copyable<T>
{
};
template <> struct is_trivially_serializable<char const*> { struct type { static constexpr bool value = false;}; };
template <> struct is_trivially_serializable<char *> { struct type { static constexpr bool value = false;}; };
template <> struct is_trivially_serializable<std::string_view> { struct type { static constexpr bool value = false;}; };
template<> struct is_trivially_serializable<::ucred> { struct type { static constexpr bool value = false;}; };
template<> struct is_trivially_serializable<fd> { struct type { static constexpr bool value = false;}; };
template<typename T> struct is_trivially_serializable<std::span<T>> { struct type { static constexpr bool value = false;}; };
template <typename T>
constexpr bool is_trivially_serializable_v = is_trivially_serializable<T>::type::value;

template <typename T>
void encode_item(packet& encoded_msg, type<uint32_t>, T&& param)
{
}

template <typename T>
void encode_item(packet& encoded_msg, type<::ucred>, T&& param)
{
    encoded_msg.add_cred();
}

template <typename T>
void encode_item(packet& encoded_msg, type<fd>, T&& param)
{
    encoded_msg.add_fd(param);
}

template <typename T>
void encode_item(packet& encoded_msg, type<std::string_view>, T&& param)
{
    encoded_msg.add_fd(param);
}

namespace detail
{
namespace impl
{
template <typename S>
struct to_list;
template <c::method_name M, typename R, typename... S>
struct to_list<tiny_ipc::impl::method<M,R(S...)>>
{
    using type = kvasir::mpl::list<std::decay_t<S>...>;
};
template <typename T, typename U>
requires is_trivially_serializable_v<T> && std::is_same_v<T, std::decay_t<U>>
int internal_encode_item(packet& encoded_msg, type<T>, U&& param)
{
    encoded_msg.add_data({static_cast<char const*>(&param), sizeof(U)});
    return 0;
}

template <typename T, typename U>
requires (is_trivially_serializable_v<T> && (!std::is_same_v<T, std::decay_t<U>>)) int internal_encode_item(packet & encoded_msg, type<T>,
                                                                                                         U&& param)
{
    T temp = param;
    encoded_msg.add_data({static_cast<char const*>(&temp), sizeof(U)});
    return 0;
}

template <typename U, typename T>
requires (!is_trivially_serializable_v<T>) int internal_encode_item(packet & encoded_msg, type<T>, U&& param)
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
    using signature_list = typename impl::to_list<Signature>::type;
    impl::encode_items(encoded_msg, signature_list{}, std::forward<Ts>(params)...);
}
}  // namespace detail
}  // namespace tiny_ipc

#endif
