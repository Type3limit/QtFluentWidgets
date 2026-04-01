#pragma once

#include <QDialog>
#include <QColor>
#include <QPoint>
#include <QPointer>
#include <QVector>
#include <QPair>

#include "Fluent/FluentBorderEffect.h"

namespace Fluent {

enum class ColorPickerMode {
    Solid,
    LinearGradient,
    RadialGradient
};

struct FluentGradientStop {
    qreal    position = 0.0;
    QColor   color;
};

struct FluentGradientResult {
    ColorPickerMode          mode = ColorPickerMode::Solid;
    QColor                   solidColor;
    QVector<FluentGradientStop> stops;     // populated in gradient modes
    int                      angle = 0;   // degrees, 0=left→right (LinearGradient only)
};

class FluentColorDialog final : public QDialog
{
    Q_OBJECT
public:
    explicit FluentColorDialog(const QColor &initial, QWidget *parent = nullptr);

    QColor selectedColor() const;

    void setCurrentColor(const QColor &color);
    QColor currentColor() const;

    void setResetColor(const QColor &color);
    QColor resetColor() const;

    ColorPickerMode colorPickerMode() const { return m_mode; }
    FluentGradientResult gradientResult() const;

    // Gradient support toggle — when disabled the mode combo is hidden and
    // the dialog operates in solid-colour mode only.
    void setGradientEnabled(bool enabled);
    bool isGradientEnabled() const { return m_gradientEnabled; }

    int gradientAngle() const { return m_gradientAngle; }

signals:
    void colorChanged(const QColor &color);
    void gradientResultChanged(const Fluent::FluentGradientResult &result);

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void applyUiFromColor(bool emitSignal);
    void applyColorFromUi(bool emitSignal);
    void setSelectedColorInternal(const QColor &color, bool emitSignal);
    void setMode(ColorPickerMode mode);

    QColor m_selected;
    QColor m_reset;
    ColorPickerMode m_mode = ColorPickerMode::Solid;

    bool m_gradientEnabled = true;
    int  m_gradientAngle   = 0;

    QWidget *m_svPanel        = nullptr;
    QWidget *m_hueStrip       = nullptr;
    QWidget *m_alphaStrip     = nullptr;
    QWidget *m_previewSwatch  = nullptr;
    QWidget *m_hexEdit        = nullptr;
    QWidget *m_alphaSpin      = nullptr;
    QWidget *m_gradientStopBar    = nullptr;
    QWidget *m_gradientContainer  = nullptr;
    QWidget *m_modeCombo          = nullptr;   // FluentComboBox in title bar
    QWidget *m_angleEditor        = nullptr;   // FluentAngleSelector for gradient angle

    bool m_uiUpdating      = false;
    bool m_suppressAutoClose = false;

    QPointer<QWidget> m_dragHandle;
    bool   m_dragging   = false;
    QPoint m_dragOffset;

    FluentBorderEffect m_border{this};
};

} // namespace Fluent


