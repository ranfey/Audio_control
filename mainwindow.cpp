#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <iostream>

MainWindow::MainWindow(QSettings *settings, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_settings(settings)
{
    ui->setupUi(this);

    this->setWindowIcon(QIcon(":/img/icon.ico"));

    // 读取设置
    if (!m_settings->contains("Style/RowHeight"))
    {
        m_settings->setValue("Style/RowHeight", 90);
    }
    if (!m_settings->contains("Style/RowWidth"))
    {
        m_settings->setValue("Style/RowWidth", 200);
    }
    if (!m_settings->contains("Style/MaxWindowHeight"))
    {
        m_settings->setValue("Style/MaxWindowHeight", 300);
    }

    int rowHeight = m_settings->value("Style/RowHeight", 90).toInt();
    int rowWidth = m_settings->value("Style/RowWidth", 200).toInt();
    int maxHeight = m_settings->value("Style/MaxWindowHeight", 300).toInt();
    this->rowHeight = rowHeight;
    this->rowWidth = rowWidth;
    this->maxWindowHeight = maxHeight;
    // 退出状态
    m_settings->beginGroup("MainWindow");
    this->restoreGeometry(m_settings->value("geometry").toByteArray());
    this->restoreState(m_settings->value("windowState").toByteArray());
    m_settings->endGroup();

    // 无边框 + 不显示任务栏 + 永远在普通窗口下面 (初始状态)
    this->setWindowFlags(
        Qt::FramelessWindowHint |
        Qt::Tool |
        Qt::WindowStaysOnBottomHint);

    this->setAttribute(Qt::WA_TranslucentBackground); // 透明窗口

    QPalette Fuck = QPalette();
    Fuck.setColor(QPalette::Background, QColor(100, 100, 100, 2)); // 伪造透明阻止鼠标穿透，傻逼完了
    this->setPalette(Fuck);

    QTimer *refreshTimer = new QTimer(this);
    this->connect(refreshTimer, &QTimer::timeout, this, &MainWindow::refreshSessions);
    refreshTimer->start(100); // 每100毫秒刷新一次 //动态构建音量合成器
    // refreshSessions();

    this->show(); // 初始显示
}

MainWindow::~MainWindow()
{

    // 保存退出状态
    m_settings->beginGroup("MainWindow");
    m_settings->setValue("geometry", saveGeometry());
    m_settings->setValue("windowState", saveState());
    m_settings->endGroup();
    CoUninitialize();
    delete ui;
}

void MainWindow::toggleFrameless() // 切换移动模式
{
    frameless = !frameless;

    // 先隐藏窗口
    this->hide();

    // 修改窗口标志
    if (frameless)
    {
        this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);
    }
    else
    {
        this->setWindowFlags(Qt::Tool | Qt::WindowStaysOnBottomHint); // 移除 Frameless
    }

    // 重新显示窗口（必要）
    this->show();
}

void MainWindow::refreshSessions() // 刷新音频会话
{
    QSet<DWORD> newPIDs;
    QList<SessionInfo> current = scanSessions();

    // 新增 session
    for (const SessionInfo &s : current)
    {
        if (shouldFilterOut(s.pid, s.volume))
        {
            continue;
        }

        newPIDs.insert(s.pid);
        if (!sessionMap.contains(s.pid))
        {
            QWidget *row = createSessionRow(s.pid, s.volume);
            ui->sessionLayout->addWidget(row);

            sessionMap.insert(s.pid, {s.pid, s.volume, row});
        }
    }

    // 移除不存在的 session
    for (auto it = sessionMap.begin(); it != sessionMap.end();)
    {
        if (!newPIDs.contains(it.key()))
        {
            QWidget *row = it.value().widget;
            ui->sessionLayout->removeWidget(row);
            row->deleteLater();
            it = sessionMap.erase(it);
        }
        else
        {
            ++it;
        }
    }

    int totalHeight = ui->sessionLayout->count() * rowHeight + 12;

    int actualHeight = qMin(totalHeight, maxWindowHeight);
    int windowWidth = rowWidth * 2;

    this->setFixedSize(windowWidth, actualHeight);

    // 若使用 QScrollArea，可同步更新高度限制：
    if (ui->scrollArea)
    {
        ui->sessionContainer->setFixedHeight(totalHeight);
        ui->scrollArea->setFixedHeight(actualHeight);
    }
}

