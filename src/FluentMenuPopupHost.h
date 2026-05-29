#pragma once

#include "Fluent/FluentQtCompat.h"
#include "Fluent/FluentBorderEffect.h"

#include <QMenu>
#include <QPointer>
#include <QWidget>

#include <functional>
#include <limits>

class QAction;
class QEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QTimer;
class QVariantAnimation;

namespace Fluent {

class MenuActionListWidget;

class FluentMenuPopupHost final : public QWidget
{
public:
    explicit FluentMenuPopupHost(QMenu *sourceMenu);

    std::function<void()> onClosed;
    std::function<void(QAction *)> onActionTriggered;
    std::function<void()> onEntered;

    void closeFromParent();
    bool isClosingFromParent() const;
    QMenu *menu() const;

    void popupAt(const QPoint &contentTopLeft,
                 QAction *atAction = nullptr,
                 int slideOffsetY = std::numeric_limits<int>::min());

    QSize sizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void handleForeignMouseMove(const QPoint &globalPos);
    void updateHoverAction(QAction *action);
    void triggerAction(QAction *action);
    void syncChildPopup();
    QPoint submenuPopupPosition(QAction *action, const QSize &popupSize) const;
    void closeChildPopup();
    void scheduleCloseChildPopup(int delayMs = 320);
    void cancelPendingChildClose();
    void startOpenAnimation(const QRect &targetGeometry, int slideOffsetY);
    void stopOpenAnimation(bool resetOpacity = true);

    QPointer<QMenu> m_menu;
    MenuActionListWidget *m_view = nullptr;
    QPointer<FluentMenuPopupHost> m_childPopup;
    QPointer<FluentMenuPopupHost> m_parentHost;
    QTimer *m_pendingCloseTimer = nullptr;
    bool m_closingFromParent = false;
    QAction *m_hoverAction = nullptr;
    FluentBorderEffect m_border;
    QVariantAnimation *m_openFadeAnim = nullptr;
    QVariantAnimation *m_openSlideAnim = nullptr;
};

} // namespace Fluent
