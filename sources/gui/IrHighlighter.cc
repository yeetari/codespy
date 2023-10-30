#include <codespy/gui/IrHighlighter.hh>

namespace codespy::gui {

IrHighlighter::IrHighlighter(QTextDocument *parent) : QSyntaxHighlighter(parent) {
    QTextCharFormat keyword_format;
    keyword_format.setForeground(Qt::darkBlue);
    keyword_format.setFontWeight(QFont::Bold);
    m_rules.emplace("\\b(array_length"
                    "|add"
                    "|sub"
                    "|mul"
                    "|div"
                    "|rem"
                    "|shl"
                    "|shr"
                    "|ushr"
                    "|and"
                    "|or"
                    "|xor"
                    "|br"
                    "|call"
                    "|special"
                    "|cast"
                    "|to"
                    "|catch"
                    "|cmp_eq"
                    "|cmp_ne"
                    "|cmp_lt"
                    "|cmp_gt"
                    "|cmp_le"
                    "|cmp_ge"
                    "|instance_of"
                    "|lcmp"
                    "|fcmpl"
                    "|fcmpg"
                    "|dcmpl"
                    "|dcmpg"
                    "|load"
                    "|load_array"
                    "|load_field"
                    "|monitor_enter"
                    "|monitor_exit"
                    "|neg"
                    "|new"
                    "|new_array"
                    "|phi"
                    "|ret"
                    "|store"
                    "|store_array"
                    "|store_field"
                    "|switch"
                    "|throw"
                    ")\\b",
                    keyword_format, false);

    QTextCharFormat type_format;
    type_format.setForeground(QColor("#499F68"));
    type_format.setFontWeight(QFont::Bold);
    m_rules.emplace("\\b(any|void)\\b", type_format, false);
    m_rules.emplace("\\b[if][0-9]+\\b", type_format, false);
    m_rules.emplace("#[a-zA-Z\\/]+", type_format, true);

    QTextCharFormat label_format;
    label_format.setForeground(Qt::red);
    label_format.setFontWeight(QFont::Bold);
    m_rules.emplace("\\bL[0-9]+\\b", label_format, true);

    QTextCharFormat value_format;
    value_format.setForeground(Qt::darkGreen);
    m_rules.emplace("%[alv][0-9]+", value_format, false);

    QTextCharFormat function_format;
    function_format.setForeground(QColor("orange"));
    m_rules.emplace("@[a-zA-Z.\\/<>]+", function_format, true);

    QTextCharFormat constant_format;
    constant_format.setForeground(QColor("#2E86AB"));
    m_rules.emplace("\\b(poison|null)\\b", constant_format, false);
    m_rules.emplace("\\$[0-9.]+", constant_format, false);
    m_rules.emplace("\".*\"", constant_format, false);

    for (auto &rule : m_rules) {
        rule.regex.optimize();
    }
}

void IrHighlighter::highlightBlock(const QString &text) {
    for (const auto &rule : m_rules) {
        auto match_iterator = rule.regex.globalMatch(text);
        while (match_iterator.hasNext()) {
            auto match = match_iterator.next();
            auto format = rule.format;
            if (rule.underline && m_underline) {
                format.setAnchor(true);
                format.setAnchorHref("ababa");
                format.setFontUnderline(true);
            }
            setFormat(match.capturedStart(), match.capturedLength(), format);
        }
    }
}

} // namespace codespy::gui
