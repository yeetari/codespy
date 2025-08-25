#include <codespy/gui/BytecodeHighlighter.hh>

namespace codespy::gui {

BytecodeHighlighter::BytecodeHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent) {
    QTextCharFormat keyword_format;
    keyword_format.setForeground(Qt::darkBlue);
    keyword_format.setFontWeight(QFont::Bold);
    m_rules.emplace("\\b(class"
                    "|extends"
                    "|field"
                    "|method"
                    "|a?[ilfdabcs](load|store)"
                    "|[ilfdabcs]?return"
                    "|[ilfdbcs]2[ilfdbcs]"
                    "|aconst_null"
                    "|iconst_m1"
                    "|iconst_[0-5]"
                    "|lconst_[01]"
                    "|fconst_[012]"
                    "|dconst_[01]"
                    "|[bs]ipush"
                    "|ldc"
                    "|[fd]cmp[gl]"
                    "|new"
                    "|getfield"
                    "|getstatic"
                    "|putfield"
                    "|putstatic"
                    "|invoke(interface|special|static|virtual)"
                    "|[ilfd](add|sub|mul|div|rem|neg)"
                    "|[il](shl|and|or|xor)"
                    "|[il]u?shr"
                    "|monitor(enter|exit)"
                    "|arraylength"
                    "|athrow"
                    "|dup2?(_x[12])?"
                    "|pop2?"
                    "|swap"
                    "|checkcast"
                    "|instanceof"
                    "|iinc"
                    "|goto"
                    "|if(non)?null"
                    "|if(_[ai]cmp)?(eq|ne|lt|ge|gt|le)"
                    "|(lookup|table)switch"
                    ")\\b",
                    keyword_format);

//    QTextCharFormat type_format;
//    type_format.setForeground(QColor("#499F68"));
//    type_format.setFontWeight(QFont::Bold);
//    m_rules.emplace("\\b(any|void)\\b", type_format, false);
//    m_rules.emplace("\\b[if][0-9]+\\b", type_format, false);
//    m_rules.emplace("#[a-zA-Z\\/]+", type_format, true);
//
//    QTextCharFormat label_format;
//    label_format.setForeground(Qt::red);
//    label_format.setFontWeight(QFont::Bold);
//    m_rules.emplace("\\bL[0-9]+\\b", label_format, true);
//
//    QTextCharFormat value_format;
//    value_format.setForeground(Qt::darkGreen);
//    m_rules.emplace("%[alv][0-9]+", value_format, false);
//
//    QTextCharFormat function_format;
//    function_format.setForeground(QColor("orange"));
//    m_rules.emplace("@[a-zA-Z.\\/<>]+", function_format, true);
//
//    QTextCharFormat constant_format;
//    constant_format.setForeground(QColor("#2E86AB"));
//    m_rules.emplace("\\b(poison|null)\\b", constant_format, false);
//    m_rules.emplace("\\$[0-9.]+", constant_format, false);
//    m_rules.emplace("\".*\"", constant_format, false);

    for (auto &rule : m_rules) {
        rule.regex.optimize();
    }
}

void BytecodeHighlighter::highlightBlock(const QString &text) {
    for (const auto &rule : m_rules) {
        auto match_iterator = rule.regex.globalMatch(text);
        while (match_iterator.hasNext()) {
            auto match = match_iterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }
}

} // namespace codespy::gui
