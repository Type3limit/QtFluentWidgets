#pragma once

#include <QMenu>

#include <QPointer>

#include <QRect>

#include "Fluent/FluentBorderEffect.h"

class QVariantAnimation;
class QMouseEvent;
class QPaintEvent;
class QShowEvent;
class QHideEvent;
class QResizeEvent;

namespace Fluent {

class FluentMenu final : public QMenu
{
    Q_OBJECT
public:
    explicit FluentMenu(QWidget *parent = nullptr);
    explicit FluentMenu(const QString &title, QWidget *parent = nullptr);
    void popup(const QPoint &pos, QAction *atAction = nullptr);
    QAction *exec(const QPoint &pos, QAction *atAction = nullptr);

    // Convenience: create Fluent submenus so level-2+ menus stay consistent.
    FluentMenu *addFluentMenu(const QString &title);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void closePopupHost();
    void ensureSubMenusFluent();
    void applyTheme();
    void startHoverAnimation(qreal endValue);

    void startPopupAnimation();
    void updateHighlightForAction(QAction *action, bool animate);
    QRect highlightTargetRect(QAction *action) const;
    void updateWindowMask();

    QAction *m_hoverAction = nullptr;
    qreal m_hoverLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;

    QAction *m_highlightAction = nullptr;
    QRect m_highlightRect;
    QVariantAnimation *m_highlightAnim = nullptr;

    QVariantAnimation *m_popupFadeAnim = nullptr;
    QVariantAnimation *m_popupSlideAnim = nullptr;
    QPointer<QWidget> m_popupHost;

    FluentBorderEffect m_border;
};

} // namespace Fluent