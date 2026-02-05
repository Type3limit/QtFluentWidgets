#include "Fluent/FluentLabel.h"
#include "Fluent/FluentTheme.h"

#include <QEvent>

namespace Fluent {

FluentLabel::FluentLabel(QWidget *parent)
    : QLabel(parent)
{
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentLabel::applyTheme);
}

FluentLabel::FluentLabel(const QString &text, QWidget *parent)
    : QLabel(text, parent)
{
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentLabel::applyTheme);
}

void FluentLabel::changeEvent(QEvent *event)
{
    QLabel::changeEvent(event);

    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentLabel::applyTheme()
{
    const auto &colors = ThemeManager::instance().colors();

    // Don't clobber user styles (e.g. font-weight) on theme changes.
    // We append a tiny color rule with a marker so we can update it later.
    static const QString kMarker = QStringLiteral("/*FluentLabelTheme*/");
    QString user = styleSheet();
    const int idx = user.indexOf(kMarker);
    if (idx >= 0) {
        user = user.left(idx);
    }

    const QColor text = isEnabled() ? colors.text : colors.disabledText;
    const QString themed = user + kMarker + QStringLiteral(" color: %1;").arg(text.name());
    if (QLabel::styleSheet() != themed) {
        QLabel::setStyleSheet(themed);
    }
}

} // namespace Fluent
