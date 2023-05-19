#pragma once

#include <codespy/container/Vector.hh>
#include <codespy/support/String.hh>

namespace codespy {

class AstNode {
public:
    virtual ~AstNode() = default;
};

class AstSequence : public AstNode {
    Vector<AstNode *> m_children;

public:
    void add_child(AstNode *child) { m_children.push(child); }

    const Vector<AstNode *> &children() const { return m_children; }
};

class IfStatement : public AstNode {
    AstNode *m_condition;
    AstNode *m_then_body;
    AstNode *m_else_body{nullptr};

public:
    IfStatement(AstNode *condition, AstNode *then_body) : m_condition(condition), m_then_body(then_body) {}

    bool has_else_body() const { return m_else_body != nullptr; }
    AstNode *condition() const { return m_condition; }
    AstNode *then_body() const { return m_then_body; }
    AstNode *else_body() const { return m_else_body; }
};

class AstComment : public AstNode {
    String m_text;

public:
    explicit AstComment(String text) : m_text(std::move(text)) {}

    const String &text() const { return m_text; }
};

} // namespace codespy
