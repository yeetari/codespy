#pragma once

// NOLINTBEGIN(bugprone-macro-parentheses)
namespace codespy::ir {

#define INST(opcode, Class) class Class;
#include <codespy/ir/Instructions.in>

class Visitor {
public:
#define INST(opcode, Class) virtual void visit(Class &) = 0;
#include <codespy/ir/Instructions.in>
};

} // namespace codespy::ir
// NOLINTEND(bugprone-macro-parentheses)
