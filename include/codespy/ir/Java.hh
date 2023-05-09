#pragma once

#include <codespy/container/List.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Value.hh>

namespace codespy::ir {

class Context;

class JavaField : public Value, public ListNode {
    String m_name;
    bool m_is_instance;

public:
    static constexpr auto k_kind = ValueKind::JavaField;

    JavaField(String name, Type *type, bool is_instance);

    bool is_instance() const { return m_is_instance; }
    const String &name() const { return m_name; }
};

class JavaClass {
    Context &m_context;
    String m_name;
    List<JavaField> m_fields;
    List<Function> m_methods;

public:
    JavaClass(Context &context, String name);

    JavaField *ensure_field(StringView name, Type *type, bool is_instance);
    Function *ensure_method(StringView name, FunctionType *type);

    const String &name() const { return m_name; }
    const List<JavaField> &fields() const { return m_fields; }
    const List<Function> &methods() const { return m_methods; }
};

} // namespace codespy::ir
