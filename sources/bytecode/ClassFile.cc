#include <bytecode/ClassFile.hh>

#include <bytecode/Definitions.hh>
#include <bytecode/Visitor.hh>
#include <container/FixedBuffer.hh>
#include <support/SpanStream.hh>
#include <support/Stream.hh>

#include <cstdint>
#include <iostream>
#include <tuple>

namespace jamf {
namespace {

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

    void set_bytes(FixedBuffer<std::uint8_t> &&bytes) { m_bytes = jamf::move(bytes); }
    void set_offset(std::uint16_t index, std::size_t offset) { m_offsets[index] = offset; }
    void set_string(std::uint16_t index, String &&string) { m_utf_cache[index] = jamf::move(string); }
    std::uint16_t size() const { return m_offsets.size(); }
};

Constant ConstantPool::read_constant(std::uint16_t index) const {
    switch (static_cast<ConstantKind>(m_bytes[m_offsets[index] - 1])) {
    case ConstantKind::Integer:
    case ConstantKind::Float:
    case ConstantKind::Long:
    case ConstantKind::Double:
    case ConstantKind::Class:
        assert(false);
    case ConstantKind::String:
        return read_string_like(index);
    default:
        jamf::unreachable();
    }
}

// Extract (owner, name, descriptor) from Fieldref_info, Methodref_info, InterfaceMethodref_info
std::tuple<StringView, StringView, StringView> ConstantPool::read_ref(std::uint16_t index) const {
    SpanStream stream(m_bytes.span().subspan(m_offsets[index]));
    const auto class_index = JAMF_ASSUME(stream.read_be<std::uint16_t>());
    const auto name_and_type_index = JAMF_ASSUME(stream.read_be<std::uint16_t>());
    SpanStream name_and_type_stream(m_bytes.span().subspan(m_offsets[name_and_type_index]));
    const auto name_index = JAMF_ASSUME(name_and_type_stream.read_be<std::uint16_t>());
    const auto descriptor_index = JAMF_ASSUME(name_and_type_stream.read_be<std::uint16_t>());
    return std::make_tuple(read_string_like(class_index), read_utf(name_index), read_utf(descriptor_index));
}

// Extract string from entries that only hold a UTF index (Class_info, String_info)
StringView ConstantPool::read_string_like(std::uint16_t index) const {
    SpanStream stream(m_bytes.span().subspan(m_offsets[index]));
    const auto name_index = JAMF_ASSUME(stream.read_be<std::uint16_t>());
    return m_utf_cache[name_index];
}

StringView ConstantPool::read_utf(std::uint16_t index) const {
    return m_utf_cache[index];
}

template <typename F>
Result<void, ParseError, StreamError> iterate_attributes(Stream &stream, ConstantPool &constant_pool, F callback) {
    auto count = JAMF_TRY(stream.read_be<std::uint16_t>());
    while (count-- > 0) {
        const auto &name = constant_pool.read_utf(JAMF_TRY(stream.read_be<std::uint16_t>()));
        const auto length = JAMF_TRY(stream.read_be<std::uint32_t>());
        if (!JAMF_TRY(callback(name))) {
            JAMF_TRY(stream.seek(length, SeekMode::Add));
        }
    }
    return {};
}

Result<void, ParseError, StreamError> parse_code(Stream &stream, Visitor &visitor, ConstantPool &constant_pool) {
    const auto max_stack = JAMF_TRY(stream.read_be<std::uint16_t>());
    const auto max_locals = JAMF_TRY(stream.read_be<std::uint16_t>());
    visitor.visit_code(max_stack, max_locals);

    const auto code_length = JAMF_TRY(stream.read_be<std::uint32_t>());
    const auto code_end = JAMF_ASSUME(stream.seek(0, SeekMode::Add)) + code_length;
    while (JAMF_ASSUME(stream.seek(0, SeekMode::Add)) < code_end) {
        const auto opcode = static_cast<Opcode>(JAMF_TRY(stream.read_byte()));

        if (opcode >= Opcode::ICONST_M1 && opcode <= Opcode::ICONST_5) {
            const auto value = static_cast<int>(opcode - Opcode::ICONST_M1) - 1;
            assert(false);
            continue;
        }

        // <x>load_<n>
        if (opcode >= Opcode::ILOAD_0 && opcode <= Opcode::ALOAD_3) {
            const auto local_index = (opcode - Opcode::ILOAD_0) & 0b11u;
            const auto type = static_cast<BaseType>((opcode - Opcode::ILOAD_0) >> 2u);
            visitor.visit_load(type, local_index);
            continue;
        }

        // <x>load <n>
        if (opcode >= Opcode::ILOAD && opcode <= Opcode::ALOAD) {
            const auto local_index = JAMF_TRY(stream.read_byte());
            const auto type = static_cast<BaseType>(opcode - Opcode::ILOAD);
            visitor.visit_load(type, local_index);
            continue;
        }

        // <x>store_<n>
        if (opcode >= Opcode::ISTORE_0 && opcode <= Opcode::ASTORE_3) {
            const auto local_index = (opcode - Opcode::ISTORE_0) & 0b11u;
            const auto type = static_cast<BaseType>((opcode - Opcode::ISTORE_0) >> 2u);
            visitor.visit_store(type, local_index);
            continue;
        }

        // <x>store <n>
        if (opcode >= Opcode::ISTORE && opcode <= Opcode::ASTORE) {
            const auto local_index = JAMF_TRY(stream.read_byte());
            const auto type = static_cast<BaseType>(opcode - Opcode::ISTORE);
            visitor.visit_store(type, local_index);
            continue;
        }

        // i2<x>
        if (opcode >= Opcode::I2L && opcode < Opcode::L2I) {
            // [0, 1, 2] -> [1, 2, 3]
            const auto to_type = static_cast<BaseType>(opcode - Opcode::I2L + 1);
            visitor.visit_cast(BaseType::Int, to_type);
            continue;
        }

        // l2<x>
        if (opcode >= Opcode::L2I && opcode < Opcode::F2I) {
            // [0, 1, 2] -> [0, 2, 3]
            Array map{0, 2, 3};
            const auto to_type = static_cast<BaseType>(map[opcode - Opcode::L2I]);
            visitor.visit_cast(BaseType::Long, to_type);
            continue;
        }

        // f2<x>
        if (opcode >= Opcode::F2I && opcode < Opcode::D2I) {
            // [0, 1, 2] -> [0, 1, 3]
            Array map{0, 1, 3};
            const auto to_type = static_cast<BaseType>(map[opcode - Opcode::F2I]);
            visitor.visit_cast(BaseType::Float, to_type);
            continue;
        }

        // d2<x>
        if (opcode >= Opcode::D2I && opcode < Opcode::I2B) {
            // [0, 1, 2] -> [0, 1, 2]
            const auto to_type = static_cast<BaseType>(opcode - Opcode::F2I);
            visitor.visit_cast(BaseType::Double, to_type);
            continue;
        }

        // <x>return
        if (opcode >= Opcode::IRETURN && opcode <= Opcode::RETURN) {
            visitor.visit_return(static_cast<BaseType>(opcode - Opcode::IRETURN));
            continue;
        }

        if (opcode >= Opcode::GET_STATIC && opcode <= Opcode::INVOKE_INTERFACE) {
            const auto ref_index = JAMF_TRY(stream.read_be<std::uint16_t>());
            const auto [owner, name, descriptor] = constant_pool.read_ref(ref_index);
            switch (opcode) {
            case Opcode::GET_STATIC:
                visitor.visit_get_field(owner, name, descriptor, false);
                break;
            case Opcode::INVOKE_VIRTUAL:
                visitor.visit_invoke(InvokeKind::Virtual, owner, name, descriptor);
                break;
            case Opcode::INVOKE_SPECIAL:
                visitor.visit_invoke(InvokeKind::Special, owner, name, descriptor);
                break;
            default:
                jamf::unreachable();
            }
            continue;
        }

        switch (opcode) {
        case Opcode::ACONST_NULL:
            break;
        case Opcode::LCONST_0:
            break;
        case Opcode::LCONST_1:
            break;
        case Opcode::FCONST_0:
            break;
        case Opcode::FCONST_1:
            break;
        case Opcode::FCONST_2:
            break;
        case Opcode::DCONST_0:
            break;
        case Opcode::DCONST_1:
            break;
        case Opcode::BIPUSH:
            break;
        case Opcode::SIPUSH:
            break;
        case Opcode::LDC:
            visitor.visit_constant(constant_pool.read_constant(JAMF_TRY(stream.read_byte())));
            continue;
        case Opcode::LDC_W:
        case Opcode::LDC2_W:
            visitor.visit_constant(constant_pool.read_constant(JAMF_TRY(stream.read_be<std::uint16_t>())));
            continue;
        case Opcode::POP:
            break;
        case Opcode::POP2:
            break;
        case Opcode::DUP:
            break;
        case Opcode::DUP_X1:
            break;
        case Opcode::DUP_X2:
            break;
        case Opcode::DUP2:
            break;
        case Opcode::DUP2_X1:
            break;
        case Opcode::DUP2_X2:
            break;
        case Opcode::SWAP:
            break;
        }

        std::cerr << "Unknown opcode: " << static_cast<std::uint32_t>(opcode) << '\n';
        return ParseError::UnknownOpcode;
    }
    return {};
}

} // namespace

