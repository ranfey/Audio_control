#include "mainwindow.h"
#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QObject>
#include <QWidget>

class MainTray : public QSystemTrayIcon
{
    Q_OBJECT

public:
    explicit MainTray(MainWindow *mainWindow, QSettings *settings, QWidget *parent = nullptr);
    ~MainTray();

private:
    QApplication *m_app;
    QSettings *m_settings;
    MainWindow *m_mainWindow;
};