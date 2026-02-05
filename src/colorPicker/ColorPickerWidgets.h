#pragma once

#include <QColor>
#include <QWidget>

class QEvent;
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
    void enterEvent(QEnterEvent *event) override;
#else
    void enterEvent(QEvent *event) override;
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

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor m_color = QColor(0, 103, 192);
};

class HueStrip final : public QWidget
{
    Q_OBJECT
public:
    explicit HueStrip(QWidget *parent = nullptr);

    int value() const { return m_value; }
    void setValue(int v);

signals:
    void valueChanged(int v);
    void valueChangeFinished(int v);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void setFromY(qreal y, bool emitSignal);

    int m_value = 0;
    bool m_dragging = false;
    bool m_hover = false;
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
    void enterEvent(QEvent *event) override;
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

} // namespace Fluent::ColorPicker
