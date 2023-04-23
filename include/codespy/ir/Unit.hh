#pragma once

namespace codespy::ir {

class Unit {
    Graph<Function> m_call_graph;

public:
    template <typename... Args>
    Function *create_function(Args &&... args);
};

}
