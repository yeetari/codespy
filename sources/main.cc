#include <codespy/bytecode/ClassFile.hh>
#include <codespy/bytecode/Dumper.hh>
#include <codespy/bytecode/Frontend.hh>
#include <codespy/bytecode/Visitor.hh>
#include <codespy/container/Vector.hh>
#include <codespy/ir/Context.hh>
#include <codespy/ir/Dumper.hh>
#include <codespy/ir/Function.hh>
#include <codespy/ir/Java.hh>
#include <codespy/support/Enum.hh>
#include <codespy/support/Print.hh>
#include <codespy/support/SpanStream.hh>
#include <codespy/support/StringBuilder.hh>
#include <codespy/transform/ExceptionPruner.hh>
#include <codespy/transform/LocalPromoter.hh>

#include <fstream>
#include <iostream>
#include <miniz/miniz.h>

using namespace codespy;

int main(int, char **argv) {
    ir::Context context;
    bc::Frontend frontend(context);

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
            SpanStream stream(codespy::make_span(static_cast<std::uint8_t *>(data), size));
            CODESPY_EXPECT(bc::parse_class(stream, frontend));
            free(data);
        }
    }
    mz_zip_reader_end(&zip_archive);

    auto class_map = std::move(frontend.class_map());
    for (const auto &[name, clazz] : class_map) {
        for (auto *function : clazz.methods()) {
            ir::prune_exceptions(function);
            ir::promote_locals(function);
            codespy::println(ir::dump_code(function));
        }
    }
}
