// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TINY_IPC_DETAIL_FORWARD_ITEM_H_INCLUDED
#define TINY_IPC_DETAIL_FORWARD_ITEM_H_INCLUDED
#include <tiny_tuple/map.h>
#include <tiny_ipc/proto_def.hpp>  // id of item and name of
#include <tiny_ipc/detail/protocol.hpp>

namespace tiny_ipc::detail
{
template <c::interface I, typename... Ts, typename F>
void forward_item(std::size_t id, tiny_tuple::map<Ts...>& ts, F&& f)
{
    int execute_f_on_matching_id[] = {
        ((id == id_of_item<I, name_of<Ts>>) ? (f(tiny_tuple::get<name_of<Ts>>(ts), get_signature<I, name_of<Ts>>{}), 0) : 0)...};
    (void)execute_f_on_matching_id;
}

template <c::protocol P, typename... Ts, typename F>
void forward_item(uint32_t interface_id, uint16_t id, tiny_tuple::map<Ts...>& ts, F&& f)
{
    int execute_f_on_matching_id[] = {
        ((interface_id == name_of<Ts>::hash)
             ? (forward_item<get_interface<P, name_of<Ts>>>(id, tiny_tuple::get<name_of<Ts>>(ts), std::forward<F>(f)), 0)
             : 0)...};
    (void)execute_f_on_matching_id;
}
}  // namespace tiny_ipc::detail

#endif
