#pragma once

#include <codespy/container/Vector.hh>
#include <codespy/ir/Constant.hh>
#include <codespy/ir/Type.hh>
#include <codespy/support/UniquePtr.hh>

#include <unordered_map>

namespace codespy::ir {

class Context {
    struct ConstantIntKey {
        std::int64_t value;
        std::uint16_t bit_width;

        friend bool operator==(const ConstantIntKey &lhs, const ConstantIntKey &rhs) {
            return lhs.value == rhs.value && lhs.bit_width == rhs.bit_width;
        }
    };

    struct FunctionTypeKey {
        Type *return_type;
        Span<Type *> parameter_types;

        friend bool operator==(const FunctionTypeKey &lhs, const FunctionTypeKey &rhs) {
            if (lhs.return_type != rhs.return_type) {
                return false;
            }
            if (lhs.parameter_types.size() != rhs.parameter_types.size()) {
                return false;
            }
            return std::equal(lhs.parameter_types.begin(), lhs.parameter_types.end(), rhs.parameter_types.begin());
        }
    };

    struct ConstantIntKeyHash {
        std::size_t operator()(const ConstantIntKey &key) const {
            // TODO: Actual int hash.
            return codespy::hash_combine(key.value, key.bit_width);
        }
    };

    struct FunctionTypeKeyHash {
        std::size_t operator()(const FunctionTypeKey &key) const {
            // TODO: Actual pointer hash.
            auto hash = std::hash<Type *>{}(key.return_type);
            for (auto *type : key.parameter_types) {
                hash = codespy::hash_combine(hash, std::hash<Type *>{}(type));
            }
            return hash;
        }
    };

private:
    Type m_any_type{TypeKind::Any};
    Type m_label_type{TypeKind::Label};
    Type m_float_type{TypeKind::Float};
    Type m_double_type{TypeKind::Double};
    Type m_void_type{TypeKind::Void};
    std::unordered_map<Type *, UniquePtr<ArrayType>> m_array_types;
    std::unordered_map<std::uint16_t, UniquePtr<IntType>> m_int_types;
    std::unordered_map<String, UniquePtr<ReferenceType>> m_reference_types;
    std::unordered_map<FunctionTypeKey, UniquePtr<FunctionType>, FunctionTypeKeyHash> m_function_types;

    Value m_constant_null;
    std::unordered_map<double, UniquePtr<ConstantDouble>> m_double_constants;
    std::unordered_map<float, UniquePtr<ConstantFloat>> m_float_constants;
    std::unordered_map<ConstantIntKey, UniquePtr<ConstantInt>, ConstantIntKeyHash> m_int_constants;
    std::unordered_map<String, UniquePtr<ConstantString>> m_string_constants;
    std::unordered_map<Type *, UniquePtr<PoisonValue>> m_poison_values;

public:
    Context();
    Context(const Context &) = delete;
    Context(Context &&) = delete;
    ~Context() = default;

    Context &operator=(const Context &) = delete;
    Context &operator=(Context &&) = delete;

    Type *any_type() { return &m_any_type; }
    Type *label_type() { return &m_label_type; }
    Type *float_type() { return &m_float_type; }
    Type *double_type() { return &m_double_type; }
    Type *void_type() { return &m_void_type; }

    ArrayType *array_type(Type *element_type);
    FunctionType *function_type(Type *return_type, Vector<Type *> &&parameter_types);
    IntType *int_type(std::uint16_t bit_width);
    ReferenceType *reference_type(String class_name);

    Value *constant_null() { return &m_constant_null; }
    ConstantDouble *constant_double(double value);
    ConstantFloat *constant_float(float value);
    ConstantInt *constant_int(IntType *type, std::int64_t value);
    ConstantString *constant_string(String value);
    PoisonValue *poison_value(Type *type);
};

} // namespace codespy::ir
