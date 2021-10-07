// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef TINY_IPC_FD_H_INCLUDED
#define TINY_IPC_FD_H_INCLUDED

#include <unistd.h>
#include <memory>

namespace tiny_ipc
{
// marks a file descriptor to only weakly referencing it
// as a result the fd structure will not close the file descriptor when
// it falls out of scope
struct weak_ref
{
    int file_descriptor;
};

/**
 * Copyable and moveable and reference counting file descriptor wrapper
 */
struct fd
{
    explicit fd(int file_handle)
        : file_descriptor(new int{file_handle},
                          [](int* handle)
                          {
                              if (!handle) return;
                              if (*handle > -1) ::close(*handle);
                              delete handle;
                          })
    {
    }
    explicit fd(weak_ref do_not_close) : file_descriptor(std::make_shared<int>(do_not_close.file_descriptor)) {}
    fd() : fd(-1) {}
    fd(fd&& other) : file_descriptor(std::move(other.file_descriptor)) {}
    fd(fd const&)                = default;
    fd&                  operator=(fd const&) = default;
    inline               operator int() const { return file_descriptor ? *file_descriptor : -1; }
    std::shared_ptr<int> file_descriptor;
};
}  // namespace tiny_ipc

#endif
