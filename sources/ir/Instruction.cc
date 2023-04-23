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
    case Opcode::Call:
        visitor.visit(static_cast<CallInst &>(*this));
        break;
    case Opcode::LoadField:
        visitor.visit(static_cast<LoadFieldInst &>(*this));
        break;
    case Opcode::Return:
        visitor.visit(static_cast<ReturnInst &>(*this));
        break;
    default:
        assert(false);
    }
}

void Instruction::remove_from_parent() {
    m_parent->remove(this);
}

} // namespace codespy::ir
