#include <codespy/ir/BasicBlock.hh>

#include <codespy/ir/Context.hh>
#include <codespy/ir/Function.hh>

namespace codespy::ir {

BasicBlock::BasicBlock(Context &context, Function *parent)
    : Value(k_kind, context.label_type()), m_context(context), m_parent(parent) {}

BasicBlock::iterator BasicBlock::remove(Instruction *inst) {
    return m_insts.erase(iterator(inst));
}

List<BasicBlock>::iterator BasicBlock::remove_from_parent() {
    assert(user_begin() == user_end());
    return m_parent->remove_block(this);
}

bool BasicBlock::has_terminator() const {
    return !m_insts.empty() && m_insts.last()->is_terminator();
}

Instruction *BasicBlock::terminator() const {
    assert(has_terminator());
    return m_insts.last();
}

} // namespace codespy::ir
