#include <codespy/bytecode/Frontend.hh>

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
    auto full_name = codespy::format("{}.{}", owner, name);
    auto *&slot = m_function_map[full_name];
    if (slot == nullptr) {
        auto *type = parse_function_type(descriptor, this_type);
        slot = new ir::Function(*m_context, std::move(full_name), type);
    }
    return slot;
}

ir::BasicBlock *Frontend::materialise_block(std::int32_t offset) {
    auto &slot = m_block_map.at(offset);
    if (slot.block == nullptr) {
        assert(slot.entry_stack.empty());
        slot.block = m_function->append_block();
        slot.entry_stack.ensure_size(m_stack.size());
        std::memcpy(slot.entry_stack.data(), m_stack.data(), m_stack.size_bytes());
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

void Frontend::visit_code(std::uint16_t max_stack, std::uint16_t max_locals) {
    m_stack.ensure_capacity(max_stack);
    m_local_map.reserve(max_locals);

    // Create entry block.
    m_block = m_function->append_block();
    m_block_map.emplace(0, BlockInfo{m_block, {}});
}

void Frontend::visit_jump_target(std::int32_t offset) {
    m_block_map[offset];
}

void Frontend::visit_exception_range(std::int32_t, std::int32_t, std::int32_t handler_pc, StringView type_name) {
    // TODO: catch instruction.
    auto &handler_info = m_block_map[handler_pc];
    assert(handler_info.block == nullptr);
    handler_info.block = m_function->append_block();
    handler_info.entry_stack.push(handler_info.block->append<ir::NewInst>(m_context->reference_type(type_name)));
}

void Frontend::visit_offset(std::int32_t offset) {
    // Check whether this is a jump target and create an immediate jump if needed.
    if (offset != 0 && m_block_map.contains(offset)) {
        auto *target = materialise_block(offset);
        if (!m_block->has_terminator()) {
            m_block->append<ir::BranchInst>(target);
        }
        m_block = target;

        assert(m_stack.empty());
        for (auto *value : m_block_map.at(offset).entry_stack) {
            m_stack.push(value);
        }
    }
}

void Frontend::visit_constant(Constant constant) {
    if (constant.has<std::int32_t>()) {
        m_stack.push(m_context->constant_int(m_context->int_type(32), constant.get<std::int32_t>()));
    } else if (constant.has<std::int64_t>()) {
        m_stack.push(m_context->constant_int(m_context->int_type(64), constant.get<std::int64_t>()));
    } else if (constant.has<StringView>()) {
        m_stack.push(m_context->constant_string(constant.get<StringView>()));
    } else {
        assert(false);
    }
}

void Frontend::visit_load(BaseType base_type, std::uint8_t local_index) {
    if (local_index < m_function->parameter_count()) {
        m_stack.push(m_function->argument(local_index));
        return;
    }

    ir::Type *type = lower_base_type(base_type);
    m_stack.push(m_block->append<ir::LoadInst>(type, materialise_local(local_index)));
}

void Frontend::visit_store(BaseType, std::uint8_t local_index) {
    if (local_index < m_function->parameter_count()) {
        // TODO: Handle assigns to arguments.
        assert(false);
    }

    ir::Value *value = m_stack.take_last();
    m_block->append<ir::StoreInst>(materialise_local(local_index), value);
}

void Frontend::visit_array_load(BaseType) {
    ir::Value *index = m_stack.take_last();
    ir::Value *array_ref = m_stack.take_last();
    assert(array_ref->type()->kind() == ir::TypeKind::Array);
    ir::Type *element_type = static_cast<ir::ArrayType *>(array_ref->type())->element_type();
    // TODO: Check element_type compatible with base_type.
    m_stack.push(m_block->append<ir::LoadArrayInst>(element_type, array_ref, index));
}

void Frontend::visit_array_store(BaseType) {
    ir::Value *value = m_stack.take_last();
    ir::Value *index = m_stack.take_last();
    ir::Value *array_ref = m_stack.take_last();
    m_block->append<ir::StoreArrayInst>(array_ref, index, value);
}

void Frontend::visit_cast(BaseType, BaseType) {
    assert(false);
}

void Frontend::visit_compare(BaseType, bool) {
    assert(false);
}

void Frontend::visit_new(StringView descriptor) {
    ir::Type *type = parse_type(descriptor);
    if (type->kind() != ir::TypeKind::Array) {
        // NEW
        m_stack.push(m_block->append<ir::NewInst>(type));
        return;
    }

    // Else one of NEWARRAY, ANEWARRAY, MULTIANEWARRAY.
    std::uint8_t dimensions = 0;
    for (auto *ty = type; ty->kind() == ir::TypeKind::Array; ty = static_cast<ir::ArrayType *>(ty)->element_type()) {
        dimensions++;
    }

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

void Frontend::visit_put_field(StringView, StringView, StringView, bool) {
    assert(false);
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
    default:
        codespy::unreachable();
    }
}

void Frontend::visit_math_op(BaseType base_type, MathOp math_op) {
    // Use base_type in case one or both of lhs and rhs is any type.
    ir::Value *rhs = m_stack.take_last();
    ir::Value *lhs = m_stack.take_last();
    m_stack.push(m_block->append<ir::BinaryInst>(lower_base_type(base_type), lower_binary_math_op(math_op), lhs, rhs));
}

void Frontend::visit_monitor_op(MonitorOp) {
    assert(false);
}

void Frontend::visit_reference_op(ReferenceOp reference_op) {
    assert(reference_op == ReferenceOp::Throw);
    ir::Value *exception_ref = m_stack.take_last();
    m_block->append<ir::ThrowInst>(exception_ref);
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
    case StackOp::Dup:
        m_stack.push(m_stack.last());
        break;
    default:
        assert(false);
    }
}

void Frontend::visit_type_op(TypeOp, StringView) {
    assert(false);
}

void Frontend::visit_iinc(std::uint8_t local_index, std::int32_t increment) {
    auto *local = m_local_map.at(local_index);
    auto *type = m_context->int_type(32);
    auto *value = m_block->append<ir::LoadInst>(type, local);
    auto *constant = m_context->constant_int(type, increment);
    auto *new_value = m_block->append<ir::BinaryInst>(type, ir::BinaryOp::Add, value, constant);
    m_block->append<ir::StoreInst>(local, new_value);
}

void Frontend::visit_goto(std::int32_t offset) {
    m_block->append<ir::BranchInst>(materialise_block(offset));
}

static ir::CompareOp lower_compare_op(CompareOp compare_op) {
    switch (compare_op) {
    case CompareOp::Equal:
        return ir::CompareOp::Equal;
    case CompareOp::NotEqual:
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
    }
    ir::Value *lhs = m_stack.take_last();
    if (rhs == nullptr) {
        assert(lhs->type()->kind() == ir::TypeKind::Integer);
        rhs = m_context->constant_int(static_cast<ir::IntType *>(lhs->type()), 0);
    }

    auto *false_target = m_function->append_block();
    auto *true_target = materialise_block(true_offset);
    auto *compare = m_block->append<ir::CompareInst>(lower_compare_op(compare_op), lhs, rhs);
    m_block->append<ir::BranchInst>(true_target, false_target, compare);
    m_block = false_target;
}

void Frontend::visit_table_switch(std::int32_t, std::int32_t, std::int32_t,
                                  Span<std::int32_t>) {
    assert(false);
}

void Frontend::visit_lookup_switch(std::int32_t, Span<std::pair<std::int32_t, std::int32_t>>) {
    assert(false);
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
