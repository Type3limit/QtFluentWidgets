#include "Fluent/FluentAnnotatedScrollBar.h"

#include "Fluent/FluentTheme.h"
#include "Fluent/FluentToolTip.h"

#include <QAbstractScrollArea>
#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QScrollBar>
#include <QWheelEvent>

#include <limits>

namespace Fluent {

namespace {

QVector<FluentAnnotatedScrollBarRange> normalizedRanges(const QVector<FluentAnnotatedScrollBarRange> &ranges)
{
    QVector<FluentAnnotatedScrollBarRange> result;
    result.reserve(ranges.size());

    for (FluentAnnotatedScrollBarRange range : ranges) {
        if (range.text.trimmed().isEmpty()) {
            continue;
        }
        if (range.end < range.start) {
            qSwap(range.start, range.end);
        }
        result.push_back(range);
    }

    return result;
}

bool normalizeSource(FluentAnnotatedScrollBarSource *source)
{
    if (!source) {
        return false;
    }

    source->group = source->group.trimmed();
    source->text = source->text.trimmed();

    if (source->end < source->start) {
        qSwap(source->start, source->end);
    }

    if (source->group.isEmpty()) {
        source->group = source->text;
    }
    if (source->text.isEmpty()) {
        source->text = source->group;
    }

    return !source->group.isEmpty();
}

void insertGroupedSource(QVector<FluentAnnotatedScrollBarSource> &target, const FluentAnnotatedScrollBarSource &source)
{
    int insertionIndex = target.size();
    for (int index = 0; index < target.size(); ++index) {
        if (target.at(index).group == source.group) {
            insertionIndex = index + 1;
        }
    }

    target.insert(insertionIndex, source);
}

int overlapSize(int topValue, int bottomValue, int start, int end)
{
    return qMin(bottomValue, end) - qMax(topValue, start) + 1;
}

qreal normalizedValue(int value, int minimum, int maximum)
{
    if (maximum <= minimum) {
        return 0.0;
    }
    return qBound<qreal>(0.0, qreal(value - minimum) / qreal(maximum - minimum), 1.0);
}

} // namespace

FluentAnnotatedScrollBar::FluentAnnotatedScrollBar(QWidget *parent)
    : QWidget(parent)
{
    setMouseTracking(true);
    setAttribute(Qt::WA_Hover, true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setMinimumWidth(92);
}

void FluentAnnotatedScrollBar::setScrollArea(QAbstractScrollArea *area)
{
    if (m_scrollArea == area) {
        return;
    }

    if (m_scrollArea && m_scrollArea->viewport()) {
        m_scrollArea->viewport()->removeEventFilter(this);
    }

    m_scrollArea = area;
    if (m_scrollArea && m_scrollArea->viewport()) {
        m_scrollArea->viewport()->installEventFilter(this);
    }

    reconnectScrollBar(m_scrollArea ? m_scrollArea->verticalScrollBar() : nullptr);
}

QAbstractScrollArea *FluentAnnotatedScrollBar::scrollArea() const
{
    return m_scrollArea.data();
}

void FluentAnnotatedScrollBar::setScrollBar(QScrollBar *scrollBar)
{
    if (m_scrollArea && m_scrollArea->viewport()) {
        m_scrollArea->viewport()->removeEventFilter(this);
    }
    m_scrollArea = nullptr;
    reconnectScrollBar(scrollBar);
}

QScrollBar *FluentAnnotatedScrollBar::scrollBar() const
{
    return m_scrollBar.data();
}

void FluentAnnotatedScrollBar::setAnnotatedRanges(const QVector<FluentAnnotatedScrollBarRange> &ranges)
{
    const QVector<FluentAnnotatedScrollBarRange> next = normalizedRanges(ranges);
    const bool hadSources = m_usingSources || !m_sources.isEmpty();
    if (!hadSources && !m_usingSources && m_ranges == next) {
        return;
    }

    m_usingSources = false;
    m_sources.clear();
    m_groupFirstSource.clear();
    m_groupLastSource.clear();
    m_ranges = next;
    m_hoverRange = -1;
    handleLinkedScrollBarChanged(false);
    emit annotatedRangesChanged();
    if (hadSources) {
        emit sourcesChanged();
    }
}

QVector<FluentAnnotatedScrollBarRange> FluentAnnotatedScrollBar::annotatedRanges() const
{
    return m_ranges;
}

void FluentAnnotatedScrollBar::clearAnnotatedRanges()
{
    const bool hadSources = m_usingSources || !m_sources.isEmpty();
    if (!hadSources && !m_usingSources && m_ranges.isEmpty()) {
        return;
    }

    m_usingSources = false;
    m_ranges.clear();
    m_sources.clear();
    m_groupFirstSource.clear();
    m_groupLastSource.clear();
    m_hoverRange = -1;
    hideToolTip();
    applyCurrentIndexes(-1, -1, true, false);
    emit annotatedRangesChanged();
    if (hadSources) {
        emit sourcesChanged();
    }
}

void FluentAnnotatedScrollBar::setSources(const QVector<FluentAnnotatedScrollBarSource> &sources)
{
    QVector<FluentAnnotatedScrollBarSource> next;
    next.reserve(sources.size());

    for (FluentAnnotatedScrollBarSource source : sources) {
        if (!normalizeSource(&source)) {
            continue;
        }
        insertGroupedSource(next, source);
    }

    setSourcesInternal(next);
}

QVector<FluentAnnotatedScrollBarSource> FluentAnnotatedScrollBar::sources() const
{
    return m_sources;
}

void FluentAnnotatedScrollBar::addSource(const FluentAnnotatedScrollBarSource &source)
{
    FluentAnnotatedScrollBarSource nextSource = source;
    if (!normalizeSource(&nextSource)) {
        return;
    }

    QVector<FluentAnnotatedScrollBarSource> next = m_usingSources ? m_sources : QVector<FluentAnnotatedScrollBarSource>();
    insertGroupedSource(next, nextSource);
    setSourcesInternal(next);
}

void FluentAnnotatedScrollBar::addSources(const QVector<FluentAnnotatedScrollBarSource> &sources)
{
    QVector<FluentAnnotatedScrollBarSource> next = m_usingSources ? m_sources : QVector<FluentAnnotatedScrollBarSource>();
    next.reserve(next.size() + sources.size());

    for (FluentAnnotatedScrollBarSource source : sources) {
        if (!normalizeSource(&source)) {
            continue;
        }
        insertGroupedSource(next, source);
    }

    setSourcesInternal(next);
}

void FluentAnnotatedScrollBar::clearSources()
{
    if (!m_usingSources && m_sources.isEmpty()) {
        return;
    }

    setSourcesInternal({});
}

QStringList FluentAnnotatedScrollBar::groups() const
{
    QStringList result;
    result.reserve(m_ranges.size());
    for (const FluentAnnotatedScrollBarRange &range : m_ranges) {
        result.push_back(range.text);
    }
    return result;
}

int FluentAnnotatedScrollBar::currentRangeIndex() const
{
    return m_currentRange;
}

QString FluentAnnotatedScrollBar::currentRangeText() const
{
    if (m_currentRange < 0 || m_currentRange >= m_ranges.size()) {
        return QString();
    }
    return m_ranges.at(m_currentRange).text;
}

void FluentAnnotatedScrollBar::setCurrentRangeIndex(int index)
{
    if (index < 0 || index >= m_ranges.size()) {
        return;
    }

    const int sourceIndex = firstSourceIndexForRange(index);
    if (m_scrollBar) {
        const int targetValue = targetValueForRange(index);
        if (m_scrollBar->value() != targetValue) {
            m_scrollBar->setValue(targetValue);
            return;
        }
    }

    applyCurrentIndexes(index, sourceIndex, true, true);
}

int FluentAnnotatedScrollBar::currentGroupIndex() const
{
    return currentRangeIndex();
}

QString FluentAnnotatedScrollBar::currentGroup() const
{
    return currentRangeText();
}

void FluentAnnotatedScrollBar::setCurrentGroup(const QString &group)
{
    const QString normalizedGroup = group.trimmed();
    if (normalizedGroup.isEmpty()) {
        return;
    }

    for (int index = 0; index < m_ranges.size(); ++index) {
        if (m_ranges.at(index).text == normalizedGroup) {
            setCurrentRangeIndex(index);
            return;
        }
    }
}

int FluentAnnotatedScrollBar::currentSourceIndex() const
{
    return m_currentSource;
}

FluentAnnotatedScrollBarSource FluentAnnotatedScrollBar::currentSource() const
{
    if (m_currentSource < 0 || m_currentSource >= m_sources.size()) {
        return {};
    }
    return m_sources.at(m_currentSource);
}

void FluentAnnotatedScrollBar::setCurrentSourceIndex(int index)
{
    if (index < 0 || index >= m_sources.size()) {
        return;
    }

    const int rangeIndex = groupIndexForSourceIndex(index);
    if (m_scrollBar) {
        const int targetValue = targetValueForSource(index);
        if (m_scrollBar->value() != targetValue) {
            m_scrollBar->setValue(targetValue);
            return;
        }
    }

    applyCurrentIndexes(rangeIndex, index, true, true);
}

void FluentAnnotatedScrollBar::setShowToolTipOnScroll(bool enabled)
{
    if (m_showToolTipOnScroll == enabled) {
        return;
    }

    m_showToolTipOnScroll = enabled;
    if (!m_showToolTipOnScroll) {
        hideToolTip();
    }
}

bool FluentAnnotatedScrollBar::showToolTipOnScroll() const
{
    return m_showToolTipOnScroll;
}

void FluentAnnotatedScrollBar::setToolTipDuration(int msec)
{
    m_toolTipDuration = qMax(100, msec);
}

int FluentAnnotatedScrollBar::toolTipDuration() const
{
    return m_toolTipDuration;
}

QSize FluentAnnotatedScrollBar::sizeHint() const
{
    return QSize(104, 220);
}

bool FluentAnnotatedScrollBar::eventFilter(QObject *watched, QEvent *event)
{
    if (m_scrollArea && watched == m_scrollArea->viewport() && event) {
        switch (event->type()) {
        case QEvent::Resize:
        case QEvent::Show:
        case QEvent::Wheel:
            update();
            break;
        default:
            break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void FluentAnnotatedScrollBar::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    if (!painter.isActive()) {
        return;
    }
    painter.setRenderHint(QPainter::Antialiasing, true);

    const auto &colors = ThemeManager::instance().colors();
    const QRectF rail = railRect();
    const QRectF labelBounds = labelBoundsRect();
    const QVector<QRectF> labelRects = layoutLabelRects();

    QColor railColor = colors.border;
    railColor.setAlpha(110);
    painter.setPen(Qt::NoPen);
    painter.setBrush(railColor);
    painter.drawRoundedRect(rail, rail.width() / 2.0, rail.width() / 2.0);

    if (m_scrollBar && m_scrollBar->maximum() > m_scrollBar->minimum()) {
        QColor handleColor = colors.accent;
        handleColor.setAlpha(m_draggingHandle ? 235 : (m_hoverHandle ? 225 : 210));
        painter.setBrush(handleColor);
        painter.drawRoundedRect(handleRect(), rail.width() / 2.0, rail.width() / 2.0);
    }

    QFont labelFont = font();
    labelFont.setPointSizeF(qMax(8.0, labelFont.pointSizeF() - 0.5));
    painter.setFont(labelFont);

    for (int i = 0; i < m_ranges.size() && i < labelRects.size(); ++i) {
        const QRectF rect = labelRects.at(i);
        const bool active = (i == m_currentRange);
        const bool hovered = (i == m_hoverRange);

        QColor connector = active ? colors.accent : colors.border;
        connector.setAlpha(active ? 190 : 80);
        painter.setPen(QPen(connector, active ? 1.6 : 1.0, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(rail.right() + 4.0, rect.center().y()), QPointF(rect.left() - 6.0, rect.center().y()));

        QColor dotColor = connector;
        painter.setPen(Qt::NoPen);
        painter.setBrush(dotColor);
        painter.drawEllipse(QPointF(rail.center().x(), rect.center().y()), active ? 3.2 : 2.2, active ? 3.2 : 2.2);

        QColor fill = Qt::transparent;
        if (active) {
            fill = colors.accent;
        } else if (hovered) {
            fill = colors.hover;
            fill.setAlpha(140);
        }

        if (fill.alpha() > 0) {
            painter.setBrush(fill);
            painter.drawRoundedRect(rect, 8.0, 8.0);
        }

        QColor textColor = active ? QColor(Qt::white) : colors.subText;
        if (hovered && !active) {
            textColor = colors.text;
        }

        painter.setPen(textColor);
        painter.drawText(rect.adjusted(10, 0, -8, 0), Qt::AlignVCenter | Qt::AlignLeft,
                         painter.fontMetrics().elidedText(m_ranges.at(i).text, Qt::ElideRight, int(labelBounds.width()) - 18));
    }
}

void FluentAnnotatedScrollBar::mouseMoveEvent(QMouseEvent *event)
{
    if (!event) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    if (m_draggingHandle && m_scrollBar) {
        m_scrollBar->setValue(valueForHandleTop(event->pos().y() - m_handleDragOffset));
        event->accept();
        return;
    }

    const bool nextHoverHandle = handleRect().contains(event->pos()) && m_scrollBar && m_scrollBar->maximum() > m_scrollBar->minimum();
    bool needsUpdate = false;
    if (m_hoverHandle != nextHoverHandle) {
        m_hoverHandle = nextHoverHandle;
        updateCursorShape();
        needsUpdate = true;
    }

    const int nextHover = m_hoverHandle ? -1 : rangeIndexAt(event->pos());
    if (nextHover != m_hoverRange) {
        m_hoverRange = nextHover;
        needsUpdate = true;
    }

    if (needsUpdate) {
        update();
    }

    QWidget::mouseMoveEvent(event);
}

void FluentAnnotatedScrollBar::mousePressEvent(QMouseEvent *event)
{
    if (!event || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    if (m_scrollBar && handleRect().contains(event->pos()) && m_scrollBar->maximum() > m_scrollBar->minimum()) {
        m_draggingHandle = true;
        m_hoverHandle = true;
        m_handleDragOffset = event->pos().y() - handleRect().top();
        grabMouse();
        updateCursorShape();
        maybeShowCurrentToolTip();
        update();
        event->accept();
        return;
    }

    if (!m_scrollBar) {
        QWidget::mousePressEvent(event);
        return;
    }

    const int rangeIndex = rangeIndexAt(event->pos());
    if (rangeIndex >= 0) {
        m_scrollBar->setValue(targetValueForRange(rangeIndex));
    } else if (railRect().contains(event->pos())) {
        m_scrollBar->setValue(valueForPosition(event->pos()));
    }

    event->accept();
}

void FluentAnnotatedScrollBar::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_draggingHandle && event && event->button() == Qt::LeftButton) {
        m_draggingHandle = false;
        if (mouseGrabber() == this) {
            releaseMouse();
        }
        m_hoverHandle = handleRect().contains(event->pos()) && m_scrollBar && m_scrollBar->maximum() > m_scrollBar->minimum();
        updateCursorShape();
        maybeShowCurrentToolTip();
        update();
        event->accept();
        return;
    }

    QWidget::mouseReleaseEvent(event);
}

void FluentAnnotatedScrollBar::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (m_draggingHandle) {
        return;
    }

    bool needsUpdate = false;
    if (m_hoverRange != -1) {
        m_hoverRange = -1;
        needsUpdate = true;
    }
    if (m_hoverHandle) {
        m_hoverHandle = false;
        updateCursorShape();
        needsUpdate = true;
    }
    if (needsUpdate) {
        update();
    }
}

void FluentAnnotatedScrollBar::wheelEvent(QWheelEvent *event)
{
    if (!event || !m_scrollBar) {
        QWidget::wheelEvent(event);
        return;
    }

    const int delta = event->angleDelta().y();
    if (delta == 0) {
        event->ignore();
        return;
    }

    const int direction = delta > 0 ? -1 : 1;
    const int steps = qMax(1, qAbs(delta) / 120);
    m_scrollBar->setValue(m_scrollBar->value() + direction * steps * qMax(1, m_scrollBar->singleStep()));
    event->accept();
}

QRectF FluentAnnotatedScrollBar::railRect() const
{
    return QRectF(12.0, 12.0, 6.0, qMax(36.0, height() - 24.0));
}

QRectF FluentAnnotatedScrollBar::handleRect() const
{
    const QRectF rail = railRect();
    if (!m_scrollBar || m_scrollBar->maximum() <= m_scrollBar->minimum()) {
        return rail;
    }

    const int minimum = m_scrollBar->minimum();
    const int maximum = m_scrollBar->maximum();
    const int pageStep = qMax(1, m_scrollBar->pageStep());
    const qreal totalSpan = qMax<qreal>(1.0, qreal(maximum - minimum + pageStep));
    const qreal handleHeight = qBound<qreal>(24.0, rail.height() * qreal(pageStep) / totalSpan, rail.height());
    const qreal top = rail.top() + (rail.height() - handleHeight) * normalizedValue(m_scrollBar->value(), minimum, maximum);

    return QRectF(rail.left(), top, rail.width(), handleHeight);
}

QRectF FluentAnnotatedScrollBar::labelBoundsRect() const
{
    return QRectF(28.0, 8.0, qMax(44.0, width() - 34.0), qMax(40.0, height() - 16.0));
}

QVector<QRectF> FluentAnnotatedScrollBar::layoutLabelRects() const
{
    QVector<QRectF> rects;
    rects.reserve(m_ranges.size());
    if (m_ranges.isEmpty()) {
        return rects;
    }

    const QRectF bounds = labelBoundsRect();
    QFont labelFont = font();
    labelFont.setPointSizeF(qMax(8.0, labelFont.pointSizeF() - 0.5));
    const QFontMetrics fm(labelFont);
    const qreal itemHeight = qMax<qreal>(22.0, fm.height() + 8.0);
    const qreal gap = 4.0;

    int minimum = 0;
    int maximum = 0;
    if (m_scrollBar) {
        minimum = m_scrollBar->minimum();
        maximum = qMax(m_scrollBar->minimum() + 1, m_scrollBar->maximum() + qMax(1, m_scrollBar->pageStep()));
    } else if (!m_ranges.isEmpty()) {
        minimum = m_ranges.first().start;
        maximum = qMax(minimum + 1, m_ranges.last().end);
    }

    qreal previousBottom = bounds.top() - gap;
    for (const FluentAnnotatedScrollBarRange &range : m_ranges) {
        const int centerValue = range.start + (range.end - range.start) / 2;
        const qreal centerY = bounds.top() + normalizedValue(centerValue, minimum, maximum) * bounds.height();
        qreal top = centerY - itemHeight / 2.0;
        top = qMax(top, previousBottom + gap);
        rects.push_back(QRectF(bounds.left(), top, bounds.width(), itemHeight));
        previousBottom = rects.back().bottom();
    }

    if (!rects.isEmpty() && rects.back().bottom() > bounds.bottom()) {
        qreal overflow = rects.back().bottom() - bounds.bottom();
        for (QRectF &rect : rects) {
            rect.translate(0.0, -overflow);
        }

        for (int i = rects.size() - 2; i >= 0; --i) {
            const qreal maxBottom = rects.at(i + 1).top() - gap;
            if (rects[i].bottom() > maxBottom) {
                rects[i].moveBottom(maxBottom);
            }
        }

        if (rects.front().top() < bounds.top()) {
            const qreal shiftDown = bounds.top() - rects.front().top();
            for (QRectF &rect : rects) {
                rect.translate(0.0, shiftDown);
            }
        }
    }

    return rects;
}

int FluentAnnotatedScrollBar::rangeIndexForViewport(int topValue, int bottomValue) const
{
    if (m_ranges.isEmpty()) {
        return -1;
    }

    int bestIndex = -1;
    int bestOverlap = -1;
    for (int i = 0; i < m_ranges.size(); ++i) {
        const FluentAnnotatedScrollBarRange &range = m_ranges.at(i);
        const int overlap = qMin(bottomValue, range.end) - qMax(topValue, range.start) + 1;
        if (overlap > bestOverlap) {
            bestOverlap = overlap;
            bestIndex = i;
        }
    }

    if (bestOverlap > 0) {
        return bestIndex;
    }

    return rangeIndexForValue(topValue + qMax(0, bottomValue - topValue) / 2);
}

int FluentAnnotatedScrollBar::rangeIndexForValue(int value) const
{
    if (m_ranges.isEmpty()) {
        return -1;
    }

    for (int i = 0; i < m_ranges.size(); ++i) {
        const FluentAnnotatedScrollBarRange &range = m_ranges.at(i);
        if (value >= range.start && value <= range.end) {
            return i;
        }
    }

    int nearestIndex = 0;
    int nearestDistance = std::numeric_limits<int>::max();
    for (int i = 0; i < m_ranges.size(); ++i) {
        const FluentAnnotatedScrollBarRange &range = m_ranges.at(i);
        const int center = range.start + (range.end - range.start) / 2;
        const int distance = qAbs(center - value);
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestIndex = i;
        }
    }

    return nearestIndex;
}

int FluentAnnotatedScrollBar::rangeIndexAt(const QPoint &pos) const
{
    const QVector<QRectF> labelRects = layoutLabelRects();
    for (int i = 0; i < labelRects.size(); ++i) {
        if (labelRects.at(i).contains(pos)) {
            return i;
        }
    }
    return -1;
}

int FluentAnnotatedScrollBar::valueForPosition(const QPoint &pos) const
{
    if (!m_scrollBar) {
        return 0;
    }

    const QRectF rail = railRect();
    if (m_scrollBar->maximum() <= m_scrollBar->minimum()) {
        return m_scrollBar->minimum();
    }

    const qreal ratio = qBound<qreal>(0.0, (pos.y() - rail.top()) / qMax<qreal>(1.0, rail.height()), 1.0);
    return m_scrollBar->minimum() + qRound(ratio * (m_scrollBar->maximum() - m_scrollBar->minimum()));
}

int FluentAnnotatedScrollBar::valueForHandleTop(qreal handleTop) const
{
    if (!m_scrollBar) {
        return 0;
    }

    const QRectF rail = railRect();
    const QRectF handle = handleRect();
    const qreal available = qMax<qreal>(0.0, rail.height() - handle.height());
    const qreal clampedTop = qBound(rail.top(), handleTop, rail.top() + available);
    const qreal ratio = available <= 0.0 ? 0.0 : (clampedTop - rail.top()) / available;
    return m_scrollBar->minimum() + qRound(ratio * (m_scrollBar->maximum() - m_scrollBar->minimum()));
}

int FluentAnnotatedScrollBar::targetValueForRange(int index) const
{
    if (!m_scrollBar || index < 0 || index >= m_ranges.size()) {
        return 0;
    }

    const FluentAnnotatedScrollBarRange &range = m_ranges.at(index);
    const int target = range.start;
    return qBound(m_scrollBar->minimum(), target, m_scrollBar->maximum());
}

int FluentAnnotatedScrollBar::targetValueForSource(int index) const
{
    if (!m_scrollBar || index < 0 || index >= m_sources.size()) {
        return 0;
    }

    return qBound(m_scrollBar->minimum(), m_sources.at(index).start, m_scrollBar->maximum());
}

int FluentAnnotatedScrollBar::sourceIndexForViewport(int topValue, int bottomValue) const
{
    if (!m_usingSources || m_sources.isEmpty()) {
        return -1;
    }

    int bestIndex = -1;
    int bestOverlap = -1;
    for (int index = 0; index < m_sources.size(); ++index) {
        const FluentAnnotatedScrollBarSource &source = m_sources.at(index);
        const int overlap = overlapSize(topValue, bottomValue, source.start, source.end);
        if (overlap > bestOverlap) {
            bestOverlap = overlap;
            bestIndex = index;
        }
    }

    if (bestOverlap > 0) {
        return bestIndex;
    }

    return sourceIndexForValue(topValue + qMax(0, bottomValue - topValue) / 2);
}

int FluentAnnotatedScrollBar::sourceIndexForValue(int value) const
{
    if (!m_usingSources || m_sources.isEmpty()) {
        return -1;
    }

    for (int index = 0; index < m_sources.size(); ++index) {
        const FluentAnnotatedScrollBarSource &source = m_sources.at(index);
        if (value >= source.start && value <= source.end) {
            return index;
        }
    }

    int nearestIndex = 0;
    int nearestDistance = std::numeric_limits<int>::max();
    for (int index = 0; index < m_sources.size(); ++index) {
        const FluentAnnotatedScrollBarSource &source = m_sources.at(index);
        const int center = source.start + (source.end - source.start) / 2;
        const int distance = qAbs(center - value);
        if (distance < nearestDistance) {
            nearestDistance = distance;
            nearestIndex = index;
        }
    }

    return nearestIndex;
}

int FluentAnnotatedScrollBar::groupIndexForSourceIndex(int sourceIndex) const
{
    if (sourceIndex < 0 || sourceIndex >= m_sources.size()) {
        return -1;
    }

    for (int index = 0; index < m_groupFirstSource.size() && index < m_groupLastSource.size(); ++index) {
        if (sourceIndex >= m_groupFirstSource.at(index) && sourceIndex <= m_groupLastSource.at(index)) {
            return index;
        }
    }

    return -1;
}

int FluentAnnotatedScrollBar::firstSourceIndexForRange(int rangeIndex) const
{
    if (!m_usingSources || rangeIndex < 0 || rangeIndex >= m_groupFirstSource.size()) {
        return -1;
    }
    return m_groupFirstSource.at(rangeIndex);
}

void FluentAnnotatedScrollBar::reconnectScrollBar(QScrollBar *scrollBar)
{
    if (m_scrollBar == scrollBar) {
        return;
    }

    if (m_scrollBar) {
        disconnect(m_scrollBar, nullptr, this, nullptr);
    }

    m_scrollBar = scrollBar;
    if (!m_scrollBar) {
        m_hoverHandle = false;
        hideToolTip();
        applyCurrentIndexes(-1, -1, true, false);
        updateCursorShape();
        return;
    }

    connect(m_scrollBar, &QScrollBar::valueChanged, this, [this](int) {
        handleLinkedScrollBarChanged(true);
    });
    connect(m_scrollBar, &QScrollBar::rangeChanged, this, [this](int, int) {
        handleLinkedScrollBarChanged(false);
    });
    connect(m_scrollBar, &QObject::destroyed, this, [this]() {
        m_scrollBar = nullptr;
        m_hoverHandle = false;
        hideToolTip();
        applyCurrentIndexes(-1, -1, true, false);
        updateCursorShape();
    });

    updateCursorShape();
    handleLinkedScrollBarChanged(false);
}

void FluentAnnotatedScrollBar::handleLinkedScrollBarChanged(bool showToolTip)
{
    int nextRange = -1;
    int nextSource = -1;
    if (!m_ranges.isEmpty()) {
        const int topValue = m_scrollBar ? m_scrollBar->value() : m_ranges.first().start;
        const int bottomValue = m_scrollBar ? (m_scrollBar->value() + qMax(1, m_scrollBar->pageStep()) - 1) : topValue;
        nextRange = rangeIndexForViewport(topValue, bottomValue);
        nextSource = sourceIndexForViewport(topValue, bottomValue);
    }

    applyCurrentIndexes(nextRange, nextSource, true, showToolTip);
}

void FluentAnnotatedScrollBar::setSourcesInternal(const QVector<FluentAnnotatedScrollBarSource> &sources)
{
    const bool previousUsingSources = m_usingSources;
    const QVector<FluentAnnotatedScrollBarSource> previousSources = m_sources;
    const QVector<FluentAnnotatedScrollBarRange> previousRanges = m_ranges;

    if (previousUsingSources && previousSources == sources) {
        return;
    }

    m_usingSources = true;
    m_sources = sources;
    rebuildRangesFromSources();
    m_hoverRange = -1;
    handleLinkedScrollBarChanged(false);

    emit sourcesChanged();
    if (!previousUsingSources || previousRanges != m_ranges) {
        emit annotatedRangesChanged();
    }
}

void FluentAnnotatedScrollBar::rebuildRangesFromSources()
{
    m_ranges.clear();
    m_groupFirstSource.clear();
    m_groupLastSource.clear();

    if (m_sources.isEmpty()) {
        return;
    }

    int sourceIndex = 0;
    while (sourceIndex < m_sources.size()) {
        const QString group = m_sources.at(sourceIndex).group;
        int start = m_sources.at(sourceIndex).start;
        int end = m_sources.at(sourceIndex).end;
        const int firstSource = sourceIndex;
        int lastSource = sourceIndex;

        while (lastSource + 1 < m_sources.size() && m_sources.at(lastSource + 1).group == group) {
            ++lastSource;
            start = qMin(start, m_sources.at(lastSource).start);
            end = qMax(end, m_sources.at(lastSource).end);
        }

        m_ranges.push_back({start, end, group});
        m_groupFirstSource.push_back(firstSource);
        m_groupLastSource.push_back(lastSource);

        sourceIndex = lastSource + 1;
    }
}

void FluentAnnotatedScrollBar::applyCurrentIndexes(int rangeIndex, int sourceIndex, bool emitSignals, bool showToolTip)
{
    const int nextRange = (rangeIndex >= 0 && rangeIndex < m_ranges.size()) ? rangeIndex : -1;
    const int nextSource = (m_usingSources && sourceIndex >= 0 && sourceIndex < m_sources.size()) ? sourceIndex : -1;

    const bool rangeChanged = m_currentRange != nextRange;
    const bool sourceChanged = m_currentSource != nextSource;

    m_currentRange = nextRange;
    m_currentSource = nextSource;

    update();

    if (emitSignals) {
        if (rangeChanged) {
            emit currentRangeChanged(m_currentRange, currentRangeText());
            emit currentGroupChanged(m_currentRange, currentGroup());
        }
        if (sourceChanged) {
            const FluentAnnotatedScrollBarSource source = currentSource();
            emit currentSourceChanged(m_currentSource, source.group, source.text);
        }
    }

    if (showToolTip) {
        maybeShowCurrentToolTip();
    }
}

QString FluentAnnotatedScrollBar::toolTipTextForCurrentState() const
{
    if (m_currentSource >= 0 && m_currentSource < m_sources.size()) {
        const QString text = m_sources.at(m_currentSource).text.trimmed();
        if (!text.isEmpty()) {
            return text;
        }
    }

    return currentRangeText();
}

void FluentAnnotatedScrollBar::maybeShowCurrentToolTip()
{
    const QString text = toolTipTextForCurrentState();
    if (!m_showToolTipOnScroll || !isVisible() || m_currentRange < 0 || m_currentRange >= m_ranges.size() || text.isEmpty()) {
        return;
    }

    const QVector<QRectF> labelRects = layoutLabelRects();
    const QRectF anchorRect = (m_currentRange < labelRects.size()) ? labelRects.at(m_currentRange) : railRect();
    const QPoint globalPos = mapToGlobal(QPoint(width() - 2, qRound(anchorRect.center().y())));
    FluentToolTip::showText(globalPos, text, this, m_toolTipDuration);
}

void FluentAnnotatedScrollBar::hideToolTip()
{
    FluentToolTip::hideText();
}

void FluentAnnotatedScrollBar::updateCursorShape()
{
    if (!isEnabled() || !m_scrollBar || m_scrollBar->maximum() <= m_scrollBar->minimum()) {
        unsetCursor();
        return;
    }

    if (m_draggingHandle) {
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (m_hoverHandle) {
        setCursor(Qt::OpenHandCursor);
        return;
    }

    unsetCursor();
}

} // namespace Fluent