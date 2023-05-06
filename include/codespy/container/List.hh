#pragma once

#include <codespy/container/ListNode.hh>
#include <codespy/support/UniquePtr.hh>

#include <concepts>
#include <iterator>
#include <utility>

namespace codespy {

template <typename T>
class ListIterator;

template <typename T>
class List {
    UniquePtr<ListNode> m_sentinel;

public:
    using iterator = ListIterator<T>;

    List();
    List(const List &) = delete;
    List(List &&) noexcept = default;
    ~List();

    List &operator=(const List &) = delete;
    List &operator=(List &&) noexcept = default;

    void clear();
    template <std::derived_from<T> U, typename... Args>
    U *emplace(iterator it, Args &&...args);
    void insert(iterator it, T *elem);
    iterator erase(iterator it);
    iterator erase(iterator first, iterator last);

    iterator begin() const;
    iterator end() const;

    T *first() const { return *begin(); }
    T *last() const { return *(--end()); }

    bool empty() const;
    std::size_t size_slow() const;
};

template <typename T>
class ListIterator {
    ListNode *m_elem;

public:
    explicit ListIterator(ListNode *elem) : m_elem(elem) {}

    ListIterator &operator++() {
        m_elem = m_elem->next();
        return *this;
    }
    ListIterator &operator--() {
        m_elem = m_elem->prev();
        return *this;
    }
    ListIterator operator++(int) {
        ListIterator tmp = *this;
        ++*this;
        return tmp;
    }
    ListIterator operator--(int) {
        ListIterator tmp = *this;
        --*this;
        return tmp;
    }

    bool operator==(const ListIterator &other) const {
        return m_elem == other.m_elem;
    }

    T *operator*() const { return static_cast<T *>(m_elem); }
    T *operator->() const { return static_cast<T *>(m_elem); }
    ListNode *elem() const { return m_elem; }
};

template <typename T>
List<T>::List() {
    m_sentinel = codespy::make_unique<ListNode>();
    m_sentinel->m_prev = m_sentinel.ptr();
    m_sentinel->m_next = m_sentinel.ptr();
}

template <typename T>
List<T>::~List() {
    clear();
}

template <typename T>
void List<T>::clear() {
    erase(begin(), end());
}

template <typename T>
template <std::derived_from<T> U, typename... Args>
U *List<T>::emplace(iterator it, Args &&...args) {
    auto *elem = new U(std::forward<Args>(args)...);
    insert(it, elem);
    return elem;
}

template <typename T>
void List<T>::insert(iterator it, T *elem) {
    auto *prev = it.elem()->prev();
    elem->m_prev = prev;
    elem->m_next = it.elem();
    it.elem()->m_prev = elem;
    prev->m_next = elem;
}

template <typename T>
List<T>::iterator List<T>::erase(iterator it) {
    auto *prev = it->prev();
    auto *next = it->next();
    next->m_prev = prev;
    prev->m_next = next;
    ListNodeTraits<T>::destroy_node(*it++);
    return it;
}

template <typename T>
List<T>::iterator List<T>::erase(iterator first, iterator last) {
    while (first != last) {
        first = erase(first);
    }
    return last;
}

template <typename T>
List<T>::iterator List<T>::begin() const {
    return ++end();
}

template <typename T>
List<T>::iterator List<T>::end() const {
    return iterator(m_sentinel.ptr());
}

template <typename T>
bool List<T>::empty() const {
    return begin() == end();
}

template <typename T>
std::size_t List<T>::size_slow() const {
    return std::distance(begin(), end());
}

} // namespace codespy

namespace std {

template <typename T>
struct iterator_traits<codespy::ListIterator<T>> {
    using difference_type = std::ptrdiff_t;
    using value_type = std::remove_cv<T>;
    using pointer = T *;
    using reference = T &;
    using iterator_category = std::bidirectional_iterator_tag;
};

} // namespace std
