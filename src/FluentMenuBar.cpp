#include "Fluent/FluentMenuBar.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentPopupSurface.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentMenu.h"
#include "FluentMenuPopupHost.h"

#include <QActionEvent>
#include <QApplication>
#include <QEvent>
#include <QMenu>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QScreen>
#include <QStyle>
#include <QStyleFactory>
#include <QToolButton>
#include <QVariantAnimation>
#include <QWidget>

namespace Fluent {

namespace {
constexpr int kFluentMenuPopupGap = 5;
constexpr int kMenuBarOuterMargin = 4;
constexpr int kMenuBarItemMinWidth = 34;
constexpr int kMenuBarItemHPadding = 12;
constexpr int kMenuBarItemIconTextGap = 6;
constexpr int kMenuBarItemArrowWidth = 18;
constexpr int kMenuBarItemPaintInset = 2;

QString textWithoutMnemonic(QString text)
{
    text.remove(QLatin1Char('&'));
    return text;
}

void markActionLayoutDirty(QHash<QAction *, QRect> &rects, QSize &sizeHint, bool &dirty)
{
    dirty = true;
    sizeHint = QSize();
    rects.clear();
}

}

FluentMenuBar::FluentMenuBar(QWidget *parent)
    : QMenuBar(parent)
{
    // QApplication style sheets wrap every widget in QStyleSheetStyle. FluentMenuBar is
    // fully custom-painted, so keep it on a local base style and avoid the expensive
    // QMenuBar polish path that has also proven crash-prone during startup.
    if (QStyle *baseStyle = QStyleFactory::create(QStringLiteral("Fusion"))) {
        baseStyle->setParent(this);
        setStyle(baseStyle);
    }
    setStyleSheet(QString());
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setNativeMenuBar(false);

    // Keep menubar popup menus Fluent even if user calls QMenuBar::addMenu().
    ensureMenusFluent();

    m_hoverAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_highlightAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_highlightAnim, FluentMotionRole::Selection);
    connect(m_highlightAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_highlightRect = value.toRect();
        update();
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentMenuBar::applyTheme);
}

void FluentMenuBar::changeEvent(QEvent *event)
{
    const QEvent::Type type = event ? event->type() : QEvent::None;
    if (type != QEvent::StyleChange && type != QEvent::PaletteChange) {
        QMenuBar::changeEvent(event);
    }
    switch (type) {
    case QEvent::EnabledChange:
        applyTheme();
        break;
    case QEvent::FontChange:
        invalidateActionLayout();
        break;
    case QEvent::LayoutDirectionChange:
        invalidateActionLayout();
        break;
    case QEvent::StyleChange:
    case QEvent::PaletteChange:
        markActionLayoutDirty(m_actionRects, m_cachedSizeHint, m_layoutDirty);
        update();
        break;
    default:
        break;
    }
}

void FluentMenuBar::actionEvent(QActionEvent *event)
{
    QMenuBar::actionEvent(event);

    if (event->type() == QEvent::ActionRemoved) {
        QAction *action = event->action();
        m_actionMenus.remove(action);
        m_actionRects.remove(action);
        if (m_hoverAction == action) {
            m_hoverAction = nullptr;
        }
        if (m_pressedAction == action) {
            m_pressedAction = nullptr;
        }
        if (m_highlightAction == action) {
            m_highlightAction = nullptr;
            m_highlightRect = QRect();
        }
        if (m_openAction == action) {
            m_openAction = nullptr;
        }
    }

    // When actions/menus are added by external code, ensure they're Fluent.
    if (event->type() == QEvent::ActionAdded || event->type() == QEvent::ActionChanged) {
        ensureMenusFluent();
    }

    invalidateActionLayout();
}

void FluentMenuBar::applyTheme()
{
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    FluentMotion::configure(m_highlightAnim, FluentMotionRole::Selection);

    // This widget is fully custom-painted. Avoid QMenuBar style sheets here:
    // polishing a QMenuBar inside the title bar is surprisingly expensive on
    // Windows and was visible in demo startup logs as a >1s menu-bar phase.
    ensureMenusFluent();
    update();
}

