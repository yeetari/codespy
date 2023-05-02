#include <codespy/bytecode/ClassFile.hh>

#include <codespy/bytecode/Definitions.hh>
#include <codespy/bytecode/Visitor.hh>
#include <codespy/container/FixedBuffer.hh>
#include <codespy/support/Format.hh>
#include <codespy/support/Print.hh>
#include <codespy/support/SpanStream.hh>
#include <codespy/support/Stream.hh>

#include <cstdint>
#include <iostream>
#include <tuple>

namespace codespy::bc {

class ConstantPool {
    FixedBuffer<std::uint8_t> m_bytes;
    Vector<std::size_t, std::uint16_t> m_offsets;
    Vector<String, std::uint16_t> m_utf_cache;

public:
    explicit ConstantPool(std::uint16_t size) : m_offsets(size), m_utf_cache(size) {}

    Constant read_constant(std::uint16_t index) const;
    std::tuple<StringView, StringView, StringView> read_ref(std::uint16_t index) const;
    StringView read_string_like(std::uint16_t index) const;
    StringView read_utf(std::uint16_t index) const;

    void set_bytes(FixedBuffer<std::uint8_t> &&bytes) { m_bytes = std::move(bytes); }
    void set_offset(std::uint16_t index, std::size_t offset) { m_offsets[index] = offset; }
    void set_string(std::uint16_t index, String &&string) { m_utf_cache[index] = std::move(string); }
    std::uint16_t size() const { return m_offsets.size(); }
};

Constant ConstantPool::read_constant(std::uint16_t index) const {
    switch (static_cast<ConstantKind>(m_bytes[m_offsets[index] - 1])) {
    case ConstantKind::Integer: {
        SpanStream stream(m_bytes.span().subspan(m_offsets[index]));
        return static_cast<std::int32_t>(CODESPY_ASSUME(stream.read_be<std::uint32_t>()));
    }
    case ConstantKind::Float: {
        SpanStream stream(m_bytes.span().subspan(m_offsets[index]));
        const auto as_int = CODESPY_EXPECT(stream.read_be<std::uint32_t>());
        return std::bit_cast<float>(as_int);
    }
    case ConstantKind::Long: {
        SpanStream stream(m_bytes.span().subspan(m_offsets[index]));
        return static_cast<std::int64_t>(CODESPY_ASSUME(stream.read_be<std::uint64_t>()));
    }
    case ConstantKind::Double: {
        SpanStream stream(m_bytes.span().subspan(m_offsets[index]));
        const auto as_int = CODESPY_EXPECT(stream.read_be<std::uint64_t>());
        return std::bit_cast<double>(as_int);
    }
    case ConstantKind::Class:
    case ConstantKind::String:
        return read_string_like(index);
    default:
        codespy::unreachable();
    }
}

// Extract (owner, name, descriptor) from Fieldref_info, Methodref_info, InterfaceMethodref_info
std::tuple<StringView, StringView, StringView> ConstantPool::read_ref(std::uint16_t index) const {
    SpanStream stream(m_bytes.span().subspan(m_offsets[index]));
    const auto class_index = CODESPY_ASSUME(stream.read_be<std::uint16_t>());
    const auto name_and_type_index = CODESPY_ASSUME(stream.read_be<std::uint16_t>());
    SpanStream name_and_type_stream(m_bytes.span().subspan(m_offsets[name_and_type_index]));
    const auto name_index = CODESPY_ASSUME(name_and_type_stream.read_be<std::uint16_t>());
    const auto descriptor_index = CODESPY_ASSUME(name_and_type_stream.read_be<std::uint16_t>());
    return std::make_tuple(read_string_like(class_index), read_utf(name_index), read_utf(descriptor_index));
}

// Extract string from entries that only hold a UTF index (Class_info, String_info)
StringView ConstantPool::read_string_like(std::uint16_t index) const {
    SpanStream stream(m_bytes.span().subspan(m_offsets[index]));
    const auto name_index = CODESPY_ASSUME(stream.read_be<std::uint16_t>());
    return m_utf_cache[name_index];
}

StringView ConstantPool::read_utf(std::uint16_t index) const {
    return m_utf_cache[index];
}

template <typename F>
static Result<void, ParseError, StreamError> iterate_attributes(Stream &stream, ConstantPool &constant_pool,
                                                                F callback) {
    auto count = CODESPY_TRY(stream.read_be<std::uint16_t>());
    while (count-- > 0) {
        const auto &name = constant_pool.read_utf(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        const auto length = CODESPY_TRY(stream.read_be<std::uint32_t>());
        if (!CODESPY_TRY(callback(name))) {
            CODESPY_TRY(stream.seek(length, SeekMode::Add));
        }
    }
    return {};
}

static Result<void, ParseError, StreamError> parse_code(Stream &stream, ClassVisitor &visitor, ConstantPool &constant_pool) {
    const auto max_stack = CODESPY_TRY(stream.read_be<std::uint16_t>());
    const auto max_locals = CODESPY_TRY(stream.read_be<std::uint16_t>());
    const auto code_length = CODESPY_TRY(stream.read_be<std::uint32_t>());

    auto buffer = FixedBuffer<std::uint8_t>::create_uninitialised(code_length);
    CODESPY_TRY(stream.read(buffer.span()));

    auto exception_count = CODESPY_TRY(stream.read_be<std::uint16_t>());
    while (exception_count-- > 0) {
        const auto start_pc = static_cast<std::int16_t>(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        const auto end_pc = static_cast<std::int16_t>(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        const auto handler_pc = static_cast<std::int16_t>(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        const auto type_index = CODESPY_TRY(stream.read_be<std::uint16_t>());
        const auto type_name = type_index != 0 ? constant_pool.read_string_like(type_index) : "java/lang/Throwable";
        visitor.visit_exception_range(start_pc, end_pc, handler_pc, type_name);
    }

    CodeAttribute code(constant_pool, max_stack, max_locals, std::move(buffer));
    visitor.visit_code(code);
    return {};
}

Result<void, ParseError, StreamError> parse_class(Stream &stream, ClassVisitor &visitor) {
    const auto magic = CODESPY_TRY(stream.read_be<std::uint32_t>());
    if (magic != 0xcafebabe) {
        return ParseError::BadMagic;
    }

    // Skip minor and major numbers.
    CODESPY_TRY(stream.read_be<std::uint16_t>()); // minor
    CODESPY_TRY(stream.read_be<std::uint16_t>()); // major

    // Skip past constant pool, but create an index->offset map as well as preloading strings.
    ConstantPool constant_pool(CODESPY_TRY(stream.read_be<std::uint16_t>()));
    for (std::uint16_t i = 1; i < constant_pool.size(); i++) {
        const auto tag = static_cast<ConstantKind>(CODESPY_TRY(stream.read_byte()));
        constant_pool.set_offset(i, CODESPY_ASSUME(stream.seek(0, SeekMode::Add)) - 10);

        switch (tag) {
        case ConstantKind::Utf8: {
            const auto length = CODESPY_TRY(stream.read_be<std::uint16_t>());
            String string(length);
            CODESPY_TRY(stream.read({string.data(), length}));
            constant_pool.set_string(i, std::move(string));
            break;
        }
        case ConstantKind::Integer:
        case ConstantKind::Float:
            CODESPY_TRY(stream.seek(4, SeekMode::Add));
            break;
        case ConstantKind::Long:
        case ConstantKind::Double:
            CODESPY_TRY(stream.seek(8, SeekMode::Add));
            // If a CONSTANT_Long_info or CONSTANT_Double_info structure is the entry at index n ... the constant pool
            // index n+1 must be valid but is considered unusable.
            i++;
            break;
        case ConstantKind::Class:
        case ConstantKind::String:
            CODESPY_TRY(stream.seek(2, SeekMode::Add));
            break;
        case ConstantKind::MethodHandle:
            CODESPY_TRY(stream.seek(3, SeekMode::Add));
            break;
        case ConstantKind::FieldRef:
        case ConstantKind::MethodRef:
        case ConstantKind::InterfaceMethodRef:
        case ConstantKind::NameAndType:
        case ConstantKind::Dynamic:
        case ConstantKind::InvokeDynamic:
            CODESPY_TRY(stream.seek(4, SeekMode::Add));
            break;
        default:
            std::cerr << "Unknown constant kind " << static_cast<std::uint16_t>(tag) << '\n';
            return ParseError::UnknownConstantPoolEntry;
        }
    }

    // Reload the constant pool into a bytebuffer.
    const auto cp_size = CODESPY_ASSUME(stream.seek(0, SeekMode::Add)) - 10;
    auto cp_bytes = FixedBuffer<std::uint8_t>::create_uninitialised(cp_size);
    CODESPY_TRY(stream.seek(10, SeekMode::Set));
    CODESPY_TRY(stream.read(cp_bytes.span()));
    constant_pool.set_bytes(std::move(cp_bytes));

    CODESPY_TRY(stream.read_be<std::uint16_t>()); // access flags

    // Valid index into the constant_pool table to a CONSTANT_Class_info struct.
    const auto this_class = CODESPY_TRY(stream.read_be<std::uint16_t>());

    // Must either be zero or valid index to CONSTANT_Class_info.
    // In practice only zero for class Object.
    const auto super_class = CODESPY_TRY(stream.read_be<std::uint16_t>());

    // Each interfaces[i] must be CONSTANT_Class_info
    auto interface_count = CODESPY_TRY(stream.read_be<std::uint16_t>());
    Vector<String> interfaces;
    interfaces.ensure_capacity(interface_count);
    while (interface_count-- > 0) {
        interfaces.push(constant_pool.read_string_like(CODESPY_TRY(stream.read_be<std::uint16_t>())));
    }

    visitor.visit(constant_pool.read_string_like(this_class), constant_pool.read_string_like(super_class));

    auto field_count = CODESPY_TRY(stream.read_be<std::uint16_t>());
    while (field_count-- > 0) {
        CODESPY_TRY(stream.read_be<std::uint16_t>()); // access flags
        const auto name = constant_pool.read_utf(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        const auto descriptor = constant_pool.read_utf(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        CODESPY_TRY(
            iterate_attributes(stream, constant_pool, [&](StringView name) -> Result<bool, ParseError, StreamError> {
                if (name != "ConstantValue") {
                    return false;
                }
                // TODO: Handle ConstantValue.
                CODESPY_TRY(stream.read_be<std::uint16_t>());
                return true;
            }));
        visitor.visit_field(name, descriptor);
    }

    auto method_count = CODESPY_TRY(stream.read_be<std::uint16_t>());
    while (method_count-- > 0) {
        const auto access_flags = static_cast<AccessFlags>(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        const auto name = constant_pool.read_utf(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        const auto descriptor = constant_pool.read_utf(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        visitor.visit_method(access_flags, name, descriptor);

        CODESPY_TRY(
            iterate_attributes(stream, constant_pool, [&](StringView name) -> Result<bool, ParseError, StreamError> {
                if (name != "Code") {
                    return false;
                }

                CODESPY_TRY(parse_code(stream, visitor, constant_pool));

                CODESPY_TRY(
                    iterate_attributes(stream, constant_pool, [](StringView) -> Result<bool, ParseError, StreamError> {
                        return false;
                    }));
                return true;
            }));
    }

    // Iterate ClassFile attributes.
    CODESPY_TRY(iterate_attributes(stream, constant_pool, [](StringView name) -> Result<bool, ParseError, StreamError> {
        // Spec says these are important.
        if (name == "BootstrapMethods") {
            return ParseError::UnhandledAttribute;
        }
        if (name == "NestHost") {
            return ParseError::UnhandledAttribute;
        }
        if (name == "NestMembers") {
            return ParseError::UnhandledAttribute;
        }
        if (name == "PermitedSubclasses") {
            return ParseError::UnhandledAttribute;
        }
        return false;
    }));
    return {};
}

Result<std::int32_t, ParseError, StreamError> CodeAttribute::parse_inst(std::int32_t pc, CodeVisitor &visitor) {
    SpanStream stream(m_buffer.span().subspan(pc));
    const auto opcode = static_cast<Opcode>(CODESPY_TRY(stream.read_byte()));

    if (opcode >= Opcode::ICONST_M1 && opcode <= Opcode::ICONST_5) {
        const auto value = static_cast<std::int32_t>(opcode - Opcode::ICONST_M1) - 1;
        visitor.visit_constant(value);
        return 1;
    }

    // <x>load_<n>
    if (opcode >= Opcode::ILOAD_0 && opcode <= Opcode::ALOAD_3) {
        const auto local_index = (opcode - Opcode::ILOAD_0) & 0b11u;
        const auto type = static_cast<BaseType>((opcode - Opcode::ILOAD_0) >> 2u);
        visitor.visit_load(type, local_index);
        return 1;
    }

    // <x>load <n>
    if (opcode >= Opcode::ILOAD && opcode <= Opcode::ALOAD) {
        const auto local_index = CODESPY_TRY(stream.read_byte());
        const auto type = static_cast<BaseType>(opcode - Opcode::ILOAD);
        visitor.visit_load(type, local_index);
        return 2;
    }

    // <x>store_<n>
    if (opcode >= Opcode::ISTORE_0 && opcode <= Opcode::ASTORE_3) {
        const auto local_index = (opcode - Opcode::ISTORE_0) & 0b11u;
        const auto type = static_cast<BaseType>((opcode - Opcode::ISTORE_0) >> 2u);
        visitor.visit_store(type, local_index);
        return 1;
    }

    // <x>store <n>
    if (opcode >= Opcode::ISTORE && opcode <= Opcode::ASTORE) {
        const auto local_index = CODESPY_TRY(stream.read_byte());
        const auto type = static_cast<BaseType>(opcode - Opcode::ISTORE);
        visitor.visit_store(type, local_index);
        return 2;
    }

    // <x>aload
    if (opcode >= Opcode::IALOAD && opcode <= Opcode::SALOAD) {
        const auto type = static_cast<BaseType>(opcode - Opcode::IALOAD);
        visitor.visit_array_load(type);
        return 1;
    }

    // <x>astore
    if (opcode >= Opcode::IASTORE && opcode <= Opcode::SASTORE) {
        const auto type = static_cast<BaseType>(opcode - Opcode::IASTORE);
        visitor.visit_array_store(type);
        return 1;
    }

    // i2<x>
    if (opcode >= Opcode::I2L && opcode < Opcode::L2I) {
        // [0, 1, 2] -> [1, 2, 3]
        const auto to_type = static_cast<BaseType>(opcode - Opcode::I2L + 1);
        visitor.visit_cast(BaseType::Int, to_type);
        return 1;
    }

    // l2<x>
    if (opcode >= Opcode::L2I && opcode < Opcode::F2I) {
        // [0, 1, 2] -> [0, 2, 3]
        Array map{0, 2, 3};
        const auto to_type = static_cast<BaseType>(map[opcode - Opcode::L2I]);
        visitor.visit_cast(BaseType::Long, to_type);
        return 1;
    }

    // f2<x>
    if (opcode >= Opcode::F2I && opcode < Opcode::D2I) {
        // [0, 1, 2] -> [0, 1, 3]
        Array map{0, 1, 3};
        const auto to_type = static_cast<BaseType>(map[opcode - Opcode::F2I]);
        visitor.visit_cast(BaseType::Float, to_type);
        return 1;
    }

    // d2<x>
    if (opcode >= Opcode::D2I && opcode < Opcode::I2B) {
        // [0, 1, 2] -> [0, 1, 2]
        const auto to_type = static_cast<BaseType>(opcode - Opcode::F2I);
        visitor.visit_cast(BaseType::Double, to_type);
        return 1;
    }

    // <x>{add, sub, mul, div, rem, neg}
    if (opcode >= Opcode::IADD && opcode <= Opcode::DNEG) {
        const auto math_op = static_cast<MathOp>((opcode - Opcode::IADD) >> 2u);
        const auto type = static_cast<BaseType>((opcode - Opcode::IADD) & 0b11u);
        visitor.visit_math_op(type, math_op);
        return 1;
    }

    // Arithmetic and logical shifts.
    if (opcode == Opcode::ISHL || opcode == Opcode::LSHL) {
        const auto type = static_cast<BaseType>(opcode - Opcode::ISHL);
        visitor.visit_math_op(type, MathOp::Shl);
        return 1;
    }
    if (opcode == Opcode::ISHR || opcode == Opcode::LSHR) {
        const auto type = static_cast<BaseType>(opcode - Opcode::ISHR);
        visitor.visit_math_op(type, MathOp::Shr);
        return 1;
    }
    if (opcode == Opcode::IUSHR || opcode == Opcode::LUSHR) {
        const auto type = static_cast<BaseType>(opcode - Opcode::IUSHR);
        visitor.visit_math_op(type, MathOp::UShr);
        return 1;
    }

    // Bitwise operators.
    if (opcode == Opcode::IAND || opcode == Opcode::LAND) {
        const auto type = static_cast<BaseType>(opcode - Opcode::IAND);
        visitor.visit_math_op(type, MathOp::And);
        return 1;
    }
    if (opcode == Opcode::IOR || opcode == Opcode::LOR) {
        const auto type = static_cast<BaseType>(opcode - Opcode::IOR);
        visitor.visit_math_op(type, MathOp::Or);
        return 1;
    }
    if (opcode == Opcode::IXOR || opcode == Opcode::LXOR) {
        const auto type = static_cast<BaseType>(opcode - Opcode::IXOR);
        visitor.visit_math_op(type, MathOp::Xor);
        return 1;
    }

    // if<op>
    if (opcode >= Opcode::IFEQ && opcode <= Opcode::IFLE) {
        const auto compare_op = static_cast<CompareOp>(opcode - Opcode::IFEQ);
        const auto true_offset = static_cast<std::int16_t>(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        visitor.visit_if_compare(compare_op, static_cast<std::int32_t>(true_offset) + pc, CompareRhs::Zero);
        return 3;
    }

    // if_icmp<op>
    if (opcode >= Opcode::IF_ICMPEQ && opcode <= Opcode::IF_ACMPNE) {
        const auto compare_op = static_cast<CompareOp>(opcode - Opcode::IF_ICMPEQ);
        const auto true_offset = static_cast<std::int16_t>(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        visitor.visit_if_compare(compare_op, static_cast<std::int32_t>(true_offset) + pc, CompareRhs::Stack);
        return 3;
    }

    // <x>return
    if (opcode >= Opcode::IRETURN && opcode < Opcode::RETURN) {
        visitor.visit_return(static_cast<BaseType>(opcode - Opcode::IRETURN));
        return 1;
    }

    if (opcode == Opcode::ARRAYLENGTH || opcode == Opcode::ATHROW) {
        const auto reference_op = static_cast<ReferenceOp>(opcode - Opcode::ARRAYLENGTH);
        visitor.visit_reference_op(reference_op);
        return 1;
    }

    if (opcode == Opcode::CHECKCAST || opcode == Opcode::INSTANCEOF) {
        const auto type_op = static_cast<TypeOp>(opcode - Opcode::CHECKCAST);
        const auto type_name = m_constant_pool.read_string_like(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        visitor.visit_type_op(type_op, type_name);
        return 3;
    }

    if (opcode == Opcode::MONITORENTER || opcode == Opcode::MONITOREXIT) {
        const auto monitor_op = static_cast<MonitorOp>(opcode - Opcode::MONITORENTER);
        visitor.visit_monitor_op(monitor_op);
        return 1;
    }

    if (opcode >= Opcode::GET_STATIC && opcode <= Opcode::INVOKE_INTERFACE) {
        const auto ref_index = CODESPY_TRY(stream.read_be<std::uint16_t>());
        const auto [owner, name, descriptor] = m_constant_pool.read_ref(ref_index);
        switch (opcode) {
        case Opcode::GET_STATIC:
            visitor.visit_get_field(owner, name, descriptor, false);
            return 3;
        case Opcode::PUT_STATIC:
            visitor.visit_put_field(owner, name, descriptor, false);
            return 3;
        case Opcode::GET_FIELD:
            visitor.visit_get_field(owner, name, descriptor, true);
            return 3;
        case Opcode::PUT_FIELD:
            visitor.visit_put_field(owner, name, descriptor, true);
            return 3;
        case Opcode::INVOKE_VIRTUAL:
            visitor.visit_invoke(InvokeKind::Virtual, owner, name, descriptor);
            return 3;
        case Opcode::INVOKE_SPECIAL:
            visitor.visit_invoke(InvokeKind::Special, owner, name, descriptor);
            return 3;
        case Opcode::INVOKE_STATIC:
            visitor.visit_invoke(InvokeKind::Static, owner, name, descriptor);
            return 3;
        case Opcode::INVOKE_INTERFACE:
            CODESPY_TRY(stream.read_byte());
            CODESPY_TRY(stream.read_byte());
            visitor.visit_invoke(InvokeKind::Interface, owner, name, descriptor);
            return 5;
        default:
            codespy::unreachable();
        }
    }

    if (opcode >= Opcode::POP && opcode <= Opcode::SWAP) {
        visitor.visit_stack_op(static_cast<StackOp>(opcode - Opcode::POP));
        return 1;
    }

    if (opcode == Opcode::TABLESWITCH) {
        CODESPY_TRY(stream.seek(3 - (pc & 3u), SeekMode::Add));
        const auto default_pc = CODESPY_TRY(stream.read_be<std::int32_t>()) + pc;
        const auto low = static_cast<std::int32_t>(CODESPY_TRY(stream.read_be<std::uint32_t>()));
        const auto high = static_cast<std::int32_t>(CODESPY_TRY(stream.read_be<std::uint32_t>()));
        Vector<std::int32_t> table(high - low + 1);
        for (auto &case_pc : table) {
            case_pc = CODESPY_TRY(stream.read_be<std::int32_t>()) + pc;
        }
        visitor.visit_table_switch(low, high, default_pc, table.span());
        return static_cast<std::int32_t>(CODESPY_ASSUME(stream.seek(0, SeekMode::Add)));
    }

    if (opcode == Opcode::LOOKUPSWITCH) {
        CODESPY_TRY(stream.seek(3 - (pc & 3u), SeekMode::Add));
        const auto default_pc = CODESPY_TRY(stream.read_be<std::int32_t>()) + pc;
        const auto entry_count = CODESPY_TRY(stream.read_be<std::int32_t>());
        Vector<std::pair<std::int32_t, std::int32_t>> table(entry_count);
        for (std::int32_t i = 0; i < entry_count; i++) {
            const auto key = CODESPY_TRY(stream.read_be<std::int32_t>());
            const auto case_pc = CODESPY_TRY(stream.read_be<std::int32_t>()) + pc;
            table[i] = std::make_pair(key, case_pc);
        }
        visitor.visit_lookup_switch(default_pc, table.span());
        return static_cast<std::int32_t>(CODESPY_ASSUME(stream.seek(0, SeekMode::Add)));
    }

    if (opcode == Opcode::WIDE) {
        const auto subopcode = static_cast<Opcode>(CODESPY_TRY(stream.read_byte()));
        const auto local_index = CODESPY_TRY(stream.read_be<std::int16_t>());
        if (subopcode >= Opcode::ILOAD && subopcode <= Opcode::ALOAD) {
            const auto type = static_cast<BaseType>(subopcode - Opcode::ILOAD);
            visitor.visit_load(type, local_index);
        } else if (subopcode >= Opcode::ISTORE && subopcode <= Opcode::ASTORE) {
            const auto type = static_cast<BaseType>(subopcode - Opcode::ISTORE);
            visitor.visit_store(type, local_index);
        } else if (subopcode == Opcode::IINC) {
            const auto constant = CODESPY_TRY(stream.read_be<std::int16_t>());
            visitor.visit_iinc(local_index, constant);
        }
        return static_cast<std::int32_t>(CODESPY_ASSUME(stream.seek(0, SeekMode::Add)));
    }

    switch (opcode) {
    case Opcode::ACONST_NULL:
        visitor.visit_constant(NullReference{});
        return 1;
    case Opcode::LCONST_0:
        visitor.visit_constant(std::int64_t(0));
        return 1;
    case Opcode::LCONST_1:
        visitor.visit_constant(std::int64_t(1));
        return 1;
    case Opcode::FCONST_0:
        visitor.visit_constant(0.0f);
        return 1;
    case Opcode::FCONST_1:
        visitor.visit_constant(1.0f);
        return 1;
    case Opcode::FCONST_2:
        visitor.visit_constant(2.0f);
        return 1;
    case Opcode::DCONST_0:
        visitor.visit_constant(0.0);
        return 1;
    case Opcode::DCONST_1:
        visitor.visit_constant(1.0);
        return 1;
    case Opcode::BIPUSH:
        visitor.visit_constant(static_cast<std::int32_t>(static_cast<std::int8_t>(CODESPY_TRY(stream.read_byte()))));
        return 2;
    case Opcode::SIPUSH:
        visitor.visit_constant(
            static_cast<std::int32_t>(static_cast<std::int16_t>(CODESPY_TRY(stream.read_be<std::uint16_t>()))));
        return 3;
    case Opcode::LDC:
        visitor.visit_constant(m_constant_pool.read_constant(CODESPY_TRY(stream.read_byte())));
        return 2;
    case Opcode::LDC_W:
    case Opcode::LDC2_W:
        visitor.visit_constant(m_constant_pool.read_constant(CODESPY_TRY(stream.read_be<std::uint16_t>())));
        return 3;
    case Opcode::IINC: {
        const auto local_index = CODESPY_TRY(stream.read_byte());
        const auto constant = static_cast<std::int8_t>(CODESPY_TRY(stream.read_byte()));
        visitor.visit_iinc(local_index, constant);
        return 3;
    }
    case Opcode::I2B:
        visitor.visit_cast(BaseType::Int, BaseType::Byte);
        return 1;
    case Opcode::I2C:
        visitor.visit_cast(BaseType::Int, BaseType::Char);
        return 1;
    case Opcode::I2S:
        visitor.visit_cast(BaseType::Int, BaseType::Short);
        return 1;
    case Opcode::LCMP:
        visitor.visit_compare(BaseType::Long, false);
        return 1;
    case Opcode::FCMPL:
        visitor.visit_compare(BaseType::Float, false);
        return 1;
    case Opcode::FCMPG:
        visitor.visit_compare(BaseType::Float, true);
        return 1;
    case Opcode::DCMPL:
        visitor.visit_compare(BaseType::Double, false);
        return 1;
    case Opcode::DCMPG:
        visitor.visit_compare(BaseType::Double, true);
        return 1;
    case Opcode::GOTO: {
        const auto offset = static_cast<std::int16_t>(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        visitor.visit_goto(static_cast<std::int32_t>(offset) + pc);
        return 3;
    }
    case Opcode::RETURN:
        visitor.visit_return(BaseType::Void);
        return 1;
    case Opcode::NEW: {
        const auto class_name = m_constant_pool.read_string_like(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        visitor.visit_new(codespy::format("L{};", class_name));
        return 3;
    }
    case Opcode::NEWARRAY:
        switch (CODESPY_TRY(stream.read_byte())) {
        case 4:
            visitor.visit_new("[Z");
            break;
        case 5:
            visitor.visit_new("[C");
            break;
        case 6:
            visitor.visit_new("[F");
            break;
        case 7:
            visitor.visit_new("[D");
            break;
        case 8:
            visitor.visit_new("[B");
            break;
        case 9:
            visitor.visit_new("[S");
            break;
        case 10:
            visitor.visit_new("[I");
            break;
        case 11:
            visitor.visit_new("[J");
            break;
        default:
            return ParseError::InvalidArrayType;
        }
        return 2;
    case Opcode::ANEWARRAY: {
        const auto class_name = m_constant_pool.read_string_like(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        visitor.visit_new(codespy::format("[L{};", class_name));
        return 3;
    }
    case Opcode::MULTIANEWARRAY: {
        const auto descriptor = m_constant_pool.read_string_like(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        CODESPY_TRY(stream.read_byte());
        visitor.visit_new(descriptor);
        return 4;
    }
    case Opcode::IFNULL:
    case Opcode::IFNONNULL: {
        const auto true_offset = static_cast<std::int16_t>(CODESPY_TRY(stream.read_be<std::uint16_t>()));
        visitor.visit_if_compare(opcode == Opcode::IFNULL ? CompareOp::ReferenceEqual : CompareOp::ReferenceNotEqual,
                                 static_cast<std::int32_t>(true_offset) + pc, CompareRhs::Null);
        return 3;
    }
    default:
        break;
    }
    std::cerr << "Unknown opcode: " << static_cast<std::uint32_t>(opcode) << '\n';
    return ParseError::UnknownOpcode;
}

} // namespace codespy::bc
