#pragma once

#include <support/Union.hh>
#include <support/Utility.hh>

#include <assert.h>
#include <stdint.h>

namespace jamf {

template <typename... Ts>
class Variant {
    static_assert(sizeof...(Ts) < 255, "Variant too large");
    template <typename... Us>
    friend class Variant;

private:
    Union<Ts...> m_union;
    uint8_t m_index;

    template <typename T>
    static consteval auto index_of() {
        return TypeList<Ts...>::template index_of<T>();
    }

    template <typename T>
    bool maybe_destruct() {
        if (m_index != index_of<T>()) {
            return false;
        }
        m_union.template release<T>();
        return true;
    }

    template <typename T, typename... Us>
    bool maybe_move(Variant<Us...> &from) {
        if (from.m_index != Variant<Us...>::template index_of<T>()) {
            return false;
        }
        m_union.template set<T>(move(from.m_union.template get<T>()));
        m_index = from.m_index;
        return true;
    }

    template <typename T, typename... Us>
    bool maybe_downcast(Variant<Us...> &new_variant) const {
        if (m_index != index_of<T>()) {
            return false;
        }
        if constexpr (TypeList<Us...>::template contains<T>()) {
            new_variant.m_union.template set<T>(m_union.template get<T>());
            new_variant.m_index = Variant<Us...>::template index_of<T>();
            return true;
        }
        return false;
    }

    template <typename... Us>
    struct Downcast {
        Variant<Us...> operator()(const Variant &variant) {
            Variant<Us...> new_variant;
            [[maybe_unused]] bool success = (variant.maybe_downcast<Ts, Us...>(new_variant) || ...);
            assert(success);
            return new_variant;
        }
    };

    template <typename U>
    struct Downcast<U> {
        U operator()(const Variant &variant) { return variant.get<U>(); }
    };

    Variant() = default;

public:
    template <ContainsType<Ts...> T>
    Variant(const T &value) : m_union(value), m_index(index_of<T>()) {}
    template <ContainsType<Ts...> T>
    Variant(T &&value) : m_union(move(value)), m_index(index_of<T>()) {} // NOLINT
    Variant(const Variant &) = delete;
    template <ContainsType<Ts...>... Us>
    Variant(Variant<Us...> &&);
    ~Variant();

    Variant &operator=(const Variant &) = delete;
    Variant &operator=(Variant &&);

    template <ContainsType<Ts...>... Us>
    auto downcast() const;

    template <ContainsType<Ts...> T>
    T &get();
    template <ContainsType<Ts...> T>
    const T &get() const;

    template <ContainsType<Ts...> T>
    bool has() const;

    template <ContainsType<Ts...> T>
    void set(const T &value);
    template <ContainsType<Ts...> T>
    void set(T &&value);

    uint8_t index() const { return m_index; }
};

template <typename... Ts>
template <ContainsType<Ts...>... Us>
Variant<Ts...>::Variant(Variant<Us...> &&other) {
    (maybe_move<Us, Us...>(other) || ...);
}

template <typename... Ts>
Variant<Ts...>::~Variant() {
    (maybe_destruct<Ts>() || ...);
}

template <typename... Ts>
Variant<Ts...> &Variant<Ts...>::operator=(Variant &&other) {
    if (this != &other) {
        (maybe_destruct<Ts>() || ...);
        (maybe_move<Ts, Ts...>(other) || ...);
    }
    return *this;
}

template <typename... Ts>
template <ContainsType<Ts...>... Us>
auto Variant<Ts...>::downcast() const {
    return Downcast<Us...>{}(*this);
}

template <typename... Ts>
template <ContainsType<Ts...> T>
T &Variant<Ts...>::get() {
    assert(m_index == index_of<T>());
    return m_union.template get<T>();
}

template <typename... Ts>
template <ContainsType<Ts...> T>
const T &Variant<Ts...>::get() const {
    assert(m_index == index_of<T>());
    return m_union.template get<T>();
}

template <typename... Ts>
template <ContainsType<Ts...> T>
bool Variant<Ts...>::has() const {
    return m_index == index_of<T>();
}

template <typename... Ts>
template <ContainsType<Ts...> T>
void Variant<Ts...>::set(const T &value) {
    (maybe_destruct<Ts>() || ...);
    m_union.set(value);
    m_index = index_of<T>();
}

template <typename... Ts>
template <ContainsType<Ts...> T>
void Variant<Ts...>::set(T &&value) {
    (maybe_destruct<Ts>() || ...);
    m_union.set(forward<T>(value));
    m_index = index_of<T>();
}

} // namespace jamf
