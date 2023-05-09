#include <codespy/gui/MainWindow.hh>

#include <codespy/gui/IrHighlighter.hh>
#include <codespy/gui/TextEdit.hh>
#include <codespy/gui/TreeModel.hh>

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QSyntaxHighlighter>
#include <QTextDocument>
#include <QTreeView>

namespace codespy::gui {

MainWindow::MainWindow(const QString &text) {
    auto *file_menu = menuBar()->addMenu("&File");

    auto *open_action = file_menu->addAction("&Open");
    file_menu->addAction(open_action);
    auto *exit_action = file_menu->addAction("E&xit");
    file_menu->addAction(exit_action);

    auto *splitter = new QSplitter;
    setCentralWidget(splitter);

    QFont font;
    font.setFamily("DroidSansMono");
    font.setFixedPitch(true);
    font.setPointSize(12);

    auto *tree_model = new TreeModel;
    auto *tree_view = new QTreeView(centralWidget());
    tree_view->setModel(tree_model);

    auto *editor = new TextEdit(centralWidget());
    editor->setFont(font);
    editor->setReadOnly(true);
    editor->setPlainText(text);

    splitter->setSizes({200, 1000});
    resize(1024, 768);
}

} // namespace codespy::gui
