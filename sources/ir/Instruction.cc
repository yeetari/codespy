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
#define INST(opcode, Class) case opcode: visitor.visit(*as<Class>()); break;
#include <codespy/ir/Instructions.in>
    }
}

void Instruction::remove_from_parent() {
    m_parent->remove(this);
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
