#pragma once

#include <QDialog>
#include <QColor>
#include <QPoint>
#include <QPointer>

#include "Fluent/FluentBorderEffect.h"

namespace Fluent {

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

signals:
    void colorChanged(const QColor &color);

protected:
    bool event(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void applyUiFromColor(bool emitSignal);
    void applyColorFromUi(bool emitSignal);
    void setSelectedColorInternal(const QColor &color, bool emitSignal);

    QColor m_selected;
    QColor m_reset;

    QWidget *m_svPanel = nullptr;
    QWidget *m_hueStrip = nullptr;
    QWidget *m_alphaStrip = nullptr;
    QWidget *m_previewSwatch = nullptr;
    QWidget *m_hexEdit = nullptr;
    QWidget *m_alphaSpin = nullptr;

    bool m_uiUpdating = false;

    bool m_suppressAutoClose = false;

    QPointer<QWidget> m_dragHandle;
    bool m_dragging = false;
    QPoint m_dragOffset;

    FluentBorderEffect m_border{this};
};

} // namespace Fluent