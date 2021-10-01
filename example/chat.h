#ifndef CHAT_H_INCLUDED
#define CHAT_H_INCLUDED

#include <tiny_ipc/proto_def.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/socket.h>
namespace chat
{
namespace ti = tiny_ipc;
using namespace ti::literals;

constexpr auto chat = ti::protocol(           //
    ti::method<bool(::ucred)>("connect"_m),   //
    ti::method<void(std::string)>("send"_m),  //
    ti::signal<void(std::string)>("text_added"_s));
using chat_protocol = std::remove_const_t<std::decay_t<decltype(chat)>>;
}  // namespace chat

#endif
