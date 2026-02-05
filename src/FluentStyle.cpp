#include "Fluent/FluentStyle.h"

#include <QtMath>

namespace Fluent {

namespace {
WindowMetrics g_windowMetrics;
}

ControlMetrics Style::metrics()
{
    ControlMetrics m;
    m.height = 32;
    // Keep consistent with Theme::*StyleSheet() defaults (e.g. 6px for inputs/buttons)
    // so custom-painted controls visually align with QSS-styled ones.
    m.radius = 6;
    m.paddingX = 10;
    m.paddingY = 6;
    m.iconAreaWidth = 28;
    return m;
}

WindowMetrics Style::windowMetrics()
{
    // Keep these conservative and platform-friendly; FluentMainWindow can override if needed.
    return g_windowMetrics;
}

void Style::setWindowMetrics(const WindowMetrics &metrics)
{
    WindowMetrics m = metrics;

    // Defensive clamps: demo may allow text input that bypasses slider ranges.
    m.accentBorderTraceDurationMs = qBound(1, m.accentBorderTraceDurationMs, 60000);
    m.accentBorderTraceEnableOvershoot = qBound<qreal>(0.0, m.accentBorderTraceEnableOvershoot, 0.25);
    m.accentBorderTraceEnableOvershootAt = qBound<qreal>(0.0, m.accentBorderTraceEnableOvershootAt, 0.99);

    g_windowMetrics = m;
}

QColor Style::mix(const QColor &a, const QColor &b, qreal t)
{
    t = qBound<qreal>(0.0, t, 1.0);
    return QColor::fromRgbF(
        a.redF() + (b.redF() - a.redF()) * t,
        a.greenF() + (b.greenF() - a.greenF()) * t,
        a.blueF() + (b.blueF() - a.blueF()) * t,
        a.alphaF() + (b.alphaF() - a.alphaF()) * t);
}

QColor Style::withAlpha(const QColor &c, int alpha)
{
    QColor out = c;
    out.setAlpha(qBound(0, alpha, 255));
    return out;
}

QPainterPath Style::roundedRectPath(const QRectF &rect, qreal radius)
{
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    return path;
}

qreal Style::roundedRectPerimeter(const QRectF &rect, qreal radius)
{
    const qreal w = qMax<qreal>(0.0, rect.width());
    const qreal h = qMax<qreal>(0.0, rect.height());
    if (w <= 0.0 || h <= 0.0) {
        return 0.0;
    }

    // Clamp radius to a valid range for the given rect.
    radius = qBound<qreal>(0.0, radius, qMin(w, h) / 2.0);
    if (radius <= 0.0) {
        return 2.0 * (w + h);
    }

    // Rounded-rect perimeter: straight edges + full circle from 4 quarter arcs.
    const qreal straightW = qMax<qreal>(0.0, w - 2.0 * radius);
    const qreal straightH = qMax<qreal>(0.0, h - 2.0 * radius);
    const qreal pi = qAcos(-1.0);
    return 2.0 * (straightW + straightH) + 2.0 * pi * radius;
}

void Style::paintMarqueeBorder(
    QPainter &p,
    const QRectF &rect,
    qreal radius,
    const QColor &color,
    qreal t,
    qreal width,
    qreal highlightFraction)
{
    if (!p.isActive()) {
        return;
    }

    t = qBound<qreal>(0.0, t, 1.0);
    width = qMax<qreal>(0.1, width);
    highlightFraction = qBound<qreal>(0.02, highlightFraction, 0.9);

    const qreal perimeter = roundedRectPerimeter(rect, radius);
    if (perimeter <= 1.0) {
        return;
    }

    // Single visible segment (dash) traveling around the border.
    qreal dash = qMax<qreal>(2.0 * width, perimeter * highlightFraction);
    qreal gap = qMax<qreal>(2.0 * width, perimeter - dash);

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    pen.setDashPattern({dash, gap});
    pen.setDashOffset(perimeter * t);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawPath(roundedRectPath(rect, radius));

    p.restore();
}

void Style::paintTraceBorder(
    QPainter &p,
    const QRectF &rect,
    qreal radius,
    const QColor &color,
    qreal progress,
    qreal width,
    qreal startOffset)
{
    if (!p.isActive()) {
        return;
    }

    const qreal rawProgress = progress;
    progress = qBound<qreal>(0.0, rawProgress, 1.0);
    width = qMax<qreal>(0.1, width);
    startOffset = qBound<qreal>(0.0, startOffset, 1.0);

    const qreal perimeter = roundedRectPerimeter(rect, radius);
    if (perimeter <= 1.0) {
        return;
    }

    // Avoid degenerate dash patterns: draw full border directly.
    // If the animation overshoots (rawProgress > 1), apply a tiny pulse to feel more Fluent.
    if (progress >= 0.999) {
        const qreal overshoot = qBound<qreal>(0.0, rawProgress - 1.0, 0.10);
        const qreal widthMul = 1.0 + 2.0 * overshoot;

        QColor c = color;
        if (overshoot > 0.0) {
            c.setAlphaF(qBound<qreal>(0.0, c.alphaF() * (1.0 + 1.5 * overshoot), 1.0));
        }

        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(c, width * widthMul, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        p.drawPath(roundedRectPath(rect, radius));
        p.restore();
        return;
    }

    if (progress <= 0.001) {
        return;
    }

    const qreal dash = qMax<qreal>(2.0 * width, perimeter * progress);
    const qreal gap = qMax<qreal>(2.0 * width, perimeter - dash);

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    QPen pen(color, width, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    pen.setDashPattern({dash, gap});
    // Rotate start position along the perimeter.
    pen.setDashOffset(perimeter * startOffset);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);
    p.drawPath(roundedRectPath(rect, radius));

    p.restore();
}

void Style::paintControlSurface(
    QPainter &p,
    const QRectF &rect,
    const ThemeColors &colors,
    qreal hoverLevel,
    qreal focusLevel,
    bool enabled,
    bool pressed)
{
    if (!p.isActive()) {
        return;
    }

    hoverLevel = qBound<qreal>(0.0, hoverLevel, 1.0);
    focusLevel = qBound<qreal>(0.0, focusLevel, 1.0);

    const auto m = metrics();

    QColor fill = colors.surface;
    if (!enabled) {
        fill = mix(colors.surface, colors.hover, 0.35);
    } else if (pressed) {
        fill = mix(colors.surface, colors.pressed, 0.65);
    } else if (hoverLevel > 0.0) {
        fill = mix(colors.surface, colors.hover, 0.45 * hoverLevel);
    }

    const QColor stroke = enabled ? colors.border : mix(colors.border, colors.disabledText, 0.35);

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);

    // Align strokes to device pixels to reduce jaggies on rounded corners (notably on HiDPI).
    qreal dpr = 1.0;
    if (p.device()) {
        dpr = p.device()->devicePixelRatioF();
        if (dpr <= 0.0) {
            dpr = 1.0;
        }
    }
    const qreal px = 0.5 / dpr;

    const QRectF r = rect.adjusted(px, px, -px, -px);
    p.setPen(QPen(stroke, 1.0));
    p.setBrush(fill);
    p.drawPath(roundedRectPath(r, m.radius));

    if (enabled && focusLevel > 0.0) {
        QColor focus = colors.focus;
        focus.setAlphaF(0.9 * focusLevel);
        p.setPen(QPen(focus, 2.0));
        p.setBrush(Qt::NoBrush);
        p.drawPath(roundedRectPath(r.adjusted(1.0, 1.0, -1.0, -1.0), m.radius - 1));
    }

    p.restore();
}

void Style::drawChevronDown(QPainter &p, const QPointF &center, const QColor &color, qreal size, qreal strokeWidth)
{
    if (!p.isActive()) {
        return;
    }

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    const qreal half = size / 2.0;
    QPainterPath path;
    path.moveTo(center.x() - half, center.y() - half / 2.0);
    path.lineTo(center.x(), center.y() + half / 2.0);
    path.lineTo(center.x() + half, center.y() - half / 2.0);
    p.drawPath(path);

    p.restore();
}

void Style::drawChevronUp(QPainter &p, const QPointF &center, const QColor &color, qreal size, qreal strokeWidth)
{
    if (!p.isActive()) {
        return;
    }

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    const qreal half = size / 2.0;
    QPainterPath path;
    path.moveTo(center.x() - half, center.y() + half / 2.0);
    path.lineTo(center.x(), center.y() - half / 2.0);
    path.lineTo(center.x() + half, center.y() + half / 2.0);
    p.drawPath(path);

    p.restore();
}

void Style::drawChevronLeft(QPainter &p, const QPointF &center, const QColor &color, qreal size, qreal strokeWidth)
{
    if (!p.isActive()) {
        return;
    }

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    const qreal half = size / 2.0;
    QPainterPath path;
    path.moveTo(center.x() + half / 2.0, center.y() - half);
    path.lineTo(center.x() - half / 2.0, center.y());
    path.lineTo(center.x() + half / 2.0, center.y() + half);
    p.drawPath(path);

    p.restore();
}

void Style::drawChevronRight(QPainter &p, const QPointF &center, const QColor &color, qreal size, qreal strokeWidth)
{
    if (!p.isActive()) {
        return;
    }

    p.save();
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(color, strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.setBrush(Qt::NoBrush);

    const qreal half = size / 2.0;
    QPainterPath path;
    path.moveTo(center.x() - half / 2.0, center.y() - half);
    path.lineTo(center.x() + half / 2.0, center.y());
    path.lineTo(center.x() - half / 2.0, center.y() + half);
    p.drawPath(path);

    p.restore();
}

void Style::drawChevronUpDown(QPainter &p, const QPointF &center, const QColor &color, qreal size, qreal strokeWidth, qreal gap)
{
    drawChevronUp(p, QPointF(center.x(), center.y() - gap / 2.0), color, size, strokeWidth);
    drawChevronDown(p, QPointF(center.x(), center.y() + gap / 2.0), color, size, strokeWidth);
}

void Style::paintElevationShadow(QPainter &p, const QRectF &panelRect, qreal radius, const ThemeColors &colors, qreal strength, int layers)
{
    if (!p.isActive()) {
        return;
    }

    strength = qBound<qreal>(0.0, strength, 2.0);
    layers = qMax(1, layers);

    const bool dark = colors.background.lightnessF() < 0.5;
    QColor shadowBase = dark ? QColor(0, 0, 0, static_cast<int>(140 * strength))
                             : QColor(0, 0, 0, static_cast<int>(90 * strength));

    for (int i = 0; i < layers; ++i) {
        const qreal t = static_cast<qreal>(i) / static_cast<qreal>(layers);
        QColor c = shadowBase;
        c.setAlphaF(shadowBase.alphaF() * (1.0 - t) * (0.35 * strength));
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(panelRect.adjusted(-2 - i, -2 - i, 2 + i, 2 + i), radius + i, radius + i);
    }
}

} // namespace Fluent
