#pragma once

#include <QEasingCurve>
#include <QLayout>
#include <QStyle>

#include <QList>
#include <QHash>

class QPropertyAnimation;
class QTimer;
class QElapsedTimer;

namespace Fluent {

class FluentFlowLayout final : public QLayout
{
public:
    explicit FluentFlowLayout(QWidget *parent = nullptr, int margin = 0, int hSpacing = 12, int vSpacing = 12);
    ~FluentFlowLayout() override;

    void addItem(QLayoutItem *item) override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QLayoutItem *takeAt(int index) override;

    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int heightForWidth(int width) const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QSize minimumSize() const override;

    int horizontalSpacing() const;
    int verticalSpacing() const;

    void setHorizontalSpacing(int spacing);
    void setVerticalSpacing(int spacing);

    // When enabled, items are laid out with a uniform width derived from the available width.
    // The number of columns is computed automatically using minimumItemWidth().
    void setUniformItemWidthEnabled(bool enabled);
    bool uniformItemWidthEnabled() const;

    void setMinimumItemWidth(int width);
    int minimumItemWidth() const;

    // Avoid column-count thrashing near the threshold while resizing.
    // When uniformItemWidthEnabled is true, the layout will keep the current column count until
    // the width crosses the next/previous threshold by this many pixels.
    void setColumnHysteresis(int px);
    int columnHysteresis() const;

    // Animate geometry changes (useful for smoother reflow).
    void setAnimationEnabled(bool enabled);
    bool animationEnabled() const;
    void setAnimationDuration(int ms);
    int animationDuration() const;
    void setAnimationEasing(const QEasingCurve &curve);
    QEasingCurve animationEasing() const;

    // Limits how often animations are restarted during continuous relayout (e.g. window resize).
    // When an animation is already running and the target changes, we update endValue; we only
    // restart if at least throttleMs has elapsed. Set to 0 to always restart.
    void setAnimationThrottle(int throttleMs);
    int animationThrottle() const;

    // If false, layout changes are applied immediately during interactive resize and a single
    // animation is played after resize settles (debounced). This usually feels smoother.
    void setAnimateWhileResizing(bool enabled);
    bool animateWhileResizing() const;

    // Debounce delay (ms) used when animateWhileResizing is false.
    void setResizeAnimationDebounce(int ms);
    int resizeAnimationDebounce() const;

    // Grouping / full-row support:
    // If a widget inside this layout has property `fluentFlowFullRow` set to true,
    // it will start on a new line and occupy the full available width.
    // This makes it easy to implement a "group title + group tiles" structure:
    // add a title label with fluentFlowFullRow=true, then add tiles.
    static constexpr const char *kFullRowProperty = "fluentFlowFullRow";
    static constexpr const char *kBreakBeforeProperty = "fluentFlowBreakBefore";
    static constexpr const char *kBreakAfterProperty = "fluentFlowBreakAfter";

    // If any widget inside this layout has property `fluentFlowDisableAnimation` set to true,
    // the layout will skip geometry animations for that relayout pass (apply immediately).
    // Useful when an item animates its own height (e.g. collapsible card) to avoid flicker.
    static constexpr const char *kDisableAnimationProperty = "fluentFlowDisableAnimation";

private:
    int smartSpacing(QStyle::PixelMetric pm) const;
    int doLayout(const QRect &rect, bool testOnly) const;
    QList<QRect> computeItemGeometries(const QRect &rect, int *outUsedHeight = nullptr) const;
    void applyItemGeometries(const QList<QRect> &geometries) const;
    void animateToItemGeometries(const QList<QRect> &geometries);

    QList<QLayoutItem *> m_items;
    int m_hSpace = 12;
    int m_vSpace = 12;

    bool m_uniformWidthEnabled = false;
    int m_minItemWidth = 320;

    int m_columnHysteresis = 14;
    mutable int m_cachedCols = 0;

    bool m_animationEnabled = false;
    int m_animationDurationMs = 140;
    QEasingCurve m_animationEasing = QEasingCurve(QEasingCurve::OutCubic);

    int m_animationThrottleMs = 50;

    bool m_animateWhileResizing = true;
    int m_resizeDebounceMs = 90;
    QTimer *m_resizeDebounceTimer = nullptr;
    QList<QRect> m_pendingGeometries;
    bool m_hasPendingGeometries = false;

    // Persistent animations per widget to avoid heavy churn during resize.
    QHash<QWidget *, QPropertyAnimation *> m_animations;
    QHash<QWidget *, int> m_animLastRestartMs;
    QElapsedTimer *m_animClock = nullptr;
};

} // namespace Fluent
