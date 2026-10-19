#include "Fluent/FluentStyle.h"

#include <QPaintDevice>
#include <QPixmap>
#include <QPixmapCache>
#include <QtMath>

#include <vector>

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

QColor Style::controlHoverFill(const FluentThemeTokens &tokens)
{
    if (tokens.dark) {
        return tokens.neutral.fillSecondary;
    }
    return mix(tokens.neutral.card, tokens.neutral.cardHover, 0.55);
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

    const auto tokens = Theme::tokens(colors);

    QColor fill = tokens.neutral.card;
    if (!enabled) {
        fill = mix(tokens.neutral.card, tokens.neutral.background, tokens.dark ? 0.48 : 0.35);
    } else if (pressed) {
        fill = mix(tokens.neutral.card, tokens.neutral.fillTertiary, tokens.dark ? 0.42 : 0.34);
    } else if (hoverLevel > 0.0) {
        fill = mix(fill, controlHoverFill(tokens), hoverLevel);
    }

    const QColor stroke = enabled ? tokens.neutral.strokeSubtle
                                  : mix(tokens.neutral.strokeSubtle, colors.disabledText, tokens.dark ? 0.28 : 0.18);

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
        QColor focus = tokens.accent.base;
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

namespace {

// One box-blur pass over a single-channel buffer; samples outside are zero.
void boxBlurLine(const uchar *src, uchar *dst, int count, int stride, int radius)
{
    const int window = radius * 2 + 1;
    int sum = 0;
    for (int i = 0; i <= radius && i < count; ++i) {
        sum += src[i * stride];
    }
    for (int i = 0; i < count; ++i) {
        dst[i * stride] = static_cast<uchar>(sum / window);
        const int addIndex = i + radius + 1;
        const int removeIndex = i - radius;
        if (addIndex < count) {
            sum += src[addIndex * stride];
        }
        if (removeIndex >= 0) {
            sum -= src[removeIndex * stride];
        }
    }
}

// Renders a rounded rect of shapeSize (device px) padded by 3 * boxRadius on each
// side, blurs its alpha with three box passes (~Gaussian) and tints it.
QPixmap blurredRoundedRect(const QSize &shapeSize, qreal radiusPx, int boxRadius, const QColor &color, qreal dpr)
{
    const int pad = boxRadius * 3;
    const QString key = QStringLiteral("fluent-soft-shadow:%1x%2:r%3:b%4:c%5:d%6")
                            .arg(shapeSize.width())
                            .arg(shapeSize.height())
                            .arg(radiusPx, 0, 'f', 2)
                            .arg(boxRadius)
                            .arg(color.rgba(), 0, 16)
                            .arg(dpr, 0, 'f', 2);
    QPixmap cached;
    if (QPixmapCache::find(key, &cached)) {
        return cached;
    }

    const int w = shapeSize.width() + pad * 2;
    const int h = shapeSize.height() + pad * 2;
    QImage mask(w, h, QImage::Format_ARGB32_Premultiplied);
    mask.fill(Qt::transparent);
    {
        QPainter mp(&mask);
        mp.setRenderHint(QPainter::Antialiasing, true);
        mp.setPen(Qt::NoPen);
        mp.setBrush(Qt::white);
        mp.drawRoundedRect(QRectF(pad, pad, shapeSize.width(), shapeSize.height()), radiusPx, radiusPx);
    }

    std::vector<uchar> a(static_cast<size_t>(w) * h);
    std::vector<uchar> b(a.size());
    for (int y = 0; y < h; ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(mask.constScanLine(y));
        for (int x = 0; x < w; ++x) {
            a[static_cast<size_t>(y) * w + x] = static_cast<uchar>(qAlpha(line[x]));
        }
    }
    for (int pass = 0; pass < 3; ++pass) {
        for (int y = 0; y < h; ++y) {
            boxBlurLine(a.data() + static_cast<size_t>(y) * w, b.data() + static_cast<size_t>(y) * w, w, 1, boxRadius);
        }
        for (int x = 0; x < w; ++x) {
            boxBlurLine(b.data() + x, a.data() + x, h, w, boxRadius);
        }
    }

    QImage tinted(w, h, QImage::Format_ARGB32_Premultiplied);
    const int ca = color.alpha();
    for (int y = 0; y < h; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(tinted.scanLine(y));
        for (int x = 0; x < w; ++x) {
            const int alpha = a[static_cast<size_t>(y) * w + x] * ca / 255;
            line[x] = qRgba(color.red() * alpha / 255, color.green() * alpha / 255, color.blue() * alpha / 255, alpha);
        }
    }

    QPixmap pixmap = QPixmap::fromImage(tinted);
    pixmap.setDevicePixelRatio(dpr);
    QPixmapCache::insert(key, pixmap);
    return pixmap;
}

} // namespace

void Style::paintSoftShadow(QPainter &p, const QRectF &rect, qreal radius, const QColor &color, qreal blurRadius, const QPointF &offset)
{
    if (!p.isActive() || rect.isEmpty() || color.alpha() == 0) {
        return;
    }

    const qreal dpr = p.device() ? qMax<qreal>(1.0, p.device()->devicePixelRatioF()) : 1.0;
    const int boxRadius = qMax(1, qRound(qMax<qreal>(1.0, blurRadius) * dpr / 3.0));
    const qreal padLogical = boxRadius * 3 / dpr;
    radius = qBound<qreal>(0.0, radius, qMin(rect.width(), rect.height()) / 2.0);

    // Snap to device pixels so the nine-patch pieces butt together exactly.
    const auto snap = [dpr](qreal v) { return qRound(v * dpr) / dpr; };
    const QRectF raw = rect.translated(offset).adjusted(-padLogical, -padLogical, padLogical, padLogical);
    const QRectF target(QPointF(snap(raw.left()), snap(raw.top())), QPointF(snap(raw.right()), snap(raw.bottom())));

    // Nine-patch: corners come from a small blurred rect, the straight middle
    // stretches. The corner zone covers radius + blur on both sides of the edge.
    const int marginPx = qCeil(radius * dpr) + boxRadius * 6;
    const qreal marginLogical = marginPx / dpr;
    const bool ninePatch = target.width() >= 2.0 * marginLogical + 1.0 && target.height() >= 2.0 * marginLogical + 1.0;

    p.save();
    // No AA / smoothing: antialiased piece edges and bilinear sampling across
    // piece borders both leave visible seams. Corners draw 1:1, middles stretch
    // a single uniform pixel, so nothing is lost.
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setRenderHint(QPainter::SmoothPixmapTransform, !ninePatch);
    if (!ninePatch) {
        const QSize shapePx(qMax(1, qRound(rect.width() * dpr)), qMax(1, qRound(rect.height() * dpr)));
        p.drawPixmap(target, blurredRoundedRect(shapePx, radius * dpr, boxRadius, color, dpr),
                     QRectF(QPointF(0, 0), QSizeF(shapePx.width() + boxRadius * 6, shapePx.height() + boxRadius * 6)));
        p.restore();
        return;
    }

    // Shape just big enough for two corner zones plus a 1px stretchable middle.
    const int sidePx = (marginPx - boxRadius * 3) * 2 + 1;
    const QPixmap pm = blurredRoundedRect(QSize(sidePx, sidePx), radius * dpr, boxRadius, color, dpr);
    const qreal fullPx = pm.width();

    const qreal sx[4] = {0.0, static_cast<qreal>(marginPx), fullPx - marginPx, fullPx};
    const qreal sy[4] = {0.0, static_cast<qreal>(marginPx), static_cast<qreal>(pm.height()) - marginPx, static_cast<qreal>(pm.height())};
    const qreal tx[4] = {target.left(), target.left() + marginLogical, target.right() - marginLogical, target.right()};
    const qreal ty[4] = {target.top(), target.top() + marginLogical, target.bottom() - marginLogical, target.bottom()};
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            p.drawPixmap(QRectF(QPointF(tx[col], ty[row]), QPointF(tx[col + 1], ty[row + 1])),
                         pm,
                         QRectF(QPointF(sx[col], sy[row]), QPointF(sx[col + 1], sy[row + 1])));
        }
    }
    p.restore();
}

} // namespace Fluent
