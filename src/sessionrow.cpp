#include "sessionrow.h"


SessionRow::SessionRow(const AudioSessionData &data, int w, int h, QWidget *parent)
    : QWidget(parent), m_baseHeight(h), m_expandSize(h / 2), m_pid(data.pid)
{
    int fullWidth = w * 2; 
    int idleWidth = int(fullWidth / kHoverScaleFactor);
    int marginDelta = fullWidth - idleWidth;
    m_idleLeftMargin = 10 + marginDelta;
    m_leftMargin = m_idleLeftMargin;

    // 初始化基础属性
    this->setMinimumHeight(h);
    this->setMaximumHeight(h);
    
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setStyleSheet("background: transparent;");

    QHBoxLayout *layout = new QHBoxLayout(this);
    updateMargins();
    layout->setSpacing(8);

    // [控件 1] 音量滑动条
    m_slider = new QSlider(Qt::Horizontal);
    m_slider->setRange(0, 100);
    m_slider->setInvertedAppearance(true);
    m_slider->setInvertedControls(false); // 保持常规操作右大左小
    m_slider->setMinimumWidth(w);
    // 移除固定高度，允许垂直扩展以增加触摸区域
    m_slider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // 设置 QSlider 样式 (粉色系)
    m_slider->setStyleSheet(R"(
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

    m_slider->setValue(int(data.volume * 100));

    const int initialW = w; 
    
    // 连接信号：当值改变时，发送音量变更信号
    connect(m_slider, &QSlider::valueChanged, this, [this](int v)
            {
                // 如果用户手动拖动，更新静音状态
                if (v > 0 && m_isMuted) {
                    m_isMuted = false;
                    m_iconLabel->setPixmap(m_originPixmap);
                    m_iconLabel->setStyleSheet(R"(
                        QLabel#sessionIcon {
                            background: transparent;
                            border-radius: 4px;
                            padding: 2px;
                            border: 2px solid transparent;
                        }
                    )");
                }
                emit volumeChanged(m_pid, v);
            });

    // [控件 2] 进程图标
    m_originPixmap = QPixmap();
    if (!data.exePath.isEmpty())
    {
        SHFILEINFOW info{};
        // 宽字符转换以兼容 Windows API
        std::wstring wpath = data.exePath.toStdWString();
        if (SHGetFileInfoW(wpath.c_str(), 0, &info, sizeof(info), SHGFI_ICON | SHGFI_LARGEICON))
        {
            m_originPixmap = QtWin::fromHICON(info.hIcon);
            DestroyIcon(info.hIcon);
        }
    }

    m_iconLabel = new QLabel;
    m_iconLabel->setObjectName("sessionIcon");
    m_iconLabel->setFixedSize(m_iconSize, m_iconSize);
    m_iconLabel->setPixmap(m_originPixmap);
    m_iconLabel->setScaledContents(true);
    m_iconLabel->setStyleSheet(R"(
        QLabel#sessionIcon {
            background: transparent;
            border-radius: 4px;
            padding: 2px;
            border: 2px solid transparent;
        }
    )");
    m_iconLabel->installEventFilter(this); // 安装事件过滤器

    layout->addWidget(m_slider, 1);
    layout->addWidget(m_iconLabel);

    // [视觉效果] 边缘光晕阴影
    m_glowEffect = new QGraphicsDropShadowEffect(this);
    m_glowEffect->setBlurRadius(0);
    m_glowEffect->setColor(Qt::transparent);
    m_glowEffect->setOffset(0, 0);
    this->setGraphicsEffect(m_glowEffect);

    // [动画组] 定义各项属性动画
    m_animHeight = new QPropertyAnimation(this, "fixedHeight", this);
    m_animHeight->setDuration(500);
    m_animHeight->setEasingCurve(QEasingCurve::OutCubic);
    
    // 静音 Q 弹动画 (复用 iconSize 属性，但使用不同曲线)
    m_animMute = new QPropertyAnimation(this, "iconSize", this);
    m_animMute->setDuration(1000);
    m_animMute->setEasingCurve(QEasingCurve::OutElastic); // Q 弹效果

    m_animTopMargin = new QPropertyAnimation(this, "topMargin", this);
    m_animTopMargin->setDuration(500);
    m_animTopMargin->setEasingCurve(QEasingCurve::OutCubic);

    m_animBottomMargin = new QPropertyAnimation(this, "bottomMargin", this);
    m_animBottomMargin->setDuration(500);
    m_animBottomMargin->setEasingCurve(QEasingCurve::OutCubic);

    m_animLeftMargin = new QPropertyAnimation(this, "leftMargin", this);
    m_animLeftMargin->setDuration(500);
    m_animLeftMargin->setEasingCurve(QEasingCurve::OutCubic);

    m_animIconSize = new QPropertyAnimation(this, "iconSize", this);
    m_animIconSize->setDuration(500);
    m_animIconSize->setEasingCurve(QEasingCurve::OutCubic);

    m_animGlowRadius = new QPropertyAnimation(this, "glowRadius", this);
    m_animGlowRadius->setDuration(500);
    m_animGlowRadius->setEasingCurve(QEasingCurve::OutCubic);

    m_animGlowColor = new QPropertyAnimation(this, "glowColor", this);
    m_animGlowColor->setDuration(500);
    m_animGlowColor->setEasingCurve(QEasingCurve::OutCubic);

    // 关键：当尺寸/边距发生变化时，请求父级重新布局，以便窗口大小随之改变
    connect(m_animHeight, &QPropertyAnimation::valueChanged, this, &SessionRow::layoutRequest);
    connect(m_animTopMargin, &QPropertyAnimation::valueChanged, this, &SessionRow::layoutRequest);
    connect(m_animBottomMargin, &QPropertyAnimation::valueChanged, this, &SessionRow::layoutRequest);
    connect(m_animLeftMargin, &QPropertyAnimation::valueChanged, this, [this](){ update(); });
    connect(m_animIconSize, &QPropertyAnimation::valueChanged, this, [this](){ update(); });

    QTimer::singleShot(0, this, [this](){
        emit layoutRequest();
    });
}

