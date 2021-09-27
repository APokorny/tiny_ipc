# Tiny IPC

This is an IPC library tailored for establishing a simple and fast communication
between two linux processes using unix domain sockets. It does use boost asio types
for easy integration, but uses posix sendmsg and posix recvmsg to perform IO.

It supports fd passing and user credentials via specific signature types.

## Dependencies

* CMake CPM
* Kvasir MPL
* Boot Asio
* tiny\_tuple
