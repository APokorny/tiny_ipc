// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TINY_IPC_DETAIL_PROTOCOL_H_INCLUDED
#define TINY_IPC_DETAIL_PROTOCOL_H_INCLUDED

#include <tiny_ipc/proto_def.h>
#include <kvasir/mpl/algorithm/all.hpp>
#include <kvasir/mpl/algorithm/find_if.hpp>
#include <kvasir/mpl/sequence/size.hpp>
#include <kvasir/mpl/sequence/front.hpp>
#include <kvasir/mpl/types/bool.hpp>
#include <boost/system/error_code.hpp>
#include <utility>

namespace tiny_ipc
{
struct msg_id
{
    uint32_t interface;
    uint16_t id;
    uint16_t cookie;
    auto     operator<=>(msg_id const&) const = default;
};
struct msg_header
{
    msg_id   id;
    uint16_t payload;
    uint16_t control;
    auto     operator<=>(msg_header const&) const = default;
};
namespace c = concepts;
namespace detail
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
    template <typename V, typename... Es>
    struct f_impl<tiny_ipc::interface<N, V, Es...>> : kvasir::mpl::true_
    {
    };
    template <typename Item>
    using f = f_impl<Item>;
};

template <c::interface_name N, c::version V>
struct is_interface_version
{
    template <typename Item>
    struct f_impl : kvasir::mpl::false_
    {
    };
    template <typename... Es>
    struct f_impl<tiny_ipc::interface<N, V, Es...>> : kvasir::mpl::true_
    {
    };
    template <typename Item>
    using f = f_impl<Item>;
};

template <c::protocol P, c::interface_id I, c::element_name N>
constexpr bool is_in_protocol =
    kvasir::mpl::call<
        kvasir::mpl::unpack<kvasir::mpl::find_if<is_interface_version<typename I::name, typename I::version>,
                                                 kvasir::mpl::unpack<kvasir::mpl::find_if<is_named_element<N>, kvasir::mpl::size<>>>>>,
        P>::value != 0;

template <c::protocol P, c::interface_id I>
struct is_in_proto
{
    template <typename Name>
    using f = kvasir::mpl::call<
        kvasir::mpl::unpack<kvasir::mpl::find_if<is_interface_version<typename I::name, typename I::version>,
                                                 kvasir::mpl::unpack<kvasir::mpl::find_if<is_named_element<Name>, kvasir::mpl::size<>>>>>,
        P>;
};

template <c::protocol P, c::interface_id I, typename Names>
constexpr bool are_in_protocol = kvasir::mpl::call<kvasir::mpl::unpack<kvasir::mpl::all<is_in_proto<P, I>>>, Names>::value != 0;

template <c::interface I, c::element_name N>
using get_signature = typename kvasir::mpl::call<kvasir::mpl::unpack<kvasir::mpl::find_if<is_named_element<N>, kvasir::mpl::front<>>>, I>;

}  // namespace detail
}  // namespace tiny_ipc

#endif
