#include <codespy/bytecode/Frontend.hh>

#include <codespy/bytecode/ClassFile.hh>
#include <codespy/ir/BasicBlock.hh>
#include <codespy/ir/Constant.hh>
#include <codespy/ir/Context.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Instructions.hh>
#include <codespy/ir/Type.hh>
#include <codespy/support/Format.hh>
#include <codespy/support/StringBuilder.hh>

namespace codespy::bc {

Frontend::Frontend() {
    m_context = codespy::make_unique<ir::Context>();
}

Frontend::~Frontend() = default;

ir::Type *Frontend::lower_base_type(BaseType base_type) {
    switch (base_type) {
    case BaseType::Int:
        return m_context->int_type(32);
    case BaseType::Long:
        return m_context->int_type(64);
    case BaseType::Float:
        return m_context->float_type();
    case BaseType::Double:
        return m_context->double_type();
    case BaseType::Reference:
        return m_context->reference_type("java/lang/Object");
    case BaseType::Byte:
        return m_context->int_type(8);
    case BaseType::Char:
    case BaseType::Short:
        return m_context->int_type(16);
    case BaseType::Void:
        return m_context->void_type();
    }
}

ir::Type *Frontend::parse_type(StringView descriptor, std::size_t *length) {
    assert(!descriptor.empty());
    if (length != nullptr) {
        *length = 1;
    }

    switch (descriptor[0]) {
    case 'B':
        return m_context->int_type(8);
    case 'C':
    case 'S':
        return m_context->int_type(16);
    case 'D':
        return m_context->double_type();
    case 'F':
        return m_context->float_type();
    case 'I':
        return m_context->int_type(32);
    case 'J':
        return m_context->int_type(64);
    case 'V':
        return m_context->void_type();
    case 'Z':
        return m_context->int_type(1);
    case '[': {
        std::size_t sub_length = 0;
        auto *element_type = parse_type(descriptor.substr(1), &sub_length);
        if (length != nullptr) {
            *length = sub_length + 1;
        }
        return m_context->array_type(element_type);
    }
    case 'L': {
        StringBuilder sb;
        for (std::size_t i = 1; i < descriptor.length(); i++) {
            if (descriptor[i] == ';') {
                break;
            }
            sb.append(descriptor[i]);
        }
        if (length != nullptr) {
            *length = sb.length() + 2;
        }
        return m_context->reference_type(sb.build());
    }
    }
    assert(false);
}

ir::FunctionType *Frontend::parse_function_type(StringView descriptor, ir::Type *this_type) {
    assert(!descriptor.empty() && descriptor[0] == '(');
    descriptor = descriptor.substr(1);

    Vector<ir::Type *> parameter_types;
    if (this_type != nullptr) {
        parameter_types.push(this_type);
    }
    while (!descriptor.empty() && descriptor[0] != ')') {
        std::size_t length;
        parameter_types.push(parse_type(descriptor, &length));
        descriptor = descriptor.substr(length);
    }
    descriptor = descriptor.substr(1);

    auto *return_type = parse_type(descriptor);
    return m_context->function_type(return_type, std::move(parameter_types));
}

ir::Function *Frontend::materialise_function(StringView owner, StringView name, StringView descriptor,
                                             ir::Type *this_type) {
    auto mangled_name = codespy::format("{}.{}#{}", owner, name, descriptor);
    auto *&slot = m_function_map[mangled_name];
    if (slot == nullptr) {
        auto human_name = codespy::format("{}.{}", owner, name);
        auto *type = parse_function_type(descriptor, this_type);
        slot = new ir::Function(*m_context, human_name, type);
    } else {
#ifndef NDEBUG
        auto *type = parse_function_type(descriptor, this_type);
        assert(type == slot->function_type());
#endif
    }
    return slot;
}

ir::BasicBlock *Frontend::materialise_block(std::int32_t offset, bool save_stack) {
    // TODO: Could potentially emit PHIs straight away instead of localising?
    auto &slot = m_block_map.at(offset);
    if (slot.handler) {
        // No-one should jump to a handler.
        // TODO: catch instruction.
        assert(slot.block == nullptr);
        slot.block = m_function->append_block();
        return slot.block;
    }

    if (slot.block == nullptr) {
        assert(slot.entry_stack.empty());
        slot.block = m_function->append_block();
        if (!m_stack.empty()) {
            // Localise stack.
            assert(save_stack);
            slot.entry_stack.ensure_capacity(m_stack.size());
            for (auto *value : m_stack) {
                auto *local = m_function->append_local(value->type());
                m_block->append<ir::StoreInst>(local, value);
                slot.entry_stack.push(local);
            }
        }
        m_queue.push_back(offset);
    } else if (save_stack) {
        assert(m_stack.size() <= slot.entry_stack.size());
        const auto difference = slot.entry_stack.size() - m_stack.size();
        for (std::uint16_t i = 0; i < m_stack.size(); i++) {
            auto *local = slot.entry_stack[i + difference];
            m_block->append<ir::StoreInst>(local, m_stack[i]);
        }
    }
    return slot.block;
}

ir::Value *Frontend::materialise_local(std::uint16_t index) {
    auto *&slot = m_local_map[index];
    if (slot == nullptr) {
        // Use any type as we don't know what type(s) the local may hold.
        // TODO: If SSA can be generated from the beginning (in this frontend pass), we wouldn't need to do this.
        slot = m_function->append_local(m_context->any_type());
    }
    return slot;
}

void Frontend::visit(StringView this_name, StringView) {
    m_this_name = this_name;
}

void Frontend::visit_field(StringView, StringView) {}

void Frontend::visit_method(AccessFlags access_flags, StringView name, StringView descriptor) {
    m_block_map.clear();
    m_local_map.clear();
    m_stack.clear();

    ir::Type *this_type = nullptr;
    if ((access_flags & AccessFlags::Static) != AccessFlags::Static) {
        this_type = m_context->reference_type(m_this_name);
    }
    m_function = materialise_function(m_this_name, name, descriptor, this_type);
}

void Frontend::visit_code(CodeAttribute &code) {
    m_stack.ensure_capacity(code.max_stack());
    m_local_map.reserve(code.max_locals());

    struct JumpTargetVisitor final : public CodeVisitor {
        std::unordered_map<std::int32_t, BlockInfo> &block_map;

        JumpTargetVisitor(std::unordered_map<std::int32_t, BlockInfo> &block_map) : block_map(block_map) {}

        void visit_goto(std::int32_t offset) override { block_map[offset]; }
        void visit_if_compare(CompareOp, std::int32_t true_offset, CompareRhs) override { block_map[true_offset]; }
        void visit_table_switch(std::int32_t, std::int32_t, std::int32_t default_pc,
                                Span<std::int32_t> table) override {
            block_map[default_pc];
            for (std::int32_t pc : table) {
                block_map[pc];
            }
        }
        void visit_lookup_switch(std::int32_t default_pc, Span<std::pair<std::int32_t, std::int32_t>> table) override {
            block_map[default_pc];
            for (const auto &[key, case_pc] : table) {
                block_map[case_pc];
            }
        }
    } visitor(m_block_map);
    for (std::int32_t pc = 0; pc < code.code_end();) {
        pc += CODESPY_EXPECT(code.parse_inst(pc, visitor));
    }

    m_queue.push_front(0);
    while (!m_queue.empty()) {
        auto pc = m_queue.front();
        m_queue.pop_front();

        auto &block_info = m_block_map[pc];
        if (std::exchange(block_info.visited, true)) {
            continue;
        }

        assert(pc == 0 || m_block->has_terminator());
        m_block = materialise_block(pc, /*save_stack*/ false);

        // Clear stack; materialise_block will have localised any stack variables into the entry stack.
        m_stack.clear();

        if (block_info.handler) {
            m_stack.push(m_block->append<ir::NewInst>(m_context->reference_type(block_info.handler_type)));
        }

        // Load entry stack.
        for (auto *local : block_info.entry_stack) {
            auto *value = m_block->append<ir::LoadInst>(local->type(), local);
            m_stack.push(value);
        }

        // Copy arguments to locals.
        if (pc == 0) {
            for (std::uint32_t i = 0; auto *argument : m_function->arguments()) {
                auto *local = materialise_local(i++);
                m_block->append<ir::StoreInst>(local, argument);
            }
        }

        do {
            pc += CODESPY_EXPECT(code.parse_inst(pc, *this));
        } while (!m_block->has_terminator() && !m_block_map.contains(pc));

        // Insert immediate jump if needed.
        if (!m_block->has_terminator()) {
            m_block->append<ir::BranchInst>(materialise_block(pc, /*save_stack*/ true));
        }

        // Materialise false target if needed.
        if (auto *branch = m_block->terminator()->as<ir::BranchInst>()) {
            if (!branch->is_conditional()) {
                continue;
            }

            // Ensure we process the false target (the fallthrough) next.
            m_queue.push_front(pc);

            if (m_block_map[pc].block != nullptr) {
                auto *false_target = branch->false_target();
                false_target->replace_all_uses_with(m_block_map[pc].block);
                false_target->remove_from_parent();
                continue;
            }

            m_block_map[pc].block = branch->false_target();

            // TODO: Don't search like this.
            const Stack *src_stack = nullptr;
            for (const auto &[_, _info] : m_block_map) {
                if (_info.block == branch->true_target()) {
                    src_stack = &_info.entry_stack;
                }
            }

            if (src_stack == nullptr) {
                continue;
            }

            auto &dst_stack = m_block_map[pc].entry_stack;
            assert(dst_stack.empty());
            dst_stack.ensure_capacity(src_stack->size());
            for (auto *local : *src_stack) {
                dst_stack.push(local);
            }
        }
    }
}

void Frontend::visit_exception_range(std::int32_t, std::int32_t, std::int32_t handler_pc, StringView type_name) {
    // TODO: catch instruction, not right at all.
    auto &handler_info = m_block_map[handler_pc];
    handler_info.handler = true;
    handler_info.handler_type = type_name;
    m_queue.push_back(handler_pc);
}

void Frontend::visit_constant(Constant constant) {
    if (constant.has<NullReference>()) {
        m_stack.push(m_context->constant_null());
    } else if (constant.has<std::int32_t>()) {
        m_stack.push(m_context->constant_int(m_context->int_type(32), constant.get<std::int32_t>()));
    } else if (constant.has<std::int64_t>()) {
        m_stack.push(m_context->constant_int(m_context->int_type(64), constant.get<std::int64_t>()));
    } else if (constant.has<float>()) {
        m_stack.push(m_context->constant_float(constant.get<float>()));
    } else if (constant.has<double>()) {
        m_stack.push(m_context->constant_double(constant.get<double>()));
    } else if (constant.has<StringView>()) {
        m_stack.push(m_context->constant_string(constant.get<StringView>()));
    }
}

void Frontend::visit_load(BaseType base_type, std::uint8_t local_index) {
    ir::Type *type = lower_base_type(base_type);
    m_stack.push(m_block->append<ir::LoadInst>(type, materialise_local(local_index)));
}

void Frontend::visit_store(BaseType, std::uint8_t local_index) {
    ir::Value *value = m_stack.take_last();
    m_block->append<ir::StoreInst>(materialise_local(local_index), value);
}

void Frontend::visit_array_load(BaseType base_type) {
    ir::Value *index = m_stack.take_last();
    ir::Value *array_ref = m_stack.take_last();

    ir::Type *element_type;
    if (array_ref->type()->kind() == ir::TypeKind::Array) {
        // We can get exact type.
        element_type = static_cast<ir::ArrayType *>(array_ref->type())->element_type();
    } else {
        // Otherwise, fallback to base_type (e.g. if array_ref was loaded from a local).
        element_type = lower_base_type(base_type);
    }
    m_stack.push(m_block->append<ir::LoadArrayInst>(element_type, array_ref, index));
}

void Frontend::visit_array_store(BaseType) {
    ir::Value *value = m_stack.take_last();
    ir::Value *index = m_stack.take_last();
    ir::Value *array_ref = m_stack.take_last();
    m_block->append<ir::StoreArrayInst>(array_ref, index, value);
}

void Frontend::visit_cast(BaseType, BaseType to_type) {
    ir::Value *value = m_stack.take_last();
    m_stack.push(m_block->append<ir::CastInst>(lower_base_type(to_type), value));
}

void Frontend::visit_compare(BaseType base_type, bool greater_on_nan) {
    ir::Value *rhs = m_stack.take_last();
    ir::Value *lhs = m_stack.take_last();
    ir::Type *type = lower_base_type(base_type);
    m_stack.push(m_block->append<ir::JavaCompareInst>(type, lhs, rhs, greater_on_nan));
}

void Frontend::visit_new(StringView descriptor, std::uint8_t dimensions) {
    ir::Type *type = parse_type(descriptor);
    if (type->kind() != ir::TypeKind::Array) {
        // NEW
        m_stack.push(m_block->append<ir::NewInst>(type));
        return;
    }

    // Else one of NEWARRAY, ANEWARRAY, MULTIANEWARRAY.
#ifndef NDEBUG
    std::uint8_t type_dimensions = 0;
    for (auto *ty = type; ty->kind() == ir::TypeKind::Array; ty = static_cast<ir::ArrayType *>(ty)->element_type()) {
        type_dimensions++;
    }
    assert(dimensions <= type_dimensions);
#endif

    // TODO(small-vector)
    Vector<ir::Value *> counts(dimensions);
    for (std::uint32_t i = counts.size(); i > 0; i--) {
        counts[i - 1] = m_stack.take_last();
    }
    m_stack.push(m_block->append<ir::NewArrayInst>(type, counts.span()));
}

void Frontend::visit_get_field(StringView owner, StringView name, StringView descriptor, bool instance) {
    ir::Value *object_ref = nullptr;
    if (instance) {
        object_ref = m_stack.take_last();
    }
    m_stack.push(m_block->append<ir::LoadFieldInst>(parse_type(descriptor), owner, name, object_ref));
}

void Frontend::visit_put_field(StringView owner, StringView name, StringView, bool instance) {
    ir::Value *value = m_stack.take_last();
    ir::Value *object_ref = nullptr;
    if (instance) {
        object_ref = m_stack.take_last();
    }
    m_block->append<ir::StoreFieldInst>(owner, name, value, object_ref);
}

void Frontend::visit_invoke(InvokeKind kind, StringView owner, StringView name, StringView descriptor) {
    ir::Type *this_type = nullptr;
    if (kind != InvokeKind::Static) {
        this_type = m_context->reference_type(owner);
    }
    auto *function = materialise_function(owner, name, descriptor, this_type);

    Vector<ir::Value *> arguments(function->function_type()->parameter_types().size());
    for (std::uint32_t i = arguments.size(); i > 0; i--) {
        arguments[i - 1] = m_stack.take_last();
    }

    auto *call = m_block->append<ir::CallInst>(function, arguments.span());
    call->set_is_invoke_special(kind == InvokeKind::Special);
    if (call->type() != m_context->void_type()) {
        m_stack.push(call);
    }
}

static ir::BinaryOp lower_binary_math_op(MathOp math_op) {
    switch (math_op) {
    case MathOp::Add:
        return ir::BinaryOp::Add;
    case MathOp::Sub:
        return ir::BinaryOp::Sub;
    case MathOp::Mul:
        return ir::BinaryOp::Mul;
    case MathOp::Div:
        return ir::BinaryOp::Div;
    case MathOp::Rem:
        return ir::BinaryOp::Rem;
    case MathOp::Shl:
        return ir::BinaryOp::Shl;
    case MathOp::Shr:
        return ir::BinaryOp::Shr;
    case MathOp::UShr:
        return ir::BinaryOp::UShr;
    case MathOp::And:
        return ir::BinaryOp::And;
    case MathOp::Or:
        return ir::BinaryOp::Or;
    case MathOp::Xor:
        return ir::BinaryOp::Xor;
    default:
        assert(false);
    }
}

void Frontend::visit_math_op(BaseType base_type, MathOp math_op) {
    // Use base_type in case one or both of lhs and rhs is any type.
    ir::Type *type = lower_base_type(base_type);
    ir::Value *rhs = m_stack.take_last();
    if (math_op == MathOp::Neg) {
        m_stack.push(m_block->append<ir::NegateInst>(type, rhs));
        return;
    }
    ir::Value *lhs = m_stack.take_last();
    m_stack.push(m_block->append<ir::BinaryInst>(type, lower_binary_math_op(math_op), lhs, rhs));
}

void Frontend::visit_monitor_op(MonitorOp monitor_op) {
    ir::Value *object_ref = m_stack.take_last();
    switch (monitor_op) {
    case MonitorOp::Enter:
        m_block->append<ir::MonitorInst>(ir::MonitorOp::Enter, object_ref);
        break;
    case MonitorOp::Exit:
        m_block->append<ir::MonitorInst>(ir::MonitorOp::Exit, object_ref);
        break;
    }
}

void Frontend::visit_reference_op(ReferenceOp reference_op) {
    switch (reference_op) {
    case ReferenceOp::ArrayLength: {
        ir::Value *array_ref = m_stack.take_last();
        m_stack.push(m_block->append<ir::ArrayLengthInst>(array_ref));
        break;
    }
    case ReferenceOp::Throw: {
        ir::Value *exception_ref = m_stack.take_last();
        m_block->append<ir::ThrowInst>(exception_ref);
        break;
    }
    }
}

void Frontend::visit_stack_op(StackOp stack_op) {
    switch (stack_op) {
    case StackOp::Pop2:
        m_stack.pop();
        if (m_stack.empty()) {
            break;
        }
        [[fallthrough]];
    case StackOp::Pop:
        m_stack.pop();
        break;
    case StackOp::Dup: {
        // TODO: vec.push(vec.last()) unsafe.
        ir::Value *last = m_stack.last();
        m_stack.push(last);
        break;
    }
    case StackOp::DupX1: {
        ir::Value *value1 = m_stack.take_last();
        ir::Value *value2 = m_stack.take_last();
        m_stack.push(value1);
        m_stack.push(value2);
        m_stack.push(value1);
        break;
    }
    case StackOp::DupX2: {
        ir::Value *value1 = m_stack.take_last();
        ir::Value *value2 = m_stack.take_last();
        ir::Value *value3 = nullptr;
        if (!m_stack.empty()) {
            value3 = m_stack.take_last();
        }
        m_stack.push(value1);
        if (value3 != nullptr) {
            m_stack.push(value3);
        }
        m_stack.push(value2);
        m_stack.push(value1);
        break;
    }
    case StackOp::Dup2: {
        ir::Value *value1 = m_stack.take_last();
        ir::Value *value2 = nullptr;
        if (!m_stack.empty()) {
            value2 = m_stack.take_last();
        }
        if (value2 != nullptr) {
            m_stack.push(value2);
        }
        m_stack.push(value1);
        if (value2 != nullptr) {
            m_stack.push(value2);
        }
        m_stack.push(value1);
        break;
    }
    case StackOp::Dup2X1: {
        ir::Value *value1 = m_stack.take_last();
        ir::Value *value2 = m_stack.take_last();
        ir::Value *value3 = nullptr;
        if (!m_stack.empty()) {
            value3 = m_stack.take_last();
        }

        if (value3 != nullptr) {
            m_stack.push(value2);
        }
        m_stack.push(value1);
        if (value3 != nullptr) {
            m_stack.push(value3);
        }
        m_stack.push(value2);
        m_stack.push(value1);
        break;
    }
    default:
        assert(false);
    }
}

void Frontend::visit_type_op(TypeOp type_op, StringView descriptor) {
    ir::Type *check_type = parse_type(descriptor);
    ir::Value *value = m_stack.take_last();
    switch (type_op) {
    case TypeOp::CheckCast:
        m_stack.push(m_block->append<ir::CastInst>(check_type, value));
        break;
    case TypeOp::InstanceOf:
        m_stack.push(m_block->append<ir::InstanceOfInst>(check_type, value));
        break;
    }
}

void Frontend::visit_iinc(std::uint8_t local_index, std::int32_t increment) {
    auto *local = materialise_local(local_index);
    auto *type = m_context->int_type(32);
    auto *value = m_block->append<ir::LoadInst>(type, local);
    auto *constant = m_context->constant_int(type, increment);
    auto *new_value = m_block->append<ir::BinaryInst>(type, ir::BinaryOp::Add, value, constant);
    m_block->append<ir::StoreInst>(local, new_value);
}

void Frontend::visit_goto(std::int32_t offset) {
    m_block->append<ir::BranchInst>(materialise_block(offset, /*save_stack*/ true));

    // Fully clear the stack after a GOTO, there is no fallthrough to the next instruction.
    m_stack.clear();
}

static ir::CompareOp lower_compare_op(CompareOp compare_op) {
    switch (compare_op) {
    case CompareOp::Equal:
    case CompareOp::ReferenceEqual:
        return ir::CompareOp::Equal;
    case CompareOp::NotEqual:
    case CompareOp::ReferenceNotEqual:
        return ir::CompareOp::NotEqual;
    case CompareOp::LessThan:
        return ir::CompareOp::LessThan;
    case CompareOp::GreaterEqual:
        return ir::CompareOp::GreaterEqual;
    case CompareOp::GreaterThan:
        return ir::CompareOp::GreaterThan;
    case CompareOp::LessEqual:
        return ir::CompareOp::LessEqual;
    default:
        assert(false);
    }
}

void Frontend::visit_if_compare(CompareOp compare_op, std::int32_t true_offset, CompareRhs compare_rhs) {
    ir::Value *rhs = nullptr;
    if (compare_rhs == CompareRhs::Stack) {
        rhs = m_stack.take_last();
    } else if (compare_rhs == CompareRhs::Null) {
        rhs = m_context->constant_null();
    }
    ir::Value *lhs = m_stack.take_last();
    if (rhs == nullptr) {
        assert(lhs->type()->kind() == ir::TypeKind::Integer);
        rhs = m_context->constant_int(static_cast<ir::IntType *>(lhs->type()), 0);
    }

    auto *false_target = m_function->append_block();
    auto *true_target = materialise_block(true_offset, /*save_stack*/ true);
    auto *compare = m_block->append<ir::CompareInst>(lower_compare_op(compare_op), lhs, rhs);
    m_block->append<ir::BranchInst>(true_target, false_target, compare);
}

template <typename F>
void Frontend::emit_switch(std::size_t case_count, std::int32_t default_pc, F next_case) {
    ir::Value *key_value = m_stack.take_last();
    auto *key_type = static_cast<ir::IntType *>(key_value->type());
    auto *default_target = materialise_block(default_pc, /*save_stack*/ true);

    Vector<std::pair<ir::Value *, ir::BasicBlock *>> targets(case_count);
    for (std::size_t i = 0; i < case_count; i++) {
        const auto [case_value, offset] = next_case();
        targets[i] = std::make_pair(m_context->constant_int(key_type, case_value),
                                    materialise_block(offset, /*save_stack*/ true));
    }
    m_block->append<ir::SwitchInst>(key_value, default_target, targets.span());
}

void Frontend::visit_table_switch(std::int32_t low, std::int32_t, std::int32_t default_pc, Span<std::int32_t> table) {
    std::int32_t index = low;
    emit_switch(table.size(), default_pc, [&] {
        index++;
        return std::make_pair(index - 1, table[index - low - 1]);
    });
}

void Frontend::visit_lookup_switch(std::int32_t default_pc, Span<std::pair<std::int32_t, std::int32_t>> table) {
    auto *it = table.begin();
    emit_switch(table.size(), default_pc, [&] {
        return *it++;
    });
}

void Frontend::visit_return(BaseType type) {
    if (type == BaseType::Void) {
        m_block->append<ir::ReturnInst>(nullptr);
        return;
    }

    ir::Value *value = m_stack.take_last();
    m_block->append<ir::ReturnInst>(value);
}

Vector<ir::Function *> Frontend::functions() {
    Vector<ir::Function *> functions;
    functions.ensure_capacity(m_function_map.size());
    for (const auto &[name, function] : m_function_map) {
        functions.push(function);
    }
    return functions;
}

} // namespace codespy::bc
