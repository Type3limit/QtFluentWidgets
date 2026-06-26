#pragma once

#include "Fluent/FluentExport.h"

#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QConicalGradient>
#include <QPainter>
#include <QPainterPath>
#include <QRect>

namespace Fluent {

enum class FluentSurfaceLevel {
    Background,
    Pane,
    Card,
    Raised,
    Popup,
    Modal
};

enum class FluentElevationLevel {
    None,
    Low,
    Medium,
    High
};

struct FLUENT_EXPORT FluentSurfaceSpec {
    FluentSurfaceLevel level = FluentSurfaceLevel::Card;
    FluentElevationLevel elevation = FluentElevationLevel::None;

    qreal radius = 8.0;
    qreal borderWidth = 1.0;
    qreal borderInset = 0.5;

    bool clearToTransparent = false;
    bool drawBorder = true;
    bool enabled = true;
    bool hovered = false;
    bool pressed = false;
    qreal hoverLevel = 1.0;
    qreal pressLevel = 1.0;

    QColor surfaceOverride;
    QColor borderColorOverride;
    QColor tintColor;
    qreal tintStrength = 0.0;
};

inline QColor fluentSurfaceFill(const ThemeColors &colors, const FluentSurfaceSpec &spec)
{
    const auto tokens = Theme::tokens(colors);
    QColor fill;
    switch (spec.level) {
    case FluentSurfaceLevel::Background:
        fill = tokens.neutral.background;
        break;
    case FluentSurfaceLevel::Pane:
        fill = tokens.neutral.layer;
        break;
    case FluentSurfaceLevel::Raised:
        fill = Style::mix(tokens.neutral.card, colors.text, tokens.dark ? 0.055 : 0.012);
        break;
    case FluentSurfaceLevel::Popup:
        fill = tokens.dark
            ? Style::mix(tokens.neutral.card, colors.text, 0.075)
            : tokens.neutral.card;
        break;
    case FluentSurfaceLevel::Modal:
        fill = tokens.dark
            ? Style::mix(tokens.neutral.card, colors.text, 0.095)
            : tokens.neutral.card;
        break;
    case FluentSurfaceLevel::Card:
    default:
        fill = tokens.neutral.card;
        break;
    }

    if (spec.surfaceOverride.isValid()) {
        fill = spec.surfaceOverride;
    }
    if (spec.tintColor.isValid() && spec.tintStrength > 0.0) {
        fill = Style::mix(fill, spec.tintColor, qBound<qreal>(0.0, spec.tintStrength, 1.0));
    }

    if (!spec.enabled) {
        fill = Style::mix(fill, tokens.neutral.background, tokens.dark ? 0.48 : 0.35);
    } else if (spec.pressed) {
        const qreal amount = qBound<qreal>(0.0, spec.pressLevel, 1.0);
        fill = Style::mix(fill, tokens.neutral.fillTertiary, (tokens.dark ? 0.42 : 0.34) * amount);
    } else if (spec.hovered) {
        const qreal amount = qBound<qreal>(0.0, spec.hoverLevel, 1.0);
        fill = Style::mix(fill, tokens.neutral.cardHover, (tokens.dark ? 0.70 : 0.55) * amount);
    }
    return fill;
}

inline QColor fluentSurfaceBorder(const ThemeColors &colors, const FluentSurfaceSpec &spec)
{
    if (spec.borderColorOverride.isValid()) {
        return spec.borderColorOverride;
    }

    const auto tokens = Theme::tokens(colors);
    QColor border;
    switch (spec.level) {
    case FluentSurfaceLevel::Popup:
    case FluentSurfaceLevel::Modal:
        border = tokens.neutral.stroke;
        break;
    case FluentSurfaceLevel::Raised:
        border = tokens.neutral.stroke;
        break;
    case FluentSurfaceLevel::Card:
        border = tokens.neutral.strokeSubtle;
        break;
    case FluentSurfaceLevel::Pane:
    case FluentSurfaceLevel::Background:
    default:
        border = tokens.neutral.strokeSubtle;
        break;
    }

    if (!spec.enabled) {
        border = Style::mix(border, colors.disabledText, tokens.dark ? 0.28 : 0.18);
    } else if (spec.hovered) {
        const qreal amount = qBound<qreal>(0.0, spec.hoverLevel, 1.0);
        border = Style::mix(border, tokens.neutral.strokeStrong, (tokens.dark ? 0.38 : 0.25) * amount);
    }
    return border;
}

inline void paintFluentSurfaceShadow(QPainter &p, const QRectF &rect, const ThemeColors &colors, const FluentSurfaceSpec &spec)
{
    if (spec.elevation == FluentElevationLevel::None) {
        return;
    }

    const auto tokens = Theme::tokens(colors);
    int steps = 0;
    qreal alpha = 0.0;
    qreal offset = 0.0;
    switch (spec.elevation) {
    case FluentElevationLevel::High:
        steps = 5;
        alpha = tokens.dark ? 0.30 : 0.14;
        offset = 5.0;
        break;
    case FluentElevationLevel::Medium:
        steps = 4;
        alpha = tokens.dark ? 0.24 : 0.10;
        offset = 3.0;
        break;
    case FluentElevationLevel::Low:
        steps = 2;
        alpha = tokens.dark ? 0.18 : 0.07;
        offset = 1.5;
        break;
    case FluentElevationLevel::None:
    default:
        return;
    }

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    for (int i = steps; i >= 1; --i) {
        QColor shadow = tokens.elevation.shadow;
        shadow.setAlphaF(qBound<qreal>(0.0, alpha * (static_cast<qreal>(i) / steps) * 0.42, 1.0));
        const qreal spread = static_cast<qreal>(i);
        const QRectF shadowRect = rect.translated(0.0, offset).adjusted(-spread, -spread, spread, spread);
        p.setBrush(shadow);
        p.drawRoundedRect(shadowRect, spec.radius + spread, spec.radius + spread);
    }
    p.restore();
}

inline void paintFluentSurface(QPainter &p, const QRectF &rect, const ThemeColors &colors, const FluentSurfaceSpec &spec)
{
    if (!p.isActive()) {
        return;
    }

    p.setRenderHint(QPainter::Antialiasing, true);

    if (spec.clearToTransparent) {
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(rect.toAlignedRect(), Qt::transparent);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    QRectF panelRect = rect.adjusted(spec.borderInset, spec.borderInset, -spec.borderInset, -spec.borderInset);
    paintFluentSurfaceShadow(p, panelRect, colors, spec);

    p.setBrush(fluentSurfaceFill(colors, spec));
    p.setPen(spec.drawBorder ? QPen(fluentSurfaceBorder(colors, spec), spec.borderWidth) : Qt::NoPen);
    p.drawRoundedRect(panelRect, spec.radius, spec.radius);
}

// Common frameless dialog/window frame painter.
// Intended for translucent top-level widgets that draw a rounded surface + 1px border.
struct FLUENT_EXPORT FluentFrameSpec {
    qreal radius = 10.0;
    bool maximized = false;

    // If true, clears the entire widget rect to transparent first.
    bool clearToTransparent = true;

    // If invalid, uses the modal Fluent surface token.
    QColor surfaceOverride;

    // If invalid, uses (accentBorderEnabled ? tokens.accent.base : tokens.neutral.strokeSubtle).
    QColor borderColorOverride;

    bool accentBorderEnabled = true;

    // Optional accent trace animation (used by FluentDialog).
    bool traceEnabled = false;
    QColor traceColor;
    qreal traceT = 0.0; // normalized [0, 1]

    qreal borderWidth = 1.0;
    qreal borderInset = 0.5; // keep stroke inside the window edges
};

inline QColor fluentFrameSurface(const ThemeColors &colors, const FluentFrameSpec &spec)
{
    FluentSurfaceSpec surface;
    surface.level = FluentSurfaceLevel::Modal;
    surface.elevation = FluentElevationLevel::None;
    surface.surfaceOverride = spec.surfaceOverride;
    return fluentSurfaceFill(colors, surface);
}

inline QColor fluentFrameBorder(const ThemeColors &colors, const FluentFrameSpec &spec)
{
    if (spec.borderColorOverride.isValid()) {
        return spec.borderColorOverride;
    }

    const auto tokens = Theme::tokens(colors);
    return spec.accentBorderEnabled ? tokens.accent.base : tokens.neutral.strokeSubtle;
}

// True when this frame should render the rotating "flow" accent border instead
// of a solid stroke (only when the accent border is on and the style is Flow).
inline bool fluentFlowBorderActive(const FluentFrameSpec &spec)
{
    return spec.accentBorderEnabled
        && !spec.maximized
        && ThemeManager::instance().accentBorderStyle() == ThemeManager::AccentBorderStyle::Flow;
}

// Strokes a rounded-rect outline with the shared flow conic gradient at the
// current ThemeManager::flowAngle(). Caller fills the surface first (NoPen).
inline void paintFluentFlowStroke(QPainter &p, const QRectF &panelRect, qreal radius, qreal borderWidth)
{
    const QList<QColor> stops = ThemeManager::instance().resolvedFlowColors();
    QConicalGradient grad(panelRect.center(), ThemeManager::instance().flowAngle());
    const int n = stops.size();
    for (int i = 0; i < n; ++i) {
        grad.setColorAt(qBound(0.0, qreal(i) / n, 1.0), stops.at(i));
    }
    grad.setColorAt(1.0, stops.first()); // close the loop seamlessly
    QPen pen(QBrush(grad), qMax<qreal>(1.5, borderWidth + 0.5));
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(panelRect, radius, radius);
}

inline void paintFluentFrame(QPainter &p, const QRect &widgetRect, const ThemeColors &colors, const FluentFrameSpec &spec)
{
    if (!p.isActive()) {
        return;
    }

    p.setRenderHint(QPainter::Antialiasing, true);

    if (spec.clearToTransparent) {
        // Clear to transparent first; avoids corner artifacts on translucent windows.
        p.setCompositionMode(QPainter::CompositionMode_Source);
        p.fillRect(widgetRect, Qt::transparent);
        p.setCompositionMode(QPainter::CompositionMode_SourceOver);
    }

    const qreal radius = spec.maximized ? 0.0 : spec.radius;

    QRectF panelRect(widgetRect);
    panelRect.adjust(spec.borderInset, spec.borderInset, -spec.borderInset, -spec.borderInset);

    const QColor surface = fluentFrameSurface(colors, spec);
    const QColor border = fluentFrameBorder(colors, spec);
    const bool flow = fluentFlowBorderActive(spec);

    p.setPen(flow ? QPen(Qt::NoPen) : QPen(border, spec.borderWidth));
    p.setBrush(surface);
    p.drawRoundedRect(panelRect, radius, radius);

    if (flow) {
        paintFluentFlowStroke(p, panelRect, radius, spec.borderWidth);
    } else if (spec.traceEnabled) {
        Style::paintTraceBorder(p, panelRect, radius, spec.traceColor, spec.traceT, spec.borderWidth, 0.0);
    }
}

// Paint a fluent rounded panel in an arbitrary rectangle.
// Useful when the frame lives inside a larger translucent widget (e.g. MessageBox panel).
// Note: clearing uses the painter's current clip; callers should clear separately if they
// need to guarantee clearing the entire window regardless of clip.
inline void paintFluentPanel(QPainter &p, const QRectF &panelRect, const ThemeColors &colors, const FluentFrameSpec &spec)
{
    if (!p.isActive()) {
        return;
    }

    p.setRenderHint(QPainter::Antialiasing, true);

    const qreal radius = spec.maximized ? 0.0 : spec.radius;

    const QColor surface = fluentFrameSurface(colors, spec);
    const QColor border = fluentFrameBorder(colors, spec);
    const bool flow = fluentFlowBorderActive(spec);

    p.setPen(flow ? QPen(Qt::NoPen) : QPen(border, spec.borderWidth));
    p.setBrush(surface);
    p.drawRoundedRect(panelRect, radius, radius);

    if (flow) {
        paintFluentFlowStroke(p, panelRect, radius, spec.borderWidth);
    } else if (spec.traceEnabled) {
        Style::paintTraceBorder(p, panelRect, radius, spec.traceColor, spec.traceT, spec.borderWidth, 0.0);
    }
}

} // namespace Fluent
