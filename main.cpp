#include "mainwindow.h"
#include "maintray.h"



int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString iniPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(iniPath, QSettings::IniFormat);

    MainWindow w(&settings);//主窗口

    MainTray t(&w, &settings);//托盘图标

    return a.exec();
}
