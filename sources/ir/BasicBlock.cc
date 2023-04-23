#include <codespy/ir/BasicBlock.hh>

#include <codespy/ir/Context.hh>

namespace codespy::ir {

BasicBlock::BasicBlock(Context &context) : Value(k_kind, context.label_type()), m_context(context) {}

BasicBlock::iterator BasicBlock::remove(Instruction *inst) {
    return m_insts.erase(iterator(inst));
}

} // namespace codespy::ir
