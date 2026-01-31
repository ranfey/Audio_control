#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <iostream>
#include <QGraphicsDropShadowEffect>

class ScrollFadeMask : public QWidget
{
public:
    explicit ScrollFadeMask(QWidget *parent = nullptr)
        : QWidget(parent)
    {
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_TranslucentBackground);
    }

    void setFadeHeight(int h)
    {
        if (fadeHeight == h) return;
        fadeHeight = h;
        update();
    }

protected:
    void paintEvent(QPaintEvent *)
    {
        if (fadeHeight <= 0)
            return;

        QPainter p(this);

        // 用本层 alpha 乘掉下面内容
        p.setCompositionMode(QPainter::CompositionMode_DestinationIn);

        // 上遮罩（alpha: 0 → 1）
        QLinearGradient top(0, 0, 0, fadeHeight);
        top.setColorAt(0.0, QColor(0, 0, 0, 0));     // 完全透明
        top.setColorAt(1.0, QColor(0, 0, 0, 255));   // 完全不透明

        p.fillRect(0, 0, width(), fadeHeight, top);

        // 下遮罩（alpha: 1 → 0）
        QLinearGradient bottom(0, height() - fadeHeight, 0, height());
        bottom.setColorAt(0.0, QColor(0, 0, 0, 255));
        bottom.setColorAt(1.0, QColor(0, 0, 0, 0));

        p.fillRect(0, height() - fadeHeight, width(), fadeHeight, bottom);
    }


private:
    int fadeHeight = 0;
};


class SessionRow : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int fixedHeight READ height WRITE setFixedHeightAnim)
    Q_PROPERTY(int glowRadius READ glowRadius WRITE setGlowRadius)
    Q_PROPERTY(QColor glowColor READ glowColor WRITE setGlowColor)
    Q_PROPERTY(int topMargin READ topMargin WRITE setTopMargin)
    Q_PROPERTY(int bottomMargin READ bottomMargin WRITE setBottomMargin)

public:
    SessionRow(DWORD pid, ISimpleAudioVolume *volume, int w, int h, QWidget *parent = nullptr)
        : QWidget(parent), m_baseHeight(h), m_expandSize((h * 2 - h) / 2)
    {
        this->setMinimumHeight(h);
        this->setMaximumHeight(h);
        
        this->setAttribute(Qt::WA_TranslucentBackground);
        this->setStyleSheet("background: transparent;");

        QHBoxLayout *layout = new QHBoxLayout(this);
        layout->setContentsMargins(10, 6, 10, 6);
        layout->setSpacing(8);

        // Slider
        QSlider *slider = new QSlider(Qt::Horizontal);
        slider->setRange(0, 100);
        slider->setInvertedAppearance(true);
        slider->setInvertedControls(false);
        slider->setMinimumWidth(w);
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

        // Icon
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

        // Glow
        m_glowEffect = new QGraphicsDropShadowEffect(this);
        m_glowEffect->setBlurRadius(0);
        m_glowEffect->setColor(Qt::transparent);
        m_glowEffect->setOffset(0, 0);
        this->setGraphicsEffect(m_glowEffect);

        // Animations
        m_animHeight = new QPropertyAnimation(this, "fixedHeight", this);
        m_animHeight->setDuration(500);
        m_animHeight->setEasingCurve(QEasingCurve::OutCubic);

        m_animTopMargin = new QPropertyAnimation(this, "topMargin", this);
        m_animTopMargin->setDuration(500);
        m_animTopMargin->setEasingCurve(QEasingCurve::OutCubic);

        m_animBottomMargin = new QPropertyAnimation(this, "bottomMargin", this);
        m_animBottomMargin->setDuration(500);
        m_animBottomMargin->setEasingCurve(QEasingCurve::OutCubic);

        m_animGlowRadius = new QPropertyAnimation(this, "glowRadius", this);
        m_animGlowRadius->setDuration(500);
        m_animGlowRadius->setEasingCurve(QEasingCurve::OutCubic);

        m_animGlowColor = new QPropertyAnimation(this, "glowColor", this);
        m_animGlowColor->setDuration(500);
        m_animGlowColor->setEasingCurve(QEasingCurve::OutCubic);
    }

    void setFixedHeightAnim(int h) {
        this->setMinimumHeight(h);
        this->setMaximumHeight(h);
    }

    int glowRadius() const { return m_glowEffect->blurRadius(); }
    void setGlowRadius(int r) { m_glowEffect->setBlurRadius(r); }

    QColor glowColor() const { return m_glowEffect->color(); }
    void setGlowColor(const QColor &c) { m_glowEffect->setColor(c); }

    int topMargin() const { return m_topMargin; }
    void setTopMargin(int m) {
        m_topMargin = m;
        updateMargins();
    }

    int bottomMargin() const { return m_bottomMargin; }
    void setBottomMargin(int m) {
        m_bottomMargin = m;
        updateMargins();
    }

    void updateMargins() {
        setContentsMargins(10, m_topMargin, 10, m_bottomMargin);
    }

    void setFocused(bool on)
    {
        if (on)
        {
            // 高度
            m_animHeight->stop();
            m_animHeight->setStartValue(height());
            m_animHeight->setEndValue(m_baseHeight + m_expandSize * 2);
            m_animHeight->start();

            // margin
            m_animTopMargin->stop();
            m_animTopMargin->setStartValue(topMargin());
            m_animTopMargin->setEndValue(m_expandSize);
            m_animTopMargin->start();

            m_animBottomMargin->stop();
            m_animBottomMargin->setStartValue(bottomMargin());
            m_animBottomMargin->setEndValue(m_expandSize);
            m_animBottomMargin->start();

            // glow
            m_animGlowRadius->stop();
            m_animGlowRadius->setStartValue(glowRadius());
            m_animGlowRadius->setEndValue(40);
            m_animGlowRadius->start();

            m_animGlowColor->stop();
            m_animGlowColor->setStartValue(glowColor());
            m_animGlowColor->setEndValue(QColor(255,119,170,200));
            m_animGlowColor->start();
        }
        else
        {
            m_animHeight->stop();
            m_animHeight->setStartValue(height());
            m_animHeight->setEndValue(m_baseHeight);
            m_animHeight->start();

            m_animTopMargin->stop();
            m_animTopMargin->setStartValue(topMargin());
            m_animTopMargin->setEndValue(0);
            m_animTopMargin->start();

            m_animBottomMargin->stop();
            m_animBottomMargin->setStartValue(bottomMargin());
            m_animBottomMargin->setEndValue(0);
            m_animBottomMargin->start();

            m_animGlowRadius->stop();
            m_animGlowRadius->setStartValue(glowRadius());
            m_animGlowRadius->setEndValue(0);
            m_animGlowRadius->start();

            m_animGlowColor->stop();
            m_animGlowColor->setStartValue(glowColor());
            m_animGlowColor->setEndValue(QColor(Qt::transparent));
            m_animGlowColor->start();
        }
    }

