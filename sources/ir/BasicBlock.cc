#include <codespy/ir/BasicBlock.hh>

#include <codespy/ir/Context.hh>
#include <codespy/ir/Function.hh>

namespace codespy::ir {

BasicBlock::BasicBlock(Context &context, Function *parent)
    : Value(k_kind, context.label_type()), m_context(context), m_parent(parent) {}

void BasicBlock::add_handler(Type *exception_type, BasicBlock *target) {
    m_handlers.emplace<ExceptionHandler>(m_handlers.end(), this, exception_type, target);
}

void BasicBlock::remove(Instruction *inst) {
    if (ir::value_is<ExceptionHandler>(inst)) {
        m_handlers.erase(List<ExceptionHandler>::iterator(inst));
        return;
    }
    m_insts.erase(iterator(inst));
}

void BasicBlock::remove_from_parent() {
    m_parent->remove_block(this);
}

BasicBlock *BasicBlock::successor(unsigned index) const {
    const auto successor_count = terminator()->successor_count();
    if (index < successor_count) {
        return terminator()->successor(index);
    }
    return std::next(m_handlers.begin(), index - successor_count)->target();
}

unsigned BasicBlock::successor_count() const {
    return terminator()->successor_count() + m_handlers.size_slow();
}

bool BasicBlock::has_terminator() const {
    return !m_insts.empty() && m_insts.last()->is_terminator();
}

Instruction *BasicBlock::terminator() const {
    assert(has_terminator());
    return m_insts.last();
}

} // namespace codespy::ir
