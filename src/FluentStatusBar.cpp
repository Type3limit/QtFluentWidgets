#include "Fluent/FluentStatusBar.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>

namespace Fluent {

FluentStatusBar::FluentStatusBar(QWidget *parent)
    : QStatusBar(parent)
{
    setSizeGripEnabled(false);
    setAttribute(Qt::WA_StyledBackground, true);
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentStatusBar::applyTheme);
}

void FluentStatusBar::changeEvent(QEvent *event)
{
    QStatusBar::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentStatusBar::applyTheme()
{
    const QString next = Theme::statusBarStyle(ThemeManager::instance().colors());
    if (styleSheet() != next) {
        setStyleSheet(next);
    }
}

} // namespace Fluent
