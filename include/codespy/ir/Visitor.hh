#pragma once

namespace codespy::ir {

class BinaryInst;
class BranchInst;
class CallInst;
class CompareInst;
class LoadInst;
class LoadFieldInst;
class ReturnInst;
class StoreInst;

class Visitor {
public:
    virtual void visit(BinaryInst &binary) = 0;
    virtual void visit(BranchInst &branch) = 0;
    virtual void visit(CallInst &call) = 0;
    virtual void visit(CompareInst &compare) = 0;
    virtual void visit(LoadInst &load) = 0;
    virtual void visit(LoadFieldInst &load_field) = 0;
    virtual void visit(ReturnInst &return_inst) = 0;
    virtual void visit(StoreInst &store) = 0;
};

} // namespace codespy::ir
