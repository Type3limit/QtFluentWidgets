#include "Fluent/FluentGroupBox.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>

namespace Fluent {

FluentGroupBox::FluentGroupBox(QWidget *parent)
    : QGroupBox(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setContentsMargins(12, 20, 12, 12);
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentGroupBox::applyTheme);
}

FluentGroupBox::FluentGroupBox(const QString &title, QWidget *parent)
    : QGroupBox(title, parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setContentsMargins(12, 20, 12, 12);
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentGroupBox::applyTheme);
}

void FluentGroupBox::changeEvent(QEvent *event)
{
    QGroupBox::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentGroupBox::applyTheme()
{
    const QString next = Theme::groupBoxStyle(ThemeManager::instance().colors());
    if (styleSheet() != next) {
        setStyleSheet(next);
    }
}

} // namespace Fluent
