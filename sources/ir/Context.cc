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
    for (const auto &type : m_function_types) {
        if (type->return_type() != return_type) {
            continue;
        }
        if (type->parameter_types().size() != parameter_types.size()) {
            continue;
        }
        bool match = true;
        for (std::size_t i = 0; i < parameter_types.size(); i++) {
            match &= type->parameter_types()[i] == parameter_types[i];
        }
        if (match) {
            return type.ptr();
        }
    }
    return m_function_types.emplace(new FunctionType(return_type, std::move(parameter_types))).ptr();
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
    for (const auto &constant : m_double_constants) {
        if (constant->value() == value) {
            return constant.ptr();
        }
    }
    return m_double_constants.emplace(new ConstantDouble(&m_double_type, value)).ptr();
}

ConstantFloat *Context::constant_float(float value) {
    for (const auto &constant : m_float_constants) {
        if (constant->value() == value) {
            return constant.ptr();
        }
    }
    return m_float_constants.emplace(new ConstantFloat(&m_float_type, value)).ptr();
}

ConstantInt *Context::constant_int(IntType *type, std::int64_t value) {
    for (const auto &constant : m_int_constants) {
        if (constant->type() == type && constant->value() == value) {
            return constant.ptr();
        }
    }
    return m_int_constants.emplace(new ConstantInt(type, value)).ptr();
}

ConstantString *Context::constant_string(String value) {
    if (!m_string_constants.contains(value)) {
        auto *type = reference_type("java/lang/String");
        m_string_constants.emplace(value, codespy::make_unique<ConstantString>(type, value));
    }
    return m_string_constants.at(value).ptr();
}

} // namespace codespy::ir