bool MainWindow::shouldFilterOut(DWORD pid, ISimpleAudioVolume *volume) // 过滤条件
{
    // 获取当前进程路径
    QString exePath = getProcessPath(pid);
    if (exePath.isEmpty())
        return true;

    QString exeName = QFileInfo(exePath).fileName().toLower();

    // 默认排除名单
    static const QStringList defaultSkipNames = {
        "system", "idle", "audiodg.exe", "runtimebroker.exe"};
    if (defaultSkipNames.contains(exeName))
        return true;

    // 获取自身音量
    float selfVol = 0.0f;
    if (FAILED(volume->GetMasterVolume(&selfVol)))
        return true;

    // 判断是否已有同名进程且有声音(自动隐藏手动静音的同名进程)
    bool sameNameWithSound = false;

    for (auto it = sessionMap.begin(); it != sessionMap.end(); ++it)
    {
        if (it.key() == pid)
            continue;

        QString otherExeName = QFileInfo(getProcessPath(it.key())).fileName().toLower();
        if (otherExeName == exeName)
        {
            ISimpleAudioVolume *otherVol = it.value().volume;
            if (otherVol)
            {
                float otherVolume = 0.0f;
                if (SUCCEEDED(otherVol->GetMasterVolume(&otherVolume)) && otherVolume > 0.01f)
                {
                    sameNameWithSound = true;
                    break;
                }
            }
        }
    }

    // 如果自己静音，且已有同名不静音 → 过滤
    return (selfVol <= 0.01f && sameNameWithSound);
}

QString MainWindow::getProcessPath(DWORD pid)
{
    HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProc)
        return {};

    wchar_t exePath[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    QueryFullProcessImageNameW(hProc, 0, exePath, &size);
    CloseHandle(hProc);

    return QString::fromWCharArray(exePath);
}

QList<SessionInfo> MainWindow::scanSessions() // 枚举当前系统的所有音频 Session
{
    QList<SessionInfo> list;

    IMMDeviceEnumerator *deviceEnumerator = nullptr;
    IMMDevice *device = nullptr;
    IAudioSessionManager2 *sessionManager = nullptr;
    IAudioSessionEnumerator *sessionEnumerator = nullptr;

    CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER,
                     __uuidof(IMMDeviceEnumerator), (void **)&deviceEnumerator);

    deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &device);

    device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER, nullptr,
                     (void **)&sessionManager);

    sessionManager->GetSessionEnumerator(&sessionEnumerator);

    int count = 0;
    sessionEnumerator->GetCount(&count);

    for (int i = 0; i < count; ++i)
    {
        IAudioSessionControl *ctrl = nullptr;
        sessionEnumerator->GetSession(i, &ctrl);

        IAudioSessionControl2 *ctrl2 = nullptr;
        ctrl->QueryInterface(__uuidof(IAudioSessionControl2), (void **)&ctrl2);

        DWORD pid = 0;
        ctrl2->GetProcessId(&pid);

        ISimpleAudioVolume *volume = nullptr;
        ctrl2->QueryInterface(__uuidof(ISimpleAudioVolume), (void **)&volume);

        list.append({pid, volume});

        ctrl2->Release();
        ctrl->Release();
    }

    sessionEnumerator->Release();
    sessionManager->Release();
    device->Release();
    deviceEnumerator->Release();

    return list;
}

QWidget *MainWindow::createSessionRow(DWORD pid, ISimpleAudioVolume *volume) // 创建 UI 行
{
    QWidget *row = new QWidget;
    row->setMinimumHeight(rowHeight);
    row->setAttribute(Qt::WA_TranslucentBackground);
    row->setStyleSheet(R"(background: transparent;)");

    QHBoxLayout *layout = new QHBoxLayout(row);
    layout->setContentsMargins(10, 6, 10, 6);
    layout->setSpacing(8);

    // -------- 音量条 --------
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 100);
    slider->setInvertedAppearance(true);
    slider->setInvertedControls(false);
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

    connect(slider, &QSlider::valueChanged, this, [volume](int v)
            { volume->SetMasterVolume(v / 100.0f, nullptr); });

    // -------- 获取图标 --------
    QPixmap iconPixmap;
    {
        HANDLE hProc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        if (hProc)
        {
            wchar_t exePath[MAX_PATH] = {};
            DWORD size = MAX_PATH;
            if (QueryFullProcessImageNameW(hProc, 0, exePath, &size))
            {
                SHFILEINFOW info{};
                if (SHGetFileInfoW(exePath, 0, &info, sizeof(info), SHGFI_ICON | SHGFI_LARGEICON))
                {
                    iconPixmap = QtWin::fromHICON(info.hIcon);
                    DestroyIcon(info.hIcon);
                }
            }
            CloseHandle(hProc);
        }
    }

    QLabel *icon = new QLabel;
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
    return row;
}