SessionRow::~SessionRow()
{
}

void SessionRow::setFixedHeightAnim(int h) {
    this->setMinimumHeight(h);
    this->setMaximumHeight(h);
}

int SessionRow::glowRadius() const { return m_glowEffect->blurRadius(); }
void SessionRow::setGlowRadius(int r) { m_glowEffect->setBlurRadius(r); }

QColor SessionRow::glowColor() const { return m_glowEffect->color(); }
void SessionRow::setGlowColor(const QColor &c) { m_glowEffect->setColor(c); }

int SessionRow::topMargin() const { return m_topMargin; }
void SessionRow::setTopMargin(int m) {
    m_topMargin = m;
    updateMargins();
}

int SessionRow::bottomMargin() const { return m_bottomMargin; }
void SessionRow::setBottomMargin(int m) {
    m_bottomMargin = m;
    updateMargins();
}

int SessionRow::leftMargin() const { return m_leftMargin; }
void SessionRow::setLeftMargin(int m) {
    m_leftMargin = m;
    updateMargins();
}

int SessionRow::iconSize() const { return m_iconSize; }
void SessionRow::setIconSize(int s) {
    m_iconSize = s;
    if (m_iconLabel) m_iconLabel->setFixedSize(s, s);
}

void SessionRow::updateMargins() {
    setContentsMargins(m_leftMargin, m_topMargin, 10, m_bottomMargin);
}

void SessionRow::enterEvent(QEvent *event) {
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

    // 左边距动画 - 恢复到10 (填充)
    m_animLeftMargin->stop();
    m_animLeftMargin->setStartValue(leftMargin());
    m_animLeftMargin->setEndValue(10);
    m_animLeftMargin->start();

    // 图标动画 - 放大
    m_animIconSize->stop();
    m_animIconSize->setStartValue(iconSize());
    m_animIconSize->setEndValue(int(defaultIconSize * kHoverScaleFactor));
    m_animIconSize->start();

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

void SessionRow::leaveEvent(QEvent *event) {
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

    // 恢复左边距 - 到计算好的Idle值
    m_animLeftMargin->stop();
    m_animLeftMargin->setStartValue(leftMargin());
    m_animLeftMargin->setEndValue(m_idleLeftMargin); 
    m_animLeftMargin->start();

    // 恢复图标
    if (m_animMute->state() == QAbstractAnimation::Running) {
        m_animMute->stop();
    }
    m_animIconSize->stop();
    m_animIconSize->setStartValue(iconSize());
    m_animIconSize->setEndValue(defaultIconSize);
    m_animIconSize->start();

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

bool SessionRow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_iconLabel && event->type() == QEvent::MouseButtonRelease) {
        toggleMute();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void SessionRow::toggleMute() {
    m_isMuted = !m_isMuted;

    // 播放 Q 弹动画
    // 先停止可能正在进行的 hover 动画
    m_animIconSize->stop(); 
    m_animMute->stop();

    // 确定目标大小
    int targetSize = rect().contains(mapFromGlobal(QCursor::pos())) ? (defaultIconSize * kHoverScaleFactor) : defaultIconSize;
    
    // 动画逻辑
    m_animMute->setStartValue(int(targetSize * 0.7));
    m_animMute->setEndValue(targetSize);
    m_animMute->start();

    if (m_isMuted) {
        m_lastVolume = m_slider->value();
        m_slider->setValue(0); // 这会触发 valueChanged 信号从而发送静音给系统
        
        // 视觉变灰
        if (!m_originPixmap.isNull()) {
            QPixmap mutedPixmap(m_originPixmap.size());
            mutedPixmap.fill(Qt::transparent);
            QPainter p(&mutedPixmap);
            p.setOpacity(0.5);
            p.drawPixmap(0, 0, m_originPixmap);
            p.end();
            m_iconLabel->setPixmap(mutedPixmap);
        }
        
    } else {
        // 恢复音量
        if (m_lastVolume == 0) m_lastVolume = 30; // 避免恢复到 0
        m_slider->setValue(m_lastVolume);

        // 恢复视觉
        m_iconLabel->setPixmap(m_originPixmap);
    }
}
