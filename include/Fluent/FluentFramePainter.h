#pragma once

#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QPainter>
#include <QRect>

namespace Fluent {

// Common frameless dialog/window frame painter.
// Intended for translucent top-level widgets that draw a rounded surface + 1px border.
struct FluentFrameSpec {
    qreal radius = 10.0;
    bool maximized = false;

    // If true, clears the entire widget rect to transparent first.
    bool clearToTransparent = true;

    // If invalid, uses ThemeColors::surface.
    QColor surfaceOverride;

    // If invalid, uses (accentBorderEnabled ? colors.accent : colors.border).
    QColor borderColorOverride;

    bool accentBorderEnabled = true;

    // Optional accent trace animation (used by FluentDialog).
    bool traceEnabled = false;
    QColor traceColor;
    qreal traceT = 0.0; // normalized [0, 1]

    qreal borderWidth = 1.0;
    qreal borderInset = 0.5; // keep stroke inside the window edges
};

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

    const QColor surface = spec.surfaceOverride.isValid() ? spec.surfaceOverride : colors.surface;

    const QColor border = spec.borderColorOverride.isValid()
        ? spec.borderColorOverride
        : (spec.accentBorderEnabled ? colors.accent : colors.border);

    p.setPen(QPen(border, spec.borderWidth));
    p.setBrush(surface);
    p.drawRoundedRect(panelRect, radius, radius);

    if (spec.traceEnabled) {
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

    const QColor surface = spec.surfaceOverride.isValid() ? spec.surfaceOverride : colors.surface;

    const QColor border = spec.borderColorOverride.isValid()
        ? spec.borderColorOverride
        : (spec.accentBorderEnabled ? colors.accent : colors.border);

    p.setPen(QPen(border, spec.borderWidth));
    p.setBrush(surface);
    p.drawRoundedRect(panelRect, radius, radius);

    if (spec.traceEnabled) {
        Style::paintTraceBorder(p, panelRect, radius, spec.traceColor, spec.traceT, spec.borderWidth, 0.0);
    }
}

} // namespace Fluent
