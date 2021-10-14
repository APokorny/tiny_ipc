# Tiny IPC

This is an IPC library tailored for establishing a simple and fast communication
between two linux processes using unix domain sockets. It does use boost asio types
for easy integration, but uses posix sendmsg and posix recvmsg to perform IO.

Out of the box it supports fd passing and user credentials via specific signature types.
As stated below credentials are directly supported via the linux c library structure
`ucred`. Meanwhile file descriptor shall be passed as `tiny_ipc::fd` instances, because
the underlying type `int` is not distinguishable from other integers.

## Basic concepts and Protocol definition

The communication model is multiple clients and single server. The server provides interfaces
with versions, that may offer methods to invoke, and optionally send back reply values.
The clients can invoke those methods. The server can send unsolicited messages to the client
- so called signals - not to be confused with process signals. 

```
// throughout this documentation we use the short form namespace:
namespace ti = tiny_ipc;
```

### Interface and version

All methods and signals are grouped via interfaces. Every interface has a name and version. 
Name and version combined have to be unique within one protocol. 
```
constexpr ti::protocol example_protocol( 
    ti::interface("calculator"_i, "1.0"_v,
      ti::method<int(std::string, int, std::vector<float>)>("calculate"_m),
      ti::signal<void(std::string, std::string)>("important_news"_s))
    );
```
 
### Method

A Method can have arbitrary C++ types as parameters and must have a unique name. 
A method is declared like this:

```
constexpr ti::protocol example_protocol( 
    /* ... */
    ti::interface("calculator"_i, "1.0"_v,
      tiny_ipc::method<int(std::string, int, std::vector<float>)>("calculate"_m)
      ),
    /* ... */
    );
using calc_server = decltype(example_protocol);
}
```

The method declaration uses a compile time user defined literal with the suffix `_m`
and needs a C++ function signature for encoding and decoding to work. So in the given
exampe `calculate` needs three parameters to work:

```

// declare the interface and version to use
constexpr ti::interface_id calc("calculator"_i, "1.0"_v);

ti::execute_method<calc_server>(calc, "calculate"_m, connection, "example string", 4, std::vector<float>(1.0f,14.0f,123.0f), 
    [](int reply_value){
        std::cout << "The result is: " << reply_value << "\n";
    });

```

Please note, that in the example above, the literal string does not exactly match the signature in the first parameter.
So the caller passes `(char const*, int, std::vector<float> &&)`, while the signature stated `(std::string, int, std::vector<float>)`. 
But this does not affect the operation nor cause an unncessary conversions nor copies even when the signature states parameters as 
values instead of i.e references. 

More details on convenience type promotions and user defined types in the types section.

Besides passing the parameter the caller also has to provide a callback to handle the result value.
The execution of the callback happens within the `asio::io_context` thread, and will only happen when 
the server replied. So yes the callback is copied and the copy will outlive this call.

### Signal

Just like methods a Signal can have arbitrary C++ types as parameters and must have 
a unique name within the interface, but there is no return value transported.
A signal is declared like this:

```
constexpr auto example_protocol = ti::protocol( 
    /* ... */
    ti::interface("calculator"_i, "1.0"_v,
      ti::signal<void(std::string, std::string)>("important_news"_s),
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
Any unknown type that is not trivially copyable will require a custom overload
of `encode_item` and `decode_item`:

```
namespace tiny_ipc
{
void encode_item(packet& encoded_msg, type<YourType>, YourType const& param)

YourType decode_item(packet& encoded_msg, type<YourType>);
}
```

Note that for encoding the type to be encoded is passed separately from the 
value, using the `type<>` wrapper. This allows adding convenience conversions
For strings for example there is a convenience code path that directly transfers
character arrays and the new `std::string_view` without going through a 
`std::string` instance. 

This is achieved via multiple overloads of `encode`:

```
namespace tiny_ipc
{
void encode_item(packet& encoded_msg, type<std::string>, std::string const& param)
{...}

void encode_item(packet& encoded_msg, type<std::string>, std::string_view const& param)
{...}

void encode_item(packet& encoded_msg, type<std::string>, char const* param)
{...}
}
```


### Special Unix types

File descriptors and user credentials.
User credentials have to be verified by the kernel or even converted if the sending process
is in a different user namespace than the receiving process. Hence user credentials
are not transfered as normal payload of the message. Instead they are transfered
in the so called control data of the message. 

This is a detail handled by the library, so users do not have to take this into account, simply use 
`ucred` as a type in the signature. Necessary headers are also included by the library. 

For file descriptors it is slightly different, because they are handled as integers in the linux 
kernel and posix API. So it is necessary to use a distinct type in the signature. Therefor the 
library provides the structure `tiny_ipc::fd`:
```
struct weak_ref // marks
{
    int file_descriptor;
};

struct fd
{
    explicit fd(int file_handle); // will take ownership and close
    explicit fd(weak_ref do_not_close)
    fd(fd const&)                = default;
    fd&                  operator=(fd const&) = default;
    inline               operator int() const { return file_descriptor ? *file_descriptor : -1; }
    std::shared_ptr<int> file_descriptor;
};
}
```

A user may use different user defined types, but then has to provide a custom `encode_item` and `decode_item`
overload that treats the type as file descriptor:

```
inline CustomFD decode_item(detail::message_parser& msg, type<CustomFD>) { return CustomFD{msg.consume_fd()}; }
```

But note that the file descriptor needs to be duplicated with dup then, because `fd::~fd` would close it.

### Maintaining compatibility with Interfaces and Versions

The encoding scheme is done for each interface individually. Each method and signal is enumerated. 
To allow making changes to an interface another interface version can be added. 
Within the new version different new and different method and signals can be defined.

## Usage

See chat client and server for reference in examples folder.

## Exposing the protocol to other languages

### Expose via C Interface and type mapping



### Expose encoding scheme by generating stubs

## Dependencies

* CMake CPM
* Kvasir MPL
* Boot Asio
* tiny\_tuple
