#include "src/mainwindow.h"
#include "src/maintray.h"
#include "src/audiocontroller.h"
#include <objbase.h>

int main(int argc, char *argv[])
{
    CoInitialize(NULL);
    QApplication a(argc, argv);
    
    qRegisterMetaType<AudioSessionData>("AudioSessionData");

    QString iniPath = QCoreApplication::applicationDirPath() + "/config.ini";
    QSettings settings(iniPath, QSettings::IniFormat);

    // 传入配置
    AudioController controller(&settings);
    
    MainWindow w(&settings);//主窗口

    QObject::connect(&controller, &AudioController::sessionAdded, &w, &MainWindow::onSessionAdded);
    QObject::connect(&controller, &AudioController::sessionRemoved, &w, &MainWindow::onSessionRemoved);
    QObject::connect(&w, &MainWindow::volumeChanged, &controller, &AudioController::setVolume);

    controller.start();

    MainTray t(&w, &settings);//托盘图标

    int ret = a.exec();
    CoUninitialize();
    return ret;
}
