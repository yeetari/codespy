#pragma once

#include <codespy/container/Vector.hh>
#include <codespy/ir/Constant.hh>
#include <codespy/ir/Type.hh>
#include <codespy/support/UniquePtr.hh>

#include <unordered_map>

namespace codespy::ir {

class Context {
    Type m_any_type{TypeKind::Any};
    Type m_label_type{TypeKind::Label};
    Type m_float_type{TypeKind::Float};
    Type m_double_type{TypeKind::Double};
    Type m_void_type{TypeKind::Void};
    std::unordered_map<Type *, UniquePtr<ArrayType>> m_array_types;
    std::unordered_map<std::uint16_t, UniquePtr<IntType>> m_int_types;
    std::unordered_map<String, UniquePtr<ReferenceType>> m_reference_types;
    Vector<UniquePtr<FunctionType>> m_function_types;

    Vector<UniquePtr<ConstantInt>> m_int_constants;
    std::unordered_map<String, UniquePtr<ConstantString>> m_string_constants;

public:
    Type *any_type() { return &m_any_type; }
    Type *label_type() { return &m_label_type; }
    Type *float_type() { return &m_float_type; }
    Type *double_type() { return &m_double_type; }
    Type *void_type() { return &m_void_type; }

    ArrayType *array_type(Type *element_type);
    FunctionType *function_type(Type *return_type, Vector<Type *> &&parameter_types);
    IntType *int_type(std::uint16_t bit_width);
    ReferenceType *reference_type(String class_name);

    ConstantInt *constant_int(IntType *type, std::int64_t value);
    ConstantString *constant_string(String value);
};

} // namespace codespy::ir
