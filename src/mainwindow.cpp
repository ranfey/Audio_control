#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <iostream>
#include <QGraphicsDropShadowEffect>
#include "sessionrow.h"

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


// SessionRow removed because it is now in its own file


MainWindow::MainWindow(QSettings *settings, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_settings(settings)
{
    ui->setupUi(this);

    initWindowStyle(); // 初始化窗口样式

    initUi(true); // 构建 UI 元素

    this->show(); // 显示窗口
}

MainWindow::~MainWindow()
{
    // 保存退出状态
    m_settings->beginGroup("MainWindow");
    m_settings->setValue("geometry", saveGeometry());
    m_settings->setValue("windowState", saveState());
    m_settings->endGroup();

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
void MainWindow::initUi(bool forceCreate)
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

// 初始化窗口样式与加载配置
void MainWindow::initWindowStyle()
{
    this->setWindowIcon(QIcon(":/img/icon.ico"));

    // 读取或设置默认配置
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

    // 设置透明背景并防止鼠标穿透（通过设置极其微小的透明度）
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, QColor(100, 100, 100, 2)); 
    this->setPalette(pal);
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
    initUi(false);
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
    connect(row, &SessionRow::layoutRequest, this, &MainWindow::updateWindowGeometry);
    return row;
}

// 切换窗口交互模式（移动编辑 / 只能查看）
void MainWindow::toggleInteractMode()
{
    frameless = !frameless;

    // 先隐藏窗口以应用标志更改
    this->hide();

    // 修改窗口标志
    if (frameless)
    {
        // 无边框工具窗口，底部显示
        this->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);
    }
    else
    {
        // 只有底部显示，允许系统边框（以便移动/调整大小，如果系统允许）
        // 注意：根据原逻辑，这里是移除 FramelessWindowHint
        this->setWindowFlags(Qt::Tool | Qt::WindowStaysOnBottomHint); 
    }

    // 重新显示窗口
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