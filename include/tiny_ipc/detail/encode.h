#ifndef TINY_IPC_DETAIL_ENCODE_H_INCLUDED
#define TINY_IPC_DETAIL_ENCODE_H_INCLUDED

namespace tiny_ipc::detail
{
template <typename Signature, typename... Ts>
void encode(packet& encoded_msg, Ts&&... params)
{
}
}  // namespace tiny_ipc::detail

#endif
