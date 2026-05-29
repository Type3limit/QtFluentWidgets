#include "Fluent/FluentCommandBar.h"

#include "Fluent/FluentDropDownButton.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentIcon.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToolButton.h"

#include <QAction>
#include <QAbstractButton>
#include <QHBoxLayout>
#include <QMenu>
#include <QPainter>
#include <QResizeEvent>
#include <QScopedValueRollback>

namespace Fluent {

FluentCommandBar::FluentCommandBar(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, false);
    setAutoFillBackground(false);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    setMinimumHeight(42);

    m_layout = new QHBoxLayout(this);
    m_layout->setContentsMargins(6, 5, 6, 5);
    m_layout->setSpacing(6);
    ensureOverflowButton();

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, QOverload<>::of(&FluentCommandBar::update));
}

QAction *FluentCommandBar::addAction(const QString &text)
{
    auto *action = new QAction(text, this);
    m_ownedActions.append(action);
    addCommand(action);
    return action;
}

QAction *FluentCommandBar::addAction(const QIcon &icon, const QString &text)
{
    auto *action = new QAction(icon, text, this);
    m_ownedActions.append(action);
    addCommand(action);
    return action;
}

void FluentCommandBar::addCommand(QAction *action)
{
    if (!action) {
        return;
    }

    QWidget::addAction(action);
    QWidget *button = createButtonForAction(action);
    m_actionWidgets.insert(action, button);
    m_actionOrder.append(action);
    const int overflowIndex = m_overflowButton ? m_layout->indexOf(m_overflowButton) : -1;
    m_layout->insertWidget(overflowIndex >= 0 ? overflowIndex : m_layout->count(), button);
    syncActionWidget(action);
    connect(action, &QAction::changed, this, [this, action]() {
        syncActionWidget(action);
        updateOverflow();
    });
    updateOverflow();
}

void FluentCommandBar::addSeparator()
{
    auto *sep = new QWidget(this);
    sep->setFixedSize(1, 24);
    sep->setProperty("fluentCommandBarSeparator", true);
    const int overflowIndex = m_overflowButton ? m_layout->indexOf(m_overflowButton) : -1;
    m_layout->insertWidget(overflowIndex >= 0 ? overflowIndex : m_layout->count(), sep);
    updateOverflow();
}

void FluentCommandBar::addWidget(QWidget *widget)
{
    if (!widget) {
        return;
    }
    widget->setParent(this);
    const int overflowIndex = m_overflowButton ? m_layout->indexOf(m_overflowButton) : -1;
    m_layout->insertWidget(overflowIndex >= 0 ? overflowIndex : m_layout->count(), widget);
    updateOverflow();
}

void FluentCommandBar::clear()
{
    while (QLayoutItem *item = m_layout->takeAt(0)) {
        if (auto *widget = item->widget()) {
            if (widget == m_overflowButton) {
                widget->setVisible(false);
                widget->setParent(this);
            } else {
                widget->deleteLater();
            }
        }
        delete item;
    }
    m_actionWidgets.clear();
    m_actionOrder.clear();
    qDeleteAll(m_ownedActions);
    m_ownedActions.clear();
    ensureOverflowButton();
    updateOverflow();
}

QSize FluentCommandBar::iconSize() const
{
    return m_iconSize;
}

void FluentCommandBar::setIconSize(const QSize &size)
{
    if (m_iconSize == size || !size.isValid()) {
        return;
    }
    m_iconSize = size;
    for (auto it = m_actionWidgets.constBegin(); it != m_actionWidgets.constEnd(); ++it) {
        syncActionWidget(it.key());
    }
    updateOverflow();
}

void FluentCommandBar::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        for (auto it = m_actionWidgets.constBegin(); it != m_actionWidgets.constEnd(); ++it) {
            if (it.value()) {
                it.value()->setEnabled(isEnabled() && it.key()->isEnabled());
            }
        }
    }
}

void FluentCommandBar::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateOverflow();
}

void FluentCommandBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const auto &colors = ThemeManager::instance().colors();

    QPainter p(this);
    if (!p.isActive()) {
        return;
    }
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = QRectF(rect()).adjusted(0.5, 0.5, -0.5, -0.5);
    FluentSurfaceSpec surface;
    surface.level = FluentSurfaceLevel::Raised;
    surface.radius = ThemeManager::instance().tokens().radius.overlay;
    surface.enabled = isEnabled();
    surface.hovered = false;
    surface.elevation = FluentElevationLevel::None;
    paintFluentSurface(p, r, colors, surface);

    for (int i = 0; i < m_layout->count(); ++i) {
        QWidget *widget = m_layout->itemAt(i)->widget();
        if (!widget || !widget->property("fluentCommandBarSeparator").toBool()) {
            continue;
        }
        p.setPen(QPen(ThemeManager::instance().tokens().neutral.strokeSubtle, 1.0));
        const QRect wr = widget->geometry();
        p.drawLine(QPointF(wr.center().x(), wr.top()), QPointF(wr.center().x(), wr.bottom()));
    }
}

