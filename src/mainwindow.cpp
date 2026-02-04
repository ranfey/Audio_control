#include "mainwindow.h"
#include "ui_mainwindow.h"


class ScrollFadeMask : public QWidget
{
    Q_OBJECT
public:
    explicit ScrollFadeMask(QWidget *parent = nullptr,int rowHeight = 0)
        : QWidget(parent), rowHeight(rowHeight)
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
    int rowHeight = 0;
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

        // 下遮罩（alpha: 1 → 0），整体上移 rowHeight
        QLinearGradient bottom(0, height() - fadeHeight - rowHeight, 0, height() - rowHeight);
        bottom.setColorAt(0.0, QColor(0, 0, 0, 255));
        bottom.setColorAt(1.0, QColor(0, 0, 0, 0));

        p.fillRect(0, height() - fadeHeight - rowHeight, width(), fadeHeight, bottom);

        // 完全透明遮罩，高度为 rowHeight
        p.fillRect(0, height() - rowHeight, width(), rowHeight, QColor(0, 0, 0, 0));

    }


private:
    int fadeHeight = 0;
};


MainWindow::MainWindow(QSettings *settings, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_settings(settings)
{
    ui->setupUi(this);

    initWindowStyle(); // 初始化窗口样式

    initUi(); // 构建 UI 元素

    if (m_settings->value("Style/Visible", true).toBool()) {
        this->show(); // 显示窗口
    }
}

MainWindow::~MainWindow()
{
    // 保存退出状态
    m_settings->beginGroup("MainWindow");
    m_settings->setValue("geometry", saveGeometry());
    m_settings->setValue("windowState", saveState());
    m_settings->endGroup();
    
    m_settings->setValue("Style/Visible", this->isVisible());

    // 释放 QTimer
    if (m_scrollAnim)
    {
        delete m_scrollAnim;
        m_scrollAnim = nullptr;
    }

    // 释放 topSpacers 和 bottomSpacers
    qDeleteAll(topSpacers);
    topSpacers.clear();

    qDeleteAll(bottomSpacers);
    bottomSpacers.clear();

    // 释放 sessionMap
    qDeleteAll(sessionMap);
    sessionMap.clear();

    // 释放 m_fadeMask
    if (m_fadeMask)
    {
        delete m_fadeMask;
        m_fadeMask = nullptr;
    }

    delete ui;
}

// 初始化/构建界面 UI 元素
void MainWindow::initUi()
{
    ui->scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 关键：在 viewport 上装事件过滤器，拦截 wheel
    ui->scrollArea->viewport()->installEventFilter(this);

    // 预建一个动画
    m_scrollAnim = new QPropertyAnimation(ui->scrollArea->verticalScrollBar(),
                                        QByteArrayLiteral("value"),
                                        this);

    m_scrollAnim->setEasingCurve(QEasingCurve::OutCubic);
    m_scrollAnim->setDuration(180);

    int need =maxWindowHeight/1.8 - rowHeight/2;
    int count = (need + rowHeight - 1) / rowHeight;

    // ---------- 顶部 ----------
    for (int i = 0; i < count; ++i)
    {
        QWidget *w = new QWidget;
        w->setFixedHeight(rowHeight);
        ui->sessionLayout->insertWidget(0, w);
        topSpacers.append(w);
    }

    // ---------- 底部 ----------
    for (int i = 0; i < count; ++i)
    {
        QWidget *w = new QWidget;
        w->setFixedHeight(rowHeight);
        ui->sessionLayout->addWidget(w);
        bottomSpacers.append(w);
    }
    
    QWidget *vp = ui->scrollArea->viewport();

    auto *mask = new ScrollFadeMask(vp, rowHeight);
    mask->raise();
    mask->resize(vp->size());
    m_fadeMask = mask;

    // 只在这里装一次过滤器
    vp->installEventFilter(this);

    // ---------- 2. 计算遮罩高度（合并 calcFadeHeight） ----------
    constexpr double ROW_RATIO  = 0.8;
    constexpr double VIEW_RATIO = 0.3;
    constexpr int MIN_PX = 150;
    constexpr int MAX_PX = 500;

    int byRow  = int(rowHeight * ROW_RATIO);
    int byView = int(maxWindowHeight * VIEW_RATIO);
    int fade   = qBound(MIN_PX, qMin(byRow, byView), MAX_PX);

    // ---------- 3. 应用 ----------
    mask->setFadeHeight(fade);
}

// 初始化窗口样式与加载配置
void MainWindow::initWindowStyle()
{
    this->setWindowIcon(QIcon(":/img/icon.ico"));

    // 定义默认配置
    QMap<QString, int> defaultSettings = {
        {"Style/RowHeight", 75},
        {"Style/RowWidth", 200},
        {"Style/MaxWindowHeight", 500}
    };

    // 读取或设置默认配置
    for (auto it = defaultSettings.begin(); it != defaultSettings.end(); ++it) {
        if (!m_settings->contains(it.key())) {
            m_settings->setValue(it.key(), it.value());
        }
    }

    // 读取配置值
    this->rowHeight = m_settings->value("Style/RowHeight", defaultSettings["Style/RowHeight"]).toInt();
    this->rowWidth = m_settings->value("Style/RowWidth", defaultSettings["Style/RowWidth"]).toInt();
    this->maxWindowHeight = m_settings->value("Style/MaxWindowHeight", defaultSettings["Style/MaxWindowHeight"]).toInt();
    
    // 读取窗口层级，默认为 LevelTop (2)
    int level = m_settings->value("Style/WindowLevel", LevelTop).toInt();
    m_windowLevel = static_cast<WindowLevel>(level);
    
    // 读取 Frameless 状态
    this->frameless = m_settings->value("Style/Frameless", true).toBool();

    // 退出状态
    m_settings->beginGroup("MainWindow");
    this->restoreGeometry(m_settings->value("geometry").toByteArray());
    this->restoreState(m_settings->value("windowState").toByteArray());
    m_settings->endGroup();

    // 应用初始窗口 Flags
    Qt::WindowFlags flags = Qt::Tool;
    
    if (frameless)
        flags |= Qt::FramelessWindowHint;

    if (m_windowLevel == LevelTop)
        flags |= Qt::WindowStaysOnTopHint;
    else if (m_windowLevel == LevelBottom)
        flags |= Qt::WindowStaysOnBottomHint;

    this->setWindowFlags(flags);

    this->setAttribute(Qt::WA_TranslucentBackground); // 透明窗口

    // 初始化鼠标穿透状态
    bool mouseThrough = m_settings->value("Style/MouseThrough", false).toBool();
    setMouseThrough(mouseThrough);
}


void MainWindow::onSessionAdded(const AudioSessionData &data)
{
    if (sessionMap.contains(data.pid))
        return;

    QWidget *row = createSessionRow(data);
    int idx = ui->sessionLayout->count();
    if (!bottomSpacers.isEmpty())
    {
        idx = ui->sessionLayout->indexOf(bottomSpacers.first());
    }
    ui->sessionLayout->insertWidget(idx, row);
    sessionMap.insert(data.pid, row);

    // Update Layout
    updateWindowGeometry();
}

void MainWindow::onSessionRemoved(DWORD pid)
{
    if (!sessionMap.contains(pid))
        return;

    QWidget *row = sessionMap.take(pid);
    ui->sessionLayout->removeWidget(row);
    row->deleteLater();

    // Update Layout
    updateWindowGeometry();
}

void MainWindow::updateWindowGeometry()
{
    // 更新 Spacers 数量
    int need = maxWindowHeight/1.8 - rowHeight/2;
    int count = (need + rowHeight - 1) / rowHeight;

    // ---------- 顶部 ----------
    while (topSpacers.size() < count-1)
    {
        QWidget *w = new QWidget;
        w->setFixedHeight(rowHeight);
        ui->sessionLayout->insertWidget(0, w);
        topSpacers.append(w);
    }
    while (topSpacers.size() > count-1)
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

    // 计算遮罩高度
    constexpr double ROW_RATIO  = 0.8;
    constexpr double VIEW_RATIO = 0.3;
    constexpr int MIN_PX = 150;
    constexpr int MAX_PX = 500;

    int byRow  = int(rowHeight * ROW_RATIO);
    int byView = int(maxWindowHeight * VIEW_RATIO);
    int fade   = qBound(MIN_PX, qMin(byRow, byView), MAX_PX);

    if (m_fadeMask)
    {
        ScrollFadeMask *mask = static_cast<ScrollFadeMask*>(m_fadeMask);
        mask->setFadeHeight(fade);
        mask->resize(ui->scrollArea->viewport()->size());
    }

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

QWidget *MainWindow::createSessionRow(const AudioSessionData &data)
{
    auto *row = new SessionRow(data, rowWidth, rowHeight);
    connect(row, &SessionRow::volumeChanged, this, &MainWindow::volumeChanged);
    connect(row, &SessionRow::layoutRequest, this, &MainWindow::updateWindowGeometry);//ui更新发生内存泄漏
    return row;
}

// 切换窗口交互模式（移动编辑 / 只能查看）
void MainWindow::toggleInteractMode()
{
    setFrameless(!frameless);
}

void MainWindow::setFrameless(bool isFrameless)
{
    if (frameless == isFrameless) return;
    
    frameless = isFrameless;
    m_settings->setValue("Style/Frameless", frameless);

    // 先隐藏窗口以应用标志更改
    bool wasVisible = this->isVisible();
    if (wasVisible) this->hide();

    Qt::WindowFlags flags = Qt::Tool;
    if (frameless)
    {
        flags |= Qt::FramelessWindowHint;
    }

    if (m_windowLevel == LevelTop)
    {
        flags |= Qt::WindowStaysOnTopHint;
    }
    else if (m_windowLevel == LevelBottom)
    {
        flags |= Qt::WindowStaysOnBottomHint;
    }

    this->setWindowFlags(flags);

    // 重新显示窗口
    if (wasVisible) this->show();
}

// 设置窗口层级
void MainWindow::setWindowLevel(WindowLevel level)
{
    if (m_windowLevel == level) return;
    m_windowLevel = level;

    // 保存设置
    m_settings->setValue("Style/WindowLevel", (int)m_windowLevel);

    // 应用更改
    this->hide();

    Qt::WindowFlags flags = Qt::Tool;
    if (frameless)
    {
        flags |= Qt::FramelessWindowHint;
    }

    if (m_windowLevel == LevelTop)
    {
        flags |= Qt::WindowStaysOnTopHint;
    }
    else if (m_windowLevel == LevelBottom)
    {
        flags |= Qt::WindowStaysOnBottomHint;
    }

    this->setWindowFlags(flags);
    this->show();
}

void MainWindow::setMouseThrough(bool isThrough)
{
    m_mouseThrough = isThrough;
    m_settings->setValue("Style/MouseThrough", m_mouseThrough);
    
    // 设置透明背景并防止鼠标穿透（通过设置极其微小的透明度）
    // 0: 穿透, 1: 不穿透
    int alpha = m_mouseThrough ? 0 : 1; 

    QPalette pal = this->palette();
    pal.setColor(QPalette::Window, QColor(100, 100, 100, alpha)); 
    this->setPalette(pal);
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