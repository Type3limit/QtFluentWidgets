#include "Fluent/FluentFlowLayout.h"

#include <QStyle>
#include <QElapsedTimer>
#include <QPropertyAnimation>
#include <QTimer>
#include <QWidget>
#include <qvariant.h>

namespace Fluent {

FluentFlowLayout::FluentFlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
    : QLayout(parent)
    , m_hSpace(hSpacing)
    , m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);

    m_animClock = new QElapsedTimer();
    m_animClock->start();

    m_resizeDebounceTimer = new QTimer(this);
    m_resizeDebounceTimer->setSingleShot(true);
    QObject::connect(m_resizeDebounceTimer, &QTimer::timeout, this, [this]() {
        if (!m_animationEnabled || m_animateWhileResizing || !m_hasPendingGeometries) {
            return;
        }
        animateToItemGeometries(m_pendingGeometries);
        m_hasPendingGeometries = false;
    });
}

FluentFlowLayout::~FluentFlowLayout()
{
    for (auto it = m_animations.begin(); it != m_animations.end(); ++it) {
        if (it.value()) {
            it.value()->stop();
            it.value()->deleteLater();
        }
    }
    m_animations.clear();
    m_animLastRestartMs.clear();
    delete m_animClock;
    m_animClock = nullptr;

    QLayoutItem *item = nullptr;
    while ((item = takeAt(0)) != nullptr) {
        delete item;
    }
}

void FluentFlowLayout::addItem(QLayoutItem *item)
{
    m_items.append(item);
}

int FluentFlowLayout::count() const
{
    return m_items.size();
}

QLayoutItem *FluentFlowLayout::itemAt(int index) const
{
    return m_items.value(index);
}

QLayoutItem *FluentFlowLayout::takeAt(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return nullptr;
    }
    return m_items.takeAt(index);
}

Qt::Orientations FluentFlowLayout::expandingDirections() const
{
    return {};
}

bool FluentFlowLayout::hasHeightForWidth() const
{
    return true;
}

int FluentFlowLayout::heightForWidth(int width) const
{
    return doLayout(QRect(0, 0, width, 0), true);
}

void FluentFlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);

    int usedHeight = 0;
    const auto geometries = computeItemGeometries(rect, &usedHeight);
    Q_UNUSED(usedHeight)

    if (m_animationEnabled) {
        bool disableAnimationsThisPass = false;
        for (auto *item : m_items) {
            if (!item) {
                continue;
            }
            QWidget *w = item->widget();
            if (w && w->property(kDisableAnimationProperty).toBool()) {
                disableAnimationsThisPass = true;
                break;
            }
        }

        if (disableAnimationsThisPass) {
            // Stop any in-flight geometry animations so they don't fight with a child height animation.
            for (auto it = m_animations.begin(); it != m_animations.end(); ++it) {
                if (it.value()) {
                    it.value()->stop();
                }
            }

            QWidget *pw = parentWidget();
            const bool hadUpdates = pw ? pw->updatesEnabled() : true;
            if (pw) {
                pw->setUpdatesEnabled(false);
            }
            applyItemGeometries(geometries);
            if (pw) {
                pw->setUpdatesEnabled(hadUpdates);
                pw->update();
            }

            if (m_resizeDebounceTimer) {
                m_resizeDebounceTimer->stop();
            }
            m_hasPendingGeometries = false;
            return;
        }

        if (m_animateWhileResizing) {
            animateToItemGeometries(geometries);
            return;
        }

        // Debounced: apply immediately for responsiveness, animate once after resize settles.
        QWidget *pw = parentWidget();
        const bool hadUpdates = pw ? pw->updatesEnabled() : true;
        if (pw) {
            pw->setUpdatesEnabled(false);
        }
        applyItemGeometries(geometries);
        if (pw) {
            pw->setUpdatesEnabled(hadUpdates);
            pw->update();
        }

        m_pendingGeometries = geometries;
        m_hasPendingGeometries = true;
        if (m_resizeDebounceTimer) {
            m_resizeDebounceTimer->start(m_resizeDebounceMs);
        }
        return;
    }

    // Non-animated: batch updates to reduce flicker.
    QWidget *pw = parentWidget();
    const bool hadUpdates = pw ? pw->updatesEnabled() : true;
    if (pw) {
        pw->setUpdatesEnabled(false);
    }
    applyItemGeometries(geometries);
    if (pw) {
        pw->setUpdatesEnabled(hadUpdates);
        pw->update();
    }
}

QSize FluentFlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FluentFlowLayout::minimumSize() const
{
    QSize size;
    for (auto *item : m_items) {
        size = size.expandedTo(item->minimumSize());
    }
    const auto m = contentsMargins();
    size += QSize(m.left() + m.right(), m.top() + m.bottom());
    return size;
}

int FluentFlowLayout::horizontalSpacing() const
{
    return m_hSpace >= 0 ? m_hSpace : smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
}

int FluentFlowLayout::verticalSpacing() const
{
    return m_vSpace >= 0 ? m_vSpace : smartSpacing(QStyle::PM_LayoutVerticalSpacing);
}

