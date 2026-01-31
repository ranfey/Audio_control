#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QApplication>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QObject>
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainTray;
}
QT_END_NAMESPACE

class MainTray : public QSystemTrayIcon
{
    Q_OBJECT

public:
    explicit MainTray(MainWindow *mainWindow, QSettings *settings, QWidget *parent = nullptr);
    ~MainTray();

private:
    Ui::MainTray *ui;
    QApplication *m_app;
    QSettings *m_settings;
    MainWindow *m_mainWindow;
};