protected:
    void enterEvent(QEvent *event) override {
        // 高度动画
        m_animHeight->stop();
        m_animHeight->setStartValue(height());
        m_animHeight->setEndValue(m_baseHeight + m_expandSize * 2);
        m_animHeight->start();

        // 边距动画 - 向上下两侧挤开
        m_animTopMargin->stop();
        m_animTopMargin->setStartValue(topMargin());
        m_animTopMargin->setEndValue(m_expandSize);
        m_animTopMargin->start();

        m_animBottomMargin->stop();
        m_animBottomMargin->setStartValue(bottomMargin());
        m_animBottomMargin->setEndValue(m_expandSize);
        m_animBottomMargin->start();

        // 光晕动画
        m_animGlowRadius->stop();
        m_animGlowRadius->setStartValue(glowRadius());
        m_animGlowRadius->setEndValue(40);
        m_animGlowRadius->start();

        m_animGlowColor->stop();
        m_animGlowColor->setStartValue(glowColor());
        m_animGlowColor->setEndValue(QColor(255, 119, 170, 200));
        m_animGlowColor->start();

        QWidget::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        // 恢复高度
        m_animHeight->stop();
        m_animHeight->setStartValue(height());
        m_animHeight->setEndValue(m_baseHeight);
        m_animHeight->start();

        // 恢复边距
        m_animTopMargin->stop();
        m_animTopMargin->setStartValue(topMargin());
        m_animTopMargin->setEndValue(0);
        m_animTopMargin->start();

        m_animBottomMargin->stop();
        m_animBottomMargin->setStartValue(bottomMargin());
        m_animBottomMargin->setEndValue(0);
        m_animBottomMargin->start();

        // 关闭光晕
        m_animGlowRadius->stop();
        m_animGlowRadius->setStartValue(glowRadius());
        m_animGlowRadius->setEndValue(0);
        m_animGlowRadius->start();

        m_animGlowColor->stop();
        m_animGlowColor->setStartValue(glowColor());
        m_animGlowColor->setEndValue(QColor(Qt::transparent));
        m_animGlowColor->start();

        QWidget::leaveEvent(event);
    }

