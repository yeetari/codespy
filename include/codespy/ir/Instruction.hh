#pragma once

#include <codespy/container/ListNode.hh>
#include <codespy/ir/Value.hh>

#include <cstdint>

namespace codespy::ir {

class BasicBlock;
class Visitor;

enum class Opcode : std::uint8_t {
#define INST(opcode, Class) opcode,
#include <codespy/ir/Instructions.in>
};

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

    bool is_terminator() const;
    Opcode opcode() const { return m_opcode; }
    BasicBlock *parent() const { return m_parent; }
    bool has_operands() const { return m_operands != nullptr; }
};

template <typename T>
concept HasOpcode = requires(T) { static_cast<Opcode>(T::k_opcode); };

template <HasOpcode T> requires HasValueKind<T>
struct ValueIsImpl<T> {
    bool operator()(const Value *value) const {
        const auto *inst = value_cast<Instruction>(value);
        return inst != nullptr && inst->opcode() == T::k_opcode;
    }
};

} // namespace codespy::ir
