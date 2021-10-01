#ifndef TINY_IPC_DETAIL_FORWARD_ITEM_H_INCLUDED
#define TINY_IPC_DETAIL_FORWARD_ITEM_H_INCLUDED
#include <tiny_tuple/map.h>
#include <tiny_ipc/proto_def.h>  // id of item and name of

namespace tiny_ipc::detail
{
template <c::protocol P, typename... Ts, typename F>
void forward_item(std::size_t id, tiny_tuple::map<Ts...>& ts, F&& f)
{
    int execute_f_on_matching_id[] = {((id == id_of_item<P, name_of<Ts>>) ? (f(tiny_tuple::get<name_of<Ts>>(ts)),0) : 0)...};
    (void)execute_f_on_matching_id;
}
}  // namespace tiny_ipc::detail

#endif
