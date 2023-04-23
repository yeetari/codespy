#include <codespy/ir/Function.hh>

#include <codespy/ir/Type.hh>

namespace codespy::ir {

Argument::Argument(Type *type) : Value(k_kind, type) {}

// TODO: Get context from type.
Function::Function(Context &context, String name, FunctionType *type)
    : Value(k_kind, type), m_context(context), m_name(std::move(name)) {
    for (auto *parameter_type : type->parameter_types()) {
        m_arguments.emplace<Argument>(m_arguments.end(), parameter_type);
    }
}

BasicBlock *Function::append_block() {
    return m_blocks.emplace<BasicBlock>(m_blocks.end(), m_context);
}

Argument *Function::argument(std::size_t index) {
    auto it = m_arguments.begin();
    std::advance(it, index);
    return *it;
}

FunctionType *Function::function_type() const {
    return static_cast<FunctionType *>(type());
}

} // namespace codespy::ir
