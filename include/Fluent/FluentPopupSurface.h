#pragma once

#include "Fluent/FluentBorderEffect.h"
#include "Fluent/FluentFramePainter.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QPainter>
#include <QPainterPath>
#include <QMargins>
#include <QPoint>
#include <QRectF>
#include <QSize>
#include <QtGlobal>

namespace Fluent {
namespace PopupSurface {

/// Shared constants for all Fluent popup windows.
constexpr qreal kRadius          = 10.0;
constexpr qreal kBorderWidth     = 1.0;
constexpr int   kShadowMarginLeftPx = 12;
constexpr int   kShadowMarginTopPx = 8;
constexpr int   kShadowMarginRightPx = 12;
constexpr int   kShadowMarginBottomPx = 20;
constexpr int   kShadowMarginPx  = kShadowMarginLeftPx;

inline QMargins shadowMargins()
{
    return QMargins(kShadowMarginLeftPx,
                    kShadowMarginTopPx,
                    kShadowMarginRightPx,
                    kShadowMarginBottomPx);
}

inline QSize withShadowMargins(const QSize &contentSize)
{
    const QMargins margins = shadowMargins();
    return contentSize + QSize(margins.left() + margins.right(), margins.top() + margins.bottom());
}

inline QSize contentSizeFromShadowSize(const QSize &shadowSize)
{
    const QMargins margins = shadowMargins();
    return QSize(qMax(0, shadowSize.width() - margins.left() - margins.right()),
                 qMax(0, shadowSize.height() - margins.top() - margins.bottom()));
}

inline QPoint topLeftForContentTopLeft(const QPoint &contentTopLeft)
{
    const QMargins margins = shadowMargins();
    return contentTopLeft - QPoint(margins.left(), margins.top());
}

inline QRect shadowContentRect(const QRect &rect)
{
    const QRect content = rect.marginsRemoved(shadowMargins());
    return content.isValid() ? content : rect;
}

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

inline QPainterPath shadowContentClipPath(const QRect &rect)
{
    return contentClipPath(shadowContentRect(rect));
}

inline void paintSoftPopupShadow(QPainter &painter, const QRectF &rect, const ThemeColors &colors)
{
    const auto tokens = Theme::tokens(colors);

    // Key shadow: blur + offset sized to fit the shadow margins
    // (12 sides, 12 - 6 = 6 top, 12 + 6 = 18 bottom).
    QColor key = tokens.elevation.shadow;
    key.setAlphaF(tokens.dark ? 0.34 : 0.14);
    Style::paintSoftShadow(painter, rect, kRadius, key, 12.0, QPointF(0.0, 6.0));

    // Tight contact shadow keeps the panel edge crisp.
    QColor contact = tokens.elevation.shadow;
    contact.setAlphaF(tokens.dark ? 0.22 : 0.06);
    Style::paintSoftShadow(painter, rect, kRadius, contact, 3.0, QPointF(0.0, 1.0));
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

    FluentSurfaceSpec surface;
    surface.level = FluentSurfaceLevel::Popup;
    surface.elevation = FluentElevationLevel::Medium;
    surface.radius = kRadius;
    surface.borderWidth = kBorderWidth;
    surface.borderInset = 0.0;
    surface.surfaceOverride = frame.surfaceOverride;
    surface.borderColorOverride = frame.borderColorOverride;

    paintFluentSurface(painter, r, colors, surface);

    if (fluentFlowBorderActive(frame)) {
        // Overdraw the rotating flow accent stroke (shared with windows/dialogs).
        paintFluentFlowStroke(painter, r, kRadius, kBorderWidth);
    } else if (frame.traceEnabled) {
        Style::paintTraceBorder(painter, r, kRadius, frame.traceColor, frame.traceT, kBorderWidth, 0.0);
    }
}

/// Paints a popup panel inside an inset content rect, leaving transparent room
/// for the rounded shadow so the top-level popup window does not clip it into
/// a rectangular edge.
inline void paintPanelWithShadowMargins(QPainter &painter, const QRect &rect,
                                        const ThemeColors &colors, FluentBorderEffect *border = nullptr)
{
    const QRectF r = panelRect(shadowContentRect(rect));

    FluentFrameSpec frame;
    frame.radius = kRadius;
    frame.maximized = false;
    frame.clearToTransparent = false;
    frame.borderWidth = kBorderWidth;
    if (border) {
        border->applyToFrameSpec(frame, colors);
    }

    FluentSurfaceSpec surface;
    surface.level = FluentSurfaceLevel::Popup;
    surface.elevation = FluentElevationLevel::None;
    surface.radius = kRadius;
    surface.borderWidth = kBorderWidth;
    surface.borderInset = 0.0;
    surface.surfaceOverride = frame.surfaceOverride;
    surface.borderColorOverride = frame.borderColorOverride;

    paintSoftPopupShadow(painter, r, colors);
    paintFluentSurface(painter, r, colors, surface);

    if (fluentFlowBorderActive(frame)) {
        // Overdraw the rotating flow accent stroke (shared with windows/dialogs).
        paintFluentFlowStroke(painter, r, kRadius, kBorderWidth);
    } else if (frame.traceEnabled) {
        Style::paintTraceBorder(painter, r, kRadius, frame.traceColor, frame.traceT, kBorderWidth, 0.0);
    }
}

} // namespace PopupSurface
} // namespace Fluent

