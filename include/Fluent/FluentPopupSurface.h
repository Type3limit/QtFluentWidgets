#pragma once

#include "Fluent/FluentBorderEffect.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QPainter>
#include <QPainterPath>
#include <QRectF>

namespace Fluent {
namespace PopupSurface {

/// Shared constants for all Fluent popup windows.
constexpr qreal kRadius          = 10.0;
constexpr qreal kBorderWidth     = 1.0;
constexpr int   kOpenDurationMs  = 140;
constexpr int   kOpenSlideOffsetPx = 6;

/// The rectangle used for drawing the panel border/background (inset by half a pixel).
inline QRectF panelRect(const QRect &rect)
{
    return QRectF(rect).adjusted(0.65, 0.65, -0.65, -0.65);
}

/// Returns a rounded-rect clip path matching the popup panel's border.
/// Use this to clip calendar / picker content so it stays inside the rounded corners
/// when WA_TranslucentBackground is active.
inline QPainterPath contentClipPath(const QRect &rect)
{
    QPainterPath path;
    path.addRoundedRect(QRectF(rect).adjusted(0.5, 0.5, -0.5, -0.5), kRadius, kRadius);
    return path;
}

/// Paints the Fluent surface panel (fill + themed border + optional accent trace).
/// The caller is responsible for clearing to transparent first when
/// WA_TranslucentBackground is set.
inline void paintPanel(QPainter &painter, const QRect &rect,
                       const ThemeColors &colors, FluentBorderEffect *border)
{
    const QRectF r = panelRect(rect);

    FluentFrameSpec frame;
    frame.radius            = kRadius;
    frame.maximized         = false;
    frame.clearToTransparent = false;
    frame.borderWidth       = kBorderWidth;
    if (border) {
        border->applyToFrameSpec(frame, colors);
    }

    paintFluentPanel(painter, r, colors, frame);
}

} // namespace PopupSurface
} // namespace Fluent

