#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include "ai_types.h"

class GameWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QStackedWidget *m_stackedWidget;
    QWidget *m_menuWidget;
    GameWidget *m_gameWidget;

    void setupMenu();
    void showModeDialog();
    void startGame(GameMode mode);
    void showSettingsDialog();
    void showCreditsDialog();
};

#endif // MAINWINDOW_H