void FluentFlowLayout::setHorizontalSpacing(int spacing)
{
    m_hSpace = spacing;
    invalidate();
}

void FluentFlowLayout::setVerticalSpacing(int spacing)
{
    m_vSpace = spacing;
    invalidate();
}

void FluentFlowLayout::setUniformItemWidthEnabled(bool enabled)
{
    if (m_uniformWidthEnabled == enabled) {
        return;
    }
    m_uniformWidthEnabled = enabled;
    invalidate();
}

bool FluentFlowLayout::uniformItemWidthEnabled() const
{
    return m_uniformWidthEnabled;
}

void FluentFlowLayout::setMinimumItemWidth(int width)
{
    width = qMax(1, width);
    if (m_minItemWidth == width) {
        return;
    }
    m_minItemWidth = width;
    invalidate();
}

int FluentFlowLayout::minimumItemWidth() const
{
    return m_minItemWidth;
}

void FluentFlowLayout::setColumnHysteresis(int px)
{
    px = qMax(0, px);
    if (m_columnHysteresis == px) {
        return;
    }
    m_columnHysteresis = px;
    invalidate();
}

int FluentFlowLayout::columnHysteresis() const
{
    return m_columnHysteresis;
}

void FluentFlowLayout::setAnimationEnabled(bool enabled)
{
    if (m_animationEnabled == enabled) {
        return;
    }
    m_animationEnabled = enabled;
    // Stop any running animations when disabling.
    if (!m_animationEnabled) {
        if (m_resizeDebounceTimer) {
            m_resizeDebounceTimer->stop();
        }
        m_hasPendingGeometries = false;
        for (auto it = m_animations.begin(); it != m_animations.end(); ++it) {
            if (it.value()) {
                it.value()->stop();
            }
        }
    }
    invalidate();
}

bool FluentFlowLayout::animationEnabled() const
{
    return m_animationEnabled;
}

void FluentFlowLayout::setAnimationDuration(int ms)
{
    ms = qMax(0, ms);
    if (m_animationDurationMs == ms) {
        return;
    }
    m_animationDurationMs = ms;
}

int FluentFlowLayout::animationDuration() const
{
    return m_animationDurationMs;
}

void FluentFlowLayout::setAnimationEasing(const QEasingCurve &curve)
{
    m_animationEasing = curve;
}

QEasingCurve FluentFlowLayout::animationEasing() const
{
    return m_animationEasing;
}

void FluentFlowLayout::setAnimationThrottle(int throttleMs)
{
    throttleMs = qMax(0, throttleMs);
    if (m_animationThrottleMs == throttleMs) {
        return;
    }
    m_animationThrottleMs = throttleMs;
}

int FluentFlowLayout::animationThrottle() const
{
    return m_animationThrottleMs;
}

void FluentFlowLayout::setAnimateWhileResizing(bool enabled)
{
    if (m_animateWhileResizing == enabled) {
        return;
    }
    m_animateWhileResizing = enabled;
    if (m_resizeDebounceTimer) {
        m_resizeDebounceTimer->stop();
    }
    m_hasPendingGeometries = false;
    invalidate();
}

bool FluentFlowLayout::animateWhileResizing() const
{
    return m_animateWhileResizing;
}

void FluentFlowLayout::setResizeAnimationDebounce(int ms)
{
    ms = qMax(0, ms);
    if (m_resizeDebounceMs == ms) {
        return;
    }
    m_resizeDebounceMs = ms;
}

int FluentFlowLayout::resizeAnimationDebounce() const
{
    return m_resizeDebounceMs;
}

int FluentFlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    if (parent() == nullptr) {
        return -1;
    }
    if (parent()->isWidgetType()) {
        auto *pw = static_cast<QWidget *>(parent());
        return pw->style()->pixelMetric(pm, nullptr, pw);
    }
    return static_cast<QLayout *>(parent())->spacing();
}

int FluentFlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    int usedHeight = 0;
    const auto geometries = computeItemGeometries(rect, &usedHeight);
    if (!testOnly) {
        applyItemGeometries(geometries);
    }
    return usedHeight;
}

