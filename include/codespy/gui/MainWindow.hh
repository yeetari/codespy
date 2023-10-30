#pragma once

#include <codespy/gui/TreeModel.hh>

#include <QMainWindow>

namespace codespy::gui {

class MainWindow : public QMainWindow {
    Q_OBJECT;

public:
    MainWindow(Vector<ClassData> &&classes);
};

} // namespace codespy::gui