FluentMenu *FluentMenuBar::addFluentMenu(const QString &title)
{
    auto *menu = new FluentMenu(title, this);
    QAction *action = QMenuBar::addAction(title);
    m_actionMenus.insert(action, menu);
    invalidateActionLayout();
    return menu;
}

void FluentMenuBar::ensureMenusFluent()
{
    bool layoutChanged = false;
    const QList<QAction *> acts = actions();
    for (QAction *a : acts) {
        if (!a) {
            continue;
        }

        if (m_actionMenus.contains(a) && !m_actionMenus.value(a).isNull()) {
            continue;
        }

        QMenu *sub = a->menu();
        if (!sub) {
            continue;
        }

        if (qobject_cast<FluentMenu *>(sub)) {
            continue;
        }

        // Convert native QMenu to FluentMenu while preserving QAction instances.
        auto *fluent = new FluentMenu(sub->title(), this);
        fluent->setIcon(sub->icon());

        const QList<QAction *> subActions = sub->actions();
        for (QAction *sa : subActions) {
            if (!sa) {
                continue;
            }
            sub->removeAction(sa);
            fluent->addAction(sa);
        }

        fluent->setParent(this);
        a->setMenu(nullptr);
        m_actionMenus.insert(a, fluent);
        layoutChanged = true;
        sub->deleteLater();
    }

    // If the built-in overflow/extension toolbutton exists, ensure its popup menu is Fluent too.
    if (auto *ext = findChild<QToolButton *>(QStringLiteral("qt_menubar_ext_button"))) {
        ext->setAutoRaise(true);
        ext->setFocusPolicy(Qt::NoFocus);
        ext->setPopupMode(QToolButton::InstantPopup);

        // We don't use the native overflow mechanism at all; it looks non-Fluent and can
        // cause "File/View" to appear in a native-looking overflow menu.
        // Hide it and rely on our own click-to-popup behavior.
        ext->setVisible(false);
        ext->setEnabled(false);

        if (QMenu *m = ext->menu()) {
            if (!qobject_cast<FluentMenu *>(m)) {
                auto *fluent = new FluentMenu(m->title(), this);
                const QList<QAction *> ms = m->actions();
                for (QAction *sa : ms) {
                    if (!sa) {
                        continue;
                    }
                    m->removeAction(sa);
                    fluent->addAction(sa);
                }
                ext->setMenu(fluent);
                m->deleteLater();
            }
        }

        // Ensure it won't pop anything.
        ext->setMenu(nullptr);
    }

    if (layoutChanged) {
        invalidateActionLayout();
    }
}

QMenu *FluentMenuBar::menuForAction(QAction *action) const
{
    if (!action) {
        return nullptr;
    }

    const auto it = m_actionMenus.constFind(action);
    if (it != m_actionMenus.cend() && !it.value().isNull()) {
        return it.value().data();
    }

    return action->menu();
}

void FluentMenuBar::invalidateActionLayout()
{
    m_layoutDirty = true;
    m_cachedSizeHint = QSize();
    m_actionRects.clear();
    const int nextMinimumWidth = sizeHint().width();
    if (minimumWidth() != nextMinimumWidth) {
        setMinimumWidth(nextMinimumWidth);
    }
    updateGeometry();
    update();
}

int FluentMenuBar::actionItemWidth(QAction *action, const QFontMetrics &fm) const
{
    if (!action || action->isSeparator() || !action->isVisible()) {
        return 0;
    }

    const int textWidth = fm.horizontalAdvance(textWithoutMnemonic(action->text()));
    const int iconWidth = action->icon().isNull() ? 0 : 16 + kMenuBarItemIconTextGap;
    const int arrowWidth = menuForAction(action) ? kMenuBarItemArrowWidth : 0;
    return qMax(kMenuBarItemMinWidth,
                textWidth + iconWidth + arrowWidth + kMenuBarItemHPadding * 2 + kMenuBarItemPaintInset * 2);
}

