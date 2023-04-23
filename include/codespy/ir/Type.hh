#pragma once

#include <codespy/support/String.hh>

#include <cstdint>

namespace codespy::ir {

enum class TypeKind : std::uint8_t {
    // Primitive types
    Label,
    Float,
    Double,
    Void,

    // Derived types
    Array,
    Function,
    Integer,
    Reference,
};

class Type {
    const TypeKind m_kind;

public:
    explicit Type(TypeKind kind) : m_kind(kind) {}

    TypeKind kind() const { return m_kind; }
};

class ArrayType : public Type {
    Type *const m_element_type;

public:
    explicit ArrayType(Type *element_type) : Type(TypeKind::Array), m_element_type(element_type) {}

    Type *element_type() const { return m_element_type; }
};

class FunctionType : public Type {
    Type *m_return_type;
    Vector<Type *> m_parameter_types;

public:
    FunctionType(Type *return_type, Vector<Type *> &&parameter_types)
        : Type(TypeKind::Function), m_return_type(return_type), m_parameter_types(std::move(parameter_types)) {}

    Type *return_type() const { return m_return_type; }
    const Vector<Type *> &parameter_types() const { return m_parameter_types; }
};

class IntType : public Type {
    const std::uint16_t m_bit_width;

public:
    explicit IntType(std::uint16_t bit_width) : Type(TypeKind::Integer), m_bit_width(bit_width) {}

    std::uint16_t bit_width() const { return m_bit_width; }
};

class ReferenceType : public Type {
    const String m_class_name;

public:
    explicit ReferenceType(String class_name) : Type(TypeKind::Reference), m_class_name(std::move(class_name)) {}

    const String &class_name() const { return m_class_name; }
};

} // namespace codespy::ir
