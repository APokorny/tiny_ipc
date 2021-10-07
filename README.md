# Tiny IPC

This is an IPC library tailored for establishing a simple and fast communication
between two linux processes using unix domain sockets. It does use boost asio types
for easy integration, but uses posix sendmsg and posix recvmsg to perform IO.

It supports fd passing and user credentials via specific signature types.

## Basic concepts and Protocol definition

The communication model is multiple clients and single server. The server provides
methods to invoke, and sends replies. The clients can invoke those methods. The server
can send unsolicited messages to the client - so called signals - not to be confused
with process signals. 

### Method

A Method can have arbitrary C++ types as parameters and must have a unique name. 
A method is declared like this:

```
constexpr auto example_protocol = ti::protocol( 
    /* ... */
    tiny_ipc::method<int(std::string, int, std::vector<float>)>("calculate"_m),   
    /* ... */
    );
}
```

The method declaration uses a compile time user defined literal with the suffix `_m`
and needs a C++ function signature for encoding and decoding to work.


### Signal

Just like methods a Signal can have arbitrary C++ types as parameters and must have 
a unique name, but there is no return value transported.
A signal is declared like this:

```
constexpr auto example_protocol = ti::protocol( 
    /* ... */
    tiny_ipc::signal<void(std::string, std::string)>("important_news"_s),
    /* ... */
    );
}
```

The signal declaration uses a compile time user defined literal with the suffix `_s`
and needs a C++ function signature for encoding and decoding to work.

### Parameters and Return Values

The library will encode all trivial parameters directly, by just copying the parameter
into the message. This can be easily expanded to user defined types by specializing the
type trait `tiny_ipc::is_trivially_serializable`. 
By default it is implemented like this:
```
namespace tiny_ipc
{
template <typename T>
struct is_trivially_serializable : std::is_trivially_copyable<T>
{
};
}
```
To disable the plain copy of a type a specialization is needed: 
```
namespace tiny_ipc
{
template <>
struct is_trivially_serializable<YourType> : std::false_type {};
}
```
Any unknown type that is not trivially copyable will require a custom implementation 
of `encode_item` and `decode_item`:

```
namespace tiny_ipc
{
template <typename T, typename U>
void encode_item(packet& encoded_msg, type<T>, U&& param)

template <typename T>
T decode_item(packet& encoded_msg, type<T>)
}
```

### Special Unix types

File descriptors and user credentials.

## Usage

See chat client and server for reference in examples folder.

## Dependencies

* CMake CPM
* Kvasir MPL
* Boot Asio
* tiny\_tuple