QList<QRect> FluentFlowLayout::computeItemGeometries(const QRect &rect, int *outUsedHeight) const
{
    const auto m = contentsMargins();
    const QRect effective = rect.adjusted(m.left(), m.top(), -m.right(), -m.bottom());
    int x = effective.x();
    int y = effective.y();
    int lineHeight = 0;
    const int spaceX = horizontalSpacing();
    const int spaceY = verticalSpacing();

    const int availableW = qMax(0, effective.width());
    // Use an exclusive right edge to avoid off-by-one wrapping bugs.
    // QRect::right() is inclusive (x + w - 1), which can cause an item that exactly fits
    // to be treated as overflowing.
    const int rightEdgeExclusive = effective.x() + availableW;

    int uniformW = -1;
    if (m_uniformWidthEnabled && availableW > 0) {
        const int minW = qMax(1, m_minItemWidth);
        const int idealCols = qMax(1, (availableW + spaceX) / (minW + spaceX));
        int cols = idealCols;

        if (m_cachedCols <= 0) {
            cols = idealCols;
        } else if (idealCols > m_cachedCols) {
            const int threshold = (m_cachedCols + 1) * minW + (m_cachedCols) * spaceX;
            cols = (availableW >= threshold + m_columnHysteresis) ? idealCols : m_cachedCols;
        } else if (idealCols < m_cachedCols) {
            const int threshold = (m_cachedCols) * minW + (m_cachedCols - 1) * spaceX;
            cols = (availableW <= threshold - m_columnHysteresis) ? idealCols : m_cachedCols;
        } else {
            cols = m_cachedCols;
        }

        m_cachedCols = qMax(1, cols);
        uniformW = qMax(1, (availableW - (m_cachedCols - 1) * spaceX) / m_cachedCols);
    } else {
        m_cachedCols = 0;
    }

    QList<QRect> result;
    result.reserve(m_items.size());

    for (auto *item : m_items) {
        if (!item) {
            result.append(QRect());
            continue;
        }

        QWidget *w = item->widget();
        const bool breakBefore = w && w->property(kBreakBeforeProperty).toBool();
        const bool breakAfter = w && w->property(kBreakAfterProperty).toBool();
        const bool fullRow = w && w->property(kFullRowProperty).toBool();

        if (breakBefore || fullRow) {
            if (lineHeight > 0 || x != effective.x()) {
                x = effective.x();
                y += lineHeight + spaceY;
                lineHeight = 0;
            }
        }

        int itemW = (uniformW > 0 && !fullRow) ? uniformW : item->sizeHint().width();
        if (fullRow) {
            itemW = availableW;
        }
        itemW = qMax(1, itemW);

        if (!fullRow && lineHeight > 0 && (x + itemW) > rightEdgeExclusive) {
            x = effective.x();
            y += lineHeight + spaceY;
            lineHeight = 0;
        }

        int itemH = 0;
        if (item->hasHeightForWidth()) {
            itemH = item->heightForWidth(itemW);
        } else if (w && w->layout() && w->layout()->hasHeightForWidth()) {
            itemH = w->layout()->totalHeightForWidth(itemW);
        } else {
            itemH = item->sizeHint().height();
        }
        itemH = qMax(1, itemH);

        QRect geom(x, y, itemW, itemH);
        result.append(geom);

        if (fullRow) {
            x = effective.x();
            y += itemH + spaceY;
            lineHeight = 0;
            continue;
        }

        x += itemW + spaceX;
        lineHeight = qMax(lineHeight, itemH);

        if (breakAfter) {
            x = effective.x();
            y += lineHeight + spaceY;
            lineHeight = 0;
        }
    }

    const int usedHeight = (y - rect.y()) + lineHeight + m.top() + m.bottom();
    if (outUsedHeight) {
        *outUsedHeight = usedHeight;
    }

    return result;
}

void FluentFlowLayout::applyItemGeometries(const QList<QRect> &geometries) const
{
    const int n = qMin(geometries.size(), m_items.size());
    for (int i = 0; i < n; ++i) {
        auto *item = m_items.at(i);
        if (!item) {
            continue;
        }
        item->setGeometry(geometries.at(i));
    }
}

void FluentFlowLayout::animateToItemGeometries(const QList<QRect> &geometries)
{
    const int n = qMin(geometries.size(), m_items.size());
    const int nowMs = m_animClock ? static_cast<int>(m_animClock->elapsed()) : 0;

    for (int i = 0; i < n; ++i) {
        auto *item = m_items.at(i);
        if (!item) {
            continue;
        }
        QWidget *w = item->widget();
        if (!w) {
            item->setGeometry(geometries.at(i));
            continue;
        }

        const QRect target = geometries.at(i);
        if (w->geometry() == target) {
            continue;
        }

        auto *anim = m_animations.value(w, nullptr);
        if (!anim) {
            anim = new QPropertyAnimation(w, "geometry", this);
            m_animations.insert(w, anim);
            QObject::connect(w, &QObject::destroyed, this, [this, w]() {
                auto it = m_animations.find(w);
                if (it != m_animations.end()) {
                    if (it.value()) {
                        it.value()->stop();
                        it.value()->deleteLater();
                    }
                    m_animations.erase(it);
                }
                m_animLastRestartMs.remove(w);
            });
        }

        anim->setDuration(m_animationDurationMs);
        anim->setEasingCurve(m_animationEasing);

        // Throttle restarts during continuous resize to avoid stutter.
        const int lastStart = m_animLastRestartMs.value(w, -1000000);
        const bool running = anim->state() == QAbstractAnimation::Running;
        const bool canRestart = (m_animationThrottleMs <= 0) || ((nowMs - lastStart) >= m_animationThrottleMs);

        if (running && !canRestart) {
            // Smoothly steer the current animation to the new target.
            anim->setEndValue(target);
            continue;
        }

        // Restart (infrequently) for responsiveness.
        anim->stop();
        anim->setStartValue(w->geometry());
        anim->setEndValue(target);
        anim->start();
        m_animLastRestartMs.insert(w, nowMs);
    }
}

} // namespace Fluent
