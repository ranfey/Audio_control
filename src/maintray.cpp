#include "maintray.h"

MainTray::MainTray(MainWindow *mainWindow, QSettings *settings, QWidget *parent)
{
    // 创建托盘图标
    QSystemTrayIcon *trayIcon = new QSystemTrayIcon(QIcon(":/img/icon.ico"));
    trayIcon->setToolTip("Audio_control");

    // 创建菜单
    QMenu *trayMenu = new QMenu();
    QAction *toggleModeAction = new QAction("移动");
    toggleModeAction->setCheckable(true);

    QAction *mouseThroughAction = new QAction("鼠标穿透");
    mouseThroughAction->setCheckable(true);
    
    QAction *toggleVisibilityAction = new QAction("显示");
    toggleVisibilityAction->setCheckable(true);
    
    // 层级子菜单
    QMenu *levelMenu = new QMenu("层级");
    QAction *actTop = new QAction("置顶");
    QAction *actNormal = new QAction("置中");
    QAction *actBottom = new QAction("置底");
    
    actTop->setCheckable(true);
    actNormal->setCheckable(true);
    actBottom->setCheckable(true);

    QActionGroup *levelGroup = new QActionGroup(trayMenu);
    levelGroup->addAction(actTop);
    levelGroup->addAction(actNormal);
    levelGroup->addAction(actBottom);

    levelMenu->addAction(actTop);
    levelMenu->addAction(actNormal);
    levelMenu->addAction(actBottom);

    QAction *autoStartAction = new QAction("开机自启");
    autoStartAction->setCheckable(true);
    
    QString appName = "Audio_control";
    
    // 获取 Windows 启动目录
    wchar_t path[MAX_PATH];
    QString startupPath;
    if (SHGetSpecialFolderPathW(nullptr, path, CSIDL_STARTUP, FALSE)) {
        startupPath = QString::fromWCharArray(path);
    } else {
        startupPath = QDir::homePath() + "/AppData/Roaming/Microsoft/Windows/Start Menu/Programs/Startup";
    }
    
    QString linkPath = QDir(startupPath).filePath(appName + ".lnk");
    autoStartAction->setChecked(QFile::exists(linkPath));

    QAction *exitAction = new QAction("退出");

    trayMenu->addAction(toggleModeAction);
    trayMenu->addAction(mouseThroughAction);
    trayMenu->addAction(toggleVisibilityAction);
    trayMenu->addMenu(levelMenu); // 子菜单
    trayMenu->addAction(autoStartAction);
    trayMenu->addSeparator();
    trayMenu->addAction(exitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();

    QObject::connect(trayMenu, &QMenu::aboutToShow, [=](){
        toggleModeAction->setChecked(!mainWindow->isFrameless());
        mouseThroughAction->setChecked(mainWindow->isMouseThrough());
        toggleVisibilityAction->setChecked(mainWindow->isVisible());

        // 更新层级勾选状态
        MainWindow::WindowLevel level = mainWindow->getWindowLevel();
        actTop->setChecked(level == MainWindow::LevelTop);
        actNormal->setChecked(level == MainWindow::LevelNormal);
        actBottom->setChecked(level == MainWindow::LevelBottom);

        // 防止手动删除了快捷方式
        autoStartAction->setChecked(QFile::exists(linkPath));
    });

    QObject::connect(toggleModeAction,  &QAction::toggled, mainWindow, [mainWindow](bool checked){
        mainWindow->setFrameless(!checked);
    });

    QObject::connect(mouseThroughAction, &QAction::toggled, mainWindow, [mainWindow](bool checked){
        mainWindow->setMouseThrough(checked);
    });
    
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

    QObject::connect(autoStartAction, &QAction::toggled, [linkPath](bool checked){
        if (checked) {
            QString appPath = QCoreApplication::applicationFilePath();
            appPath = QDir::toNativeSeparators(appPath);
            QFile::link(appPath, linkPath);
        } else {
            QFile::remove(linkPath);
        }
    });

    QObject::connect(toggleVisibilityAction, &QAction::toggled, mainWindow, [mainWindow](bool checked){
        if (checked)
            mainWindow->show();
        else
            mainWindow->hide();
    });
    QObject::connect(exitAction, &QAction::triggered, mainWindow, &QApplication::quit);
}

MainTray::~MainTray()
{

}