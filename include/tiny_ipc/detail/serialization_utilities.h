// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TINY_IPC_DETAIL_SERIALIZATION_UTILITIES_H_INCLUDED
#define TINY_IPC_DETAIL_SERIALIZATION_UTILITIES_H_INCLUDED

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <kvasir/mpl/types/list.hpp>
#include <type_traits>
#include <span>
#include <sys/types.h>
#include <sys/socket.h>
#include <tiny_ipc/fd.h>
#include <tiny_ipc/proto_def.h>

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
struct is_trivially_serializable<char const *> : std::false_type
{
};
template <>
struct is_trivially_serializable<char *> : std::false_type
{
};
template <>
struct is_trivially_serializable<std::string_view> : std::false_type
{
};
template <>
struct is_trivially_serializable<::ucred> : std::false_type
{
};
template <>
struct is_trivially_serializable<fd> : std::false_type
{
};
template <typename T>
struct is_trivially_serializable<std::span<T>> : std::false_type
{
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
template <c::signal_name S, typename R, typename... T>
struct to_list<tiny_ipc::impl::signal<S, R(T...)>>
{
    using type = kvasir::mpl::list<std::decay_t<T>...>;
};
template <typename S>
struct to_return_type;
template <c::method_name M, typename R, typename... S>
struct to_return_type<tiny_ipc::impl::method<M, R(S...)>>
{
    using type = kvasir::mpl::list<std::decay_t<R>>;
};
template <typename S>
struct just_return_type;
template <c::method_name M, typename R, typename... S>
struct just_return_type<tiny_ipc::impl::method<M, R(S...)>>
{
    using type = std::decay_t<R>;
};
}  // namespace impl

template <typename S>
using to_return_type_t = typename impl::to_return_type<S>::type;
template <typename S>
using just_return_type_t = typename impl::just_return_type<S>::type;
}  // namespace detail
}  // namespace tiny_ipc

#endif
