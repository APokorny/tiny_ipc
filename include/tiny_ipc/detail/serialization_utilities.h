#ifndef TINY_IPC_DETAIL_SERIALIZATION_UTILITIES_H_INCLUDED
#define TINY_IPC_DETAIL_SERIALIZATION_UTILITIES_H_INCLUDED
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
template <>
struct is_trivially_serializable<char const *>
{
    struct type
    {
        static constexpr bool value = false;
    };
};
template <>
struct is_trivially_serializable<char *>
{
    struct type
    {
        static constexpr bool value = false;
    };
};
template <>
struct is_trivially_serializable<std::string_view>
{
    struct type
    {
        static constexpr bool value = false;
    };
};
template <>
struct is_trivially_serializable<::ucred>
{
    struct type
    {
        static constexpr bool value = false;
    };
};
template <>
struct is_trivially_serializable<fd>
{
    struct type
    {
        static constexpr bool value = false;
    };
};
template <typename T>
struct is_trivially_serializable<std::span<T>>
{
    struct type
    {
        static constexpr bool value = false;
    };
};
template <typename T>
constexpr bool is_trivially_serializable_v = is_trivially_serializable<T>::type::value;

namespace detail
{
namespace impl
{
template <typename S>
struct to_list;
template <c::method_name M, typename R, typename... S>
struct to_list<tiny_ipc::impl::method<M, R(S...)>>
{
    using type = kvasir::mpl::list<std::decay_t<S>...>;
};
}
}

#endif
