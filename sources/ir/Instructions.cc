#include <codespy/ir/Instructions.hh>

#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Context.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Java.hh>

namespace codespy::ir {

ArrayLengthInst::ArrayLengthInst(BasicBlock *parent, Value *array_ref)
    : Instruction(k_opcode, parent, parent->context().int_type(32), 1) {
    set_operand(0, array_ref);
}

BinaryInst::BinaryInst(BasicBlock *parent, Type *type, BinaryOp op, Value *lhs, Value *rhs)
    : Instruction(k_opcode, parent, type, 2), m_op(op) {
    set_operand(0, lhs);
    set_operand(1, rhs);
}

BranchInst::BranchInst(BasicBlock *parent, BasicBlock *target)
    : Instruction(k_opcode, parent, parent->context().void_type(), 1), m_is_conditional(false) {
    set_operand(0, target);
}

BranchInst::BranchInst(BasicBlock *parent, BasicBlock *true_target, BasicBlock *false_target, Value *condition)
    : Instruction(k_opcode, parent, parent->context().void_type(), 3), m_is_conditional(true) {
    set_operand(0, true_target);
    set_operand(1, false_target);
    set_operand(2, condition);
}

BasicBlock *BranchInst::target() const {
    return static_cast<BasicBlock *>(operand(0));
}

BasicBlock *BranchInst::true_target() const {
    return static_cast<BasicBlock *>(operand(0));
}

BasicBlock *BranchInst::false_target() const {
    return static_cast<BasicBlock *>(operand(1));
}

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

CastInst::CastInst(BasicBlock *parent, Type *type, Value *value) : Instruction(k_opcode, parent, type, 1) {
    set_operand(0, value);
}

CompareInst::CompareInst(BasicBlock *parent, CompareOp op, Value *lhs, Value *rhs)
    : Instruction(k_opcode, parent, parent->context().int_type(1), 2), m_op(op) {
    set_operand(0, lhs);
    set_operand(1, rhs);
}

InstanceOfInst::InstanceOfInst(BasicBlock *parent, Type *check_type, Value *value)
    : Instruction(k_opcode, parent, parent->context().int_type(1), 1), m_check_type(check_type) {
    set_operand(0, value);
}

JavaCompareInst::JavaCompareInst(BasicBlock *parent, Type *operand_type, Value *lhs, Value *rhs, bool greater_on_nan)
    : Instruction(k_opcode, parent, parent->context().int_type(32), 2), m_operand_type(operand_type),
      m_greater_on_nan(greater_on_nan) {
    set_operand(0, lhs);
    set_operand(1, rhs);
}

LoadInst::LoadInst(BasicBlock *parent, Type *type, Value *pointer) : Instruction(k_opcode, parent, type, 1) {
    set_operand(0, pointer);
}

LoadArrayInst::LoadArrayInst(BasicBlock *parent, Type *type, Value *array_ref, Value *index)
    : Instruction(k_opcode, parent, type, 2) {
    set_operand(0, array_ref);
    set_operand(1, index);
}

LoadFieldInst::LoadFieldInst(BasicBlock *parent, Type *type, JavaField *field, Value *object_ref)
    : Instruction(k_opcode, parent, type, 2) {
    set_operand(0, field);
    set_operand(1, object_ref);
}

JavaField *LoadFieldInst::field() const {
    return static_cast<JavaField *>(operand(0));
}

MonitorInst::MonitorInst(BasicBlock *parent, MonitorOp op, Value *object_ref)
    : Instruction(k_opcode, parent, parent->context().void_type(), 1), m_op(op) {
    set_operand(0, object_ref);
}

NegateInst::NegateInst(BasicBlock *parent, Type *type, Value *value) : Instruction(k_opcode, parent, type, 1) {
    set_operand(0, value);
}

NewArrayInst::NewArrayInst(BasicBlock *parent, Type *type, Span<Value *> counts)
    : Instruction(k_opcode, parent, type, counts.size()), m_dimensions(counts.size()) {
    for (unsigned i = 0; auto *count : counts) {
        set_operand(i++, count);
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

ReturnInst::ReturnInst(BasicBlock *parent, Value *value)
    : Instruction(k_opcode, parent, parent->context().void_type(), value != nullptr ? 1 : 0) {
    if (value != nullptr) {
        set_operand(0, value);
    }
}

StoreInst::StoreInst(BasicBlock *parent, Value *pointer, Value *value)
    : Instruction(k_opcode, parent, parent->context().void_type(), 2) {
    set_operand(0, pointer);
    set_operand(1, value);
}

StoreArrayInst::StoreArrayInst(BasicBlock *parent, Value *array_ref, Value *index, Value *value)
    : Instruction(k_opcode, parent, parent->context().void_type(), 3) {
    set_operand(0, array_ref);
    set_operand(1, index);
    set_operand(2, value);
}

StoreFieldInst::StoreFieldInst(BasicBlock *parent, JavaField *field, Value *value, Value *object_ref)
    : Instruction(k_opcode, parent, parent->context().void_type(), 3) {
    set_operand(0, field);
    set_operand(1, value);
    set_operand(2, object_ref);
}

JavaField *StoreFieldInst::field() const {
    return static_cast<JavaField *>(operand(0));
}

SwitchInst::SwitchInst(BasicBlock *parent, Value *value, BasicBlock *default_target,
                       Span<std::pair<Value *, BasicBlock *>> targets)
    : Instruction(k_opcode, parent, parent->context().void_type(), 2 + targets.size() * 2),
      m_case_count(targets.size()) {
    set_operand(0, value);
    set_operand(1, default_target);
    for (unsigned i = 2; const auto &[case_value, target] : targets) {
        set_operand(i++, case_value);
        set_operand(i++, target);
    }
}

BasicBlock *SwitchInst::default_target() const {
    return static_cast<BasicBlock *>(operand(1));
}

Value *SwitchInst::case_value(unsigned index) const {
    return operand(index * 2 + 2);
}

BasicBlock *SwitchInst::case_target(unsigned index) const {
    return static_cast<BasicBlock *>(operand(index * 2 + 3));
}

ThrowInst::ThrowInst(BasicBlock *parent, Value *exception_ref)
    : Instruction(k_opcode, parent, parent->context().void_type(), 1) {
    set_operand(0, exception_ref);
}

} // namespace codespy::ir
