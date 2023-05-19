#pragma once

namespace codespy::ir {

class Function;

} // namespace codespy::ir

namespace codespy {

void decompile(ir::Function *function);

} // namespace codespy
