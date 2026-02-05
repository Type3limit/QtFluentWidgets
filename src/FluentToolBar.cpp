#include "Fluent/FluentToolBar.h"
#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToolButton.h"

#include <QEvent>
#include <QActionEvent>
#include <QWidgetAction>

namespace Fluent {

FluentToolBar::FluentToolBar(const QString &title, QWidget *parent)
    : QToolBar(title, parent)
{
    setMovable(false);
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentToolBar::applyTheme);
}

void FluentToolBar::changeEvent(QEvent *event)
{
    QToolBar::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentToolBar::actionEvent(QActionEvent *event)
{
    QToolBar::actionEvent(event);

    if (event->type() == QEvent::ActionAdded) {
        QAction *action = event->action();
        if (!action || action->isSeparator() || qobject_cast<QWidgetAction *>(action)) {
            return;
        }
        if (widgetForAction(action)) {
            return;
        }

        removeAction(action);

        auto *button = new FluentToolButton(this);
        button->setDefaultAction(action);

        auto *widgetAction = new QWidgetAction(this);
        widgetAction->setDefaultWidget(button);
        insertAction(event->before(), widgetAction);
    }
}

void FluentToolBar::applyTheme()
{
    const QString next = Theme::toolBarStyle(ThemeManager::instance().colors());
    if (styleSheet() != next) {
        setStyleSheet(next);
    }
}

} // namespace Fluent