void FluentMenuBar::rebuildActionLayout() const
{
    if (!m_layoutDirty) {
        return;
    }

    m_actionRects.clear();

    const QFontMetrics fm(font());
    const int itemHeight = qMax(28, fm.height() + 12);
    int x = kMenuBarOuterMargin;

    const QList<QAction *> acts = actions();
    for (QAction *action : acts) {
        if (!action || action->isSeparator() || !action->isVisible()) {
            continue;
        }

        const int itemWidth = actionItemWidth(action, fm);
        if (itemWidth <= 0) {
            continue;
        }

        m_actionRects.insert(action, QRect(x, 0, itemWidth, itemHeight));
        x += itemWidth;
    }

    m_cachedSizeHint = QSize(qMax(kMenuBarOuterMargin * 2, x + kMenuBarOuterMargin), itemHeight);
    m_layoutDirty = false;
}

QRect FluentMenuBar::actionRect(QAction *action) const
{
    if (!action) {
        return QRect();
    }

    rebuildActionLayout();
    return m_actionRects.value(action);
}

QAction *FluentMenuBar::actionAtPosition(const QPoint &pos) const
{
    rebuildActionLayout();

    const QList<QAction *> acts = actions();
    for (QAction *action : acts) {
        if (!action || action->isSeparator() || !action->isVisible()) {
            continue;
        }

        const QRect r = m_actionRects.value(action);
        if (r.isValid() && r.contains(pos)) {
            return action;
        }
    }

    return nullptr;
}

void FluentMenuBar::mousePressEvent(QMouseEvent *event)
{
    if (event && event->button() == Qt::LeftButton) {
        QAction *a = actionAtPosition(event->pos());
        m_pressedAction = a;
        if (a && !a->isSeparator() && a->isEnabled()) {
            openMenuForAction(a);
            event->accept();
            return;
        }
        event->accept();
        return;
    }

    QMenuBar::mousePressEvent(event);
}

void FluentMenuBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (event && event->button() == Qt::LeftButton) {
        QAction *action = actionAtPosition(event->pos());
        if (action && action == m_pressedAction && action->isEnabled() && !menuForAction(action)) {
            action->trigger();
        }
        m_pressedAction = nullptr;
        event->accept();
        return;
    }
    QMenuBar::mouseReleaseEvent(event);
}

