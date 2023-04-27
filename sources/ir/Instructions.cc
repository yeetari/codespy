#include <codespy/ir/Instructions.hh>

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Context.hh>
#include <codespy/ir/Function.hh>

namespace codespy::ir {

CallInst::CallInst(BasicBlock *parent, Function *callee, Span<Value *> arguments)
    : Instruction(k_opcode, parent, callee->function_type()->return_type(), arguments.size() + 1) {
    set_operand(0, callee);
    for (std::size_t i = 0; i < arguments.size(); i++) {
        set_operand(i + 1, arguments[i]);
    }
}

Function *CallInst::callee() const {
    return static_cast<Function *>(operand(0));
}

Vector<Value *> CallInst::arguments() const {
    const auto count = callee()->function_type()->parameter_types().size();
    Vector<Value *> ret;
    ret.ensure_capacity(count);
    for (std::size_t i = 0; i < count; i++) {
        ret.push(operand(i + 1));
    }
    return ret;
}

LoadInst::LoadInst(BasicBlock *parent, Type *type, Value *pointer) : Instruction(k_opcode, parent, type, 1) {
    set_operand(0, pointer);
}

LoadFieldInst::LoadFieldInst(BasicBlock *parent, Type *type, String owner, String name, Value *object_ref)
    : Instruction(k_opcode, parent, type, object_ref != nullptr ? 1 : 0), m_owner(std::move(owner)),
      m_name(std::move(name)) {
    if (object_ref != nullptr) {
        set_operand(0, object_ref);
    }
}

// TODO: type
PhiInst::PhiInst(BasicBlock *parent, Span<std::pair<BasicBlock *, Value *>> incoming)
    : Instruction(k_opcode, parent, nullptr, incoming.size() * 2) {
    for (unsigned i = 0; const auto &[block, value] : incoming) {
        set_operand(i++, block);
        set_operand(i++, value);
    }
}

ReturnInst::ReturnInst(BasicBlock *parent) : Instruction(k_opcode, parent, parent->context().void_type(), 0) {}

StoreInst::StoreInst(BasicBlock *parent, Value *pointer, Value *value)
    : Instruction(k_opcode, parent, parent->context().void_type(), 2) {
    set_operand(0, pointer);
    set_operand(1, value);
}

} // namespace codespy::ir
