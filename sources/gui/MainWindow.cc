#include <codespy/gui/MainWindow.hh>

#include <codespy/gui/IrHighlighter.hh>
#include <codespy/gui/TextEdit.hh>
#include <codespy/gui/TreeModel.hh>
#include <codespy/support/Print.hh>

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

MainWindow::MainWindow(Vector<ClassData> &&classes) {
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

    auto *tree_model = new TreeModel(std::move(classes));
    auto *tree_view = new QTreeView(centralWidget());
    tree_view->setModel(tree_model);

    auto *ir_editor = new TextEdit(centralWidget());
    ir_editor->setFont(font);
    ir_editor->setReadOnly(true);

    auto *bc_editor = new TextEdit(centralWidget());
    bc_editor->setFont(font);
    bc_editor->setReadOnly(true);

    splitter->setSizes({200, 1000, 500});
    resize(1024, 768);

    QObject::connect(tree_view->selectionModel(), &QItemSelectionModel::currentRowChanged,
                     [=](const QModelIndex &current, const QModelIndex &) {
                         if (!current.isValid()) {
                             return;
                         }
                         ir_editor->setPlainText(tree_model->ir_text(current.internalId()));
                         bc_editor->setPlainText(tree_model->bc_text(current.internalId()));
                     });
}

} // namespace codespy::gui
