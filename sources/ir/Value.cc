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
        case Opcode::Binary:
            delete inst->as<BinaryInst>();
            break;
        case Opcode::Branch:
            delete inst->as<BranchInst>();
            break;
        case Opcode::Call:
            delete inst->as<CallInst>();
            break;
        case Opcode::Compare:
            delete inst->as<CompareInst>();
            break;
        case Opcode::Load:
            delete inst->as<LoadInst>();
            break;
        case Opcode::LoadArray:
            delete inst->as<LoadArrayInst>();
            break;
        case Opcode::LoadField:
            delete inst->as<LoadFieldInst>();
            break;
        case Opcode::New:
            delete inst->as<NewInst>();
            break;
        case Opcode::NewArray:
            delete inst->as<NewArrayInst>();
            break;
        case Opcode::Phi:
            delete inst->as<PhiInst>();
            break;
        case Opcode::Return:
            delete inst->as<ReturnInst>();
            break;
        case Opcode::Store:
            delete inst->as<StoreInst>();
            break;
        case Opcode::StoreArray:
            delete inst->as<StoreArrayInst>();
            break;
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
