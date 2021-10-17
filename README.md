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

```c++
// throughout this documentation we use the short form namespace:
namespace ti = tiny_ipc;
```

### Interface and version

All methods and signals are grouped via interfaces. Every interface has a name and version. 
Name and version combined have to be unique within one protocol. 
```c++
constexpr ti::protocol example_protocol( 
    ti::interface("calculator"_i, "1.0"_v,
      ti::method<int(std::string, int, std::vector<float>)>("calculate"_m),
      ti::signal<void(std::string, std::string)>("important_news"_s))
    );
```
 
### Method

A Method can have arbitrary c++ types as parameters and must have a unique name. 
A method is declared like this:

```c++
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
and needs a c++ function signature for encoding and decoding to work. So in the given
exampe `calculate` needs three parameters to work:

```c++

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

Just like methods a Signal can have arbitrary c++ types as parameters and must have 
a unique name within the interface, but there is no return value transported.
A signal is declared like this:

```c++
constexpr auto example_protocol = ti::protocol( 
  /* ... */
  ti::interface("calculator"_i, "1.0"_v,
  ti::signal<void(std::string, std::string)>("important_news"_s),
  /* ... */
  );
}
```

The signal declaration uses a compile time user defined literal with the suffix `_s`
and needs a c++ function signature for encoding and decoding to work.

### Parameters and Return Values

The library will encode all trivial parameters directly, by just copying the parameter
into the message. This can be easily expanded to user defined types by specializing the
type trait `tiny_ipc::is_trivially_serializable`. 
By default it is implemented like this:
```c++
namespace tiny_ipc
{
template <typename T>
struct is_trivially_serializable : std::is_trivially_copyable<T>
{
};
}
```
To disable the plain copy of a type a specialization is needed: 
```c++
namespace tiny_ipc
{
template <>
struct is_trivially_serializable<YourType> : std::false_type {};
}
```
Any unknown type that is not trivially copyable will require a custom overload
of `encode_item` and `decode_item`:

```c++
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

```c++
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
```c++
struct weak_ref
{
  int file_descriptor;
};

struct fd
{
  explicit fd(int file_handle); // will take ownership and close
  explicit fd(weak_ref do_not_close) // will not take ownership
  fd(fd const&)l
  fd&                  operator=(fd const&);
  inline               operator int() const;
  std::shared_ptr<int> file_descriptor;
};
}
```

A user may use different user defined types, but then has to provide a custom `encode_item` and `decode_item`
overload that treats the type as file descriptor:

```c++
inline CustomFD decode_item(detail::message_parser& msg, type<CustomFD>) { return CustomFD{msg.consume_fd()}; }
```

But note that the file descriptor needs to be duplicated with dup then, because `fd::~fd` would close it.

### Maintaining compatibility with Interfaces and Versions

The encoding scheme is done for each interface individually. Each method and signal is enumerated. 
To allow making changes to an interface another interface version can be added. 
Within the new version different new and different method and signals can be defined.

## Usage

See chat client and server for reference in examples folder.

### How to write a server

Some asio boilerplate is needed:
```c++
#include <tiny_ipc/server_session.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/local/basic_endpoint.hpp>

//...
  path_to_uds = "/run/service/admin";
  ::unlink(path_to_uds);
  boost::asio::io_context                                                 io_ctx;
  boost::asio::local::basic_endpoint<boost::asio::local::stream_protocol> end_point(path_to_uds);
  boost::asio::local::stream_protocol::acceptor                           acceptor(io_ctx, end_point);
```

This creates removes a potential stale unix domain socket file and creates a new one.
The acceptor has the reponsibility to handle incoming connection attempts.

For each successfull connection attempt we will need a way to store the sockets and the library 
structure `server_session` to communicate with the client.
```c++
  struct session_handler
  {
    boost::asio::local::stream_protocol::socket socket;
    tiny_ipc::server_session                    session;
    optional<ucred>                             creds;
    template<typename tiny_ipc::concepts::session_error_handler H>
    explicit session_handler(boost::asio::local::stream_protocol::socket&& s, H && error)
      : socket{std::move(s)}, session(socket, std::forward<H>(error)) {}
  };
  std::vector<std::shared_ptr<session_handler>> sessions;
```

