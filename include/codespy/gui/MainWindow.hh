#pragma once

#include <QMainWindow>

namespace codespy::gui {

class MainWindow : public QMainWindow {
    Q_OBJECT;

public:
    MainWindow(const QString &text);
};

} // namespace codespy::gui
