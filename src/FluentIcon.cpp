#include "Fluent/FluentIcon.h"

#include "Fluent/FluentTheme.h"

#include <QFile>
#include <QHash>
#include <QIconEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QRegularExpression>
#include <QSvgRenderer>
#include <QtGlobal>

QT_BEGIN_NAMESPACE
extern int qInitResources_fluent_icons();
QT_END_NAMESPACE

namespace {

void ensureFluentIconResourcesInitialized()
{
    static const bool initialized = []() {
        QT_PREPEND_NAMESPACE(qInitResources_fluent_icons)();
        return true;
    }();
    Q_UNUSED(initialized);
}

} // namespace

namespace Fluent {

namespace {

QByteArray rawSvgData(FluentIconType type)
{
    ensureFluentIconResourcesInitialized();

    static QHash<int, QByteArray> cache;
    const int key = static_cast<int>(type);
    const auto it = cache.constFind(key);
    if (it != cache.constEnd()) {
        return it.value();
    }

    QFile file(FluentIcon::resourcePath(type));
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }

    const QByteArray data = file.readAll();
    cache.insert(key, data);
    return data;
}

QByteArray tintedSvgData(FluentIconType type, const QColor &color)
{
    QByteArray data = rawSvgData(type);
    if (data.isEmpty()) {
        return data;
    }

    QColor opaque = color;
    opaque.setAlpha(255);
    QString svg = QString::fromUtf8(data);
    const QString replacement = opaque.name(QColor::HexRgb);
    const auto caseInsensitive = QRegularExpression::CaseInsensitiveOption;
    svg.replace(QRegularExpression(QStringLiteral("#000000\\b"), caseInsensitive), replacement);
    svg.replace(QRegularExpression(QStringLiteral("#000\\b"), caseInsensitive), replacement);
    svg.replace(QRegularExpression(QStringLiteral("\\bblack\\b"), caseInsensitive), replacement);
    svg.replace(QRegularExpression(QStringLiteral("rgb\\s*\\(\\s*0\\s*,\\s*0\\s*,\\s*0\\s*\\)"), caseInsensitive),
                replacement);
    return svg.toUtf8();
}

bool paintSvgIcon(QPainter *painter, FluentIconType type, const QRectF &rect, const QColor &color, qreal opacity)
{
    const QByteArray data = tintedSvgData(type, color);
    if (data.isEmpty()) {
        return false;
    }

    QSvgRenderer renderer(data);
    if (!renderer.isValid()) {
        return false;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter->setOpacity(opacity);
    renderer.render(painter, rect);
    painter->restore();
    return true;
}

class NativeIconPainter final
{
public:
    NativeIconPainter(QPainter *painter, const QColor &color)
        : m_painter(painter)
        , m_color(color)
    {
    }

    void line(qreal x1, qreal y1, qreal x2, qreal y2)
    {
        m_painter->drawLine(QPointF(x1, y1), QPointF(x2, y2));
    }

    void path(const QPainterPath &path)
    {
        m_painter->drawPath(path);
    }

    void filledPath(const QPainterPath &path)
    {
        const QPen previousPen = m_painter->pen();
        const QBrush previousBrush = m_painter->brush();
        m_painter->setPen(Qt::NoPen);
        m_painter->setBrush(QBrush(m_color));
        m_painter->drawPath(path);
        m_painter->setPen(previousPen);
        m_painter->setBrush(previousBrush);
    }

    void circle(qreal x, qreal y, qreal radius, bool fill = false)
    {
        const QPen previousPen = m_painter->pen();
        const QBrush previousBrush = m_painter->brush();
        if (fill) {
            m_painter->setPen(Qt::NoPen);
        }
        m_painter->setBrush(fill ? QBrush(m_color) : QBrush(Qt::NoBrush));
        m_painter->drawEllipse(QPointF(x, y), radius, radius);
        m_painter->setPen(previousPen);
        m_painter->setBrush(previousBrush);
    }

    static void addRoundRect(QPainterPath &path, qreal x, qreal y, qreal width, qreal height, qreal radius)
    {
        path.addRoundedRect(QRectF(x, y, width, height), radius, radius);
    }

private:
    QPainter *m_painter;
    QColor m_color;
};

using NativeIconDrawFn = void (*)(NativeIconPainter &);

void drawAdd(NativeIconPainter &icon)
{
    icon.line(12, 5, 12, 19);
    icon.line(5, 12, 19, 12);
}

void drawArrowDown(NativeIconPainter &icon)
{
    icon.line(12, 4.5, 12, 17.2);

    QPainterPath path;
    path.moveTo(7.2, 12.8);
    path.lineTo(12, 17.6);
    path.lineTo(16.8, 12.8);
    icon.path(path);
}

void drawArrowUp(NativeIconPainter &icon)
{
    icon.line(12, 19.5, 12, 6.8);

    QPainterPath path;
    path.moveTo(7.2, 11.2);
    path.lineTo(12, 6.4);
    path.lineTo(16.8, 11.2);
    icon.path(path);
}

void drawBack(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(14.5, 6.5);
    path.lineTo(9.0, 12.0);
    path.lineTo(14.5, 17.5);
    icon.path(path);
    icon.line(9.5, 12, 20, 12);
}

void drawBell(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(6.5, 16.5);
    path.cubicTo(7.7, 15.2, 8, 13.6, 8, 10.6);
    path.cubicTo(8, 7.6, 9.7, 5.5, 12, 5.5);
    path.cubicTo(14.3, 5.5, 16, 7.6, 16, 10.6);
    path.cubicTo(16, 13.6, 16.3, 15.2, 17.5, 16.5);
    path.lineTo(6.5, 16.5);
    path.closeSubpath();
    icon.path(path);
    icon.line(10.2, 18.2, 13.8, 18.2);
    icon.line(11, 20, 13, 20);
}

void drawBookmark(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(7, 4.8);
    path.lineTo(17, 4.8);
    path.lineTo(17, 19.4);
    path.lineTo(12, 15.8);
    path.lineTo(7, 19.4);
    path.closeSubpath();
    icon.path(path);
}

void drawCalendar(NativeIconPainter &icon)
{
    QPainterPath path;
    NativeIconPainter::addRoundRect(path, 4.5, 5.2, 15, 14.3, 2);
    icon.path(path);
    icon.line(8, 3.8, 8, 6.8);
    icon.line(16, 3.8, 16, 6.8);
    icon.line(4.5, 9, 19.5, 9);
    icon.circle(8, 12, 0.45, true);
    icon.circle(12, 12, 0.45, true);
    icon.circle(16, 12, 0.45, true);
    icon.circle(8, 15.5, 0.45, true);
    icon.circle(12, 15.5, 0.45, true);
}

void drawCheckmark(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(5.5, 12.5);
    path.lineTo(9.7, 16.7);
    path.lineTo(18.5, 7.3);
    icon.path(path);
}

void drawChevronDown(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(7, 9.5);
    path.lineTo(12, 14.5);
    path.lineTo(17, 9.5);
    icon.path(path);
}

void drawChevronLeft(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(14.5, 7);
    path.lineTo(9.5, 12);
    path.lineTo(14.5, 17);
    icon.path(path);
}

void drawChevronRight(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(9.5, 7);
    path.lineTo(14.5, 12);
    path.lineTo(9.5, 17);
    icon.path(path);
}

void drawChevronUp(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(7, 14.5);
    path.lineTo(12, 9.5);
    path.lineTo(17, 14.5);
    icon.path(path);
}

void drawClose(NativeIconPainter &icon)
{
    icon.line(7, 7, 17, 17);
    icon.line(17, 7, 7, 17);
}

void drawControls(NativeIconPainter &icon)
{
    QPainterPath path;
    NativeIconPainter::addRoundRect(path, 4, 7, 16, 5, 2.5);
    NativeIconPainter::addRoundRect(path, 4, 15, 16, 5, 2.5);
    icon.path(path);
    icon.circle(8, 9.5, 1.8, true);
    icon.circle(16, 17.5, 1.8, true);
}

void drawCopy(NativeIconPainter &icon)
{
    QPainterPath back;
    NativeIconPainter::addRoundRect(back, 6, 4.5, 10, 12, 1.8);
    icon.path(back);

    QPainterPath front;
    NativeIconPainter::addRoundRect(front, 8.5, 7.5, 10, 12, 1.8);
    icon.path(front);
}

void drawData(NativeIconPainter &icon)
{
    QPainterPath path;
    NativeIconPainter::addRoundRect(path, 4.5, 5, 15, 14, 1.8);
    icon.path(path);
    icon.line(4.5, 9, 19.5, 9);
    icon.line(4.5, 13.5, 19.5, 13.5);
    icon.line(9.5, 5, 9.5, 19);
}

void drawDelete(NativeIconPainter &icon)
{
    icon.line(5, 7, 19, 7);

    QPainterPath path;
    path.moveTo(9, 7);
    path.lineTo(9, 5.8);
    path.cubicTo(9, 5, 9.6, 4.5, 10.4, 4.5);
    path.lineTo(13.6, 4.5);
    path.cubicTo(14.4, 4.5, 15, 5, 15, 5.8);
    path.lineTo(15, 7);
    path.moveTo(8, 9.5);
    path.lineTo(8.7, 19);
    path.cubicTo(8.8, 19.8, 9.4, 20.5, 10.3, 20.5);
    path.lineTo(13.7, 20.5);
    path.cubicTo(14.6, 20.5, 15.2, 19.8, 15.3, 19);
    path.lineTo(16, 9.5);
    icon.path(path);
    icon.line(10.5, 11, 10.5, 17);
    icon.line(13.5, 11, 13.5, 17);
}

void drawDialog(NativeIconPainter &icon)
{
    QPainterPath path;
    NativeIconPainter::addRoundRect(path, 5, 6, 14, 10, 2);
    icon.path(path);
    icon.line(8.2, 19, 11, 16.2);
    icon.line(8.5, 9.2, 15.5, 9.2);
    icon.line(8.5, 12.5, 13, 12.5);
}

void drawDocument(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(7, 3.8);
    path.lineTo(13.2, 3.8);
    path.lineTo(18, 8.6);
    path.lineTo(18, 20.2);
    path.lineTo(7, 20.2);
    path.closeSubpath();
    path.moveTo(13, 4);
    path.lineTo(13, 9);
    path.lineTo(18, 9);
    icon.path(path);
}

void drawDownload(NativeIconPainter &icon)
{
    drawArrowDown(icon);
    icon.line(5.5, 19.2, 18.5, 19.2);
}

void drawEdit(NativeIconPainter &icon)
{
    QPainterPath body;
    body.moveTo(5.2, 18.8);
    body.lineTo(8.8, 17.9);
    body.lineTo(18.4, 8.3);
    body.cubicTo(19.1, 7.6, 19.1, 6.5, 18.4, 5.8);
    body.cubicTo(17.7, 5.1, 16.6, 5.1, 15.9, 5.8);
    body.lineTo(6.3, 15.4);
    body.closeSubpath();
    icon.path(body);
    icon.line(14.8, 6.9, 17.3, 9.4);
}

void drawEye(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(3.9, 12);
    path.cubicTo(5.7, 8.6, 8.5, 6.8, 12, 6.8);
    path.cubicTo(15.5, 6.8, 18.3, 8.6, 20.1, 12);
    path.cubicTo(18.3, 15.4, 15.5, 17.2, 12, 17.2);
    path.cubicTo(8.5, 17.2, 5.7, 15.4, 3.9, 12);
    path.closeSubpath();
    icon.path(path);
    icon.circle(12, 12, 2.7);
}

void drawFilter(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(5, 6);
    path.lineTo(19, 6);
    path.lineTo(13.8, 12);
    path.lineTo(13.8, 18);
    path.lineTo(10.2, 20);
    path.lineTo(10.2, 12);
    path.closeSubpath();
    icon.path(path);
}

void drawFolder(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(4.5, 7.2);
    path.cubicTo(4.5, 6.3, 5.2, 5.7, 6.1, 5.7);
    path.lineTo(10.1, 5.7);
    path.lineTo(12, 7.7);
    path.lineTo(17.9, 7.7);
    path.cubicTo(18.8, 7.7, 19.5, 8.4, 19.5, 9.3);
    path.lineTo(19.5, 17);
    path.cubicTo(19.5, 17.9, 18.8, 18.6, 17.9, 18.6);
    path.lineTo(6.1, 18.6);
    path.cubicTo(5.2, 18.6, 4.5, 17.9, 4.5, 17);
    path.closeSubpath();
    icon.path(path);
}

void drawGauge(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(5, 17.5);
    path.arcTo(QRectF(5, 5.5, 14, 14), 180, -180);
    icon.path(path);
    icon.line(12, 17.5, 16, 9);
    icon.line(8, 17.5, 16, 17.5);
}

void drawHeart(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(12, 19);
    path.cubicTo(7.2, 15.5, 5, 13.2, 5, 9.8);
    path.cubicTo(5, 7.5, 6.7, 6, 8.8, 6);
    path.cubicTo(10.1, 6, 11.2, 6.7, 12, 7.8);
    path.cubicTo(12.8, 6.7, 13.9, 6, 15.2, 6);
    path.cubicTo(17.3, 6, 19, 7.5, 19, 9.8);
    path.cubicTo(19, 13.2, 16.8, 15.5, 12, 19);
    path.closeSubpath();
    icon.path(path);
}

void drawHome(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(4, 11.3);
    path.lineTo(12, 4);
    path.lineTo(20, 11.3);
    path.moveTo(6.2, 10.2);
    path.lineTo(6.2, 19.5);
    path.lineTo(10.4, 19.5);
    path.lineTo(10.4, 14.4);
    path.lineTo(13.6, 14.4);
    path.lineTo(13.6, 19.5);
    path.lineTo(17.8, 19.5);
    path.lineTo(17.8, 10.2);
    icon.path(path);
}

void drawIcons(NativeIconPainter &icon)
{
    QPainterPath path;
    NativeIconPainter::addRoundRect(path, 4.6, 4.8, 6.2, 6.2, 1.4);
    NativeIconPainter::addRoundRect(path, 13.2, 4.8, 6.2, 6.2, 1.4);
    NativeIconPainter::addRoundRect(path, 4.6, 13.2, 6.2, 6.2, 1.4);
    NativeIconPainter::addRoundRect(path, 13.2, 13.2, 6.2, 6.2, 1.4);
    icon.path(path);
}

void drawInfo(NativeIconPainter &icon)
{
    icon.circle(12, 12, 8.2);
    icon.line(12, 10.8, 12, 16);
    icon.circle(12, 7.8, 0.55, true);
}

void drawInput(NativeIconPainter &icon)
{
    icon.line(5, 7.5, 19, 7.5);
    icon.line(5, 16.5, 19, 16.5);
    icon.line(8.5, 12, 14, 12);
    icon.line(15.5, 9, 15.5, 15);
}

void drawLayout(NativeIconPainter &icon)
{
    QPainterPath path;
    NativeIconPainter::addRoundRect(path, 4.5, 5, 15, 14, 1.8);
    icon.path(path);
    icon.line(9.2, 5, 9.2, 19);
    icon.line(9.2, 11, 19.5, 11);
}

void drawLink(NativeIconPainter &icon)
{
    QPainterPath left;
    left.moveTo(9.8, 8.6);
    left.lineTo(8.6, 8.6);
    left.cubicTo(6.8, 8.6, 5.4, 10, 5.4, 11.8);
    left.cubicTo(5.4, 13.6, 6.8, 15, 8.6, 15);
    left.lineTo(10.5, 15);
    icon.path(left);

    QPainterPath right;
    right.moveTo(14.2, 15.4);
    right.lineTo(15.4, 15.4);
    right.cubicTo(17.2, 15.4, 18.6, 14, 18.6, 12.2);
    right.cubicTo(18.6, 10.4, 17.2, 9, 15.4, 9);
    right.lineTo(13.5, 9);
    icon.path(right);
    icon.line(9.3, 12, 14.7, 12);
}

void drawLock(NativeIconPainter &icon)
{
    QPainterPath shackle;
    shackle.moveTo(8, 10.2);
    shackle.lineTo(8, 8.5);
    shackle.cubicTo(8, 6.2, 9.7, 4.8, 12, 4.8);
    shackle.cubicTo(14.3, 4.8, 16, 6.2, 16, 8.5);
    shackle.lineTo(16, 10.2);
    icon.path(shackle);

    QPainterPath body;
    NativeIconPainter::addRoundRect(body, 6, 10, 12, 9.2, 2);
    icon.path(body);
    icon.line(12, 13.2, 12, 16.2);
}

void drawMail(NativeIconPainter &icon)
{
    QPainterPath path;
    NativeIconPainter::addRoundRect(path, 4.5, 6.5, 15, 11.2, 1.8);
    path.moveTo(5, 7.5);
    path.lineTo(12, 12.5);
    path.lineTo(19, 7.5);
    path.moveTo(9.8, 11.1);
    path.lineTo(5.2, 16.7);
    path.moveTo(14.2, 11.1);
    path.lineTo(18.8, 16.7);
    icon.path(path);
}

void drawMenu(NativeIconPainter &icon)
{
    icon.line(5, 7, 19, 7);
    icon.line(5, 12, 19, 12);
    icon.line(5, 17, 19, 17);
}

void drawMore(NativeIconPainter &icon)
{
    icon.circle(7, 12, 1.5, true);
    icon.circle(12, 12, 1.5, true);
    icon.circle(17, 12, 1.5, true);
}

void drawPerson(NativeIconPainter &icon)
{
    icon.circle(12, 8.2, 3.2);

    QPainterPath path;
    path.moveTo(5.8, 19.2);
    path.cubicTo(6.7, 15.6, 9, 13.8, 12, 13.8);
    path.cubicTo(15, 13.8, 17.3, 15.6, 18.2, 19.2);
    icon.path(path);
}

void drawPin(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(8.2, 5.2);
    path.lineTo(15.8, 5.2);
    path.lineTo(14.5, 10.5);
    path.lineTo(18.2, 14.2);
    path.lineTo(12.8, 14.2);
    path.lineTo(12, 20);
    path.lineTo(11.2, 14.2);
    path.lineTo(5.8, 14.2);
    path.lineTo(9.5, 10.5);
    path.closeSubpath();
    icon.path(path);
}

void drawPlay(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(8.2, 5.8);
    path.lineTo(18.6, 12);
    path.lineTo(8.2, 18.2);
    path.closeSubpath();
    icon.filledPath(path);
}

void drawRefresh(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(18, 8.2);
    path.cubicTo(16.8, 6.2, 14.6, 5, 12, 5);
    path.cubicTo(8.1, 5, 5, 8.1, 5, 12);
    path.moveTo(18.2, 5.2);
    path.lineTo(18.2, 8.4);
    path.lineTo(15, 8.4);
    path.moveTo(6, 15.8);
    path.cubicTo(7.2, 17.8, 9.4, 19, 12, 19);
    path.cubicTo(15.9, 19, 19, 15.9, 19, 12);
    path.moveTo(5.8, 18.8);
    path.lineTo(5.8, 15.6);
    path.lineTo(9, 15.6);
    icon.path(path);
}

void drawSave(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(5, 4.5);
    path.lineTo(16.2, 4.5);
    path.lineTo(19, 7.3);
    path.lineTo(19, 19.5);
    path.lineTo(5, 19.5);
    path.closeSubpath();
    path.moveTo(8, 4.5);
    path.lineTo(8, 9.5);
    path.lineTo(15, 9.5);
    path.lineTo(15, 4.5);
    path.moveTo(8, 19.5);
    path.lineTo(8, 14.5);
    path.lineTo(16, 14.5);
    path.lineTo(16, 19.5);
    icon.path(path);
}

void drawSearch(NativeIconPainter &icon)
{
    icon.circle(10.8, 10.8, 6.2);
    icon.line(15.4, 15.4, 19.5, 19.5);
}

void drawSettings(NativeIconPainter &icon)
{
    icon.circle(12, 12, 2.8);
    icon.line(12, 3.9, 12, 5.5);
    icon.line(12, 18.5, 12, 20.1);
    icon.line(3.9, 12, 5.5, 12);
    icon.line(18.5, 12, 20.1, 12);
    icon.line(6.3, 6.3, 7.5, 7.5);
    icon.line(16.5, 16.5, 17.7, 17.7);
    icon.line(17.7, 6.3, 16.5, 7.5);
    icon.line(7.5, 16.5, 6.3, 17.7);
}

void drawShare(NativeIconPainter &icon)
{
    icon.circle(7.2, 12, 2);
    icon.circle(16.8, 6.8, 2);
    icon.circle(16.8, 17.2, 2);
    icon.line(9, 11.1, 15, 7.7);
    icon.line(9, 12.9, 15, 16.3);
}

void drawSort(NativeIconPainter &icon)
{
    icon.line(7, 6, 17, 6);
    icon.line(7, 12, 14, 12);
    icon.line(7, 18, 11, 18);
}

void drawStar(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(12, 4.8);
    path.lineTo(14.2, 9.2);
    path.lineTo(19.1, 9.9);
    path.lineTo(15.6, 13.3);
    path.lineTo(16.4, 18.2);
    path.lineTo(12, 15.9);
    path.lineTo(7.6, 18.2);
    path.lineTo(8.4, 13.3);
    path.lineTo(4.9, 9.9);
    path.lineTo(9.8, 9.2);
    path.closeSubpath();
    icon.path(path);
}

void drawSuccess(NativeIconPainter &icon)
{
    icon.circle(12, 12, 8.2);

    QPainterPath path;
    path.moveTo(8.2, 12.2);
    path.lineTo(10.8, 14.8);
    path.lineTo(15.8, 9.4);
    icon.path(path);
}

void drawWarning(NativeIconPainter &icon)
{
    QPainterPath path;
    path.moveTo(11, 4.8);
    path.lineTo(3.8, 18.2);
    path.cubicTo(3.4, 19, 4, 20, 4.9, 20);
    path.lineTo(19.1, 20);
    path.cubicTo(20, 20, 20.6, 19, 20.2, 18.2);
    path.lineTo(13, 4.8);
    path.cubicTo(12.6, 4, 11.4, 4, 11, 4.8);
    path.closeSubpath();
    icon.path(path);
    icon.line(12, 9, 12, 14);
    icon.circle(12, 17, 0.55, true);
}

void drawUpload(NativeIconPainter &icon)
{
    drawArrowUp(icon);
    icon.line(5.5, 19.2, 18.5, 19.2);
}

void drawWindow(NativeIconPainter &icon)
{
    QPainterPath path;
    NativeIconPainter::addRoundRect(path, 4, 5, 16, 14, 1.8);
    icon.path(path);
    icon.line(4, 8.8, 20, 8.8);
    icon.circle(7, 7, 0.45, true);
    icon.circle(9.5, 7, 0.45, true);
}

void drawZoomIn(NativeIconPainter &icon)
{
    drawSearch(icon);
    icon.line(10.8, 7.8, 10.8, 13.8);
    icon.line(7.8, 10.8, 13.8, 10.8);
}

void drawZoomOut(NativeIconPainter &icon)
{
    drawSearch(icon);
    icon.line(7.8, 10.8, 13.8, 10.8);
}

struct IconDefinition {
    FluentIconType type;
    NativeIconDrawFn draw;
    const char *resourcePath;
};

class IconRegistry final
{
public:
    static NativeIconDrawFn drawFunction(FluentIconType type)
    {
        int entryCount = 0;
        const IconDefinition *iconDefinitions = definitions(entryCount);
        for (int i = 0; i < entryCount; ++i) {
            if (iconDefinitions[i].type == type) {
                return iconDefinitions[i].draw;
            }
        }
        return nullptr;
    }

    static const char *resourcePath(FluentIconType type)
    {
        int entryCount = 0;
        const IconDefinition *iconDefinitions = definitions(entryCount);
        for (int i = 0; i < entryCount; ++i) {
            if (iconDefinitions[i].type == type) {
                return iconDefinitions[i].resourcePath;
            }
        }
        return ":/Fluent/icons/info.svg";
    }

private:
    static const IconDefinition *definitions(int &entryCount)
    {
        static const IconDefinition iconDefinitions[] = {
            {FluentIconType::Add, drawAdd, ":/Fluent/icons/add.svg"},
            {FluentIconType::ArrowDown, drawArrowDown, ":/Fluent/icons/arrow-down.svg"},
            {FluentIconType::ArrowUp, drawArrowUp, ":/Fluent/icons/arrow-up.svg"},
            {FluentIconType::Back, drawBack, ":/Fluent/icons/back.svg"},
            {FluentIconType::Bell, drawBell, ":/Fluent/icons/bell.svg"},
            {FluentIconType::Bookmark, drawBookmark, ":/Fluent/icons/bookmark.svg"},
            {FluentIconType::Calendar, drawCalendar, ":/Fluent/icons/calendar.svg"},
            {FluentIconType::Checkmark, drawCheckmark, ":/Fluent/icons/checkmark.svg"},
            {FluentIconType::ChevronDown, drawChevronDown, ":/Fluent/icons/chevron-down.svg"},
            {FluentIconType::ChevronLeft, drawChevronLeft, ":/Fluent/icons/chevron-left.svg"},
            {FluentIconType::ChevronRight, drawChevronRight, ":/Fluent/icons/chevron-right.svg"},
            {FluentIconType::ChevronUp, drawChevronUp, ":/Fluent/icons/chevron-up.svg"},
            {FluentIconType::Close, drawClose, ":/Fluent/icons/close.svg"},
            {FluentIconType::Controls, drawControls, ":/Fluent/icons/controls.svg"},
            {FluentIconType::Copy, drawCopy, ":/Fluent/icons/copy.svg"},
            {FluentIconType::Data, drawData, ":/Fluent/icons/data.svg"},
            {FluentIconType::Delete, drawDelete, ":/Fluent/icons/delete.svg"},
            {FluentIconType::Dialog, drawDialog, ":/Fluent/icons/dialog.svg"},
            {FluentIconType::Document, drawDocument, ":/Fluent/icons/document.svg"},
            {FluentIconType::Download, drawDownload, ":/Fluent/icons/download.svg"},
            {FluentIconType::Edit, drawEdit, ":/Fluent/icons/edit.svg"},
            {FluentIconType::Eye, drawEye, ":/Fluent/icons/eye.svg"},
            {FluentIconType::Filter, drawFilter, ":/Fluent/icons/filter.svg"},
            {FluentIconType::Folder, drawFolder, ":/Fluent/icons/folder.svg"},
            {FluentIconType::Gauge, drawGauge, ":/Fluent/icons/gauge.svg"},
            {FluentIconType::Heart, drawHeart, ":/Fluent/icons/heart.svg"},
            {FluentIconType::Home, drawHome, ":/Fluent/icons/home.svg"},
            {FluentIconType::Icons, drawIcons, ":/Fluent/icons/icons.svg"},
            {FluentIconType::Info, drawInfo, ":/Fluent/icons/info.svg"},
            {FluentIconType::Input, drawInput, ":/Fluent/icons/input.svg"},
            {FluentIconType::Layout, drawLayout, ":/Fluent/icons/layout.svg"},
            {FluentIconType::Link, drawLink, ":/Fluent/icons/link.svg"},
            {FluentIconType::Lock, drawLock, ":/Fluent/icons/lock.svg"},
            {FluentIconType::Mail, drawMail, ":/Fluent/icons/mail.svg"},
            {FluentIconType::Menu, drawMenu, ":/Fluent/icons/menu.svg"},
            {FluentIconType::More, drawMore, ":/Fluent/icons/more.svg"},
            {FluentIconType::Person, drawPerson, ":/Fluent/icons/person.svg"},
            {FluentIconType::Pin, drawPin, ":/Fluent/icons/pin.svg"},
            {FluentIconType::Play, drawPlay, ":/Fluent/icons/play.svg"},
            {FluentIconType::Refresh, drawRefresh, ":/Fluent/icons/refresh.svg"},
            {FluentIconType::Save, drawSave, ":/Fluent/icons/save.svg"},
            {FluentIconType::Search, drawSearch, ":/Fluent/icons/search.svg"},
            {FluentIconType::Settings, drawSettings, ":/Fluent/icons/settings.svg"},
            {FluentIconType::Share, drawShare, ":/Fluent/icons/share.svg"},
            {FluentIconType::Sort, drawSort, ":/Fluent/icons/sort.svg"},
            {FluentIconType::Star, drawStar, ":/Fluent/icons/star.svg"},
            {FluentIconType::Success, drawSuccess, ":/Fluent/icons/success.svg"},
            {FluentIconType::Upload, drawUpload, ":/Fluent/icons/upload.svg"},
            {FluentIconType::Warning, drawWarning, ":/Fluent/icons/warning.svg"},
            {FluentIconType::Window, drawWindow, ":/Fluent/icons/window.svg"},
            {FluentIconType::ZoomIn, drawZoomIn, ":/Fluent/icons/zoom-in.svg"},
            {FluentIconType::ZoomOut, drawZoomOut, ":/Fluent/icons/zoom-out.svg"},
        };
        entryCount = static_cast<int>(sizeof(iconDefinitions) / sizeof(iconDefinitions[0]));
        return iconDefinitions;
    }
};

bool paintNativeIcon(QPainter *painter, FluentIconType type, const QRectF &rect, const QColor &color, qreal opacity)
{
    if (!painter || !painter->isActive() || rect.isEmpty()) {
        return false;
    }

    const qreal side = qMin(rect.width(), rect.height());
    if (side <= 0.0) {
        return false;
    }

    const NativeIconDrawFn drawIcon = IconRegistry::drawFunction(type);
    if (!drawIcon) {
        return false;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setOpacity(opacity);
    painter->translate(rect.center());
    painter->scale(side / 24.0, side / 24.0);
    painter->translate(-12.0, -12.0);

    QPen pen(color, 1.55, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);

    NativeIconPainter iconPainter(painter, color);
    drawIcon(iconPainter);

    painter->restore();
    return true;
}

class FluentIconEngine final : public QIconEngine
{
public:
    FluentIconEngine(FluentIconType type, const FluentIconOptions &options)
        : m_type(type)
        , m_options(options)
    {
    }

    QIconEngine *clone() const override
    {
        return new FluentIconEngine(m_type, m_options);
    }

    void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State /*state*/) override
    {
        FluentIcon(m_type).paint(painter, QRectF(rect), m_options, mode);
    }

    QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) override
    {
        Q_UNUSED(state);

        QPixmap pm(size);
        pm.fill(Qt::transparent);

        QPainter painter(&pm);
        FluentIcon(m_type).paint(&painter, QRectF(QPointF(0, 0), QSizeF(size)), m_options, mode);
        return pm;
    }

private:
    FluentIconType m_type;
    FluentIconOptions m_options;
};

} // namespace

