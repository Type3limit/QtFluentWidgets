#pragma once

#include <QSlider>

class QPropertyAnimation;

namespace Fluent {

class FluentSlider final : public QSlider
{
    Q_OBJECT
    Q_PROPERTY(qreal handlePos READ handlePos WRITE setHandlePos)
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
public:
    explicit FluentSlider(Qt::Orientation orientation, QWidget *parent = nullptr);

    QSize sizeHint() const override;

    qreal handlePos() const;
    void setHandlePos(qreal value);

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void applyTheme();

    bool m_dragging = false;
    qreal m_dragOffsetPx = 0.0;

    qreal m_handlePos = 0.0;
    qreal m_hoverLevel = 0.0;
    QPropertyAnimation *m_positionAnim = nullptr;
    QPropertyAnimation *m_hoverAnim = nullptr;
};

} // namespace Fluent
