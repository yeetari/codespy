#include <codespy/ir/Java.hh>

namespace codespy::ir {

JavaField::JavaField(JavaClass *parent, String name, Type *type, bool is_instance)
    : Value(k_kind, type), m_parent(parent), m_name(std::move(name)), m_is_instance(is_instance) {}

JavaClass::JavaClass(Context &context, String name) : m_context(context), m_name(std::move(name)) {}

JavaField *JavaClass::ensure_field(StringView name, Type *type, bool is_instance) {
    for (auto *field : m_fields) {
        if (field->type() == type && field->is_instance() == is_instance && field->name() == name) {
            return field;
        }
    }
    return m_fields.emplace<JavaField>(m_fields.end(), this, name, type, is_instance);
}

Function *JavaClass::ensure_method(StringView name, FunctionType *type) {
    for (auto *method : m_methods) {
        if (method->function_type() == type && method->name() == name) {
            return method;
        }
    }
    auto *method = m_methods.emplace<Function>(m_methods.end(), m_context, name, type);
    method->set_name_prefix(m_name);
    return method;
}

} // namespace codespy::ir
