#include <codespy/ir/BasicBlock.hh>

#include <codespy/ir/Context.hh>

namespace codespy::ir {

BasicBlock::BasicBlock(Context &context) : Value(k_kind, context.label_type()), m_context(context) {}

BasicBlock::iterator BasicBlock::remove(Instruction *inst) {
    return m_insts.erase(iterator(inst));
}

bool BasicBlock::has_terminator() const {
    return !m_insts.empty() && m_insts.last()->is_terminator();
}

} // namespace codespy::ir
