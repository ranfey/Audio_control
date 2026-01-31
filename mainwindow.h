#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QtWin>
#include <QScrollArea>
#include <QDebug>
#include <shellapi.h>
#include <shlobj.h>
#include <QGraphicsOpacityEffect>
#include <QWheelEvent>
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
#include <QTimer>
#include <QFileInfo>
#include <QScroller>
#include <QScrollBar>
#include <QPropertyAnimation>
#include <QPointer>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

struct SessionInfo
{
    DWORD pid;
    ISimpleAudioVolume *volume;
    QWidget *widget;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT
private:
    bool frameless = true;
    int maxWindowHeight = 0;
    int rowWidth = 0;
    int rowHeight = 0;
    QSettings *m_settings;
    QMap<DWORD, SessionInfo> sessionMap;
    QWidget *m_fadeMask = nullptr;
    int m_fadeHeight = 0;
    QWidget *topSpacer = nullptr;
    QWidget *bottomSpacer = nullptr;

    Ui::MainWindow *ui;

    void refreshSessions();

    QList<SessionInfo> scanSessions();

    QWidget *createSessionRow(DWORD pid, ISimpleAudioVolume *volume);

    QString getProcessPath(DWORD pid);

    bool shouldFilterOut(DWORD pid, ISimpleAudioVolume *volume);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QPointer<QPropertyAnimation> m_scrollAnim;
    int m_scrollTarget = 0;

public:
    void initUi(bool forceCreate);

    void initTime();

    explicit MainWindow(QSettings *settings, QWidget *parent = nullptr);

    ~MainWindow();

    void toggleFrameless();

    void snapScrollToNearestRow();
};

#endif // MAINWINDOW_H
