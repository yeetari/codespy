#include <codespy/ir/Function.hh>

#include <codespy/ir/Type.hh>
#include <codespy/support/Format.hh>

namespace codespy::ir {

Argument::Argument(Type *type, std::uint8_t index) : Value(k_kind, type), m_index(index) {}

Local::Local(Type *type, unsigned index) : Value(k_kind, type), m_index(index) {}

// TODO: Get context from type.
Function::Function(Context &context, String name, FunctionType *type)
    : Value(k_kind, type), m_context(context), m_name(std::move(name)) {
    for (std::uint8_t index = 0; auto *parameter_type : type->parameter_types()) {
        m_arguments.emplace<Argument>(m_arguments.end(), parameter_type, index++);
    }
}

BasicBlock *Function::append_block() {
    return m_blocks.emplace<BasicBlock>(m_blocks.end(), m_context, this);
}

Local *Function::append_local(Type *type) {
    return m_locals.emplace<Local>(m_locals.end(), type, m_locals.size_slow());
}

Argument *Function::argument(std::size_t index) {
    return *std::next(m_arguments.begin(), index);
}

void Function::remove_block(BasicBlock *block) {
    assert(!block->has_uses());
    m_blocks.erase(List<BasicBlock>::iterator(block));
}

void Function::remove_local(Local *local) {
    assert(!local->has_uses());
    m_locals.erase(List<Local>::iterator(local));
}

void Function::set_name_prefix(String name_prefix) {
    m_display_name = codespy::format("{}.{}", name_prefix, m_name);
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
