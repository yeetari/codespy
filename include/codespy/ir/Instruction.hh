#pragma once

#include <codespy/container/ListNode.hh>
#include <codespy/ir/Value.hh>

#include <cstdint>

namespace codespy::ir {

class BasicBlock;
class Visitor;

enum class Opcode : std::uint8_t {
    Binary,
    Branch,
    Call,
    Cast,
    Compare,
    Load,
    Negate,
    Phi,
    Return,
    Store,
    Switch,

    // Java specific
    // TODO: Some can be intrinsics.
    ArrayLength,
    InstanceOf,
    JavaCompare,
    LoadArray,
    LoadField,
    Monitor,
    New,
    NewArray,
    StoreArray,
    StoreField,
    Throw,
};

template <typename T>
concept HasOpcode = requires(T) { static_cast<Opcode>(T::k_opcode); };

class Instruction : public Value, public ListNode {
    const Opcode m_opcode;
    BasicBlock *const m_parent;
    Use *m_operands;

protected:
    Instruction(Opcode opcode, BasicBlock *parent, Type *type, unsigned operand_count)
        : Value(k_kind, type), m_opcode(opcode), m_parent(parent),
          m_operands(operand_count != 0 ? new Use[operand_count] : nullptr) {
        for (unsigned i = 0; i < operand_count; i++) {
            m_operands[i].set_owner(this);
        }
    }
    ~Instruction();

    Value *operand(unsigned index) const;
    void set_operand(unsigned index, Value *value);

public:
    static constexpr auto k_kind = ValueKind::Instruction;

    Instruction(const Instruction &) = delete;
    Instruction(Instruction &&) = delete;

    Instruction &operator=(const Instruction &) = delete;
    Instruction &operator=(Instruction &&) = delete;

    void accept(Visitor &visitor);
    void remove_from_parent();
    BasicBlock *successor(unsigned index) const;
    unsigned successor_count() const;

    template <HasOpcode T>
    T *as();
    template <HasOpcode T>
    const T *as() const;
    template <HasOpcode T>
    bool is() const;

    bool is_terminator() const;
    Opcode opcode() const { return m_opcode; }
    BasicBlock *parent() const { return m_parent; }
    bool has_operands() const { return m_operands != nullptr; }
};

template <HasOpcode T>
T *Instruction::as() {
    return is<T>() ? static_cast<T *>(this) : nullptr;
}

template <HasOpcode T>
const T *Instruction::as() const {
    return is<T>() ? static_cast<const T *>(this) : nullptr;
}

template <HasOpcode T>
bool Instruction::is() const {
    return m_opcode == T::k_opcode;
}

} // namespace codespy::ir
