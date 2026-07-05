#include "MainWindow.h"

#include "MainWindow/MainWindowPrivate.h"

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), d(std::make_unique<MainWindowPrivate>(this)) {
    d->buildUi();
    d->buildMenuBar();
    d->applyStyle();
}

MainWindow::~MainWindow() = default;
