#pragma once

#include <QScrollBar>

class QEvent;
class QTimer;

class QVariantAnimation;

namespace Fluent {

class FluentScrollBar final : public QScrollBar
{
    Q_OBJECT
    Q_PROPERTY(qreal revealLevel READ revealLevel WRITE setRevealLevel)
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)

public:
    explicit FluentScrollBar(Qt::Orientation orientation, QWidget *parent = nullptr);

    void setForceVisible(bool visible);
    bool forceVisible() const;

    qreal revealLevel() const;
    void setRevealLevel(qreal v);

    qreal hoverLevel() const;
    void setHoverLevel(qreal v);

    void setOverlayMode(bool overlay);
    bool overlayMode() const;

protected:
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void attachToViewportIfPossible();
    void revealForInteraction();
    void updateVisibility(bool animated);
    void startRevealAnimation(qreal to);
    void startHoverAnimation(qreal to);

    bool m_forceVisible = false;
    bool m_overlayMode = true;
    bool m_pressed = false;

    qreal m_revealLevel = 0.0;
    qreal m_hoverLevel = 0.0;

    QVariantAnimation *m_revealAnim = nullptr;
    QVariantAnimation *m_hoverAnim = nullptr;

    QObject *m_viewport = nullptr;
    QTimer *m_hideTimer = nullptr;
};

} // namespace Fluent
