#pragma once

#include <codespy/container/Vector.hh>
#include <codespy/ir/Instruction.hh>
#include <codespy/support/Span.hh>
#include <codespy/support/String.hh>

#include <utility>

namespace codespy::ir {

class Function;
class JavaField;

class ArrayLengthInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::ArrayLength;

    ArrayLengthInst(BasicBlock *parent, Value *array_ref);

    Value *array_ref() const { return operand(0); }
};

enum class BinaryOp {
    Add,
    Sub,
    Mul,
    Div,
    Rem,
    Shl,
    Shr,
    UShr,
    And,
    Or,
    Xor,
};

class BinaryInst : public Instruction {
    const BinaryOp m_op;

public:
    static constexpr auto k_opcode = Opcode::Binary;

    BinaryInst(BasicBlock *parent, Type *type, BinaryOp op, Value *lhs, Value *rhs);

    BinaryOp op() const { return m_op; }
    Value *lhs() const { return operand(0); }
    Value *rhs() const { return operand(1); }
};

class BranchInst : public Instruction {
    const bool m_is_conditional;

public:
    static constexpr auto k_opcode = Opcode::Branch;

    BranchInst(BasicBlock *parent, BasicBlock *target);
    BranchInst(BasicBlock *parent, BasicBlock *true_target, BasicBlock *false_target, Value *condition);

    bool is_conditional() const { return m_is_conditional; }
    BasicBlock *target() const;
    BasicBlock *true_target() const;
    BasicBlock *false_target() const;
    Value *condition() const { return operand(2); }
};

class CallInst : public Instruction {
    bool m_is_invoke_special;

public:
    static constexpr auto k_opcode = Opcode::Call;

    CallInst(BasicBlock *parent, Function *callee, Span<Value *> arguments);

    void set_is_invoke_special(bool is_invoke_special) { m_is_invoke_special = is_invoke_special; }

    Function *callee() const;
    Vector<Value *> arguments() const;
    bool is_invoke_special() const { return m_is_invoke_special; }
};

class CastInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::Cast;

    CastInst(BasicBlock *parent, Type *type, Value *value);

    Value *value() const { return operand(0); }
};

class CatchInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::Catch;

    CatchInst(BasicBlock *parent, Type *type) : Instruction(k_opcode, parent, type, 0) {}
};

enum class CompareOp {
    Equal,
    NotEqual,
    LessThan,
    GreaterThan,
    LessEqual,
    GreaterEqual,
};

class CompareInst : public Instruction {
    const CompareOp m_op;

public:
    static constexpr auto k_opcode = Opcode::Compare;

    CompareInst(BasicBlock *parent, CompareOp op, Value *lhs, Value *rhs);

    CompareOp op() const { return m_op; }
    Value *lhs() const { return operand(0); }
    Value *rhs() const { return operand(1); }
};

class InstanceOfInst : public Instruction {
    Type *const m_check_type;

public:
    static constexpr auto k_opcode = Opcode::InstanceOf;

    InstanceOfInst(BasicBlock *parent, Type *check_type, Value *value);

    Type *check_type() const { return m_check_type; }
    Value *value() const { return operand(0); }
};

class JavaCompareInst : public Instruction {
    Type *const m_operand_type;
    const bool m_greater_on_nan;

public:
    static constexpr auto k_opcode = Opcode::JavaCompare;

    JavaCompareInst(BasicBlock *parent, Type *operand_type, Value *lhs, Value *rhs, bool greater_on_nan);

    bool greater_on_nan() const { return m_greater_on_nan; }
    Type *operand_type() const { return m_operand_type; }
    Value *lhs() const { return operand(0); }
    Value *rhs() const { return operand(1); }
};

class LoadInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::Load;

    LoadInst(BasicBlock *parent, Type *type, Value *pointer);

    Value *pointer() const { return operand(0); }
};

class LoadArrayInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::LoadArray;

    LoadArrayInst(BasicBlock *parent, Type *type, Value *array_ref, Value *index);

    Value *array_ref() const { return operand(0); }
    Value *index() const { return operand(1); }
};

class LoadFieldInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::LoadField;

    LoadFieldInst(BasicBlock *parent, Type *type, JavaField *field, Value *object_ref);

    JavaField *field() const;
    Value *object_ref() const { return operand(1); }
};

enum class MonitorOp {
    Enter,
    Exit,
};

class MonitorInst : public Instruction {
    const MonitorOp m_op;

public:
    static constexpr auto k_opcode = Opcode::Monitor;

    MonitorInst(BasicBlock *parent, MonitorOp op, Value *object_ref);

    MonitorOp op() const { return m_op; }
    Value *object_ref() const { return operand(0); }
};

class NegateInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::Negate;

    NegateInst(BasicBlock *parent, Type *type, Value *value);

    Value *value() const { return operand(0); }
};

class NewInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::New;

    NewInst(BasicBlock *parent, Type *type) : Instruction(k_opcode, parent, type, 0) {}
};

class NewArrayInst : public Instruction {
    const std::uint8_t m_dimensions;

public:
    static constexpr auto k_opcode = Opcode::NewArray;

    NewArrayInst(BasicBlock *parent, Type *type, Span<Value *> counts);

    std::uint8_t dimensions() const { return m_dimensions; }
    Value *count(unsigned index) const { return operand(index); }
};

class PhiInst : public Instruction {
    const unsigned m_incoming_count;

public:
    static constexpr auto k_opcode = Opcode::Phi;

    PhiInst(BasicBlock *parent, unsigned incoming_count);

    void set_incoming(unsigned index, BasicBlock *block, Value *value);

    unsigned incoming_count() const { return m_incoming_count; }
    BasicBlock *incoming_block(unsigned index) const;
    Value *incoming_value(unsigned index) const;
};

class ReturnInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::Return;

    ReturnInst(BasicBlock *parent, Value *value);

    bool is_void() const { return !has_operands(); }
    Value *value() const { return has_operands() ? operand(0) : nullptr; }
};

class StoreInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::Store;

    StoreInst(BasicBlock *parent, Value *pointer, Value *value);

    Value *pointer() const { return operand(0); }
    Value *value() const { return operand(1); }
};

class StoreArrayInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::StoreArray;

    StoreArrayInst(BasicBlock *parent, Value *array_ref, Value *index, Value *value);

    Value *array_ref() const { return operand(0); }
    Value *index() const { return operand(1); }
    Value *value() const { return operand(2); }
};

class StoreFieldInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::StoreField;

    StoreFieldInst(BasicBlock *parent, JavaField *field, Value *value, Value *object_ref);

    JavaField *field() const;
    Value *value() const { return operand(1); }
    Value *object_ref() const { return operand(2); }
};

class SwitchInst : public Instruction {
    const unsigned m_case_count;

public:
    static constexpr auto k_opcode = Opcode::Switch;

    SwitchInst(BasicBlock *parent, Value *value, BasicBlock *default_target,
               Span<std::pair<Value *, BasicBlock *>> targets);

    unsigned case_count() const { return m_case_count; }
    Value *value() const { return operand(0); }
    BasicBlock *default_target() const;
    Value *case_value(unsigned index) const;
    BasicBlock *case_target(unsigned index) const;
};

class ThrowInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::Throw;

    ThrowInst(BasicBlock *parent, Value *exception_ref);

    Value *exception_ref() const { return operand(0); }
};

} // namespace codespy::ir
