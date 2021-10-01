#ifndef TINY_IPC_DETAIL_TO_ITEM_H_INCLUDED
#define TINY_IPC_DETAIL_TO_ITEM_H_INCLUDED
#include <tiny_tuple/map.h>

namespace tiny_ipc::detail
{
namespace impl
{
template <typename T>
struct to_item;
template <template <class, class> typename ProtocolItem, typename Name, typename Value>
struct to_item<ProtocolItem<Name, Value>>
{
    using type = tiny_tuple::detail::item<Name, Value>;
};
}  // namespace impl
template <typename T>
using to_item = typename impl::to_item<T>::type;
}  // namespace tiny_ipc::detail

#endif
