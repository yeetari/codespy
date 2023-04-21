#pragma once

#include <support/Utility.hh>

#include <utility>

#include <stddef.h>

namespace jamf {

template <size_t>
struct TupleTag {};

template <size_t I, typename T>
struct TupleElem {
    static T elem_type(TupleTag<I>);
    [[no_unique_address]] T value;
    constexpr decltype(auto) operator[](TupleTag<I>) & { return (value); }
};

template <typename, typename...>
struct TupleBase;
template <size_t... Is, typename... Ts>
struct TupleBase<IntegerSequence<size_t, Is...>, Ts...> : TupleElem<Is, Ts>... {
    using TupleElem<Is, Ts>::elem_type...;
    using TupleElem<Is, Ts>::operator[]...;

    TupleBase() = default;
    explicit TupleBase(const Ts &...ts) requires(!is_ref<Ts> && ...) : TupleElem<Is, Ts>{ts}... {}
    explicit TupleBase(Ts &&...ts) : TupleElem<Is, Ts>{forward<Ts>(ts)}... {}
};

template <typename... Ts>
using tuple_sequence_t = make_integer_sequence<size_t, sizeof...(Ts)>;

template <typename... Ts>
struct Tuple : TupleBase<tuple_sequence_t<Ts...>, Ts...> {
    using TupleBase<tuple_sequence_t<Ts...>, Ts...>::TupleBase;
};

template <size_t I, typename T>
constexpr decltype(auto) get(T &&tuple) {
    return tuple[TupleTag<I>()];
}

template <typename... Ts>
constexpr auto forward_as_tuple(Ts &&...ts) {
    return Tuple<Ts &&...>(forward<Ts>(ts)...);
}

template <typename... Ts>
constexpr auto make_tuple(Ts &&...ts) {
    return Tuple<decay_unwrap<Ts>...>(forward<Ts>(ts)...);
}

} // namespace jamf

namespace std {

template <size_t I, typename... Ts>
struct tuple_element<I, jamf::Tuple<Ts...>> {
    using type = decltype(jamf::Tuple<Ts...>::elem_type(jamf::TupleTag<I>()));
};

template <typename... Ts>
struct tuple_size<jamf::Tuple<Ts...>> {
    static constexpr size_t value = sizeof...(Ts);
};

} // namespace std
