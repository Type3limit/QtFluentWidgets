#pragma once

#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QPainter>
#include <QPainterPath>
#include <QRectF>
#include <QtGlobal>

namespace Fluent {
namespace ButtonVisuals {

struct StateColors
{
    QColor base;
    QColor hover;
    QColor pressed;
    QColor border;
    QColor bottomBorder;
    QColor text;
};

inline QColor transparentVersion(QColor color)
{
    color.setAlpha(0);
    return color;
}

inline QColor primaryPressedFill(const FluentThemeTokens &tokens)
{
    return tokens.dark ? tokens.accent.light3 : tokens.accent.dark1;
}

inline QColor neutralHoverFill(const FluentThemeTokens &tokens)
{
    return Style::controlHoverFill(tokens);
}

inline QColor neutralPressedFill(const FluentThemeTokens &tokens)
{
    return Style::mix(tokens.neutral.card, tokens.neutral.fillTertiary, tokens.dark ? 0.42 : 0.34);
}

inline QColor neutralDisabledFill(const FluentThemeTokens &tokens)
{
    return Style::mix(tokens.neutral.card, tokens.neutral.background, tokens.dark ? 0.48 : 0.35);
}

inline QColor neutralDisabledStroke(const ThemeColors &colors, const FluentThemeTokens &tokens)
{
    return Style::mix(tokens.neutral.strokeSubtle, colors.disabledText, tokens.dark ? 0.28 : 0.18);
}

inline StateColors resolve(const ThemeColors &colors,
                           const FluentThemeTokens &tokens,
                           bool primary,
                           bool checked,
                           bool enabled,
                           bool subtleCommandButton = false,
                           qreal checkedTintAmount = 0.12)
{
    StateColors resolved;

    if (primary) {
        resolved.base = checked ? primaryPressedFill(tokens) : tokens.accent.base;
        resolved.hover = tokens.accent.light1;
        resolved.pressed = primaryPressedFill(tokens);
        resolved.border = Style::mix(tokens.accent.base, tokens.onAccent, 0.18);
        resolved.bottomBorder = resolved.border;
        resolved.text = tokens.onAccent;
    } else {
        const QColor accentTint = Style::mix(tokens.neutral.card, tokens.accent.base, checkedTintAmount);
        QColor commandHover = neutralHoverFill(tokens);
        commandHover.setAlphaF(tokens.dark ? 0.95 : 0.90);
        QColor commandPressed = neutralPressedFill(tokens);
        commandPressed.setAlphaF(tokens.dark ? 0.98 : 0.94);

        resolved.base = checked ? accentTint
                                : (subtleCommandButton ? transparentVersion(commandHover) : tokens.neutral.card);
        resolved.hover = checked ? Style::mix(accentTint, tokens.accent.base, 0.10)
                                 : (subtleCommandButton ? commandHover : neutralHoverFill(tokens));
        resolved.pressed = checked ? Style::mix(accentTint, tokens.accent.base, 0.18)
                                   : (subtleCommandButton ? commandPressed : neutralPressedFill(tokens));
        resolved.border = checked ? Style::mix(tokens.neutral.strokeStrong, tokens.accent.base, tokens.dark ? 0.60 : 0.70)
                                  : (subtleCommandButton ? QColor(0, 0, 0, 0) : tokens.neutral.strokeSubtle);
        resolved.bottomBorder = checked ? resolved.border
                                        : (subtleCommandButton ? QColor(0, 0, 0, 0) : tokens.neutral.stroke);
        resolved.text = checked ? Theme::contrastColor(accentTint) : colors.text;
    }

    if (!enabled) {
        resolved.base = subtleCommandButton ? QColor(0, 0, 0, 0) : neutralDisabledFill(tokens);
        resolved.hover = resolved.base;
        resolved.pressed = resolved.base;
        resolved.border = subtleCommandButton ? QColor(0, 0, 0, 0) : neutralDisabledStroke(colors, tokens);
        resolved.bottomBorder = resolved.border;
        resolved.text = colors.disabledText;
    }

    return resolved;
}

inline QColor fillForState(const StateColors &colors, qreal hoverLevel, qreal pressLevel)
{
    QColor fill = Style::mix(colors.base, colors.hover, qBound<qreal>(0.0, hoverLevel, 1.0));
    return Style::mix(fill, colors.pressed, qBound<qreal>(0.0, pressLevel, 1.0));
}

inline QColor textForState(QColor color, qreal pressLevel, bool enabled)
{
    if (enabled && pressLevel > 0.0) {
        const qreal alpha = color.alphaF() * (1.0 - qBound<qreal>(0.0, pressLevel, 1.0) * 0.14);
        color.setAlphaF(qBound<qreal>(0.0, alpha, 1.0));
    }
    return color;
}

inline void paintBottomStroke(QPainter &painter, const QRectF &rect, qreal radius, const QColor &color)
{
    if (!painter.isActive() || color.alpha() <= 0 || rect.isEmpty()) {
        return;
    }

    const qreal controlRadius = qBound<qreal>(0.0, radius, qMin(rect.width(), rect.height()) / 2.0);
    const qreal bandHeight = qBound<qreal>(4.0, controlRadius * 1.55, qMax<qreal>(4.0, rect.height()));
    const qreal visibleHeight = 1.0;
    const QRectF band(rect.left(), rect.bottom() - bandHeight, rect.width(), bandHeight);

    QPainterPath bottomBand;
    bottomBand.addRoundedRect(band, qMin(controlRadius, bandHeight / 2.0), qMin(controlRadius, bandHeight / 2.0));

    QPainterPath hiddenBand;
    hiddenBand.addRect(QRectF(band.left(), band.top(), band.width(), qMax<qreal>(0.0, bandHeight - visibleHeight)));
    bottomBand = bottomBand.subtracted(hiddenBand);

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(Qt::NoPen);
    painter.setBrush(color);
    painter.drawPath(bottomBand);
    painter.restore();
}

inline void paintRoundedControl(QPainter &painter,
                                const QRectF &rect,
                                qreal radius,
                                const QColor &fill,
                                const QColor &border,
                                const QColor &bottomBorder)
{
    painter.setPen(border.alpha() > 0 ? QPen(border, 1.0) : Qt::NoPen);
    painter.setBrush(fill);
    if (fill.alpha() > 0 || border.alpha() > 0) {
        painter.drawRoundedRect(rect, radius, radius);
    }
    paintBottomStroke(painter, rect, radius, bottomBorder);
}

} // namespace ButtonVisuals
} // namespace Fluent
