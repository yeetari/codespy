#pragma once

#include <codespy/ir/Value.hh>
#include <codespy/support/String.hh>

#include <utility>

namespace codespy::ir {

class ConstantInt : public Value {
    const std::uint64_t m_value;

public:
    static constexpr auto k_kind = ValueKind::ConstantInt;

    ConstantInt(Type *type, std::uint64_t value) : Value(k_kind, type), m_value(value) {}

    std::uint64_t value() const { return m_value; }
};

class ConstantString : public Value {
    const String m_value;

public:
    static constexpr auto k_kind = ValueKind::ConstantString;

    ConstantString(Type *type, String value) : Value(k_kind, type), m_value(std::move(value)) {}

    const String &value() const { return m_value; }
};

} // namespace codespy::ir
