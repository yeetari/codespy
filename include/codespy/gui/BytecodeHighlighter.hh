#pragma once

#include <codespy/container/Vector.hh>

#include <QRegularExpression>
#include <QSyntaxHighlighter>

namespace codespy::gui {

class BytecodeHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

private:
    struct Rule {
        QRegularExpression regex;
        QTextCharFormat format;

        Rule(const QString &pattern, QTextCharFormat format)
            : regex(pattern), format(std::move(format)) {}
    };
    Vector<Rule> m_rules;

protected:
    void highlightBlock(const QString &text) override;

public:
    explicit BytecodeHighlighter(QTextDocument *parent);
};

} // namespace codespy::gui
