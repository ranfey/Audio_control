#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QSettings>
#include <QtWin>        // 来自 QtWinExtras
#include <shellapi.h>
#include <shlobj.h>
#include <QScrollArea>
#include <QDebug>
#include <shellapi.h>
#include <shlobj.h>

MainWindow::MainWindow(QSettings *settings , QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settings(settings)
{
    ui->setupUi(this);

    // 读取设置
    m_settings->beginGroup("MainWindow");
    restoreGeometry(m_settings->value("geometry").toByteArray());
    restoreState(m_settings->value("windowState").toByteArray());
    m_settings->endGroup();

    // 无边框 + 不显示任务栏 + 永远在普通窗口下面 (初始状态)
    setWindowFlags(
        Qt::FramelessWindowHint |
        Qt::Tool |
        Qt::WindowStaysOnBottomHint
    );

    setAttribute(Qt::WA_TranslucentBackground, true);//透明窗口

    enumerateAudioSessions();//动态构建音量合成器

    adjustSize();  // 自动调整窗口大小

    show();  // 初始显示
}

MainWindow::~MainWindow()
{

    // 保存设置
    m_settings->beginGroup("MainWindow");
    m_settings->setValue("geometry", saveGeometry());
    m_settings->setValue("windowState", saveState());
    m_settings->endGroup();
    CoUninitialize();
    delete ui;
}


void MainWindow::toggleFrameless()// 切换编辑
{
    frameless = !frameless;

    // 先隐藏窗口
    this->hide();

    // 修改窗口标志
    if (frameless) {
        this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);
    } else {
        this->setWindowFlags(Qt::Tool | Qt::WindowStaysOnBottomHint); // 移除 Frameless
    }

    // 重新显示窗口（必要）
    this->show();
}

/* =========================================================
 * 枚举当前系统的所有音频 Session
 * ========================================================= */
void MainWindow::enumerateAudioSessions()
{
    IMMDeviceEnumerator* deviceEnumerator = nullptr;
    IMMDevice* device = nullptr;
    IAudioSessionManager2* sessionManager = nullptr;
    IAudioSessionEnumerator* sessionEnumerator = nullptr;

    // 创建设备枚举器
    CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_INPROC_SERVER,
        __uuidof(IMMDeviceEnumerator),
        (void**)&deviceEnumerator
    );

    // 获取默认输出设备（扬声器 / 耳机）
    deviceEnumerator->GetDefaultAudioEndpoint(
        eRender,
        eConsole,
        &device
    );

    // 激活 Session Manager
    device->Activate(
        __uuidof(IAudioSessionManager2),
        CLSCTX_INPROC_SERVER,
        nullptr,
        (void**)&sessionManager
    );

    // 枚举所有 Session
    sessionManager->GetSessionEnumerator(&sessionEnumerator);

    int count = 0;
    sessionEnumerator->GetCount(&count);

    for (int i = 0; i < count; ++i)
    {
        IAudioSessionControl* ctrl = nullptr;
        sessionEnumerator->GetSession(i, &ctrl);

        IAudioSessionControl2* ctrl2 = nullptr;
        ctrl->QueryInterface(
            __uuidof(IAudioSessionControl2),
            (void**)&ctrl2
        );

        DWORD pid = 0;
        ctrl2->GetProcessId(&pid);

        // 每个 Session 都有独立音量
        ISimpleAudioVolume* volume = nullptr;
        ctrl2->QueryInterface(
            __uuidof(ISimpleAudioVolume),
            (void**)&volume
        );

        // 创建 UI 行
        createSessionRow(pid, volume);

        ctrl2->Release();
        ctrl->Release();
    }

    sessionEnumerator->Release();
    sessionManager->Release();
    device->Release();
    deviceEnumerator->Release();
}

/* =========================================================
 * 为一个音频 Session 创建 UI 行
 * ========================================================= */
void MainWindow::createSessionRow(DWORD pid, ISimpleAudioVolume* volume)
{
    QWidget* row = new QWidget;

    // 不要 setFixedHeight
    row->setMinimumHeight(48);   // Win10 行高

    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(12);

    /* -------- 图标 -------- */
    QLabel* icon = new QLabel;
    icon->setFixedSize(32, 32);
    icon->setPixmap(getAppIcon(pid));
    icon->setScaledContents(true);
    layout->setAlignment(icon, Qt::AlignVCenter);

    /* -------- 音量条 -------- */
    QSlider* slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 100);
    slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    float vol = 0.0f;
    volume->GetMasterVolume(&vol);
    slider->setValue(int(vol * 100));

    connect(slider, &QSlider::valueChanged, this,
        [volume](int v){
            volume->SetMasterVolume(v / 100.0f, nullptr);
        }
    );

    layout->addWidget(icon);
    layout->addWidget(slider, 1);

    ui->sessionLayout->addWidget(row);
}


/* =========================================================
 * PID → exe 路径
 * ========================================================= */
QString MainWindow::getProcessPath(DWORD pid)
{
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return {};

    wchar_t buffer[MAX_PATH];
    DWORD size = MAX_PATH;
    QueryFullProcessImageNameW(h, 0, buffer, &size);
    CloseHandle(h);

    return QString::fromWCharArray(buffer);
}

/* =========================================================
 * exe → Win10 同源应用图标
 * ========================================================= */
QPixmap MainWindow::getAppIcon(DWORD pid)
{
    QString path = getProcessPath(pid);
    if (path.isEmpty())
        return QPixmap();

    SHFILEINFOW info{};
    if (!SHGetFileInfoW(
            (LPCWSTR)path.utf16(),
            0,
            &info,
            sizeof(info),
            SHGFI_ICON | SHGFI_SMALLICON))
    {
        return QPixmap();
    }

    // ✅ Qt5 正确：QtWinExtras
    QPixmap pix = QtWin::fromHICON(info.hIcon);

    DestroyIcon(info.hIcon);
    return pix;
}
