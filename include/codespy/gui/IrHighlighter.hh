#pragma once

#include <codespy/container/Vector.hh>

#include <QRegularExpression>
#include <QSyntaxHighlighter>

namespace codespy::gui {

class IrHighlighter : public QSyntaxHighlighter {
    Q_OBJECT

private:
    struct Rule {
        QRegularExpression regex;
        QTextCharFormat format;
        bool underline;

        Rule(const QString &pattern, QTextCharFormat format, bool underline)
            : regex(pattern), format(std::move(format)), underline(underline) {}
    };
    Vector<Rule> m_rules;
    bool m_underline{false};

protected:
    void highlightBlock(const QString &text) override;

public:
    explicit IrHighlighter(QTextDocument *parent);

    void set_underline(bool underline) { m_underline = underline; }

    auto text_block() { return currentBlock(); }
};

} // namespace codespy::gui
