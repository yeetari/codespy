#pragma once

#include <codespy/support/Utility.hh>

#include <cassert>
#include <utility>

namespace codespy {

template <typename T>
class [[nodiscard]] UniquePtr {
    T *m_ptr{nullptr};

public:
    constexpr UniquePtr() = default;
    explicit UniquePtr(T *ptr) : m_ptr(ptr) {}
    UniquePtr(const UniquePtr &) = delete;
    UniquePtr(UniquePtr &&other) : m_ptr(other.disown()) {}
    template <typename U>
    UniquePtr(UniquePtr<U> &&other) : m_ptr(other.disown()) {}
    ~UniquePtr() { clear(); }

    UniquePtr &operator=(const UniquePtr &) = delete;
    UniquePtr &operator=(UniquePtr &&);

    void clear();
    T *disown();

    explicit operator bool() const { return m_ptr != nullptr; }
    bool has_value() const { return m_ptr != nullptr; }

    T &operator*() const;
    T *operator->() const;
    T *ptr() const { return m_ptr; }
};

template <typename T>
UniquePtr<T> &UniquePtr<T>::operator=(UniquePtr &&other) {
    UniquePtr moved(std::move(other));
    std::swap(m_ptr, moved.m_ptr);
    return *this;
}

template <typename T>
void UniquePtr<T>::clear() {
    delete m_ptr;
    m_ptr = nullptr;
}

template <typename T>
T *UniquePtr<T>::disown() {
    return std::exchange(m_ptr, nullptr);
}

template <typename T>
T &UniquePtr<T>::operator*() const {
    assert(m_ptr != nullptr);
    return *m_ptr;
}

template <typename T>
T *UniquePtr<T>::operator->() const {
    assert(m_ptr != nullptr);
    return m_ptr;
}

template <typename T>
UniquePtr<T> adopt_unique(T *ptr) {
    return UniquePtr<T>(ptr);
}

template <typename T, typename... Args>
UniquePtr<T> make_unique(Args &&...args) {
    return UniquePtr<T>(new T(std::forward<Args>(args)...));
}

} // namespace codespy
