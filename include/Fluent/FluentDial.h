#pragma once

#include <QWidget>
#include "Fluent/FluentQtCompat.h"

class QMouseEvent;
class QPaintEvent;
class QWheelEvent;
class QFocusEvent;
class QEvent;
class QVariantAnimation;

namespace Fluent {

/**
 * FluentDial — compact circular knob for angle selection (0–359°).
 *
 * Convention matches the gradient angle: 0° = left→right (east),
 * values increase clockwise (same as Qt / CSS coordinate system).
 * The filled arc visualises the current angle from the 3-o'clock position.
 */
class FluentDial final : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged USER true)
    Q_PROPERTY(bool ticksVisible READ ticksVisible WRITE setTicksVisible)
    Q_PROPERTY(int tickStep READ tickStep WRITE setTickStep)
    Q_PROPERTY(int majorTickStep READ majorTickStep WRITE setMajorTickStep)
    Q_PROPERTY(bool pointerVisible READ pointerVisible WRITE setPointerVisible)
public:
    explicit FluentDial(QWidget *parent = nullptr);

    int value() const { return m_value; }
    void setValue(int angleDegrees);

    bool ticksVisible() const { return m_ticksVisible; }
    void setTicksVisible(bool visible);

    int tickStep() const { return m_tickStep; }
    void setTickStep(int stepDegrees);

    int majorTickStep() const { return m_majorTickStep; }
    void setMajorTickStep(int stepDegrees);

    bool pointerVisible() const { return m_pointerVisible; }
    void setPointerVisible(bool visible);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

signals:
    void valueChanged(int value);

protected:
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateFromPos(const QPointF &pos, bool emitSignal = true);

    int  m_value          = 0;
    bool m_dragging       = false;
    bool m_ticksVisible   = false;
    int  m_tickStep       = 15;
    int  m_majorTickStep  = 45;
    bool m_pointerVisible = true;
    qreal m_hoverLevel    = 0.0;
    qreal m_focusLevel    = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_focusAnim = nullptr;
};

} // namespace Fluent

