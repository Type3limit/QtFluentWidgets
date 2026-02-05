#pragma once

#include <QObject>
#include <QPoint>
#include <QRect>

class QWidget;
class QEvent;
class QHoverEvent;
class QMouseEvent;

namespace Fluent {

class FluentResizeHelper final : public QObject
{
    Q_OBJECT
public:
    enum Edge {
        None = 0x0,
        Left = 0x1,
        Top = 0x2,
        Right = 0x4,
        Bottom = 0x8,
        TopLeft = 0x10,
        TopRight = 0x20,
        BottomLeft = 0x40,
        BottomRight = 0x80,
    };
    Q_ENUM(Edge)
    Q_DECLARE_FLAGS(Edges, Edge)

    explicit FluentResizeHelper(QWidget *target);

    void setEnabled(bool enabled);
    bool isEnabled() const;

    void setBorderWidth(int width);
    int borderWidth() const;

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void mouseHover(QHoverEvent *event);
    void mouseLeave();
    void mousePress(QMouseEvent *event);
    void mouseRelease(QMouseEvent *event);
    void mouseMove(QMouseEvent *event);

    void updateCursorShape(const QPoint &globalPos);
    Edges calculateEdges(const QPoint &globalPos) const;

    QRect adjustedGeometryForDrag(const QPoint &globalPos) const;

    QWidget *m_target = nullptr;
    bool m_enabled = false;

    bool m_cursorChanged = false;
    bool m_leftButtonPressed = false;
    Edges m_pressEdges = None;

    int m_borderWidth = 8;

    QPoint m_pressGlobalPos;
    QRect m_pressGeometry;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FluentResizeHelper::Edges)

} // namespace Fluent
