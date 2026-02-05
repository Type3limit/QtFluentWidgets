#include "Fluent/FluentWidget.h"

#include "Fluent/FluentTheme.h"

#include <QEvent>
#include <QPainter>

namespace Fluent {

FluentWidget::FluentWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_StyledBackground, true);
    setAutoFillBackground(false);

    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentWidget::applyTheme);
}

FluentWidget::BackgroundRole FluentWidget::backgroundRole() const
{
    return m_backgroundRole;
}

void FluentWidget::setBackgroundRole(BackgroundRole role)
{
    if (m_backgroundRole == role) {
        return;
    }
    m_backgroundRole = role;
    update();
}

void FluentWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentWidget::applyTheme()
{
    update();
}

void FluentWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if (m_backgroundRole == BackgroundRole::Transparent) {
        return;
    }

    const auto &colors = ThemeManager::instance().colors();
    const QColor bg = (m_backgroundRole == BackgroundRole::Surface) ? colors.surface : colors.background;

    QPainter p(this);
    if (!p.isActive()) {
        return;
    }
    p.setPen(Qt::NoPen);
    p.setBrush(bg);
    p.drawRect(rect());
}

} // namespace Fluent