private:
    int m_baseHeight;
    int m_expandSize;
    int m_topMargin = 0;
    int m_bottomMargin = 0;
    QGraphicsDropShadowEffect *m_glowEffect;
    QPropertyAnimation *m_animHeight;
    QPropertyAnimation *m_animTopMargin;
    QPropertyAnimation *m_animBottomMargin;
    QPropertyAnimation *m_animGlowRadius;
    QPropertyAnimation *m_animGlowColor;
};


MainWindow::MainWindow(QSettings *settings, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_settings(settings)
{
    ui->setupUi(this);

    initTime(); // 初始化

    initUi(true); // 构建窗口 UI

    QTimer *refreshTimer = new QTimer(this);
    this->connect(refreshTimer, &QTimer::timeout, this, &MainWindow::refreshSessions);
    refreshTimer->start(); // 每100毫秒刷新一次 //动态构建音量合成器
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

void MainWindow::initUi(bool forceCreate) // 构建窗口 UI
{
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 关键：在 viewport 上装事件过滤器，拦截 wheel
    ui->scrollArea->viewport()->installEventFilter(this);

    // 预建一个动画（也可以懒加载）
    m_scrollAnim = new QPropertyAnimation(ui->scrollArea->verticalScrollBar(),
                                      QByteArrayLiteral("value"),
                                      this);

    m_scrollAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_scrollAnim->setDuration(180);



    int need =maxWindowHeight/1.8 - rowHeight/2;
    int count = (need + rowHeight - 1) / rowHeight;

    // ---------- 顶部 ----------
    while (topSpacers.size() < count)
    {
        QWidget *w = new QWidget;
        w->setFixedHeight(rowHeight);
        ui->sessionLayout->insertWidget(0, w);
        topSpacers.append(w);
    }
    while (topSpacers.size() > count)
    {
        QWidget *w = topSpacers.takeLast();
        ui->sessionLayout->removeWidget(w);
        w->deleteLater();
    }

    // ---------- 底部 ----------
    while (bottomSpacers.size() < count)
    {
        QWidget *w = new QWidget;
        w->setFixedHeight(rowHeight);
        ui->sessionLayout->addWidget(w);
        bottomSpacers.append(w);
    }
    while (bottomSpacers.size() > count)
    {
        QWidget *w = bottomSpacers.takeLast();
        ui->sessionLayout->removeWidget(w);
        w->deleteLater();
    }



    
    QWidget *vp = ui->scrollArea->viewport();

    // ---------- 1. 创建（只会发生一次） ----------
    if (!m_fadeMask || forceCreate)
    {
        if (m_fadeMask)
            m_fadeMask->deleteLater();

        auto *mask = new ScrollFadeMask(vp);
        mask->raise();
        mask->resize(vp->size());
        m_fadeMask = mask;

        // 只在这里装一次过滤器
        vp->installEventFilter(this);
    }

    // ---------- 2. 计算遮罩高度（合并 calcFadeHeight） ----------
    constexpr double ROW_RATIO  = 0.8;
    constexpr double VIEW_RATIO = 0.3;
    constexpr int MIN_PX = 150;
    constexpr int MAX_PX = 500;

    int byRow  = int(rowHeight * ROW_RATIO);
    int byView = int(maxWindowHeight * VIEW_RATIO);
    int fade   = qBound(MIN_PX, qMin(byRow, byView), MAX_PX);

    // ---------- 3. 应用 ----------
    ScrollFadeMask *mask = static_cast<ScrollFadeMask*>(m_fadeMask);
    mask->setFadeHeight(fade);
}

void MainWindow::initTime() // 初始化
{
    this->setWindowIcon(QIcon(":/img/icon.ico"));

    // 读取设置
    if (!m_settings->contains("Style/RowHeight"))
    {
        m_settings->setValue("Style/RowHeight", 70);
    }
    if (!m_settings->contains("Style/RowWidth"))
    {
        m_settings->setValue("Style/RowWidth", 200);
    }
    if (!m_settings->contains("Style/MaxWindowHeight"))
    {
        m_settings->setValue("Style/MaxWindowHeight", 300);
    }

    int rowHeight = m_settings->value("Style/RowHeight", 70).toInt();
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
}


void MainWindow::refreshSessions() // 刷新音频会话
{
    QSet<DWORD> newPIDs;
    QList<SessionInfo> current = scanSessions();

    for (const SessionInfo &s : current)
    {
        if (shouldFilterOut(s.pid, s.volume))
        {
            if (s.volume) s.volume->Release();   //要释放
            continue;
        }

        newPIDs.insert(s.pid);

        if (!sessionMap.contains(s.pid))
        {
            QWidget *row = createSessionRow(s.pid, s.volume);
            int idx = ui->sessionLayout->count();
            if (!bottomSpacers.isEmpty())
            {
                idx = ui->sessionLayout->indexOf(bottomSpacers.first());
            }
            ui->sessionLayout->insertWidget(idx, row);
            sessionMap.insert(s.pid, {s.pid, s.volume, row});
        }
        else
        {
            if (s.volume) s.volume->Release();
        }
    }

    // 移除不存在的 session
    for (auto it = sessionMap.begin(); it != sessionMap.end();)
    {
        if (!newPIDs.contains(it.key()))
        {
            if (it.value().volume) it.value().volume->Release();
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

    // ⚠️ 先更新 spacer / fade
    initUi(false);

    // 让 Qt 按真实布局算高度（包含 spacer + 行动画）
    ui->sessionContainer->adjustSize();
    int totalHeight = ui->sessionContainer->sizeHint().height();

    int actualHeight = qMin(totalHeight, maxWindowHeight);
    int windowWidth = rowWidth * 2;

    this->setFixedSize(windowWidth, actualHeight);

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

    // 默认排除名单处理系统音频等
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

QString MainWindow::getProcessPath(DWORD pid)// 获取进程路径
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
    std::cout << "Creating SessionRow for PID: " << pid << std::endl;
    return new SessionRow(pid, volume, rowWidth, rowHeight);
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

bool MainWindow::eventFilter(QObject *watched, QEvent *event)// 事件过滤器绑定到 viewport 上已拦截 wheel 事件使其滚动 scrollbar
{
    if (watched == ui->scrollArea->viewport())
    {
        if (event->type() == QEvent::Resize && m_fadeMask)
        {
            m_fadeMask->resize(ui->scrollArea->viewport()->size());
        }

        if (watched == ui->scrollArea->viewport() && event->type() == QEvent::Wheel)
        {

            auto *we = static_cast<QWheelEvent*>(event);

            int dy = 0;
            if (!we->pixelDelta().isNull())
            {
                // 触控板：dy 更小
                dy = we->pixelDelta().y();
            }
            else
            {
                dy = we->angleDelta().y() / 120 * 50; // 每格重写位50，按手感调
            }

            // wheel: dy>0 向上滚 scrollbar 减小
                QScrollBar *bar = ui->scrollArea->verticalScrollBar();

        if (!m_scrollAnim)
        {
            m_scrollAnim = new QPropertyAnimation(this);
            m_scrollAnim->setEasingCurve(QEasingCurve::OutCubic);
            m_scrollAnim->setDuration(180);
            m_scrollAnim->setPropertyName(QByteArrayLiteral("value"));
        }

        // 绑定目标：动画作用
        m_scrollAnim->setTargetObject(bar);

        const int current = bar->value();
        int base = current;

        // 滚轮连续触发时，以“当前 endValue”为基准叠加，手感会更像网页 smooth scroll
        if (m_scrollAnim->state() == QAbstractAnimation::Running)
        {
            bool ok = false;
            int endv = m_scrollAnim->endValue().toInt(&ok);
            if (ok) base = endv;
            m_scrollAnim->stop();
        }

        int target = base -dy;
        target = qBound(bar->minimum(), target, bar->maximum());

        m_scrollAnim->setStartValue(current);
        m_scrollAnim->setEndValue(target);

        // ⚠️ 关键：动画结束后吸附
        QObject::disconnect(m_scrollAnim, nullptr, this, nullptr);
        connect(m_scrollAnim, &QPropertyAnimation::finished,
                this, &MainWindow::snapScrollToNearestRow);

        m_scrollAnim->start();

        event->accept();
        return true;
        
        }
    }

    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::snapScrollToNearestRow()
{
    QScrollBar *bar = ui->scrollArea->verticalScrollBar();
    if (!bar)
        return;

    const int viewportH = ui->scrollArea->viewport()->height();
    const int viewportCenter = viewportH / 2;

    // 当前滚动位置
    int current = bar->value();

    // 推算“最接近中线的行索引”
    double idx = (current + viewportCenter - rowHeight / 2.0) / rowHeight;
    int targetIndex = qRound(idx);

    // 防御
    targetIndex = qMax(0, targetIndex);

    // 计算目标滚动值（让该行中心对齐中线）
    int targetScroll =
        targetIndex * rowHeight + rowHeight / 2 - viewportCenter;

    targetScroll = qBound(bar->minimum(), targetScroll, bar->maximum());

    // 平滑吸附
    m_scrollAnim->stop();
    m_scrollAnim->setStartValue(current);
    m_scrollAnim->setEndValue(targetScroll);
    m_scrollAnim->start();
}



#include "mainwindow.moc"