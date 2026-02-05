#pragma once

#include <QDialog>
#include <QPointer>

#include "Fluent/FluentBorderEffect.h"

class QLabel;
class QWidget;

class QEvent;
class QHideEvent;
class QResizeEvent;
class QShowEvent;

namespace Fluent {

class FluentResizeHelper;
class FluentToolButton;

class FluentDialog final : public QDialog
{
    Q_OBJECT
public:
    enum WindowButton {
        MinimizeButton = 0x1,
        MaximizeButton = 0x2,
        CloseButton = 0x4,
    };
    Q_DECLARE_FLAGS(WindowButtons, WindowButton)

    explicit FluentDialog(QWidget *parent = nullptr);

    // Optional modal mask (dim overlay) on top of the parent window while the dialog is visible.
    void setMaskEnabled(bool enabled);
    bool maskEnabled() const;

    void setMaskOpacity(qreal opacity);
    qreal maskOpacity() const;

    void setFluentWindowButtons(WindowButtons buttons);
    WindowButtons fluentWindowButtons() const;

    // Optional frameless resize support (disabled by default).
    void setFluentResizeEnabled(bool enabled);
    bool fluentResizeEnabled() const;

    void setFluentResizeBorderWidth(int px);
    int fluentResizeBorderWidth() const;

protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void applyTheme();
    void updateResizeHelperState();
    void ensureTitleBar();
    void updateTitleBarContent();
    void updateWindowControlIcons();
    void updateWindowMarginsForState();

    void ensureMaskOverlay();
    void teardownMaskOverlay();

    QWidget *m_titleBarHost = nullptr;
    QWidget *m_windowButtonsHost = nullptr;
    QLabel *m_titleLabel = nullptr;
    FluentToolButton *m_minBtn = nullptr;
    FluentToolButton *m_maxBtn = nullptr;
    FluentToolButton *m_closeBtn = nullptr;

    WindowButtons m_windowButtons = WindowButtons(MinimizeButton | MaximizeButton | CloseButton);

    bool m_dragging = false;
    QPoint m_dragOffset;

    bool m_resizeEnabled = false;
    FluentResizeHelper *m_resizeHelper = nullptr;

    bool m_maskEnabled = false;
    qreal m_maskOpacity = 0.35;
    QPointer<QWidget> m_maskOverlay;

    bool m_paintReady = false;

    FluentBorderEffect m_border;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FluentDialog::WindowButtons)

} // namespace Fluent