Next up we need to configure the accept handling, the accept signal on the end point will trigger asynchronously once for every
call to `async_accept`, after handling the connection attempt by adding the session structure and triggering the read 
we need to continue waiting for further clients to conenct. This looks recursive but isn't since the second parameter
of `async_accept` is a continuation lambda that will only be executed within the `io_context` thread after a connect happened.

Oh and we are using `shared_ptr` in this example becaue we want the references to `session_handler` remain stable 
even after vector resize operations. Additionally we have to ensure that the objects referred to by continuation functions 
outlive those functions. Consider that the connection is terminated either from the client or within an message handler, meanwhile
an `async_wait` with a reference to our session object still resides in the `boost::asio::io_context`. To avoid dangling references
or simply use after free we use `shared_ptr` here, and store a `shared_ptr` inside the continuations. `close` or `cancel` operations
on the socket will eventually remove outstanding continuaions but untill then the session object needs to stay alive.
```c++
void accept_connection()
{
  acceptor.async_accept(end_point,
    [this](boost::system::error_code ec, boost::asio::local::stream_protocol::socket other)
    {
      if(ec) 
      {
        // this branch is usually hit when the acceptor is disabled during shutdown.
      }
      else 
      {
        sessions.push_back(
          std::make_shared<session_handler>(std::move(other), //
          [](boost::system::error_code ec, tiny_ipc::servier_session &)
          {
            sessions.erase(                                     //
              std::remove_if(begin(sessions), end(sessions),  //
                [&s](auto const& item) { return (&s == &item->session); }),
              end(sessions));
          });

        async_read(sessions.back());
        accept_connections();
      }
    });
}
```
The library requires the user to provide a callback to be executed on errors. Errors in this case are most commonly disconnect events. 
So in the error handler above we use the reference to the session object to remove any pending references within the server. 
Still after this code executes the `session_handler` might still be alive after the next execution of the `io_context`, when all 
outstanding async operations have been cleared.

Now finally the interesting part the actual communication with the client. 
The code below registers method handlers in relation to the versioend interfaces.
Those could of course be declared as std::function objects or mapped to real C++
interfaces. 
```c++
void async_read(std::shared_ptr<session_handler> const& c) 
{
  tiny_ipc::async_dispatch_messages<your_protocol>(  //
    c->session,                                     //

    tiny_ipc::methods_of("your_main_interface"_i, "1.0"_v,

      "connect"_m = [this, c](::ucred cred, std::string const& client_name) -> bool
      {
        if (this->user_manager.test_user(cred))
        {
          user_manager.register_user(cred, client_name);
          c->creds = cred;
          return true;
        }
        else
        {
          c->session.close();
          return false;
        }
      },

      "some_other_method"_m = [this, c](std::string const& data)
      {
        if(!c->creds) return;
        service_manager->perform_something(data);
      }),
        
    tiny_ipc::methods_of("your_main_interface"_i, "1.2"_v,
      "extension_method"_m = [this](int flags, std::string const& data)
      {
        service_manager->extended_action(flags, data);
      })
    );
}
```


### How to write a client

Similar boilerplate code is needed for the client
```c++
#include <tiny_ipc/client.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/local/basic_endpoint.hpp>
#include <boost/asio/completion_condition.hpp>

// ...
  boost::asio::io_context                       io_ctx;
  boost::asio::local::stream_protocol::endpoint ep("/run/service/admin");
  boost::asio::local::stream_protocol::socket   socket(io_ctx);
```

The socket needs to be connected first

```c++
  socket.connect(m_ep);
```

A client structure to keep track then can be constructed. 
It need a reference to the socket and an error handler.
Usually errors are caused by disconnects from the server.

```c++
  tiny_ipc::client my_client(socket,
     [&io_ctx](boost::system::error_code ec, tiny_ipc::client&)
     {
       io_ctx.stop();
     });
```

At some point signal handlers for all known or necessary interfaces
have to be declared. This is similar to the method handlers on 
the server side.

```c++
  async_dispatch_messages<your_protocol>(                       //
    my_client,                                                  //
    tiny_ipc::signals_of(                                       //
      "your_main_interface"_i, "1.0"_v,                         //
      "server_signal"_s = [](std::vector<char> const&) { ... }  //
      ));
    }
```

At some point the io context can be started. 

```c++
  io_ctx.run();
```

## Exposing the protocol to other languages

### Expose via C Interface and type mapping


### Expose encoding scheme by generating stubs

## Dependencies

* CMake CPM
* Kvasir MPL
* Boot Asio
* tiny\_tuple
