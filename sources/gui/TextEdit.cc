#include <codespy/gui/TextEdit.hh>

#include <codespy/gui/IrHighlighter.hh>
#include <codespy/support/UniquePtr.hh>

#include <QApplication>
#include <QMenu>

namespace codespy::gui {

TextEdit::TextEdit(QWidget *parent) : QPlainTextEdit(parent), m_highlighter(new IrHighlighter(document())) {
    setLineWrapMode(QPlainTextEdit::NoWrap);
}

void TextEdit::contextMenuEvent(QContextMenuEvent *event) {
    auto menu = codespy::adopt_unique(createStandardContextMenu());
    if (textCursor().hasSelection()) {
        menu->addAction("Rename");
    }
    menu->exec(event->globalPos());
}

void TextEdit::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Control) {
        m_highlighter->set_underline(true);
        m_highlighter->rehighlight();
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
}

void TextEdit::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Control) {
        m_highlighter->set_underline(false);
        m_highlighter->rehighlight();
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
}

} // namespace codespy::gui
