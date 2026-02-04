#include "sessionrow.h"


SessionRow::SessionRow(const AudioSessionData &data, int w, int h, QWidget *parent)
    : QWidget(parent), m_baseHeight(h), m_expandSize(h / 2), m_pid(data.pid)
{
    // 初始化基础属性
    this->setMinimumHeight(h);
    this->setMaximumHeight(h);
    
    this->setAttribute(Qt::WA_TranslucentBackground);
    this->setStyleSheet("background: transparent;");

    QHBoxLayout *layout = new QHBoxLayout(this);
    updateMargins();
    layout->setSpacing(8);

    // [控件 1] 音量滑动条
    QSlider *slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 100);
    slider->setInvertedAppearance(true);
    slider->setInvertedControls(false); // 保持常规操作右大左小
    slider->setMinimumWidth(w);
    slider->setFixedHeight(32);

    // 设置 QSlider 样式 (粉色系)
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

    slider->setValue(int(data.volume * 100));

    // 连接信号：当值改变时，发送音量变更信号
    connect(slider, &QSlider::valueChanged, this, [this](int v)
            {
                emit volumeChanged(m_pid, v);
            });

    // [控件 2] 进程图标
    QPixmap iconPixmap;
    if (!data.exePath.isEmpty())
    {
        SHFILEINFOW info{};
        // 宽字符转换以兼容 Windows API
        std::wstring wpath = data.exePath.toStdWString();
        if (SHGetFileInfoW(wpath.c_str(), 0, &info, sizeof(info), SHGFI_ICON | SHGFI_LARGEICON))
        {
            iconPixmap = QtWin::fromHICON(info.hIcon);
            DestroyIcon(info.hIcon);
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

    // 关键：当尺寸/边距发生变化时，请求父级重新布局，以便窗口大小随之改变
    connect(m_animHeight, &QPropertyAnimation::valueChanged, this, &SessionRow::layoutRequest);
    connect(m_animTopMargin, &QPropertyAnimation::valueChanged, this, &SessionRow::layoutRequest);
    connect(m_animBottomMargin, &QPropertyAnimation::valueChanged, this, &SessionRow::layoutRequest);
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

void SessionRow::updateMargins() {
    setContentsMargins(10, m_topMargin, 10, m_bottomMargin);
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
