#pragma once

#include "Fluent/FluentTheme.h"

#include <QColor>
#include <QEasingCurve>
#include <QPainter>
#include <QPainterPath>
#include <QRectF>

namespace Fluent {

struct ControlMetrics {
    int height = 32;
    // Fluent (Win11-like) default corner radius for input/button controls.
    int radius = 6;
    int paddingX = 10;
    int paddingY = 6;
    int iconAreaWidth = 28;
};

struct WindowMetrics {
    int titleBarHeight = 40;
    int titleBarPaddingX = 10;
    int titleBarPaddingY = 6;
    int windowButtonWidth = 34;
    int windowButtonHeight = 28;
    int windowButtonSpacing = 6;
    int resizeBorder = 8;

    // Accent border trace animation tuning (used by MainWindow/Dialog/Menu/MessageBox/Toast).
    int accentBorderTraceDurationMs = 500;
    qreal accentBorderTraceEnableOvershoot = 0.04;
    qreal accentBorderTraceEnableOvershootAt = 0.88;
    QEasingCurve::Type accentBorderTraceEnableEasing = QEasingCurve::OutCubic;
    QEasingCurve::Type accentBorderTraceDisableEasing = QEasingCurve::InCubic;
};

class Style final
{
public:
    static ControlMetrics metrics();
    static WindowMetrics windowMetrics();
    static void setWindowMetrics(const WindowMetrics &metrics);

    static QColor mix(const QColor &a, const QColor &b, qreal t);
    static QColor withAlpha(const QColor &c, int alpha);

    static QPainterPath roundedRectPath(const QRectF &rect, qreal radius);

    // Approximate perimeter length for a rounded-rect path. Used for dash animations.
    static qreal roundedRectPerimeter(const QRectF &rect, qreal radius);

    // Paint a single "marquee" segment traveling along the border of a rounded rect.
    // t is normalized progress in [0, 1]. highlightFraction controls the segment length.
    static void paintMarqueeBorder(
        QPainter &p,
        const QRectF &rect,
        qreal radius,
        const QColor &color,
        qreal t,
        qreal width = 1.0,
        qreal highlightFraction = 0.18);

    // Paint a progressive border trace from 0..1 of the perimeter.
    // When progress reaches 1, it becomes a full solid stroke.
    static void paintTraceBorder(
        QPainter &p,
        const QRectF &rect,
        qreal radius,
        const QColor &color,
        qreal progress,
        qreal width = 1.0,
        qreal startOffset = 0.0);

    static void paintControlSurface(
        QPainter &p,
        const QRectF &rect,
        const ThemeColors &colors,
        qreal hoverLevel,
        qreal focusLevel,
        bool enabled,
        bool pressed);

    static void drawChevronDown(QPainter &p, const QPointF &center, const QColor &color, qreal size = 8.0, qreal strokeWidth = 1.6);
    static void drawChevronUp(QPainter &p, const QPointF &center, const QColor &color, qreal size = 8.0, qreal strokeWidth = 1.6);
    static void drawChevronLeft(QPainter &p, const QPointF &center, const QColor &color, qreal size = 8.0, qreal strokeWidth = 1.6);
    static void drawChevronRight(QPainter &p, const QPointF &center, const QColor &color, qreal size = 8.0, qreal strokeWidth = 1.6);

    static void drawChevronUpDown(
        QPainter &p,
        const QPointF &center,
        const QColor &color,
        qreal size = 7.0,
        qreal strokeWidth = 1.6,
        qreal gap = 6.0);

    static void paintElevationShadow(
        QPainter &p,
        const QRectF &panelRect,
        qreal radius,
        const ThemeColors &colors,
        qreal strength = 1.0,
        int layers = 6);
};

} // namespace Fluent
