#pragma once

#include <QPlainTextEdit>

namespace codespy::gui {

class IrHighlighter;

class TextEdit : public QPlainTextEdit {
    Q_OBJECT

private:
    IrHighlighter *const m_highlighter;

public:
    explicit TextEdit(QWidget *parent);

    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
};

} // namespace codespy::gui
