// Copyright (c) 2021 Andreas Pokorny
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
#ifndef TINY_IPC_PROTO_DEF_H_INCLUDED
#define TINY_IPC_PROTO_DEF_H_INCLUDED

#include <type_traits>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <tiny_tuple/map.h>
#include <kvasir/mpl/types/list.hpp>
#include <kvasir/mpl/algorithm/zip_with.hpp>
#include <kvasir/mpl/sequence/make_sequence.hpp>
#include <tiny_ipc/detail/to_item.hpp>

#define TINY_IPC_USE_LEGACY_LITERAL_OPERATORS
namespace tiny_ipc
{
constexpr uint32_t prime32_init  = 0x811c9dc5;
constexpr uint32_t prime32_const = 0x1000193;
namespace impl
{
template <typename S, typename F>
struct signal_callback
{
    using name = S;
    F callable;
};
template <typename M, typename F>
struct method_callback
{
    using name = M;
    F callable;
};
}  // namespace impl
#ifdef TINY_IPC_USE_LEGACY_LITERAL_OPERATORS
template <char... String>
struct version_lit
{
    static constexpr uint32_t hash = (prime32_init ^ ... ^ (static_cast<uint32_t>(String) * prime32_const));
};
template <char... String>
struct signal_lit
{
    static constexpr uint32_t hash = (prime32_init ^ ... ^ (static_cast<uint32_t>(String) * prime32_const));
};
template <char... String>
struct method_lit
{
    static constexpr uint32_t hash = (prime32_init ^ ... ^ (static_cast<uint32_t>(String) * prime32_const));
};
template <char... String>
struct interface_lit
{
    static constexpr uint32_t hash = (prime32_init ^ ... ^ (static_cast<uint32_t>(String) * prime32_const));
};
template <typename Lit>
struct method_name
{
    static constexpr uint32_t hash = Lit::hash;
    template <typename F>
    inline constexpr impl::method_callback<method_name<Lit>, F> operator=(F &&f) noexcept
    {
        return {std::move(f)};
    }
};
template <typename Lit>
struct signal_name
{
    static constexpr uint32_t hash = Lit::hash;
    template <typename F>
    inline constexpr impl::signal_callback<signal_name<Lit>, F> operator=(F &&f) noexcept
    {
        return {std::move(f)};
    }
};
template <typename Lit>
struct interface_name
{
    static constexpr uint32_t hash = Lit::hash;
};
template <typename Lit>
struct version
{
    static constexpr uint32_t hash = Lit::hash;
};

namespace literals
{
#if defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wgnu-string-literal-operator-template"
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
template <typename CharT, CharT... String>
constexpr inline interface_name<interface_lit<String...>> operator""_i() noexcept
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
    constexpr uint32_t hash() noexcept
    {
        uint32_t hash = prime32_init;
        for (char const c : data) hash = hash ^ static_cast<uint32_t>(c) * prime32_const;
        return hash;
    }
    constexpr str(char const (&b)[N]) { std::copy_n(b, N, data); }
    constexpr str(char const (&b)[N + 1]) { std::copy_n(b, N, data + 1); }
    constexpr auto       operator<=>(str const &) const &= default;
    char                 data[N];
    constexpr char       car() const { return data[0]; }
    constexpr str<N - 1> cdr() const { return str<N - 1>{data}; }
};
template <auto name>
struct method_name
{
    static constexpr uint32_t hash = name.hash();
    template <typename F>
    inline constexpr impl::method_callback<method_name<name>, F> operator=(F &&f) noexcept
    {
        return {std::move(f)};
    }
};
template <auto name>
struct signal_name
{
    static constexpr uint32_t hash = name.hash();
    template <typename F>
    inline constexpr impl::signal_callback<signal_name<name>, F> operator=(F &&f) noexcept
    {
        return {std::move(f)};
    }
};
template <auto name>
struct interface_name
{
    static constexpr uint32_t hash = name.hash();
};
template <auto name>
struct version
{
    static constexpr uint32_t hash = name.hash();
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
template <str name>
constexpr inline auto operator""_i() noexcept
{
    return interface_name<name>{};
}
}  // namespace literals
#endif

using literals::operator""_s;
using literals::operator""_m;
using literals::operator""_v;
using literals::operator""_i;

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
template <template <class...> typename C, class... Args>
struct is_same_template<const C<Args...> &, C> : std::true_type
{
};
template <template <class...> typename C, class... Args>
struct is_same_template<C<Args...> &, C> : std::true_type
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
constexpr bool is_interface_name = impl::is_name<T, interface_name>::type::value;
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
concept interface_name = is_interface_name<T>;
template <typename T>
concept element_name = is_signal_name<T> || is_method_name<T>;
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
    static constexpr uint32_t hash = Name::hash;
    using name                     = Name;
    using signature                = Sig;
};
template <c::signal_name Name, c::signature Sig>
struct signal
{
    static constexpr uint32_t hash = Name::hash;
    using name                     = Name;
    using signature                = Sig;
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

template <c::interface_name N, c::version V>
struct interface_id
{
    using name                     = N;
    using version                  = V;
    static constexpr uint32_t hash = V::hash ^ N::hash;
    interface_id(N, V) {}
};

template <c::interface_name N, c::version V, c::interface_param... Args>
struct interface
{
    using id                       = interface_id<N, V>;
    static constexpr uint32_t hash = V::hash ^ N::hash;
    constexpr interface(N &&n, V &&v, Args &&...) noexcept {}
    using ms_ids = kvasir::mpl::call<                                               //
        kvasir::mpl::zip_with<impl::to_item, kvasir::mpl::cfe<tiny_tuple::map>>,    //
        kvasir::mpl::list<typename Args::name...>,                                  //
        kvasir::mpl::eager::make_int_sequence<kvasir::mpl::uint_<sizeof...(Args)>>  //
        >;
};

template <typename T>
constexpr bool is_interface = impl::is_same_template<T, interface>::type::value;

template <typename T>
constexpr bool is_interface_id = impl::is_same_template<T, interface_id>::type::value;

template <typename T>
constexpr bool is_method_callback = impl::is_same_template<T, impl::method_callback>::type::value;
template <typename T>
constexpr bool is_signal_callback = impl::is_same_template<T, impl::signal_callback>::type::value;

namespace concepts
{
template <typename T>
concept method_callback = is_method_callback<T>;
template <typename T>
concept signal_callback = is_signal_callback<T>;
template <typename T>
concept interface = is_interface<T>;
template <typename T>
concept interface_id = is_interface_id<T>;
}  // namespace concepts

template <c::interface... Args>
struct protocol
{
    using interfaces = tiny_tuple::map<tiny_tuple::detail::item<typename Args::id, Args>...>;
    constexpr protocol(Args &&...) noexcept {}
};

template <c::interface_name IN, c::version IV, c::signal_callback... callbacks>
struct signals_of
{
    using name    = IN;
    using version = IV;
    using id      = interface_id<IN, IV>;
    using signals = kvasir::mpl::list<typename callbacks::name...>;
    tiny_tuple::map<detail::to_item<callbacks>...> dispatcher;
    signals_of(IN, IV, callbacks &&...cs) : dispatcher{detail::to_item<callbacks>{std::move(cs.callable)}...} {};
};

template <c::interface_name IN, c::version IV, c::method_callback... callbacks>
struct methods_of
{
    using name    = IN;
    using version = IV;
    using id      = interface_id<IN, IV>;
    using methods = kvasir::mpl::list<typename callbacks::name...>;
    tiny_tuple::map<detail::to_item<callbacks>...> dispatcher;
    methods_of(IN, IV, callbacks &&...cs) : dispatcher{detail::to_item<callbacks>{std::move(cs.callable)}...} {};
};

template <typename T>
constexpr bool is_protocol = impl::is_same_template<T, protocol>::type::value;
template <typename T>
constexpr bool is_signal_group = impl::is_same_template<T, signals_of>::type::value;
template <typename T>
constexpr bool is_method_group = impl::is_same_template<T, methods_of>::type::value;

namespace concepts
{
template <typename T>
concept method_group = is_method_group<T>;
template <typename T>
concept signal_group = is_signal_group<T>;
template <typename T>
concept protocol = is_protocol<T>;
};  // namespace concepts

template <c::interface I, typename T>
constexpr uint16_t id_of_item = tiny_tuple::value_type<T, typename I::ms_ids>::type::value::value;
template <c::protocol P, c::interface_id I>
using get_interface = typename tiny_tuple::value_type<I, typename P::interfaces>::type::value;
template <typename T>
using name_of = typename T::key;
}  // namespace tiny_ipc

#endif
