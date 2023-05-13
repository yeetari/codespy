#include <codespy/ir/Instruction.hh>

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Instructions.hh>
#include <codespy/ir/Visitor.hh>

namespace codespy::ir {

Instruction::~Instruction() {
    delete[] m_operands;
}

Value *Instruction::operand(unsigned index) const {
    return m_operands[index].value();
}

void Instruction::set_operand(unsigned index, Value *value) {
    m_operands[index].set(value);
}

void Instruction::accept(Visitor &visitor) {
    switch (m_opcode) {
#define INST(opcode, Class) case opcode: visitor.visit(*static_cast<Class *>(this)); break;
#include <codespy/ir/Instructions.in>
    }
}

void Instruction::remove_from_parent() {
    m_parent->remove(this);
}

// TODO: This should probably be defined on the relevant subclasses.
BasicBlock *Instruction::successor(unsigned index) const {
    if (const auto *branch = value_cast<BranchInst>(this)) {
        return index == 0 ? branch->true_target() : branch->false_target();
    }
    if (const auto *switch_inst = value_cast<SwitchInst>(this)) {
        if (index == 0) {
            return switch_inst->default_target();
        }
        return switch_inst->case_target(index - 1);
    }
    codespy::unreachable();
}

unsigned int Instruction::successor_count() const {
    if (const auto *branch = value_cast<BranchInst>(this)) {
        return branch->is_conditional() ? 2 : 1;
    }
    if (const auto *switch_inst = value_cast<SwitchInst>(this)) {
        return switch_inst->case_count() + 1;
    }
    return 0;
}

bool Instruction::is_terminator() const {
    switch (m_opcode) {
#define TERM_INST(opcode, Class) case opcode:
#include <codespy/ir/Instructions.in>
        return true;
    default:
        return false;
    }
}

} // namespace codespy::ir