void FluentMenuBar::openMenuForAction(QAction *action)
{
    if (!action || action->isSeparator() || !action->isEnabled()) {
        return;
    }

    ensureMenusFluent();

    QMenu *menu = menuForAction(action);
    if (!menu) {
        // Non-menu actions still should appear active briefly.
        updateHighlightForAction(action, true);
        startHoverAnimation(1.0);
        return;
    }

    // Toggle if the same menu is already open.
    if ((m_openPopup && m_openAction == action && m_openPopup->isVisible()) ||
        (m_openMenu && m_openAction == action && m_openMenu->isVisible())) {
        if (m_openPopup) {
            m_openPopup->close();
        }
        if (m_openMenu) {
            m_openMenu->close();
        }
        return;
    }

    if (m_openPopup && m_openPopup->isVisible()) {
        m_openPopup->close();
    }
    if (m_openMenu && m_openMenu->isVisible()) {
        m_openMenu->close();
    }

    m_openMenu = menu;
    m_openAction = action;

    updateHighlightForAction(action, true);
    startHoverAnimation(1.0);

    const QRect r = actionRect(action);
    if (!r.isValid()) {
        return;
    }
    const bool useFluentPopup = qobject_cast<FluentMenu *>(menu) != nullptr;
    QPointer<FluentMenuPopupHost> fluentPopup;
    QSize popupSize;
    if (useFluentPopup) {
        auto *popup = new FluentMenuPopupHost(menu);
        popup->setAttribute(Qt::WA_DeleteOnClose, true);
        popup->setFont(font());
        fluentPopup = popup;
        popupSize = popup->sizeHint();
    } else {
        popupSize = menu->sizeHint();
    }
    const QPoint actionTopLeft = mapToGlobal(r.topLeft());
    const QPoint actionBottomLeft = mapToGlobal(QPoint(r.left(), r.bottom()));

    QScreen *screen = QApplication::screenAt(actionBottomLeft);
    if (!screen) {
        screen = QApplication::primaryScreen();
    }
    const QRect avail = screen ? screen->availableGeometry() : QRect();

    QPoint popupPos(actionTopLeft.x(), actionBottomLeft.y() + kFluentMenuPopupGap);
    bool opensAbove = false;
    if (avail.isValid()) {
        auto popupGeometry = [useFluentPopup, &popupSize](const QPoint &contentTopLeft) {
            return QRect(useFluentPopup ? PopupSurface::topLeftForContentTopLeft(contentTopLeft) : contentTopLeft,
                         popupSize);
        };
        const QMargins popupMargins = useFluentPopup ? PopupSurface::shadowMargins() : QMargins();
        const int popupContentHeight = useFluentPopup
            ? PopupSurface::contentSizeFromShadowSize(popupSize).height()
            : popupSize.height();
        const bool fitsBelow = popupGeometry(popupPos).bottom() <= avail.bottom() + 1;
        const int aboveY = actionTopLeft.y() - kFluentMenuPopupGap - popupContentHeight;
        const bool fitsAbove = popupGeometry(QPoint(popupPos.x(), aboveY)).top() >= avail.top();
        if (!fitsBelow && fitsAbove) {
            popupPos.setY(aboveY);
            opensAbove = true;
        }

        if (popupGeometry(popupPos).right() > avail.right()) {
            popupPos.setX(avail.right() - popupSize.width() + popupMargins.left());
        }
        if (popupGeometry(popupPos).left() < avail.left()) {
            popupPos.setX(avail.left() + popupMargins.left());
        }
        if (popupGeometry(popupPos).bottom() > avail.bottom()) {
            popupPos.setY(avail.bottom() - popupSize.height() + popupMargins.top());
        }
        if (popupGeometry(popupPos).top() < avail.top()) {
            popupPos.setY(avail.top() + popupMargins.top());
            opensAbove = false;
        }
    }

    if (useFluentPopup) {
        auto *popup = fluentPopup.data();
        if (!popup) {
            return;
        }
        m_openPopup = popup;
        popup->onClosed = [this]() {
            m_openPopup = nullptr;
            m_openMenu = nullptr;
            m_openAction = nullptr;
            updateHighlightForAction(m_hoverAction, true);
        };
        popup->popupAt(popupPos,
                       nullptr,
                       opensAbove ? FluentMotion::popupSlideOffset() : -FluentMotion::popupSlideOffset());
    } else {
        QObject::connect(menu, &QMenu::aboutToHide, this, &FluentMenuBar::onOpenMenuAboutToHide, Qt::UniqueConnection);
        menu->popup(popupPos);
    }
}

void FluentMenuBar::onOpenMenuAboutToHide()
{
    if (sender() == m_openMenu) {
        m_openMenu = nullptr;
        m_openAction = nullptr;
        updateHighlightForAction(m_hoverAction, true);
    }
}

qreal FluentMenuBar::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentMenuBar::setHoverLevel(qreal value)
{
    m_hoverLevel = qBound(0.0, value, 1.0);
    update();
}

void FluentMenuBar::mouseMoveEvent(QMouseEvent *event)
{
    QAction *action = event ? actionAtPosition(event->pos()) : nullptr;
    if (action != m_hoverAction) {
        m_hoverAction = action;
        updateHighlightForAction(action, true);
        startHoverAnimation(action ? 1.0 : 0.0);

        const bool hasOpenMenu = (m_openPopup && m_openPopup->isVisible()) || (m_openMenu && m_openMenu->isVisible());
        if (hasOpenMenu && action && action != m_openAction && menuForAction(action)) {
            openMenuForAction(action);
            event->accept();
            return;
        }
    }

    if ((m_openPopup && m_openPopup->isVisible()) || (m_openMenu && m_openMenu->isVisible())) {
        event->accept();
        return;
    }

    if (event) {
        event->accept();
    }
}

void FluentMenuBar::leaveEvent(QEvent *event)
{
    startHoverAnimation(0.0);
    m_hoverAction = nullptr;
    m_pressedAction = nullptr;
    updateHighlightForAction(nullptr, true);
    if (event) {
        event->accept();
    }
}

