#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QSettings>
#include <QtWin>
#include <shellapi.h>
#include <shlobj.h>
#include <QScrollArea>
#include <QDebug>
#include <shellapi.h>
#include <shlobj.h>
#include <QGraphicsOpacityEffect>
#include <QWheelEvent>

MainWindow::MainWindow(QSettings *settings , QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_settings(settings)
{
    ui->setupUi(this);
    setWindowIcon(QIcon(":/img/icon.ico"));

    // 读取设置
    int rowHeight = m_settings->value("Style/RowHeight", 90).toInt();
    int rowWidth  = m_settings->value("Style/RowWidth", 200).toInt();
    int maxHeight = m_settings->value("Style/MaxWindowHeight", 300).toInt();
    this->rowHeight = rowHeight;
    this->rowWidth = rowWidth;
    this->maxWindowHeight = maxHeight;
    //退出状态
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

    setAttribute(Qt::WA_TranslucentBackground);//透明窗口

    QPalette Fuck = QPalette();
    Fuck.setColor(QPalette::Background, QColor(100,100,100,2));//伪造透明阻止鼠标穿透，傻逼完了
    setPalette(Fuck);

    enumerateAudioSessions();//动态构建音量合成器
    
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

    // 获取默认输出设备
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
        reflowSessionLayout();


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
    row->setMinimumHeight(rowHeight);
    row->setAttribute(Qt::WA_TranslucentBackground);
    row->setStyleSheet(R"(background: transparent;)");

    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(8);

    /* -------- 音量条 -------- */
    QSlider* slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 100);
    slider->setInvertedAppearance(true);  // 视觉反转
    slider->setInvertedControls(false);   // 滚轮方向正常

    slider->setMinimumWidth(rowWidth);
    slider->setFixedHeight(32);

    slider->setStyleSheet(R"(
        QSlider {
            background: transparent;
        }
        QSlider::groove:horizontal {
            height: 8px;
            background: #ffe4f0;
            border-radius: 4px;
        }
        QSlider::handle:horizontal {
            width: 20px;
            height: 20px;
            background: #ff77aa;
            border: 2px solid white;
            margin: -8px 0;
            border-radius: 10px;
        }
        QSlider::sub-page:horizontal {
            background: #eee;
            border-radius: 4px;
        }
        QSlider::add-page:horizontal {
            background: #ffb6d5;
            border-radius: 4px;
        }
    )");

    float vol = 0.0f;
    volume->GetMasterVolume(&vol);
    slider->setValue(int(vol * 100));

    connect(slider, &QSlider::valueChanged, this,
        [volume](int v){
            volume->SetMasterVolume(v / 100.0f, nullptr);
        }
    );

    /* -------- 获取图标 -------- */
    QPixmap iconPixmap;
    {
        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProc) {
            wchar_t exePath[MAX_PATH] = {};
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProc, 0, exePath, &size)) {
                SHFILEINFOW info{};
                if (SHGetFileInfoW(
                        exePath,
                        0,
                        &info,
                        sizeof(info),
                        SHGFI_ICON | SHGFI_LARGEICON))  // 使用大图标
                {
                    iconPixmap = QtWin::fromHICON(info.hIcon);
                    DestroyIcon(info.hIcon);
                }
            }
            CloseHandle(hProc);
        }
    }

    QLabel* icon = new QLabel;
    icon->setObjectName("sessionIcon");
    icon->setFixedSize(64, 64);
    icon->setPixmap(iconPixmap);
    icon->setScaledContents(true);
    icon->setStyleSheet(R"(
        QLabel#sessionIcon {
            background: transparent;
            border-radius: 4px;
            padding: 2px;
            border: 2px solid transparent;
        }

        QLabel#sessionIcon:hover {
            border: 2px solid #ffaad4;
        }

    )");

    layout->addWidget(slider, 1);
    layout->addWidget(icon);
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->sessionLayout->addWidget(row);
}

void MainWindow::reflowSessionLayout()
{
    int count = ui->sessionLayout->count();
    int totalHeight = count * rowHeight + 12;

    int actualHeight = qMin(totalHeight, maxWindowHeight);
    int windowWidth = rowWidth *2;

    this->setFixedSize(windowWidth, actualHeight);

    // 若使用 QScrollArea，可同步更新高度限制：
    if (ui->scrollArea) {
        ui->sessionContainer->setFixedHeight(totalHeight);
        ui->scrollArea->setFixedHeight(actualHeight);
    }

}
