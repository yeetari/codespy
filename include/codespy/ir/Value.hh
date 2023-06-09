#pragma once

#include <codespy/container/ListNode.hh>
#include <codespy/support/IteratorRange.hh>

#include <concepts>
#include <cstdint>
#include <iterator>

namespace codespy::ir {

class Type;
class Value;

class Use {
    // TODO: This should be const.
    Value *m_owner{nullptr};
    Use **m_prev{nullptr};
    Use *m_next{nullptr};
    Value *m_value{nullptr};

public:
    Use() = default;
    Use(const Use &) = delete;
    Use(Use &&) = delete;
    ~Use() {
        if (m_value != nullptr) {
            remove_from_list();
        }
    }

    Use &operator=(const Use &) = delete;
    Use &operator=(Use &&) = delete;

    void add_to_list(Use **list);
    void remove_from_list();
    void set(Value *value);
    void set_owner(Value *owner) { m_owner = owner; }

    Use *next() const { return m_next; }
    Value *owner() const { return m_owner; }
    Value *value() const { return m_value; }
};

class UserIterator {
    Use *m_use;

public:
    explicit UserIterator(Use *use) : m_use(use) {}

    UserIterator &operator++() {
        m_use = m_use->next();
        return *this;
    }
    UserIterator operator++(int) {
        UserIterator tmp = *this;
        ++*this;
        return tmp;
    }

    bool operator==(const UserIterator &other) const { return m_use == other.m_use; }

    bool at_end() const { return m_use == nullptr; }
    Value *operator*() const { return m_use->owner(); }
    Value *operator->() const { return m_use->owner(); }
};

enum class ValueKind : std::uint8_t {
    Argument,
    BasicBlock,
    ConstantDouble,
    ConstantFloat,
    ConstantInt,
    ConstantNull,
    ConstantString,
    Function,
    Instruction,
    JavaField,
    Local,
    Poison,
};

class Value {
    friend class Context;

private:
    Use *m_use_list{nullptr};
    Type *m_type{nullptr};
    const ValueKind m_kind;

protected:
    Value(ValueKind kind, Type *type) : m_type(type), m_kind(kind) {}
    ~Value();

    void set_type(Type *type) { m_type = type; }

public:
    Value(const Value &) = delete;
    Value(Value &&) = delete;

    Value &operator=(const Value &) = delete;
    Value &operator=(Value &&) = delete;

    void add_use(Use &use);
    void destroy();
    void replace_all_uses_with(Value *value);

    auto user_begin() const { return UserIterator(m_use_list); }
    auto user_end() const { return UserIterator(nullptr); } // NOLINT
    auto users() const { return codespy::make_range(user_begin(), user_end()); }
    bool has_uses() const { return user_begin() != user_end(); }

    Type *type() const { return m_type; }
    ValueKind kind() const { return m_kind; }
};

template <typename T>
concept HasValueKind = requires(T) { static_cast<ValueKind>(T::k_kind); };

template <HasValueKind T>
struct ValueIsImpl {
    bool operator()(const Value *value) const {
        return value->kind() == T::k_kind;
    }
};

template <HasValueKind T>
bool value_is(const Value *value) {
    return ValueIsImpl<T>{}(value);
}

template <HasValueKind T>
T *value_cast(Value *value) {
    return value_is<T>(value) ? static_cast<T *>(value) : nullptr;
}

template <HasValueKind T>
const T *value_cast(const Value *value) {
    return value_is<T>(value) ? static_cast<const T *>(value) : nullptr;
}

} // namespace codespy::ir

namespace codespy {

template <std::derived_from<ir::Value> T>
struct ListNodeTraits<T> {
    static void destroy_node(T *value) { value->destroy(); }
};

} // namespace codespy

namespace std {

template <>
struct iterator_traits<codespy::ir::UserIterator> {
    using difference_type = std::ptrdiff_t;
    using value_type = codespy::ir::Value *;
    using pointer = value_type *;
    using reference = value_type &;
    using iterator_category = std::forward_iterator_tag;
};

} // namespace std