QRect FluentMenuBar::highlightTargetRect(QAction *action) const
{
    if (!action || action->isSeparator()) {
        return QRect();
    }
    QRect r = actionRect(action).adjusted(kMenuBarItemPaintInset,
                                          kMenuBarItemPaintInset,
                                          -kMenuBarItemPaintInset,
                                          -kMenuBarItemPaintInset);
    return r;
}

void FluentMenuBar::updateHighlightForAction(QAction *action, bool animate)
{
    const QRect target = highlightTargetRect(action);
    m_highlightAction = action;

    if (!animate || !m_highlightAnim) {
        m_highlightRect = target;
        update();
        return;
    }

    m_highlightAnim->stop();
    m_highlightAnim->setStartValue(m_highlightRect);
    m_highlightAnim->setEndValue(target);
    m_highlightAnim->start();
}

void FluentMenuBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const auto &colors = ThemeManager::instance().colors();
    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Subtle bottom divider so MenuBar doesn't disappear in dark titlebars.
    QColor divider = colors.border;
    divider.setAlpha(70);
    painter.setPen(QPen(divider, 1));
    painter.drawLine(QPointF(0.5, height() - 0.5), QPointF(width() - 0.5, height() - 0.5));

    // Fluent-like sliding highlight.
    if (!m_highlightRect.isNull()) {
        QColor fill = colors.hover;

        const bool active = (m_highlightAction != nullptr)
            && (m_highlightAction == m_openAction || m_highlightAction == m_hoverAction);
        const qreal level = qBound<qreal>(0.0, m_hoverLevel, 1.0);
        int alpha = active ? static_cast<int>(120 + 70 * level) : static_cast<int>(80 + 60 * level);
        fill.setAlpha(qBound(0, alpha, 200));

        painter.setPen(Qt::NoPen);
        painter.setBrush(fill);
        painter.drawRoundedRect(m_highlightRect, 6, 6);
    }

    // Custom item painting to avoid native-looking menu items.
    const QList<QAction *> acts = actions();
    for (QAction *a : acts) {
        if (!a || a->isSeparator()) {
            continue;
        }

        QRect r = actionRect(a).adjusted(kMenuBarItemPaintInset,
                                         kMenuBarItemPaintInset,
                                         -kMenuBarItemPaintInset,
                                         -kMenuBarItemPaintInset);
        if (!r.isValid()) {
            continue;
        }

        const bool enabled = a->isEnabled() && isEnabled();
        const bool active = (a == m_openAction || a == m_hoverAction);

        QFont f = font();
        f.setWeight(active ? QFont::DemiBold : QFont::Normal);
        painter.setFont(f);

        const QColor textColor = enabled ? colors.text : colors.disabledText;
        painter.setPen(textColor);

        QRect textRect = r.adjusted(kMenuBarItemHPadding, 0, -kMenuBarItemHPadding, 0);
        if (!a->icon().isNull()) {
            const QRect iconRect(textRect.left(), r.center().y() - 8, 16, 16);
            a->icon().paint(&painter,
                             iconRect,
                             Qt::AlignCenter,
                             enabled ? QIcon::Normal : QIcon::Disabled);
            textRect.setLeft(iconRect.right() + 1 + kMenuBarItemIconTextGap);
        }
        if (menuForAction(a)) {
            Style::drawChevronDown(painter,
                                   QPointF(textRect.right() - 7.0, r.center().y() + 0.5),
                                   enabled ? colors.subText : colors.disabledText,
                                   6.0,
                                   1.45);
            textRect.setRight(textRect.right() - kMenuBarItemArrowWidth);
        }
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextShowMnemonic, a->text());
    }
}

QSize FluentMenuBar::sizeHint() const
{
    rebuildActionLayout();
    return m_cachedSizeHint;
}

QSize FluentMenuBar::minimumSizeHint() const
{
    // Keep minimum consistent with our width hint so the title bar doesn't squeeze it.
    QSize s = sizeHint();
    return s;
}

void FluentMenuBar::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

} // namespace Fluent
