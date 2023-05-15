#pragma once

#include <codespy/ir/Value.hh>
#include <codespy/support/String.hh>

#include <utility>

namespace codespy::ir {

class ConstantDouble : public Value {
    const double m_value;

public:
    static constexpr auto k_kind = ValueKind::ConstantDouble;

    ConstantDouble(Type *type, double value) : Value(k_kind, type), m_value(value) {}

    double value() const { return m_value; }
};

class ConstantFloat : public Value {
    const float m_value;

public:
    static constexpr auto k_kind = ValueKind::ConstantFloat;

    ConstantFloat(Type *type, float value) : Value(k_kind, type), m_value(value) {}

    float value() const { return m_value; }
};

class ConstantInt : public Value {
    const std::int64_t m_value;

public:
    static constexpr auto k_kind = ValueKind::ConstantInt;

    ConstantInt(Type *type, std::int64_t value) : Value(k_kind, type), m_value(value) {}

    std::int64_t value() const { return m_value; }
};

class ConstantString : public Value {
    const String m_value;

public:
    static constexpr auto k_kind = ValueKind::ConstantString;

    ConstantString(Type *type, String value) : Value(k_kind, type), m_value(std::move(value)) {}

    const String &value() const { return m_value; }
};

class PoisonValue : public Value {
public:
    static constexpr auto k_kind = ValueKind::Poison;

    explicit PoisonValue(Type *type) : Value(k_kind, type) {}
};

} // namespace codespy::ir
