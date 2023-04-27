#pragma once

#include <codespy/container/Vector.hh>
#include <codespy/ir/Instruction.hh>
#include <codespy/support/Span.hh>
#include <codespy/support/String.hh>

#include <utility>

namespace codespy::ir {

class Function;

// enum class BinaryOp {
//
// };
//
// class BinaryInst : public Instruction {
//     const BinaryOp m_op;
//     Value *m_lhs;
//     Value *m_rhs;
// };

// class BranchInst : public Instruction {
//     BasicBlock *m_target;
// };

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

class LoadInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::Load;

    LoadInst(BasicBlock *parent, Type *type, Value *pointer);

    Value *pointer() const { return operand(0); }
};

class LoadFieldInst : public Instruction {
    String m_owner;
    String m_name;

public:
    static constexpr auto k_opcode = Opcode::LoadField;

    LoadFieldInst(BasicBlock *parent, Type *type, String owner, String name, Value *object_ref);

    const String &owner() const { return m_owner; }
    const String &name() const { return m_name; }
    bool has_object_ref() const { return has_operands(); }
    Value *object_ref() const { return operand(0); }
};

class PhiInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::Phi;

    PhiInst(BasicBlock *parent, Span<std::pair<BasicBlock *, Value *>> incoming);
};

class ReturnInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::Return;

    explicit ReturnInst(BasicBlock *parent);
};

class StoreInst : public Instruction {
public:
    static constexpr auto k_opcode = Opcode::Store;

    StoreInst(BasicBlock *parent, Value *pointer, Value *value);

    Value *pointer() const { return operand(0); }
    Value *value() const { return operand(1); }
};

} // namespace codespy::ir
