#include <codespy/ir/Function.hh>

#include <codespy/ir/Type.hh>

namespace codespy::ir {

Argument::Argument(Type *type) : Value(k_kind, type) {}

Local::Local(Type *type) : Value(k_kind, type) {}

// TODO: Get context from type.
Function::Function(Context &context, String name, FunctionType *type)
    : Value(k_kind, type), m_context(context), m_name(std::move(name)) {
    for (auto *parameter_type : type->parameter_types()) {
        m_arguments.emplace<Argument>(m_arguments.end(), parameter_type);
    }
}

BasicBlock *Function::append_block() {
    return m_blocks.emplace<BasicBlock>(m_blocks.end(), m_context, this);
}

Local *Function::append_local(Type *type) {
    return m_locals.emplace<Local>(m_locals.end(), type);
}

Argument *Function::argument(std::size_t index) {
    auto it = m_arguments.begin();
    std::advance(it, index);
    return *it;
}

List<BasicBlock>::iterator Function::remove_block(BasicBlock *block) {
    return m_blocks.erase(List<BasicBlock>::iterator(block));
}

BasicBlock *Function::entry_block() const {
    assert(!m_blocks.empty());
    return m_blocks.first();
}

FunctionType *Function::function_type() const {
    return static_cast<FunctionType *>(type());
}

std::uint32_t Function::parameter_count() const {
    return function_type()->parameter_types().size();
}

} // namespace codespy::ir
