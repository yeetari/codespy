#pragma once

namespace codespy::ir {

class CallInst;
class LoadFieldInst;
class ReturnInst;

class Visitor {
public:
    virtual void visit(CallInst &call) = 0;
    virtual void visit(LoadFieldInst &load_field) = 0;
    virtual void visit(ReturnInst &return_inst) = 0;
};

} // namespace codespy::ir