Result<void, ParseError, StreamError> parse_class(Stream &stream, Visitor &visitor) {
    const auto magic = JAMF_TRY(stream.read_be<std::uint32_t>());
    if (magic != 0xcafebabe) {
        return ParseError::BadMagic;
    }

    // Skip minor and major numbers.
    JAMF_TRY(stream.read_be<std::uint16_t>()); // minor
    JAMF_TRY(stream.read_be<std::uint16_t>()); // major

    // Skip past constant pool, but create an index->offset map as well as preloading strings.
    ConstantPool constant_pool(JAMF_TRY(stream.read_be<std::uint16_t>()));
    for (std::uint16_t i = 1; i < constant_pool.size(); i++) {
        const auto tag = static_cast<ConstantKind>(JAMF_TRY(stream.read_byte()));
        constant_pool.set_offset(i, JAMF_ASSUME(stream.seek(0, SeekMode::Add)) - 10);

        switch (tag) {
        case ConstantKind::Utf8: {
            const auto length = JAMF_TRY(stream.read_be<std::uint16_t>());
            String string(length);
            JAMF_TRY(stream.read({string.data(), length}));
            constant_pool.set_string(i, jamf::move(string));
            break;
        }
        case ConstantKind::Integer:
        case ConstantKind::Float:
            JAMF_TRY(stream.seek(4, SeekMode::Add));
            break;
        case ConstantKind::Long:
        case ConstantKind::Double:
            JAMF_TRY(stream.seek(8, SeekMode::Add));
            // If a CONSTANT_Long_info or CONSTANT_Double_info structure is the entry at index n ... the constant pool
            // index n+1 must be valid but is considered unusable.
            i++;
            break;
        case ConstantKind::Class:
        case ConstantKind::String:
            JAMF_TRY(stream.seek(2, SeekMode::Add));
            break;
        case ConstantKind::FieldRef:
        case ConstantKind::MethodRef:
        case ConstantKind::InterfaceMethodRef:
        case ConstantKind::NameAndType:
            JAMF_TRY(stream.seek(4, SeekMode::Add));
            break;
        default:
            std::cerr << "Unknown constant kind " << static_cast<std::uint16_t>(tag) << '\n';
            return ParseError::UnknownConstantPoolEntry;
        }
    }

    // Reload the constant pool into a bytebuffer.
    const auto cp_size = JAMF_ASSUME(stream.seek(0, SeekMode::Add)) - 10;
    auto cp_bytes = FixedBuffer<std::uint8_t>::create_uninitialised(cp_size);
    JAMF_TRY(stream.seek(10, SeekMode::Set));
    JAMF_TRY(stream.read(cp_bytes.span().as<std::uint8_t, std::uint32_t>()));
    constant_pool.set_bytes(jamf::move(cp_bytes));

    JAMF_TRY(stream.read_be<std::uint16_t>()); // access flags

    // Valid index into the constant_pool table to a CONSTANT_Class_info struct.
    const auto this_class = JAMF_TRY(stream.read_be<std::uint16_t>());

    // Must either be zero or valid index to CONSTANT_Class_info.
    // In practice only zero for class Object.
    const auto super_class = JAMF_TRY(stream.read_be<std::uint16_t>());

    // Each interfaces[i] must be CONSTANT_Class_info
    auto interface_count = JAMF_TRY(stream.read_be<std::uint16_t>());
    Vector<String> interfaces;
    interfaces.ensure_capacity(interface_count);
    while (interface_count-- > 0) {
        interfaces.push(constant_pool.read_string_like(JAMF_TRY(stream.read_be<std::uint16_t>())));
    }

    visitor.visit(constant_pool.read_string_like(this_class), constant_pool.read_string_like(super_class));

    auto field_count = JAMF_TRY(stream.read_be<std::uint16_t>());
    while (field_count-- > 0) {
        JAMF_TRY(stream.read_be<std::uint16_t>()); // access flags
        const auto name = constant_pool.read_utf(JAMF_TRY(stream.read_be<std::uint16_t>()));
        const auto descriptor = constant_pool.read_utf(JAMF_TRY(stream.read_be<std::uint16_t>()));
        JAMF_TRY(
            iterate_attributes(stream, constant_pool, [&](StringView name) -> Result<bool, ParseError, StreamError> {
                if (name != "ConstantValue") {
                    return false;
                }
                // TODO: Handle ConstantValue.
                JAMF_TRY(stream.read_be<std::uint16_t>());
                return true;
            }));
        visitor.visit_field(name, descriptor);
    }

    auto method_count = JAMF_TRY(stream.read_be<std::uint16_t>());
    while (method_count-- > 0) {
        JAMF_TRY(stream.read_be<std::uint16_t>()); // access flags
        const auto name = constant_pool.read_utf(JAMF_TRY(stream.read_be<std::uint16_t>()));
        const auto descriptor = constant_pool.read_utf(JAMF_TRY(stream.read_be<std::uint16_t>()));
        visitor.visit_method(name, descriptor);

        JAMF_TRY(
            iterate_attributes(stream, constant_pool, [&](StringView name) -> Result<bool, ParseError, StreamError> {
                if (name != "Code") {
                    return false;
                }

                JAMF_TRY(parse_code(stream, visitor, constant_pool));

                auto exception_count = JAMF_TRY(stream.read_be<std::uint16_t>());
                while (exception_count-- > 0) {
                    JAMF_TRY(stream.seek(8, SeekMode::Add));
                }

                JAMF_TRY(
                    iterate_attributes(stream, constant_pool, [](StringView) -> Result<bool, ParseError, StreamError> {
                        return false;
                    }));
                return true;
            }));
    }

    // Iterate ClassFile attributes.
    JAMF_TRY(iterate_attributes(stream, constant_pool, [](StringView name) -> Result<bool, ParseError, StreamError> {
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

} // namespace jamf