QWidget *FluentCommandBar::createButtonForAction(QAction *action)
{
    QWidget *button = nullptr;
    if (action->menu()) {
        auto *drop = new FluentDropDownButton(action->text(), this);
        drop->setMenu(action->menu());
        button = drop;
    } else {
        auto *tool = new FluentToolButton(action->text(), this);
        tool->setDefaultAction(action);
        tool->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        button = tool;
    }

    syncActionWidget(action);
    if (auto *tool = qobject_cast<QAbstractButton *>(button)) {
        tool->setProperty("fluentCommandBarButton", true);
        tool->setIcon(action->icon());
        tool->setIconSize(m_iconSize);
        tool->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
    if (button) {
        button->adjustSize();
    }
    return button;
}

void FluentCommandBar::syncActionWidget(QAction *action)
{
    if (!action || !m_actionWidgets.contains(action)) {
        return;
    }

    QWidget *widget = m_actionWidgets.value(action);
    if (!widget) {
        return;
    }

    widget->setVisible(action->isVisible());
    widget->setEnabled(isEnabled() && action->isEnabled());
    widget->setToolTip(action->toolTip());

    if (auto *button = qobject_cast<QAbstractButton *>(widget)) {
        button->setText(action->text());
        button->setIcon(action->icon());
        button->setIconSize(m_iconSize);
    }
    if (!m_updatingOverflow) {
        updateOverflow();
    }
}

void FluentCommandBar::ensureOverflowButton()
{
    if (!m_overflowMenu) {
        m_overflowMenu = new FluentMenu(this);
    }

    if (!m_overflowButton) {
        m_overflowButton = new FluentToolButton(this);
        m_overflowButton->setObjectName(QStringLiteral("FluentCommandBarOverflowButton"));
        m_overflowButton->setProperty("fluentCommandBarButton", true);
        m_overflowButton->setIcon(FluentIcon::icon(FluentIconType::More));
        m_overflowButton->setIconSize(m_iconSize);
        m_overflowButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_overflowButton->setFixedSize(34, 32);
        m_overflowButton->setToolTip(tr("More"));
        connect(m_overflowButton, &QToolButton::clicked, this, [this]() {
            if (!m_overflowMenu || m_overflowMenu->isEmpty()) {
                return;
            }
            const QPoint pos = m_overflowButton->mapToGlobal(QPoint(0, m_overflowButton->height() + 4));
            m_overflowMenu->popup(pos);
        });
    }

    if (m_layout && m_layout->indexOf(m_overflowButton) < 0) {
        m_layout->addWidget(m_overflowButton);
    }
    m_overflowButton->setVisible(false);
}

void FluentCommandBar::updateOverflow()
{
    if (!m_layout || !m_overflowButton || m_updatingOverflow) {
        return;
    }

    QScopedValueRollback<bool> guard(m_updatingOverflow, true);

    QList<QAction *> visibleActions;
    visibleActions.reserve(m_actionOrder.size());
    for (const auto &actionPtr : std::as_const(m_actionOrder)) {
        QAction *action = actionPtr.data();
        if (action && action->isVisible() && m_actionWidgets.value(action)) {
            visibleActions.append(action);
        }
    }

    QSet<QAction *> overflowed;
    while (!visibleActions.isEmpty()
           && preferredWidthFor(overflowed, !overflowed.isEmpty()) > width()) {
        QAction *last = visibleActions.takeLast();
        if (last) {
            overflowed.insert(last);
        }
    }

    if (!overflowed.isEmpty()) {
        while (!visibleActions.isEmpty()
               && preferredWidthFor(overflowed, true) > width()) {
            QAction *last = visibleActions.takeLast();
            if (last) {
                overflowed.insert(last);
            }
        }
    }

    m_overflowMenu->clear();
    for (const auto &actionPtr : std::as_const(m_actionOrder)) {
        QAction *action = actionPtr.data();
        QWidget *widget = m_actionWidgets.value(action);
        if (!action || !widget) {
            continue;
        }
        const bool inOverflow = overflowed.contains(action);
        widget->setVisible(action->isVisible() && !inOverflow);
        if (inOverflow) {
            m_overflowMenu->addAction(action);
        }
    }

    m_overflowButton->setVisible(!overflowed.isEmpty());

    for (int i = 0; i < m_layout->count(); ++i) {
        QWidget *widget = m_layout->itemAt(i)->widget();
        if (!widget || !widget->property("fluentCommandBarSeparator").toBool()) {
            continue;
        }
        widget->setVisible(hasVisibleCommandAround(i, -1) && hasVisibleCommandAround(i, 1));
    }

    updateGeometry();
    update();
}

int FluentCommandBar::preferredWidthFor(const QSet<QAction *> &overflowed, bool overflowVisible) const
{
    if (!m_layout) {
        return 0;
    }

    const QMargins margins = m_layout->contentsMargins();
    int total = margins.left() + margins.right();
    int visibleCount = 0;

    for (int i = 0; i < m_layout->count(); ++i) {
        QWidget *widget = m_layout->itemAt(i)->widget();
        if (!widget) {
            continue;
        }
        if (widget == m_overflowButton) {
            if (!overflowVisible) {
                continue;
            }
        } else if (QAction *action = actionForWidget(widget)) {
            if (!action->isVisible() || overflowed.contains(action)) {
                continue;
            }
        } else if (!widget->isVisible()) {
            continue;
        }

        total += widget->sizeHint().width();
        ++visibleCount;
    }

    if (visibleCount > 1) {
        total += (visibleCount - 1) * m_layout->spacing();
    }
    return total;
}

QAction *FluentCommandBar::actionForWidget(QWidget *widget) const
{
    if (!widget) {
        return nullptr;
    }
    for (auto it = m_actionWidgets.constBegin(); it != m_actionWidgets.constEnd(); ++it) {
        if (it.value() == widget) {
            return it.key();
        }
    }
    return nullptr;
}

bool FluentCommandBar::hasVisibleCommandAround(int index, int direction) const
{
    if (!m_layout || direction == 0) {
        return false;
    }

    for (int i = index + direction; i >= 0 && i < m_layout->count(); i += direction) {
        QWidget *widget = m_layout->itemAt(i)->widget();
        if (!widget || !widget->isVisible()) {
            continue;
        }
        if (widget->property("fluentCommandBarSeparator").toBool()) {
            continue;
        }
        return true;
    }
    return false;
}

} // namespace Fluent
