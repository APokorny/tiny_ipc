#ifndef CHAT_H_INCLUDED
#define CHAT_H_INCLUDED

#include <tiny_ipc/proto_def.hpp>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <sys/socket.h>
#include <string>

namespace chat
{
namespace ti = tiny_ipc;
using namespace ti::literals;

constexpr auto chat = ti::protocol(                                     //
    ti::interface("chat"_i, "1.0"_v,                                    //
                  ti::method<bool(::ucred, std::string)>("connect"_m),  //
                  ti::method<void(std::string)>("send"_m),              //
                  ti::signal<void(std::string)>("text_added"_s)));

using chat_protocol = std::remove_const_t<decltype(chat)>;

}  // namespace chat

#endif
