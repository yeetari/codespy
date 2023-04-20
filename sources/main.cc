#include <bytecode/ClassFile.hh>
#include <bytecode/Visitor.hh>
#include <container/Vector.hh>
#include <miniz/miniz.h>
#include <support/Enum.hh>
#include <support/Print.hh>
#include <support/SpanStream.hh>

#include <fstream>
#include <iostream>

using namespace jamf;

struct CoolVisitor : public Visitor {
    void visit(StringView this_name, StringView super_name) override {
        std::cout << this_name.data() << " extends " << super_name.data() << '\n';
    }

    void visit_field(StringView name, StringView descriptor) override {
        std::cout << "  field " << name.data() << ' ' << descriptor.data() << '\n';
    }

    void visit_method(StringView name, StringView descriptor) override {
        std::cout << "  method " << name.data() << ' ' << descriptor.data() << '\n';
    }

    void visit_code(std::uint16_t, std::uint16_t) override {}

    void visit_constant(Constant constant) override {
        if (constant.has<std::int32_t>()) {
            jamf::println("    ldc {}", constant.get<std::int32_t>());
        } else if (constant.has<std::int64_t>()) {
            jamf::println("    ldc {}", constant.get<std::int64_t>());
        } else if (constant.has<float>()) {
            jamf::println("    ldc {}", constant.get<float>());
        } else if (constant.has<double>()) {
            jamf::println("    ldc {}", constant.get<double>());
        } else if (constant.has<StringView>()) {
            jamf::println("    ldc \"{}\"", constant.get<StringView>());
        }
    }

    void visit_load(BaseType type, std::uint8_t local_index) override {
        jamf::println("    load {} ({})", local_index, jamf::enum_name<1>(type));
    }

    void visit_store(BaseType type, std::uint8_t local_index) override {
        jamf::println("    store {} ({})", local_index, jamf::enum_name<1>(type));
    }

    void visit_cast(BaseType from_type, BaseType to_type) override {
        jamf::println("    cast {} -> {}", jamf::enum_name<1>(from_type), jamf::enum_name<1>(to_type));
    }

    void visit_get_field(StringView owner, StringView name, StringView descriptor, bool) override {
        jamf::println("    getfield {}.{}:{}", owner, name, descriptor);
    }

    void visit_invoke(InvokeKind kind, StringView owner, StringView name, StringView descriptor) override {
        const auto *kind_string = kind == InvokeKind::Special ? "special" : "virtual";
        jamf::println("    invoke{} {}.{}:{}", kind_string, owner, name, descriptor);
    }

    void visit_return(BaseType type) override { jamf::println("    return ({})", jamf::enum_name<1>(type)); }
};

int main(int, char **argv) {
    mz_zip_archive zip_archive{};
    mz_zip_reader_init_file(&zip_archive, argv[1], 0);
    const auto zip_entry_count = mz_zip_reader_get_num_files(&zip_archive);
    for (mz_uint i = 0; i < zip_entry_count; i++) {
        Array<char, 256> name_chars{};
        mz_zip_reader_get_filename(&zip_archive, i, name_chars.data(), name_chars.size());
        String name(name_chars.data());
        if (name.ends_with(".class")) {
            std::size_t size;
            void *data = mz_zip_reader_extract_to_heap(&zip_archive, i, &size, 0);
            SpanStream stream(jamf::make_span(static_cast<std::uint8_t *>(data), size));
            CoolVisitor visitor;
            JAMF_EXPECT(jamf::parse_class(stream, visitor));
        }
    }
    mz_zip_reader_end(&zip_archive);
}
