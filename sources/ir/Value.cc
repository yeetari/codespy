#include <codespy/ir/Value.hh>

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Constant.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Instruction.hh>
#include <codespy/ir/Instructions.hh>
#include <codespy/ir/Java.hh>

#include <utility>

namespace codespy::ir {

void Use::add_to_list(Use **list) {
    m_next = *list;
    if (m_next != nullptr) {
        m_next->m_prev = &m_next;
    }
    m_prev = list;
    *m_prev = this;
}

void Use::remove_from_list() {
    *m_prev = m_next;
    if (m_next != nullptr) {
        m_next->m_prev = m_prev;
    }
}

void Use::set(Value *value) {
    if (std::exchange(m_value, value) != nullptr) {
        remove_from_list();
    }
    if (value != nullptr) {
        value->add_use(*this);
    }
}

Value::~Value() {
    // TODO: How does LLVM not need to do this?
    replace_all_uses_with(nullptr);
}

void Value::add_use(Use &use) {
    use.add_to_list(&m_use_list);
}

void Value::destroy() {
    switch (m_kind) {
    case ValueKind::Argument:
        delete static_cast<Argument *>(this);
        break;
    case ValueKind::BasicBlock:
        delete static_cast<BasicBlock *>(this);
        break;
    case ValueKind::ConstantDouble:
        delete static_cast<ConstantDouble *>(this);
        break;
    case ValueKind::ConstantFloat:
        delete static_cast<ConstantFloat *>(this);
        break;
    case ValueKind::ConstantInt:
        delete static_cast<ConstantInt *>(this);
        break;
    case ValueKind::ConstantNull:
        break;
    case ValueKind::ConstantString:
        delete static_cast<ConstantString *>(this);
        break;
    case ValueKind::Function:
        delete static_cast<Function *>(this);
        break;
    case ValueKind::Instruction: {
        switch (static_cast<Instruction *>(this)->opcode()) {
#define INST(opcode, Class)                                                                                            \
    case Opcode::opcode:                                                                                               \
        delete static_cast<Class *>(this);                                                                             \
        break;
#include <codespy/ir/Instructions.in>
        }
        break;
    }
    case ValueKind::JavaField:
        delete static_cast<JavaField *>(this);
        break;
    case ValueKind::Local:
        delete static_cast<Local *>(this);
        break;
    }
}

void Value::replace_all_uses_with(Value *value) {
    while (m_use_list != nullptr) {
        m_use_list->set(value);
    }
}

} // namespace codespy::ir
