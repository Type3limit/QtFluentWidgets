#pragma once

#include <QColor>
#include <QGradient>
#include "Fluent/FluentQtCompat.h"
#include <QWidget>
#include <QVector>

class QContextMenuEvent;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
class QEnterEvent;
#endif

namespace Fluent::ColorPicker {

class ColorSwatchButton final : public QWidget
{
    Q_OBJECT
public:
    explicit ColorSwatchButton(const QColor &color, QWidget *parent = nullptr);

    QColor color() const { return m_color; }
    void setColor(const QColor &c);

signals:
    void clicked(const QColor &color);

protected:
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(FluentEnterEvent *event) override;
#else
    void enterEvent(FluentEnterEvent *event) override;
#endif
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_color;
    bool m_hover = false;
};

class SvPanel final : public QWidget
{
    Q_OBJECT
public:
    explicit SvPanel(QWidget *parent = nullptr);

    int hue() const { return m_h; }
    int s() const { return m_s; }
    int v() const { return m_v; }

    void setHue(int h);
    void setSv(int s, int v);

signals:
    void svChanged(int s, int v);
    void svChangeFinished(int s, int v);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void updateFromPos(const QPoint &pos);

    int m_h = 0;
    int m_s = 255;
    int m_v = 255;
    bool m_dragging = false;
};

class PreviewSwatch final : public QWidget
{
    Q_OBJECT
public:
    explicit PreviewSwatch(QWidget *parent = nullptr);

    void setColor(const QColor &c);
    void setGradientPreview(const QGradientStops &stops);
    void clearGradientPreview();

    // Gradient display options
    void setRadialMode(bool radial);
    void setGradientAngle(int angleDegrees);   // 0 = left→right, 90 = top→bottom

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_color = QColor(0, 103, 192);
    QGradientStops m_gradStops;
    bool m_showGradient = false;
    bool m_isRadial     = false;
    int  m_gradAngle    = 0;
};

class HueStrip final : public QWidget
{
    Q_OBJECT
public:
    explicit HueStrip(QWidget *parent = nullptr);

    int value() const { return m_value; }
    void setValue(int v);

    Qt::Orientation orientation() const { return m_orientation; }
    void setOrientation(Qt::Orientation o);

signals:
    void valueChanged(int v);
    void valueChangeFinished(int v);

protected:
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void setFromPos(qreal normalized, bool emitSignal);

    int m_value = 0;
    bool m_dragging = false;
    bool m_hover = false;
    Qt::Orientation m_orientation = Qt::Vertical;
};

class AlphaStrip final : public QWidget
{
    Q_OBJECT
public:
    explicit AlphaStrip(QWidget *parent = nullptr);

    int value() const { return m_value; }
    QColor baseColor() const { return m_baseColor; }

    void setValue(int v);
    void setBaseColor(const QColor &c);

signals:
    void valueChanged(int v);
    void valueChangeFinished(int v);

protected:
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void setFromX(qreal x, bool emitSignal);

    int m_value = 255;
    QColor m_baseColor = QColor(0, 103, 192);
    bool m_dragging = false;
    bool m_hover = false;
};

// ── Eye-dropper compact button ────────────────────────────────────────────
class EyeDropperButton final : public QWidget
{
    Q_OBJECT
public:
    explicit EyeDropperButton(QWidget *parent = nullptr);

signals:
    void clicked();

protected:
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    bool m_hover   = false;
    bool m_pressed = false;
};

// ── Gradient stop ──────────────────────────────────────────────────────────

struct GradientStop {
    qreal pos = 0.0;   // 0.0 – 1.0
    QColor color;
};

class GradientStopBar final : public QWidget
{
    Q_OBJECT
public:
    explicit GradientStopBar(QWidget *parent = nullptr);

    QVector<GradientStop> stops() const { return m_stops; }
    void setStops(const QVector<GradientStop> &stops);

    int selectedIndex() const { return m_selected; }
    void selectStop(int idx);

    QColor selectedColor() const;
    void setSelectedColor(const QColor &c);

    void addStop(qreal pos, const QColor &c);
    void removeSelectedStop();   // no-op if ≤ 2 stops

signals:
    void stopSelected(int idx);
    void stopsChanged();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    static constexpr int kBarPadX = 8;
    static constexpr int kBarTop  = 4;
    static constexpr int kBarH    = 24;
    static constexpr int kGap     = 4;
    static constexpr int kHandleR = 7;

    QRect barRect() const;
    int   handleCenterY() const;
    int   hitTestHandle(const QPoint &pos) const;
    qreal xToPos(int x) const;
    int   posToX(qreal pos) const;
    QColor colorAtPos(qreal pos) const;
    void  sortStops();

    QVector<GradientStop> m_stops;
    int m_selected = 0;
    int m_dragging = -1;
};

} // namespace Fluent::ColorPicker
