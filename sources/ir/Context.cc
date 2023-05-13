#include <codespy/ir/Context.hh>

namespace codespy::ir {

Context::Context() : m_constant_null(ValueKind::ConstantNull, reference_type("java/lang/Object")) {}

ArrayType *Context::array_type(Type *element_type) {
    if (!m_array_types.contains(element_type)) {
        m_array_types.emplace(element_type, codespy::make_unique<ArrayType>(element_type));
    }
    return m_array_types.at(element_type).ptr();
}

FunctionType *Context::function_type(Type *return_type, Vector<Type *> &&parameter_types) {
    auto &slot = m_function_types[FunctionTypeKey{return_type, parameter_types.span()}];
    if (!slot) {
        slot = codespy::make_unique<FunctionType>(return_type, std::move(parameter_types));
    }
    return slot.ptr();
}

IntType *Context::int_type(std::uint16_t bit_width) {
    if (!m_int_types.contains(bit_width)) {
        m_int_types.emplace(bit_width, codespy::make_unique<IntType>(bit_width));
    }
    return m_int_types.at(bit_width).ptr();
}

ReferenceType *Context::reference_type(String class_name) {
    if (!m_reference_types.contains(class_name)) {
        m_reference_types.emplace(class_name, codespy::make_unique<ReferenceType>(class_name));
    }
    return m_reference_types.at(class_name).ptr();
}

ConstantDouble *Context::constant_double(double value) {
    auto &slot = m_double_constants[value];
    if (!slot) {
        slot = codespy::make_unique<ConstantDouble>(&m_double_type, value);
    }
    return slot.ptr();
}

ConstantFloat *Context::constant_float(float value) {
    auto &slot = m_float_constants[value];
    if (!slot) {
        slot = codespy::make_unique<ConstantFloat>(&m_float_type, value);
    }
    return slot.ptr();
}

ConstantInt *Context::constant_int(IntType *type, std::int64_t value) {
    auto &slot = m_int_constants[ConstantIntKey{value, type->bit_width()}];
    if (!slot) {
        slot = codespy::make_unique<ConstantInt>(type, value);
    }
    return slot.ptr();
}

ConstantString *Context::constant_string(String value) {
    if (!m_string_constants.contains(value)) {
        auto *type = reference_type("java/lang/String");
        m_string_constants.emplace(value, codespy::make_unique<ConstantString>(type, value));
    }
    return m_string_constants.at(value).ptr();
}

} // namespace codespy::ir
