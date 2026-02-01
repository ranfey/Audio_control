#include "maintray.h"
#include <iostream>

MainTray::MainTray(MainWindow *mainWindow, QSettings *settings, QWidget *parent)
{
    // 创建托盘图标
    QSystemTrayIcon *trayIcon = new QSystemTrayIcon(QIcon(":/img/icon.ico"));
    trayIcon->setToolTip("Audio_control");

    // 创建菜单
    QMenu *trayMenu = new QMenu();
    QAction *toggleModeAction = new QAction("移动/锁定");
    QAction *showAction = new QAction("显示");
    QAction *hideAction = new QAction("隐藏");
    QAction *exitAction = new QAction("exit");

    trayMenu->addAction(toggleModeAction);
    trayMenu->addAction(showAction);
    trayMenu->addAction(hideAction);
    trayMenu->addSeparator();
    trayMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    // 信号槽连接
    QObject::connect(toggleModeAction,  &QAction::triggered, mainWindow, &MainWindow::toggleInteractMode);
    QObject::connect(showAction, &QAction::triggered, mainWindow, &QWidget::show);
    QObject::connect(hideAction, &QAction::triggered, mainWindow, &QWidget::hide);
    QObject::connect(exitAction, &QAction::triggered, mainWindow, &QApplication::quit);
}

MainTray::~MainTray()
{

}