// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TINY_IPC_DETAIL_PROTOCOL_H_INCLUDED
#define TINY_IPC_DETAIL_PROTOCOL_H_INCLUDED

#include <tiny_ipc/proto_def.h>
#include <kvasir/mpl/algorithm/find_if.hpp>
#include <kvasir/mpl/sequence/size.hpp>
#include <kvasir/mpl/sequence/front.hpp>
#include <kvasir/mpl/types/bool.hpp>
namespace tiny_ipc::detail
{
template <c::element_name N>
struct is_named_element
{
    template <typename Item>
    struct f_impl : kvasir::mpl::false_
    {
    };
    template <typename Sig>
    struct f_impl<tiny_ipc::impl::method<N, Sig>> : kvasir::mpl::true_
    {
    };
    template <typename Sig>
    struct f_impl<tiny_ipc::impl::signal<N, Sig>> : kvasir::mpl::true_
    {
    };
    template <typename Item>
    using f = f_impl<Item>;
};

template <c::protocol P, c::element_name N>
constexpr bool is_in_protocol =
    kvasir::mpl::call<kvasir::mpl::unpack<kvasir::mpl::find_if<is_named_element<N>, kvasir::mpl::size<>>>, P>::value != 0;

template <c::protocol P, c::element_name N>
using get_signature = kvasir::mpl::call<kvasir::mpl::unpack<kvasir::mpl::find_if<is_named_element<N>, kvasir::mpl::front<>>>, P>;

template <c::protocol P, c::element_name N>
using get_signature = kvasir::mpl::call<kvasir::mpl::unpack<kvasir::mpl::find_if<is_named_element<N>, kvasir::mpl::front<>>>, P>;

template <c::protocol P, c::element_name N, typename... Args>
constexpr bool is_convertible_signature = true;
}  // namespace tiny_ipc::detail

#endif
