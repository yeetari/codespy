#pragma once

#include <codespy/support/String.hh>

namespace codespy::ir {

class Function;

String dump_code(Function *function);

} // namespace codespy::ir
