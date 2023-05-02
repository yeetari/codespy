#include <codespy/ir/Value.hh>

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Constant.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Instruction.hh>
#include <codespy/ir/Instructions.hh>

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

void Value::add_use(Use &use) {
    use.add_to_list(&m_use_list);
}

void Value::destroy() {
    switch (m_kind) {
    case ValueKind::Argument:
        delete as<Argument>();
        break;
    case ValueKind::BasicBlock:
        delete as<BasicBlock>();
        break;
    case ValueKind::ConstantInt:
        delete as<ConstantInt>();
        break;
    case ValueKind::ConstantString:
        delete as<ConstantString>();
        break;
    case ValueKind::Function:
        delete as<Function>();
        break;
    case ValueKind::Instruction: {
        const auto *inst = as<Instruction>();
        switch (inst->opcode()) {
#define INST(opcode, Class) case opcode: delete inst->as<Class>(); break;
#include <codespy/ir/Instructions.in>
        }
        break;
    }
    case ValueKind::Local:
        delete as<Local>();
        break;
    }
}

void Value::replace_all_uses_with(Value *value) {
    while (m_use_list != nullptr) {
        m_use_list->set(value);
    }
}

} // namespace codespy::ir
