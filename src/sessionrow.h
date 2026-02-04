#pragma once

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QHBoxLayout>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>
#include <QEvent>
#include <QTimer>
#include <Windows.h>
#include "audiocontroller.h"
#include <QPainter>
#include <shellapi.h>
#include <QtWin>
#include <QTimer>

class QGraphicsDropShadowEffect;
class QPropertyAnimation;

class SessionRow : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int fixedHeight READ height WRITE setFixedHeightAnim)
    Q_PROPERTY(int glowRadius READ glowRadius WRITE setGlowRadius)
    Q_PROPERTY(QColor glowColor READ glowColor WRITE setGlowColor)
    Q_PROPERTY(int topMargin READ topMargin WRITE setTopMargin)
    Q_PROPERTY(int bottomMargin READ bottomMargin WRITE setBottomMargin)
    Q_PROPERTY(int leftMargin READ leftMargin WRITE setLeftMargin)
    Q_PROPERTY(int iconSize READ iconSize WRITE setIconSize)

public:
    explicit SessionRow(const AudioSessionData &data, int w, int h, QWidget *parent = nullptr);
    ~SessionRow() override;

    void setFixedHeightAnim(int h);
    
    int glowRadius() const;
    void setGlowRadius(int r);

    QColor glowColor() const;
    void setGlowColor(const QColor &c);

    int topMargin() const;
    void setTopMargin(int m);

    int bottomMargin() const;
    void setBottomMargin(int m);

    int leftMargin() const;
    void setLeftMargin(int m);

    int iconSize() const;
    void setIconSize(int s);

    void updateMargins();

signals:
    void volumeChanged(DWORD pid, int vol);
    void layoutRequest(); // Request parent to update layout

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    int m_baseHeight;
    int m_expandSize;
    int m_topMargin = 0;
    int m_bottomMargin = 0;
    int m_leftMargin = 10;
    int m_idleLeftMargin = 10;
    int defaultIconSize = 64; // 默认图标大小
    int m_iconSize = defaultIconSize;
    double kHoverScaleFactor = 1.1;//放大倍率
    DWORD m_pid;
    QGraphicsDropShadowEffect *m_glowEffect;
    QLabel *m_iconLabel = nullptr; 
    QPropertyAnimation *m_animHeight;
    QPropertyAnimation *m_animTopMargin;
    QPropertyAnimation *m_animBottomMargin;
    QPropertyAnimation *m_animLeftMargin;
    QPropertyAnimation *m_animGlowRadius;
    QPropertyAnimation *m_animGlowColor;
    QPropertyAnimation *m_animIconSize;
};
