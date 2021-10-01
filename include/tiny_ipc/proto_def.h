// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef TINY_IPC_PROTO_DEF_H_INCLUDED
#define TINY_IPC_PROTO_DEF_H_INCLUDED

#include <type_traits>
#include <cstddef>
#include <algorithm>
#include <tiny_tuple/map.h>
#include <kvasir/mpl/types/list.hpp>
#include <kvasir/mpl/algorithm/zip_with.hpp>
#include <kvasir/mpl/sequence/make_sequence.hpp>

#define TINY_IPC_USE_LEGACY_LITERAL_OPERATORS
namespace tiny_ipc
{
template <typename S, typename F>
struct signal_handler;
template <typename M, typename F>
struct method_handler;
#ifdef TINY_IPC_USE_LEGACY_LITERAL_OPERATORS
template <char... String>
struct version_lit
{
};
template <char... String>
struct signal_lit
{
};
template <char... String>
struct method_lit
{
};
template <typename Lit>
struct method_name
{
    template <typename F>
    inline constexpr method_handler<method_name<Lit>, F> operator=(F &&f) noexcept
    {
        return {std::move(f)};
    }
};
template <typename Lit>
struct signal_name
{
    template <typename F>
    inline constexpr signal_handler<signal_name<Lit>, F> operator=(F &&f) noexcept
    {
        return {std::move(f)};
    }
};
template <typename Lit>
struct version
{
};

namespace literals
{
#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored
#endif
template <typename CharT, CharT... String>
constexpr inline method_name<method_lit<String...>> operator""_m() noexcept
{
    return {};
}
template <typename CharT, CharT... String>
constexpr inline version<version_lit<String...>> operator""_v() noexcept
{
    return {};
}
template <typename CharT, CharT... String>
constexpr inline signal_name<signal_lit<String...>> operator""_s() noexcept
{
    return {};
}
}  // namespace literals
#if defined(__clang__)
#pragma GCC diagnostic pop
#endif

#else
template <std::size_t N>
struct str
{
    constexpr str(char const (&b)[N]) { std::copy_n(b, N, data); }
    constexpr str(char const (&b)[N + 1]) { std::copy_n(b, N, data + 1); }
    constexpr auto       operator<=>(str const &) const & = default;
    char                 data[N];
    constexpr char       car() const { return data[0]; }
    constexpr str<N - 1> cdr() const { return str<N - 1>{data}; }
};
template <auto name>
struct method_name
{
    template <typename F>
    inline constexpr method_handler<method_name<name>, F> operator=(F &&f) noexcept
    {
        return {std::move(f)};
    }
};
template <auto name>
struct signal_name
{
    template <typename F>
    inline constexpr signal_handler<signal_name<name>, F> operator=(F &&f) noexcept
    {
        return {std::move(f)};
    }
};
template <auto name>
struct version
{
};

namespace literals
{
template <str name>
constexpr inline auto operator""_m() noexcept
{
    return method_name<name>{};
}
template <str name>
constexpr inline auto operator""_s() noexcept
{
    return signal_name<name>{};
}
template <str name>
constexpr inline auto operator""_v() noexcept
{
    return version<name>{};
}
}  // namespace literals
#endif

using literals::operator""_s;
using literals::operator""_m;
using literals::operator""_v;

namespace impl
{
#ifdef TINY_IPC_USE_LEGACY_LITERAL_OPERATORS
template <typename T, template <class> typename C>
struct is_name : std::false_type
{
};
template <template <class> typename C, typename N>
struct is_name<C<N>, C> : std::true_type
{
};
#else
template <typename T, template <auto> typename C>
struct is_name : std::false_type
{
};
template <template <auto> typename C, auto N>
struct is_name<C<N>, C> : std::true_type
{
};
#endif
template <typename T, template <class...> typename C>
struct is_same_template : std::false_type
{
};
template <template <class...> typename C, class... Args>
struct is_same_template<C<Args...>, C> : std::true_type
{
};
template <typename Sig>
struct is_signature
{
    static constexpr bool value = false;
};

template <typename R, typename... Ps>
struct is_signature<R(Ps...)>
{
    static constexpr bool value = true;
};
}  // namespace impl

template <typename T>
constexpr bool is_method_name = impl::is_name<T, method_name>::type::value;
template <typename T>
constexpr bool is_signal_name = impl::is_name<T, signal_name>::type::value;
template <typename T>
constexpr bool is_version = impl::is_name<T, version>::type::value;
template <typename T>
constexpr bool is_signature = impl::is_signature<T>::value;

namespace concepts
{
template <typename T>
concept method_name = is_method_name<T>;
template <typename T>
concept signal_name = is_signal_name<T>;
template <typename T>
concept version = is_version<T>;
template <typename T>
concept signature = is_signature<T>;
}  // namespace concepts

namespace c = concepts;
namespace impl
{
template <c::method_name Name, c::signature Sig>
struct method
{
    using name      = Name;
    using signature = Sig;
};
template <c::signal_name Name, c::signature Sig>
struct signal
{
    using name      = Name;
    using signature = Sig;
};

}  // namespace impl

template <typename T>
constexpr bool is_method = impl::is_same_template<T, impl::method>::type::value;
template <typename T>
constexpr bool is_signal = impl::is_same_template<T, impl::signal>::type::value;

namespace concepts
{
template <typename T>
concept method = is_method<T>;
template <typename T>
concept signal = is_signal<T>;
}  // namespace concepts

template <c::signature Sig, c::method_name Name>
constexpr impl::method<Name, Sig> method(Name &&) noexcept
{
    return {};
}
template <c::signature Sig, c::signal_name Name>
constexpr impl::signal<Name, Sig> signal(Name &&) noexcept
{
    return {};
}

template <typename T>
constexpr bool is_interface_param = is_method<T> || is_signal<T>;

namespace concepts
{
template <typename T>
concept interface_param = is_interface_param<T>;
}  // namespace concepts

namespace impl
{
struct to_item
{
    template <typename A, typename B>
    using f = tiny_tuple::detail::item<A, B>;
};
};  // namespace impl

// TODO find a way to transparently introduce versions wihout messing up the
// sequence or restricting the protocol to much
template <c::interface_param... Args>
struct protocol
{
    constexpr protocol(Args &&...) noexcept {}
    using protocol_ids = kvasir::mpl::call<                                                          //
        kvasir::mpl::zip_with<impl::to_item, kvasir::mpl::cfe<tiny_tuple::map>>,                     //
        kvasir::mpl::list<typename Args::name...>,                                                   //
        kvasir::mpl::eager::make_int_sequence<kvasir::mpl::uint_<sizeof...(Args)>>  //
        >;
};

template <typename SN, typename F>
struct signal_handler
{
    F callable;
};

template <typename MN, typename F>
struct method_handler
{
    F callable;
};

template <typename T>
constexpr bool is_method_handler = impl::is_same_template<T, method_handler>::type::value;
template <typename T>
constexpr bool is_signal_handler = impl::is_same_template<T, signal_handler>::type::value;
template <typename T>
constexpr bool is_protocol = impl::is_same_template<T, protocol>::type::value;

namespace concepts
{
template <typename T>
concept method_handler = is_method_handler<T>;
template <typename T>
concept signal_handler = is_signal_handler<T>;
template <typename T>
concept protocol = is_protocol<T>;
};  // namespace concepts

template<c::protocol P,typename T>
constexpr std::size_t id_of_item = tiny_tuple::value_type<T, typename P::protocol_ids>::type::value::value;
template<typename T>
using name_of = typename T::key;
}  // namespace tiny_ipc

#endif
