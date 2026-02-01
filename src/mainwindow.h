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
#include <QSettings>
#include <QPainter>
#include <QTimer>
#include <QFileInfo>
#include <QScroller>
#include <QScrollBar>
#include <QPropertyAnimation>
#include <QPointer>
#include <QGraphicsDropShadowEffect>
#include "audiocontroller.h"
#include "sessionrow.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
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
    QMap<DWORD, QWidget*> sessionMap;
    QWidget *m_fadeMask = nullptr;
    int m_fadeHeight = 0;
    QVector<QWidget*> topSpacers;
    QVector<QWidget*> bottomSpacers;

    Ui::MainWindow *ui;

    QWidget *createSessionRow(const AudioSessionData &data);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QPointer<QPropertyAnimation> m_scrollAnim;
    int m_scrollTarget = 0;
    class SessionRow *m_focusedRow = nullptr;

public:
    void initUi(); // 构建窗口 UI
    void updateWindowGeometry(); // 更新窗口布局

    void initWindowStyle(); // 初始化窗口样式与配置

    explicit MainWindow(QSettings *settings, QWidget *parent = nullptr);

    ~MainWindow();

    void toggleInteractMode(); // 切换交互模式（移动/锁定）

    void snapScrollToNearestRow(); // 滚动吸附

public slots:
    void onSessionAdded(const AudioSessionData &data);
    void onSessionRemoved(DWORD pid);

signals:
    void volumeChanged(DWORD pid, int volume);
};

#endif // MAINWINDOW_H
