#include "Fluent/FluentIconButton.h"
#include "Fluent/FluentStyle.h"

namespace Fluent {

FluentIconButton::FluentIconButton(QWidget *parent)
    : FluentButton(parent)
{
    setText(QString());
    setIconSize(QSize(20, 20));
}

FluentIconButton::FluentIconButton(const QIcon &icon, QWidget *parent)
    : FluentButton(parent)
{
    setText(QString());
    setIcon(icon);
    setIconSize(QSize(20, 20));
}

int FluentIconButton::buttonExtent() const
{
    return m_buttonExtent;
}

void FluentIconButton::setButtonExtent(int extent)
{
    const int nextExtent = qMax(0, extent);
    if (m_buttonExtent == nextExtent) {
        return;
    }

    m_buttonExtent = nextExtent;
    updateGeometry();
}

QSize FluentIconButton::sizeHint() const
{
    const int extent = effectiveExtent();
    return QSize(extent, extent);
}

QSize FluentIconButton::minimumSizeHint() const
{
    return sizeHint();
}

int FluentIconButton::effectiveExtent() const
{
    if (m_buttonExtent > 0) {
        return m_buttonExtent;
    }

    const auto metrics = Style::metrics();
    const int iconExtent = qMax(iconSize().width(), iconSize().height());
    return qMax(metrics.height, iconExtent + metrics.paddingX * 2);
}

} // namespace Fluent