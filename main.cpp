#include "mainwindow.h"
#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSettings>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString iniPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(iniPath, QSettings::IniFormat);

    MainWindow w(&settings);//主窗口

    // 创建托盘图标
    QSystemTrayIcon *trayIcon = new QSystemTrayIcon(QIcon(":/img/icon.ico"));
    trayIcon->setToolTip("Audio_control");

    // 创建菜单
    QMenu *trayMenu = new QMenu();
    QAction *frameless = new QAction("移动");
    QAction *showAction = new QAction("显示");
    QAction *hideAction = new QAction("隐藏");
    QAction *exitAction = new QAction("exit");

    trayMenu->addAction(frameless);
    trayMenu->addAction(showAction);
    trayMenu->addAction(hideAction);
    trayMenu->addSeparator();
    trayMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    // 信号槽连接
    QObject::connect(frameless,  &QAction::triggered, &w, &MainWindow::toggleFrameless);
    QObject::connect(showAction, &QAction::triggered, &w, &QWidget::show);
    QObject::connect(hideAction, &QAction::triggered, &w, &QWidget::hide);
    QObject::connect(exitAction, &QAction::triggered, &a, &QApplication::quit);



    return a.exec();
}
