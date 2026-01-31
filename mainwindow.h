#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <Windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>
#include <endpointvolume.h>
#include <QSettings>
#include <QPainter>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    bool frameless = true;
    int maxWindowHeight = 0;
    int rowWidth = 0;
    int rowHeight = 0;
    QSettings *m_settings;

public:
    explicit MainWindow(QSettings *settings, QWidget *parent = nullptr);
    ~MainWindow();

    Ui::MainWindow *ui;

    /* ========== 核心功能函数 ========== */

    void toggleFrameless();
    // 枚举当前所有音频 Session（等同 Win10 合成器）
    void enumerateAudioSessions();

    // 为一个 Session 创建一行 UI（图标 + slider）
    void createSessionRow(DWORD pid, ISimpleAudioVolume* volume);

    void reflowSessionLayout();

};

#endif // MAINWINDOW_H