FluentIcon::FluentIcon(FluentIconType type)
    : m_type(type)
{
}

FluentIconType FluentIcon::type() const
{
    return m_type;
}

QIcon FluentIcon::toQIcon(const FluentIconOptions &options) const
{
    return QIcon(new FluentIconEngine(m_type, options));
}

void FluentIcon::paint(QPainter *painter,
                       const QRectF &rect,
                       const FluentIconOptions &options,
                       QIcon::Mode mode) const
{
    paintIcon(painter, m_type, rect, options, mode);
}

QIcon FluentIcon::icon(FluentIconType type, const FluentIconOptions &options)
{
    return FluentIcon(type).toQIcon(options);
}

void FluentIcon::paintIcon(QPainter *painter,
                           FluentIconType type,
                           const QRectF &rect,
                           const FluentIconOptions &options,
                           QIcon::Mode mode)
{
    if (!painter || !painter->isActive() || rect.isEmpty()) {
        return;
    }

    QColor color = resolveColor(options, mode);
    const qreal alpha = qBound<qreal>(0.0, options.opacity * color.alphaF(), 1.0);
    if (alpha <= 0.0) {
        return;
    }

    if (paintSvgIcon(painter, type, rect, color, alpha)) {
        return;
    }

    paintNativeIcon(painter, type, rect, color, alpha);
}

QColor FluentIcon::resolveColor(const FluentIconOptions &options, QIcon::Mode mode)
{
    if (options.color.isValid()) {
        QColor color = options.color;
        if (mode == QIcon::Disabled) {
            color.setAlphaF(qMin(color.alphaF(), 0.45));
        }
        return color;
    }

    const auto &manager = ThemeManager::instance();
    const auto &colors = manager.colors();
    const auto &tokens = manager.tokens();

    if (!options.autoTheme) {
        return mode == QIcon::Disabled ? colors.disabledText : colors.text;
    }

    if (mode == QIcon::Disabled) {
        return colors.disabledText;
    }
    if (options.reversed) {
        return tokens.onAccent;
    }
    if (mode == QIcon::Active) {
        return colors.text;
    }
    return colors.text;
}

QString FluentIcon::resourcePath(FluentIconType type)
{
    return QString::fromLatin1(IconRegistry::resourcePath(type));
}

} // namespace Fluent
