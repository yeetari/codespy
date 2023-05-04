#pragma once

#include <codespy/container/ListNode.hh>

#include <concepts>
#include <cstdint>

namespace codespy::ir {

class Type;
class Value;

class Use {
    Use **m_prev;
    Use *m_next;
    Value *m_value{nullptr};

public:
    void add_to_list(Use **list);
    void remove_from_list();
    void set(Value *value);

    Value *value() const { return m_value; }
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
    Local,
};

template <typename T>
concept HasValueKind = requires(T) { static_cast<ValueKind>(T::k_kind); };

class Value {
    friend class Context;

private:
    Use *m_use_list{nullptr};
    Type *m_type{nullptr};
    const ValueKind m_kind;

protected:
    Value(ValueKind kind, Type *type) : m_type(type), m_kind(kind) {}
    ~Value() = default;

public:
    Value(const Value &) = delete;
    Value(Value &&) = delete;

    Value &operator=(const Value &) = delete;
    Value &operator=(Value &&) = delete;

    void add_use(Use &use);
    void destroy();
    void replace_all_uses_with(Value *value);

    template <HasValueKind T>
    T *as();
    template <HasValueKind T>
    const T *as() const;
    template <HasValueKind T>
    bool is() const;

    Type *type() const { return m_type; }
    ValueKind kind() const { return m_kind; }
};

template <HasValueKind T>
T *Value::as() {
    return is<T>() ? static_cast<T *>(this) : nullptr;
}

template <HasValueKind T>
const T *Value::as() const {
    return is<T>() ? static_cast<const T *>(this) : nullptr;
}

template <HasValueKind T>
bool Value::is() const {
    return m_kind == T::k_kind;
}

} // namespace codespy::ir

namespace codespy {

template <std::derived_from<ir::Value> T>
struct ListNodeTraits<T> {
    static void destroy_node(T *value) { value->destroy(); }
};

} // namespace codespy
