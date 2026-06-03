#pragma once

#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QPainter>
#include <QRectF>
#include <QtGlobal>

namespace Fluent::Detail {

inline QColor fluentItemHoverFill(const ThemeColors &colors, qreal level)
{
    const auto tokens = Theme::tokens(colors);
    QColor fill = tokens.neutral.cardHover;
    fill.setAlphaF(qBound<qreal>(0.0, (tokens.dark ? 0.82 : 0.90) * level, 1.0));
    return fill;
}

inline QColor fluentItemSelectionFill(const ThemeColors &colors, qreal opacity)
{
    const auto tokens = Theme::tokens(colors);
    QColor fill = Style::mix(tokens.neutral.card, tokens.accent.base, tokens.dark ? 0.32 : 0.18);
    fill.setAlphaF(qBound<qreal>(0.0, opacity, 1.0));
    return fill;
}

inline QColor fluentItemDisabledSelectionFill(const ThemeColors &colors, qreal opacity)
{
    const auto tokens = Theme::tokens(colors);
    const QColor disabledBase =
        Style::mix(tokens.neutral.card, tokens.neutral.fillSecondary, tokens.dark ? 0.30 : 0.46);
    QColor fill = Style::mix(disabledBase, colors.disabledText, tokens.dark ? 0.16 : 0.10);
    fill.setAlphaF(qBound<qreal>(0.0, opacity, 1.0));
    return fill;
}

inline void paintFluentItemSelectionIndicator(QPainter &painter,
                                              const QRectF &itemRect,
                                              const ThemeColors &colors,
                                              qreal opacity,
                                              qreal leftOffset = 0.0)
{
    const auto tokens = Theme::tokens(colors);
    QColor indicator = tokens.accent.base;
    indicator.setAlphaF(qBound<qreal>(0.0, opacity, 1.0));

    painter.setPen(Qt::NoPen);
    painter.setBrush(indicator);

    constexpr qreal kWidth = 3.0;
    constexpr qreal kHeight = 18.0;
    const QRectF indicatorRect(itemRect.left() + leftOffset,
                               itemRect.center().y() - kHeight / 2.0,
                               kWidth,
                               kHeight);
    painter.drawRoundedRect(indicatorRect, kWidth / 2.0, kWidth / 2.0);
}

} // namespace Fluent::Detail
