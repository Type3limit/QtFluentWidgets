#include "Fluent/FluentMenuBar.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentMenu.h"

#include <QActionEvent>
#include <QEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QPainter>
#include <QToolButton>
#include <QVariantAnimation>

namespace Fluent {

FluentMenuBar::FluentMenuBar(QWidget *parent)
    : QMenuBar(parent)
{
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
    setNativeMenuBar(false);

    // Keep menubar popup menus Fluent even if user calls QMenuBar::addMenu().
    ensureMenusFluent();

    connect(this, &QMenuBar::hovered, this, [this](QAction *a) {
        if (a == m_hoverAction) {
            return;
        }
        m_hoverAction = a;
        updateHighlightForAction(a, true);
        startHoverAnimation(a ? 1.0 : 0.0);
    });

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(120);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        update();
    });

    m_highlightAnim = new QVariantAnimation(this);
    m_highlightAnim->setDuration(160);
    m_highlightAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_highlightAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_highlightRect = value.toRect();
        update();
    });

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentMenuBar::applyTheme);
}

void FluentMenuBar::changeEvent(QEvent *event)
{
    QMenuBar::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentMenuBar::actionEvent(QActionEvent *event)
{
    QMenuBar::actionEvent(event);

    // When actions/menus are added by external code, ensure they're Fluent.
    if (event->type() == QEvent::ActionAdded || event->type() == QEvent::ActionChanged) {
        ensureMenusFluent();
    }
}

void FluentMenuBar::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();
    // Keep it visually consistent with FluentMenu: transparent background and custom hover painting.
    const QString next = QString(
        "QMenuBar { background: transparent; border: none; color: %1; }"
        "QMenuBar::item { padding: 6px 12px; background: transparent; border-radius: 6px; }"
        "QMenuBar::item:selected { background: transparent; }"
        // Qt overflow extension button (appears when menubar is too narrow)
        "QToolButton#qt_menubar_ext_button {"
        "  background: transparent;"
        "  border: none;"
        "  border-radius: 6px;"
        "  padding: 4px 8px;"
        "  color: %1;"
        "}"
        // Keep it visually quiet; the FluentMenuBar paints its own hover highlight.
        "QToolButton#qt_menubar_ext_button:hover { background: transparent; }"
        "QToolButton#qt_menubar_ext_button:pressed { background: transparent; }"
    ).arg(colors.text.name());
    if (styleSheet() != next) {
        setStyleSheet(next);
    }

    ensureMenusFluent();
}

FluentMenu *FluentMenuBar::addFluentMenu(const QString &title)
{
    auto *menu = new FluentMenu(title, this);
    addMenu(menu);
    return menu;
}

void FluentMenuBar::ensureMenusFluent()
{
    const QList<QAction *> acts = actions();
    for (QAction *a : acts) {
        if (!a) {
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

        a->setMenu(fluent);
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
}

void FluentMenuBar::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        QAction *a = actionAt(event->pos());
        if (a && !a->isSeparator()) {
            openMenuForAction(a);
            event->accept();
            return;
        }
    }

    QMenuBar::mousePressEvent(event);
}

void FluentMenuBar::mouseReleaseEvent(QMouseEvent *event)
{
    // Swallow release to prevent QMenuBar's built-in popup logic from kicking in.
    if (event->button() == Qt::LeftButton) {
        event->accept();
        return;
    }
    QMenuBar::mouseReleaseEvent(event);
}

void FluentMenuBar::openMenuForAction(QAction *action)
{
    if (!action) {
        return;
    }

    ensureMenusFluent();

    QMenu *menu = action->menu();
    if (!menu) {
        // Non-menu actions still should appear active briefly.
        setActiveAction(action);
        updateHighlightForAction(action, true);
        startHoverAnimation(1.0);
        return;
    }

    // Toggle if the same menu is already open.
    if (m_openMenu && m_openAction == action && m_openMenu->isVisible()) {
        m_openMenu->close();
        return;
    }

    if (m_openMenu && m_openMenu->isVisible()) {
        m_openMenu->close();
    }

    m_openMenu = menu;
    m_openAction = action;

    setActiveAction(action);
    updateHighlightForAction(action, true);
    startHoverAnimation(1.0);

    QObject::connect(menu, &QMenu::aboutToHide, this, &FluentMenuBar::onOpenMenuAboutToHide, Qt::UniqueConnection);

    const QRect r = actionGeometry(action);
    QPoint popupPos = mapToGlobal(QPoint(r.left(), r.bottom() + 1));
    menu->popup(popupPos);
}

void FluentMenuBar::onOpenMenuAboutToHide()
{
    if (sender() == m_openMenu) {
        m_openMenu = nullptr;
        m_openAction = nullptr;
        setActiveAction(nullptr);
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
    QAction *action = actionAt(event->pos());
    if (action != m_hoverAction) {
        m_hoverAction = action;
        updateHighlightForAction(action, true);
        startHoverAnimation(action ? 1.0 : 0.0);
    }
    QMenuBar::mouseMoveEvent(event);
}

void FluentMenuBar::leaveEvent(QEvent *event)
{
    startHoverAnimation(0.0);
    m_hoverAction = nullptr;
    updateHighlightForAction(nullptr, true);
    QMenuBar::leaveEvent(event);
}

QRect FluentMenuBar::highlightTargetRect(QAction *action) const
{
    if (!action || action->isSeparator()) {
        return QRect();
    }
    QRect r = actionGeometry(action).adjusted(2, 2, -2, -2);
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

        const bool active = (m_highlightAction != nullptr) && (m_highlightAction == activeAction());
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

        QRect r = actionGeometry(a).adjusted(2, 2, -2, -2);
        if (!r.isValid()) {
            continue;
        }

        const bool enabled = a->isEnabled() && isEnabled();
        const bool active = (a == activeAction());

        QFont f = font();
        f.setWeight(active ? QFont::DemiBold : QFont::Normal);
        painter.setFont(f);

        const QColor textColor = enabled ? colors.text : colors.disabledText;
        painter.setPen(textColor);

        const QRect textRect = r.adjusted(10, 0, -10, 0);
        painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextShowMnemonic, a->text());
    }
}

QSize FluentMenuBar::sizeHint() const
{
    // Provide a stable width hint so layouts reserve enough space and QMenuBar
    // won't collapse actions into the native overflow menu.
    const QFontMetrics fm(font());

    int w = 0;
    const QList<QAction *> acts = actions();
    for (QAction *a : acts) {
        if (!a || a->isSeparator()) {
            continue;
        }

        QString text = a->text();
        text.remove('&');
        const int textW = fm.horizontalAdvance(text);

        // Match QSS padding: 6px 12px; plus a small safety margin.
        const int itemW = textW + 24 + 8;
        w += itemW;
    }

    // Small outer margin.
    w += 8;

    QSize base = QMenuBar::sizeHint();
    base.setWidth(qMax(base.width(), w));
    return base;
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
