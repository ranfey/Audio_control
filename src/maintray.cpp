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
    QAction *toggleVisibilityAction = new QAction("显示/隐藏");
    
    // 层级子菜单
    QMenu *levelMenu = new QMenu("层级");
    QAction *actTop = new QAction("置顶");
    QAction *actNormal = new QAction("置中");
    QAction *actBottom = new QAction("置底");
    levelMenu->addAction(actTop);
    levelMenu->addAction(actNormal);
    levelMenu->addAction(actBottom);

    QAction *exitAction = new QAction("exit");

    trayMenu->addAction(toggleModeAction);
    trayMenu->addAction(toggleVisibilityAction);
    trayMenu->addMenu(levelMenu); // 子菜单
    trayMenu->addSeparator();
    trayMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    // 信号槽连接
    QObject::connect(toggleModeAction,  &QAction::triggered, mainWindow, &MainWindow::toggleInteractMode);
    
    // 使用 Lambda 连接层级切换
    QObject::connect(actTop, &QAction::triggered, mainWindow, [mainWindow](){
        mainWindow->setWindowLevel(MainWindow::LevelTop);
    });
    QObject::connect(actNormal, &QAction::triggered, mainWindow, [mainWindow](){
        mainWindow->setWindowLevel(MainWindow::LevelNormal);
    });
    QObject::connect(actBottom, &QAction::triggered, mainWindow, [mainWindow](){
        mainWindow->setWindowLevel(MainWindow::LevelBottom);
    });

    QObject::connect(toggleVisibilityAction, &QAction::triggered, mainWindow, [mainWindow](){
        if (mainWindow->isVisible())
            mainWindow->hide();
        else
            mainWindow->show();
    });
    QObject::connect(exitAction, &QAction::triggered, mainWindow, &QApplication::quit);
}

MainTray::~MainTray()
{

}