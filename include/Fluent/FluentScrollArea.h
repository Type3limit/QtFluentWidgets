#pragma once

#include <QScrollArea>

class QEvent;
class QTimer;

namespace Fluent {

class FluentScrollBar;

class FluentScrollArea : public QScrollArea
{
    Q_OBJECT
public:
    explicit FluentScrollArea(QWidget *parent = nullptr);

    // Convenience APIs for managing the scroll area's content widget/layout.
    // Note: QScrollArea already exposes widget()/setWidget(); these helpers ensure a sensible
    // Fluent default (transparent background) and make it easy to swap layouts.
    QWidget *contentWidget() const;
    QLayout *contentLayout() const;

    // Ensures a content widget exists (creates a FluentWidget) and replaces its layout.
    // If a layout already exists, it will be deleted.
    void setContentLayout(QLayout *layout);

    void setOverlayScrollBarsEnabled(bool enabled);
    bool overlayScrollBarsEnabled() const;

    // Forces overlay scrollbars to reveal/hide immediately. Useful for "show on hover" behaviors.
    void setScrollBarsRevealed(bool revealed);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void ensureScrollBars();
    void ensureOverlayScrollBars();
    void syncOverlayScrollBars();
    void updateOverlayGeometry();
    void revealScrollBars(bool reveal);

    bool m_overlayEnabled = true;
    QTimer *m_hideTimer = nullptr;
    FluentScrollBar *m_vbar = nullptr;
    FluentScrollBar *m_hbar = nullptr;

    FluentScrollBar *m_vbarOverlay = nullptr;
    FluentScrollBar *m_hbarOverlay = nullptr;

    bool m_overlayWired = false;
};

} // namespace Fluent
