#include "Fluent/FluentNavigationView.h"
#include "Fluent/FluentMenu.h"
#include "Fluent/FluentStyle.h"
#include "Fluent/FluentTheme.h"

#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QMouseEvent>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>
#include <QVariantAnimation>

#include <algorithm>
#include <cmath>

namespace Fluent {

namespace {

struct FlatRow {
    enum Kind { Control, Item, Separator };

    Kind kind = Item;
    int  groupIndex = -1;
    int  childIndex = -1;
    bool isFooter = false;
    bool isExpander = false;
    bool expanded = false;
    int  depth = 0;

    const FluentNavigationItem *item = nullptr;

    QString key() const { return item ? item->key : QString(); }
};

struct TopItemLayout {
    int groupIndex = -1;
    bool isFooter = false;
    const FluentNavigationItem *item = nullptr;
    QRectF rect;

    QString key() const { return item ? item->key : QString(); }
};

constexpr int kRowHeight = 40;
constexpr int kSeparatorHeight = 9;
constexpr int kIconSize = 20;
constexpr int kIndicatorWidth = 3;
constexpr int kIndicatorHeight = 16;
constexpr int kTopBarHeight = 48;
constexpr int kDefaultExpandedWidth = 280;
constexpr int kDefaultCompactWidth = 48;
constexpr int kDepthIndent = 28;
constexpr int kItemPaddingX = 14;
constexpr int kControlButtonSize = 28;
constexpr int kControlButtonGap = 8;
constexpr int kTopItemGap = 4;

QString defaultGlyphFontFamily()
{
    return QStringLiteral("Segoe Fluent Icons");
}

QRectF expanderHitRect(const QRectF &rowRect)
{
    return QRectF(rowRect.right() - 36.0, rowRect.top(), 36.0, rowRect.height());
}

QRectF topExpanderHitRect(const QRectF &itemRect)
{
    return QRectF(itemRect.right() - 24.0, itemRect.top(), 24.0, itemRect.height());
}

} // namespace

struct FluentNavigationView::Private
{
    using PaneDisplayMode = FluentNavigationView::PaneDisplayMode;

    std::vector<FluentNavigationItem> items;
    std::vector<FluentNavigationItem> footerItems;
    std::vector<FlatRow> rows;

    QString selectedKey;
    int hoverRowIndex = -1;

    PaneDisplayMode displayMode = FluentNavigationView::Left;
    int expandedWidth = kDefaultExpandedWidth;
    int compactWidth = kDefaultCompactWidth;
    int currentWidth = kDefaultExpandedWidth;
    int autoCollapseThresh = 0;

    QWidget *headerWidget = nullptr;
    QWidget *observedParent = nullptr;
    QPointer<FluentMenu> itemFlyout;
    QString itemFlyoutParentKey;

    std::vector<QString> expandedGroups;

    QVariantAnimation *widthAnim = nullptr;
    QVariantAnimation *hoverAnim = nullptr;
    qreal hoverLevel = 0.0;

    QVariantAnimation *selAnim = nullptr;
    QRectF selRect;
    QRectF selStartRect;
    QRectF selTargetRect;
    qreal selOpacity = 0.0;

    bool backButtonVisible = false;
    bool backButtonEnabled = true;
    bool footerVisible = true;
    QString paneTitle;

    bool isExpandedMode() const
    {
        return displayMode == FluentNavigationView::Left;
    }

    bool isCompactMode() const
    {
        return displayMode == FluentNavigationView::LeftCompact;
    }

    bool isGroupExpanded(const QString &key) const
    {
        return std::find(expandedGroups.begin(), expandedGroups.end(), key) != expandedGroups.end();
    }

    void closeItemFlyout()
    {
        if (itemFlyout) {
            itemFlyout->close();
            itemFlyout->deleteLater();
            itemFlyout = nullptr;
        }
        itemFlyoutParentKey.clear();
    }

    void setGroupExpanded(const QString &key, bool expanded, bool exclusive = true)
    {
        if (expanded) {
            if (exclusive) {
                expandedGroups.clear();
            }
            if (std::find(expandedGroups.begin(), expandedGroups.end(), key) == expandedGroups.end()) {
                expandedGroups.push_back(key);
            }
        } else {
            expandedGroups.erase(std::remove(expandedGroups.begin(), expandedGroups.end(), key), expandedGroups.end());
        }
    }

    void rebuildRows()
    {
        rows.clear();

        if (displayMode == FluentNavigationView::Top) {
            return;
        }

        FlatRow control;
        control.kind = FlatRow::Control;
        rows.push_back(control);

        auto addItems = [&](const std::vector<FluentNavigationItem> &source, bool footer) {
            for (int i = 0; i < static_cast<int>(source.size()); ++i) {
                const auto &item = source[static_cast<size_t>(i)];

                if (item.separator) {
                    FlatRow sep;
                    sep.kind = FlatRow::Separator;
                    sep.isFooter = footer;
                    rows.push_back(sep);
                    continue;
                }

                FlatRow row;
                row.kind = FlatRow::Item;
                row.groupIndex = i;
                row.childIndex = -1;
                row.isFooter = footer;
                row.depth = 0;
                row.item = &item;
                row.isExpander = !item.children.empty();
                row.expanded = row.isExpander && isGroupExpanded(item.key);
                rows.push_back(row);

                if (isExpandedMode() && row.isExpander && row.expanded) {
                    for (int c = 0; c < static_cast<int>(item.children.size()); ++c) {
                        FlatRow child;
                        child.kind = FlatRow::Item;
                        child.groupIndex = i;
                        child.childIndex = c;
                        child.isFooter = footer;
                        child.depth = 1;
                        child.item = &item.children[static_cast<size_t>(c)];
                        rows.push_back(child);
                    }
                }
            }
        };

        addItems(items, false);

        if (footerVisible && !footerItems.empty()) {
            FlatRow sep;
            sep.kind = FlatRow::Separator;
            rows.push_back(sep);
            addItems(footerItems, true);
        }
    }

    int labelRevealWidth() const
    {
        const int transition = qMax(0, expandedWidth - compactWidth);
        return qMax(compactWidth + 72, compactWidth + (transition * 35) / 100);
    }

    int headerRevealWidth() const
    {
        const int transition = qMax(0, expandedWidth - compactWidth);
        return qMax(compactWidth + 120, compactWidth + (transition * 55) / 100);
    }

    bool showLabels() const
    {
        if (displayMode == FluentNavigationView::Top) {
            return true;
        }
        return isExpandedMode() && currentWidth >= labelRevealWidth();
    }

    bool showHeader() const
    {
        return headerWidget && isExpandedMode() && currentWidth >= headerRevealWidth();
    }

    qreal labelOpacity() const
    {
        if (displayMode == FluentNavigationView::Top) {
            return 1.0;
        }
        if (!isExpandedMode()) {
            return 0.0;
        }

        const int start = labelRevealWidth();
        const int end = start + 36;
        if (currentWidth <= start) {
            return 0.0;
        }
        if (currentWidth >= end) {
            return 1.0;
        }
        return qreal(currentWidth - start) / qreal(end - start);
    }

    int headerHeight() const
    {
        if (showHeader()) {
            return headerWidget->sizeHint().height();
        }
        return 0;
    }

    int rowHeight(int index) const
    {
        if (index < 0 || index >= static_cast<int>(rows.size())) {
            return 0;
        }
        return rows[static_cast<size_t>(index)].kind == FlatRow::Separator ? kSeparatorHeight : kRowHeight;
    }

    int rowY(int index) const
    {
        int y = headerHeight();
        for (int i = 0; i < index && i < static_cast<int>(rows.size()); ++i) {
            y += rowHeight(i);
        }
        return y;
    }

    int totalHeight() const
    {
        if (displayMode == FluentNavigationView::Top) {
            return kTopBarHeight;
        }

        int height = headerHeight();
        for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
            height += rowHeight(i);
        }
        return height;
    }

    int footerStartRow() const
    {
        for (int i = static_cast<int>(rows.size()) - 1; i >= 0; --i) {
            const auto &row = rows[static_cast<size_t>(i)];
            if (!row.isFooter && row.kind != FlatRow::Separator) {
                return i + 1;
            }
            if (row.kind == FlatRow::Separator && i > 0 && !rows[static_cast<size_t>(i) - 1].isFooter) {
                return i;
            }
        }
        return static_cast<int>(rows.size());
    }

    int footerHeightFromRow(int startRow) const
    {
        int footerHeight = 0;
        for (int i = startRow; i < static_cast<int>(rows.size()); ++i) {
            footerHeight += rowHeight(i);
        }
        return footerHeight;
    }

    int mainHeightUntilRow(int startRow) const
    {
        int y = headerHeight();
        for (int i = 0; i < startRow && i < static_cast<int>(rows.size()); ++i) {
            y += rowHeight(i);
        }
        return y;
    }

    int footerBaseY(int widgetHeight) const
    {
        const int startRow = footerStartRow();
        const int footerHeight = footerHeightFromRow(startRow);
        const int pinnedY = widgetHeight - footerHeight;
        const int stackedY = mainHeightUntilRow(startRow);
        return qMax(pinnedY, stackedY);
    }

    QRectF controlBackRect(const QRectF &rowRect) const
    {
        if (!backButtonVisible) {
            return QRectF();
        }

        return QRectF(rowRect.left() + 8.0,
                      rowRect.center().y() - kControlButtonSize / 2.0,
                      kControlButtonSize,
                      kControlButtonSize);
    }

    QRectF controlHamburgerRect(const QRectF &rowRect) const
    {
        qreal x = rowRect.left() + 8.0;
        if (backButtonVisible) {
            x += kControlButtonSize + kControlButtonGap;
        }

        return QRectF(x,
                      rowRect.center().y() - kControlButtonSize / 2.0,
                      kControlButtonSize,
                      kControlButtonSize);
    }

    QRectF topBackButtonRect() const
    {
        if (!backButtonVisible) {
            return QRectF();
        }

        return QRectF(12.0,
                      (kTopBarHeight - kControlButtonSize) / 2.0,
                      kControlButtonSize,
                      kControlButtonSize);
    }

    QString visibleKeyForSelection(const QString &key) const
    {
        if (key.isEmpty()) {
            return QString();
        }

        auto topLevelKeyFor = [&key](const std::vector<FluentNavigationItem> &source) {
            for (const auto &item : source) {
                if (item.key == key) {
                    return item.key;
                }
                for (const auto &child : item.children) {
                    if (child.key == key) {
                        return item.key;
                    }
                }
            }
            return QString();
        };

        const QString mainKey = topLevelKeyFor(items);
        if (!mainKey.isEmpty()) {
            return mainKey;
        }

        if (!footerVisible) {
            return QString();
        }

        return topLevelKeyFor(footerItems);
    }

    int topItemWidth(const FluentNavigationItem &item, const QFontMetrics &fm) const
    {
        int width = 24;
        if (!item.iconGlyph.isEmpty() || !item.icon.isNull()) {
            width += kIconSize + 8;
        }

        width += fm.horizontalAdvance(item.text);
        if (!item.children.empty()) {
            width += 18;
        }

        return qMax(56, width);
    }

    std::vector<TopItemLayout> topLayouts(int widgetWidth) const
    {
        std::vector<TopItemLayout> result;
        if (displayMode != FluentNavigationView::Top) {
            return result;
        }

        QFont textFont = QApplication::font();
        textFont.setPixelSize(14);
        const QFontMetrics fm(textFont);

        int left = 12;
        if (backButtonVisible) {
            left = qRound(topBackButtonRect().right()) + 10;
        }

        int right = widgetWidth - 12;
        std::vector<TopItemLayout> footerLayouts;
        if (footerVisible && !footerItems.empty()) {
            for (int i = static_cast<int>(footerItems.size()) - 1; i >= 0; --i) {
                const auto &item = footerItems[static_cast<size_t>(i)];
                if (item.separator) {
                    right -= 12;
                    continue;
                }

                const int itemWidth = topItemWidth(item, fm);
                right -= itemWidth;
                if (right <= left) {
                    right += itemWidth;
                    break;
                }

                TopItemLayout layout;
                layout.groupIndex = i;
                layout.isFooter = true;
                layout.item = &item;
                layout.rect = QRectF(right, 6.0, itemWidth, kTopBarHeight - 12.0);
                footerLayouts.push_back(layout);
                right -= kTopItemGap;
            }

            std::reverse(footerLayouts.begin(), footerLayouts.end());
        }

        for (int i = 0; i < static_cast<int>(items.size()); ++i) {
            const auto &item = items[static_cast<size_t>(i)];
            if (item.separator) {
                left += 12;
                continue;
            }

            const int itemWidth = topItemWidth(item, fm);
            if (left + itemWidth >= right - 8) {
                break;
            }

            TopItemLayout layout;
            layout.groupIndex = i;
            layout.item = &item;
            layout.rect = QRectF(left, 6.0, itemWidth, kTopBarHeight - 12.0);
            result.push_back(layout);
            left += itemWidth + kTopItemGap;
        }

        result.insert(result.end(), footerLayouts.begin(), footerLayouts.end());
        return result;
    }

    int topHitTest(const QPoint &pos, int widgetWidth) const
    {
        const auto layouts = topLayouts(widgetWidth);
        for (int i = 0; i < static_cast<int>(layouts.size()); ++i) {
            if (layouts[static_cast<size_t>(i)].rect.contains(pos)) {
                return i;
            }
        }
        return -1;
    }

    int hitTest(const QPoint &pos, int widgetHeight) const
    {
        const int startRow = footerStartRow();

        int y = headerHeight();
        for (int i = 0; i < startRow && i < static_cast<int>(rows.size()); ++i) {
            const int rowHeightValue = rowHeight(i);
            if (pos.y() >= y && pos.y() < y + rowHeightValue) {
                return i;
            }
            y += rowHeightValue;
        }

        y = footerBaseY(widgetHeight);
        for (int i = startRow; i < static_cast<int>(rows.size()); ++i) {
            const int rowHeightValue = rowHeight(i);
            if (pos.y() >= y && pos.y() < y + rowHeightValue) {
                return i;
            }
            y += rowHeightValue;
        }

        return -1;
    }

    QRectF rowRect(int index, int widgetWidth, int widgetHeight) const
    {
        const int startRow = footerStartRow();
        if (index < startRow) {
            return QRectF(0, rowY(index), widgetWidth, rowHeight(index));
        }

        int y = footerBaseY(widgetHeight);
        for (int i = startRow; i < index; ++i) {
            y += rowHeight(i);
        }
        return QRectF(0, y, widgetWidth, rowHeight(index));
    }

    QRectF topSelectionRectForKey(const QString &key, int widgetWidth) const
    {
        const QString visibleKey = visibleKeyForSelection(key);
        if (visibleKey.isEmpty()) {
            return QRectF();
        }

        const auto layouts = topLayouts(widgetWidth);
        for (const auto &layout : layouts) {
            if (layout.key() == visibleKey) {
                return layout.rect.adjusted(4.0, 3.0, -4.0, -3.0);
            }
        }

        return QRectF();
    }

    QRectF selectionRectForKey(const QString &key, int widgetWidth, int widgetHeight) const
    {
        if (displayMode == FluentNavigationView::Top) {
            return topSelectionRectForKey(key, widgetWidth);
        }

        auto rectForVisibleKey = [&](const QString &visibleKey) {
            for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
                if (rows[static_cast<size_t>(i)].key() == visibleKey) {
                    const QRectF row = rowRect(i, widgetWidth, widgetHeight);
                    return row.adjusted(4.0, 2.0, -4.0, -2.0);
                }
            }
            return QRectF();
        };

        const QRectF directRect = rectForVisibleKey(key);
        if (directRect.isValid()) {
            return directRect;
        }

        if (displayMode == FluentNavigationView::LeftCompact) {
            const QString fallbackKey = visibleKeyForSelection(key);
            if (!fallbackKey.isEmpty()) {
                return rectForVisibleKey(fallbackKey);
            }
        }

        return QRectF();
    }

    bool ensureKeyVisible(const QString &key)
    {
        if (key.isEmpty()) {
            return false;
        }

        auto ensureInItems = [this, &key](const std::vector<FluentNavigationItem> &source) {
            for (const auto &item : source) {
                if (item.key == key) {
                    return true;
                }
                for (const auto &child : item.children) {
                    if (child.key == key) {
                        if (displayMode != FluentNavigationView::Top && !item.key.isEmpty()) {
                            setGroupExpanded(item.key, true, true);
                        }
                        return true;
                    }
                }
            }
            return false;
        };

        return ensureInItems(items) || ensureInItems(footerItems);
    }

    void showItemFlyout(FluentNavigationView *owner, const FluentNavigationItem *item, const QPoint &popupPos)
    {
        if (!owner || !item || item->children.empty()) {
            return;
        }

        if (itemFlyout && itemFlyoutParentKey == item->key && itemFlyout->isVisible()) {
            closeItemFlyout();
            return;
        }

        closeItemFlyout();

        auto *menu = new FluentMenu(owner);
        menu->setAttribute(Qt::WA_DeleteOnClose, true);
        itemFlyout = menu;
        itemFlyoutParentKey = item->key;

        QObject::connect(menu, &QObject::destroyed, owner, [this]() {
            itemFlyout = nullptr;
            itemFlyoutParentKey.clear();
        });

        QAction *activeAction = nullptr;
        for (const auto &child : item->children) {
            QAction *action = menu->addAction(child.text);
            if (!child.icon.isNull()) {
                action->setIcon(child.icon);
            }

            QObject::connect(action, &QAction::triggered, owner, [owner, this, childKey = child.key, selectOnInvoke = child.selectsOnInvoked]() {
                if (selectOnInvoke && !childKey.isEmpty()) {
                    owner->setSelectedKey(childKey);
                }
                if (!childKey.isEmpty()) {
                    emit owner->itemInvoked(childKey);
                }
                closeItemFlyout();
            });

            if (child.key == selectedKey) {
                activeAction = action;
            }
        }

        menu->popup(popupPos, activeAction);
    }
};

FluentNavigationView::FluentNavigationView(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Private>())
{
    setMouseTracking(true);

    d->widthAnim = new QVariantAnimation(this);
    d->widthAnim->setDuration(200);
    d->widthAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->widthAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        d->currentWidth = value.toInt();
        updateModeGeometry();
        update();
    });

    d->hoverAnim = new QVariantAnimation(this);
    d->hoverAnim->setDuration(120);
    connect(d->hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        d->hoverLevel = value.toReal();
        update();
    });

    d->selAnim = new QVariantAnimation(this);
    d->selAnim->setDuration(180);
    d->selAnim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(d->selAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        const qreal t = qBound<qreal>(0.0, value.toReal(), 1.0);
        d->selRect = QRectF(
            d->selStartRect.x() + (d->selTargetRect.x() - d->selStartRect.x()) * t,
            d->selStartRect.y() + (d->selTargetRect.y() - d->selStartRect.y()) * t,
            d->selStartRect.width() + (d->selTargetRect.width() - d->selStartRect.width()) * t,
            d->selStartRect.height() + (d->selTargetRect.height() - d->selStartRect.height()) * t);
        d->selOpacity = t;
        update();
    });
    connect(d->selAnim, &QVariantAnimation::finished, this, [this]() {
        d->selOpacity = 1.0;
        update();
    });

    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, QOverload<>::of(&QWidget::update));

    d->rebuildRows();
    updateModeGeometry();
}

FluentNavigationView::~FluentNavigationView() = default;

void FluentNavigationView::setItems(const std::vector<FluentNavigationItem> &items)
{
    d->items = items;
    d->ensureKeyVisible(d->selectedKey);
    d->rebuildRows();
    d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
    d->selStartRect = d->selRect;
    d->selTargetRect = d->selRect;
    updateGeometry();
    update();
}

std::vector<FluentNavigationItem> FluentNavigationView::items() const
{
    return d->items;
}

void FluentNavigationView::addItem(const FluentNavigationItem &item)
{
    d->items.push_back(item);
    d->rebuildRows();
    d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
    d->selStartRect = d->selRect;
    d->selTargetRect = d->selRect;
    updateGeometry();
    update();
}

void FluentNavigationView::clearItems()
{
    d->items.clear();
    d->expandedGroups.clear();
    d->rebuildRows();
    d->selRect = QRectF();
    d->selStartRect = QRectF();
    d->selTargetRect = QRectF();
    updateGeometry();
    update();
}

void FluentNavigationView::setFooterItems(const std::vector<FluentNavigationItem> &items)
{
    d->footerItems = items;
    d->rebuildRows();
    d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
    d->selStartRect = d->selRect;
    d->selTargetRect = d->selRect;
    updateGeometry();
    update();
}

std::vector<FluentNavigationItem> FluentNavigationView::footerItems() const
{
    return d->footerItems;
}

void FluentNavigationView::addFooterItem(const FluentNavigationItem &item)
{
    d->footerItems.push_back(item);
    d->rebuildRows();
    d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
    d->selStartRect = d->selRect;
    d->selTargetRect = d->selRect;
    updateGeometry();
    update();
}

void FluentNavigationView::clearFooterItems()
{
    d->footerItems.clear();
    d->rebuildRows();
    d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
    d->selStartRect = d->selRect;
    d->selTargetRect = d->selRect;
    updateGeometry();
    update();
}

QString FluentNavigationView::selectedKey() const
{
    return d->selectedKey;
}

void FluentNavigationView::setSelectedKey(const QString &key)
{
    if (d->selectedKey == key) {
        return;
    }

    const QRectF oldRect = d->selectionRectForKey(d->selectedKey, width(), height());
    d->ensureKeyVisible(key);
    d->rebuildRows();
    d->selectedKey = key;
    const QRectF newRect = d->selectionRectForKey(key, width(), height());

    const bool animRunning = d->selAnim->state() == QAbstractAnimation::Running;
    d->selStartRect = animRunning ? d->selRect : oldRect;
    d->selTargetRect = newRect;
    d->selAnim->stop();
    d->selAnim->setStartValue(0.0);
    d->selAnim->setEndValue(1.0);
    d->selAnim->start();

    updateGeometry();
    emit selectedKeyChanged(key);
    update();
}

FluentNavigationView::PaneDisplayMode FluentNavigationView::paneDisplayMode() const
{
    return d->displayMode;
}

void FluentNavigationView::setPaneDisplayMode(PaneDisplayMode mode)
{
    if (d->displayMode == mode) {
        return;
    }

    const PaneDisplayMode oldMode = d->displayMode;
    const bool oldExpanded = isExpanded();

    d->closeItemFlyout();
    d->displayMode = mode;
    d->ensureKeyVisible(d->selectedKey);
    d->rebuildRows();

    if (mode == Top) {
        d->widthAnim->stop();
        d->currentWidth = d->expandedWidth;
        updateModeGeometry();
    } else if (oldMode == Top) {
        d->widthAnim->stop();
        d->currentWidth = (mode == Left) ? d->expandedWidth : d->compactWidth;
        updateModeGeometry();
    } else {
        const int target = (mode == Left) ? d->expandedWidth : d->compactWidth;
        d->widthAnim->stop();
        d->widthAnim->setStartValue(d->currentWidth);
        d->widthAnim->setEndValue(target);
        d->widthAnim->start();
    }

    d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
    d->selStartRect = d->selRect;
    d->selTargetRect = d->selRect;
    if (d->headerWidget) {
        d->headerWidget->setVisible(d->showHeader());
    }

    emit paneDisplayModeChanged(mode);
    if (oldExpanded != isExpanded()) {
        emit expandedChanged(isExpanded());
    }
    updateGeometry();
    update();
}

bool FluentNavigationView::isExpanded() const
{
    return d->displayMode == Left;
}

void FluentNavigationView::setExpanded(bool expanded)
{
    setPaneDisplayMode(expanded ? Left : LeftCompact);
}

void FluentNavigationView::toggleExpanded()
{
    if (d->displayMode == Top) {
        return;
    }
    setExpanded(!isExpanded());
}

int FluentNavigationView::expandedWidth() const
{
    return d->expandedWidth;
}

void FluentNavigationView::setExpandedWidth(int w)
{
    d->expandedWidth = qMax(w, d->compactWidth + 32);
    if (d->displayMode == Left) {
        d->currentWidth = d->expandedWidth;
        updateModeGeometry();
        update();
    }
}

int FluentNavigationView::compactWidth() const
{
    return d->compactWidth;
}

void FluentNavigationView::setCompactWidth(int w)
{
    d->compactWidth = qMax(36, w);
    d->expandedWidth = qMax(d->expandedWidth, d->compactWidth + 32);
    if (d->displayMode == LeftCompact) {
        d->currentWidth = d->compactWidth;
        updateModeGeometry();
        update();
    }
}

void FluentNavigationView::setHeaderWidget(QWidget *widget)
{
    if (d->headerWidget) {
        d->headerWidget->setParent(nullptr);
    }

    d->headerWidget = widget;
    if (widget) {
        widget->setParent(this);
        widget->setVisible(d->showHeader());
    }

    updateGeometry();
    update();
}

bool FluentNavigationView::isBackButtonVisible() const
{
    return d->backButtonVisible;
}

void FluentNavigationView::setBackButtonVisible(bool visible)
{
    if (d->backButtonVisible == visible) {
        return;
    }

    d->backButtonVisible = visible;
    emit backButtonVisibleChanged(visible);
    update();
}

bool FluentNavigationView::isBackButtonEnabled() const
{
    return d->backButtonEnabled;
}

void FluentNavigationView::setBackButtonEnabled(bool enabled)
{
    if (d->backButtonEnabled == enabled) {
        return;
    }

    d->backButtonEnabled = enabled;
    emit backButtonEnabledChanged(enabled);
    update();
}

bool FluentNavigationView::isFooterVisible() const
{
    return d->footerVisible;
}

void FluentNavigationView::setFooterVisible(bool visible)
{
    if (d->footerVisible == visible) {
        return;
    }

    d->footerVisible = visible;
    d->rebuildRows();
    d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
    d->selStartRect = d->selRect;
    d->selTargetRect = d->selRect;
    emit footerVisibleChanged(visible);
    updateGeometry();
    update();
}

QString FluentNavigationView::paneTitle() const
{
    return d->paneTitle;
}

void FluentNavigationView::setPaneTitle(const QString &title)
{
    if (d->paneTitle == title) {
        return;
    }

    d->paneTitle = title;
    emit paneTitleChanged(title);
    update();
}

void FluentNavigationView::setAutoCollapseWidth(int threshold)
{
    d->autoCollapseThresh = threshold;
    syncAutoCollapseState();
}

int FluentNavigationView::autoCollapseWidth() const
{
    return d->autoCollapseThresh;
}

QSize FluentNavigationView::sizeHint() const
{
    if (d->displayMode == Top) {
        return QSize(640, d->totalHeight());
    }
    return QSize(d->currentWidth, d->totalHeight());
}

QSize FluentNavigationView::minimumSizeHint() const
{
    if (d->displayMode == Top) {
        return QSize(280, d->totalHeight());
    }
    return QSize(d->compactWidth, d->totalHeight());
}

void FluentNavigationView::syncAutoCollapseState()
{
    if (d->autoCollapseThresh <= 0 || !d->observedParent || d->displayMode == Top) {
        return;
    }

    const int parentWidth = d->observedParent->width();
    if (d->displayMode == Left && parentWidth < d->autoCollapseThresh) {
        setPaneDisplayMode(LeftCompact);
    } else if (d->displayMode == LeftCompact && parentWidth >= d->autoCollapseThresh + 100) {
        setPaneDisplayMode(Left);
    }
}

void FluentNavigationView::updateParentEventFilter()
{
    QWidget *nextParent = parentWidget();
    if (d->observedParent == nextParent) {
        return;
    }

    if (d->observedParent) {
        d->observedParent->removeEventFilter(this);
    }

    d->observedParent = nextParent;

    if (d->observedParent) {
        d->observedParent->installEventFilter(this);
    }
}

void FluentNavigationView::updateModeGeometry()
{
    if (d->displayMode == Top) {
        setMinimumWidth(0);
        setMaximumWidth(QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        const int height = d->totalHeight();
        setMinimumHeight(height);
        setMaximumHeight(height);
    } else {
        setMinimumHeight(0);
        setMaximumHeight(QWIDGETSIZE_MAX);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        setMinimumWidth(d->currentWidth);
        setMaximumWidth(d->currentWidth);
    }

    if (d->headerWidget) {
        d->headerWidget->setVisible(d->showHeader());
    }

    updateGeometry();
}

void FluentNavigationView::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    if (!p.isActive()) {
        return;
    }

    p.setRenderHint(QPainter::Antialiasing, true);

    const auto &colors = ThemeManager::instance().colors();
    const int W = width();
    const int H = height();
    const qreal labelOpacity = d->labelOpacity();

    p.setPen(Qt::NoPen);
    p.setBrush(colors.surface);
    p.drawRoundedRect(QRectF(0, 0, W, H), 8.0, 8.0);

    auto drawHamburger = [&](const QRectF &rect, const QColor &color) {
        const QPointF center = rect.center();
        const qreal barW = 16.0;
        const qreal barH = 2.0;
        const qreal gap = 5.0;
        p.setPen(Qt::NoPen);
        p.setBrush(color);
        for (int i = -1; i <= 1; ++i) {
            QRectF bar(center.x() - barW / 2.0, center.y() + i * gap - barH / 2.0, barW, barH);
            p.drawRoundedRect(bar, 1.0, 1.0);
        }
    };

    auto drawBack = [&](const QRectF &rect, bool enabled) {
        Style::drawChevronLeft(p, rect.center(), enabled ? colors.text : colors.subText, 8.0, 1.7);
    };

    auto drawChevron = [&](const QPointF &center, bool collapsed) {
        if (collapsed) {
            Style::drawChevronDown(p, center, colors.subText, 6.0, 1.4);
        } else {
            Style::drawChevronUp(p, center, colors.subText, 6.0, 1.4);
        }
    };

    auto drawItemIcon = [&](const QRectF &iconRect, const FluentNavigationItem &item) {
        if (!item.iconGlyph.isEmpty()) {
            QFont iconFont = font();
            iconFont.setFamily(item.iconFontFamily.isEmpty() ? defaultGlyphFontFamily() : item.iconFontFamily);
            iconFont.setPixelSize(kIconSize);
            iconFont.setStyleStrategy(QFont::PreferAntialias);

            p.save();
            p.setFont(iconFont);
            p.setPen(colors.text);
            p.drawText(iconRect, Qt::AlignCenter, item.iconGlyph);
            p.restore();
        } else if (!item.icon.isNull()) {
            item.icon.paint(&p,
                            QRect(static_cast<int>(iconRect.left()),
                                  static_cast<int>(iconRect.top()),
                                  static_cast<int>(iconRect.width()),
                                  static_cast<int>(iconRect.height())));
        }
    };

    if (d->displayMode == Top) {
        const auto layouts = d->topLayouts(W);
        const QString selectedVisibleKey = d->visibleKeyForSelection(d->selectedKey);

        if (d->backButtonVisible) {
            const QRectF backRect = d->topBackButtonRect();
            QColor buttonFill = colors.hover;
            buttonFill.setAlpha(38);
            p.setBrush(buttonFill);
            p.drawRoundedRect(backRect, 4.0, 4.0);
            drawBack(backRect, d->backButtonEnabled);
        }

        p.setPen(QPen(colors.border, 1.0));
        p.drawLine(QPointF(12.0, H - 0.5), QPointF(W - 12.0, H - 0.5));

        for (int i = 0; i < static_cast<int>(layouts.size()); ++i) {
            const auto &layout = layouts[static_cast<size_t>(i)];
            if (!layout.item) {
                continue;
            }

            const bool hovered = i == d->hoverRowIndex && d->hoverLevel > 0.0;
            const bool selected = layout.key() == selectedVisibleKey;

            if (hovered) {
                QColor hover = colors.hover;
                hover.setAlphaF(0.35 * d->hoverLevel);
                p.setPen(Qt::NoPen);
                p.setBrush(hover);
                p.drawRoundedRect(layout.rect, 5.0, 5.0);
            }

            if (selected) {
                QColor fill = colors.accent;
                fill.setAlpha(24);
                p.setPen(Qt::NoPen);
                p.setBrush(fill);
                p.drawRoundedRect(layout.rect, 5.0, 5.0);

                QRectF lineRect(layout.rect.left() + 10.0, layout.rect.bottom() - 3.0, layout.rect.width() - 20.0, 3.0);
                p.setBrush(colors.accent);
                p.drawRoundedRect(lineRect, 1.5, 1.5);
            }

            qreal x = layout.rect.left() + 12.0;
            if (!layout.item->iconGlyph.isEmpty() || !layout.item->icon.isNull()) {
                QRectF iconRect(x, layout.rect.center().y() - kIconSize / 2.0, kIconSize, kIconSize);
                drawItemIcon(iconRect, *layout.item);
                x = iconRect.right() + 8.0;
            }

            qreal textRight = layout.rect.right() - 12.0;
            if (!layout.item->children.empty()) {
                textRight -= 14.0;
            }

            const int textWidth = qMax(0, static_cast<int>(textRight - x));
            if (textWidth > 0) {
                QFont textFont = font();
                textFont.setPixelSize(14);
                const QFontMetrics fm(textFont);
                const QString text = fm.elidedText(layout.item->text, Qt::ElideRight, textWidth);

                p.save();
                p.setPen(colors.text);
                p.setFont(textFont);
                p.drawText(QRectF(x, layout.rect.top(), textWidth, layout.rect.height()),
                           Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine,
                           text);
                p.restore();
            }

            if (!layout.item->children.empty()) {
                drawChevron(topExpanderHitRect(layout.rect).center(), true);
            }
        }

        return;
    }

    const int footerStartRow = d->footerStartRow();

    const bool selectionAnimating = d->selAnim->state() == QAbstractAnimation::Running;
    const QRectF selectionRect = selectionAnimating ? d->selRect : d->selectionRectForKey(d->selectedKey, W, H);
    const qreal selectionOpacity = selectionAnimating ? qBound<qreal>(0.0, d->selOpacity, 1.0) : (selectionRect.isValid() ? 1.0 : 0.0);
    if (selectionRect.isValid() && selectionOpacity > 0.0) {
        QColor fill = colors.accent;
        fill.setAlpha(qBound(0, static_cast<int>(std::lround(30.0 * selectionOpacity)), 30));
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawRoundedRect(selectionRect, 4.0, 4.0);

        QColor indicator = colors.accent;
        indicator.setAlpha(qBound(0, static_cast<int>(std::lround(255.0 * selectionOpacity)), 255));
        p.setBrush(indicator);
        QRectF indicatorRect(selectionRect.left(), selectionRect.center().y() - kIndicatorHeight / 2.0, kIndicatorWidth, kIndicatorHeight);
        p.drawRoundedRect(indicatorRect, 1.5, 1.5);
    }

    auto paintRow = [&](int index, const QRectF &rect) {
        const auto &row = d->rows[static_cast<size_t>(index)];

        if (row.kind == FlatRow::Separator) {
            p.setPen(QPen(colors.border, 1.0));
            p.drawLine(QPointF(rect.left() + 12.0, rect.center().y()), QPointF(rect.right() - 12.0, rect.center().y()));
            return;
        }

        if (row.kind == FlatRow::Control) {
            const QRectF backRect = d->controlBackRect(rect);
            const QRectF hamburgerRect = d->controlHamburgerRect(rect);
            QColor buttonFill = colors.hover;
            buttonFill.setAlpha(34);

            if (d->backButtonVisible) {
                p.setPen(Qt::NoPen);
                p.setBrush(buttonFill);
                p.drawRoundedRect(backRect, 4.0, 4.0);
                drawBack(backRect, d->backButtonEnabled);
            }

            p.setPen(Qt::NoPen);
            p.setBrush(buttonFill);
            p.drawRoundedRect(hamburgerRect, 4.0, 4.0);
            drawHamburger(hamburgerRect, colors.text);

            if (labelOpacity > 0.0 && !d->paneTitle.isEmpty()) {
                const qreal textLeft = hamburgerRect.right() + 10.0;
                const qreal textRight = rect.right() - 12.0;
                if (textRight > textLeft) {
                    QFont titleFont = font();
                    titleFont.setPixelSize(15);
                    titleFont.setWeight(QFont::DemiBold);
                    const QFontMetrics fm(titleFont);
                    const QString text = fm.elidedText(d->paneTitle, Qt::ElideRight, qMax(0, static_cast<int>(textRight - textLeft)));

                    p.save();
                    p.setOpacity(labelOpacity);
                    p.setPen(colors.text);
                    p.setFont(titleFont);
                    p.drawText(QRectF(textLeft, rect.top(), textRight - textLeft, rect.height()),
                               Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine,
                               text);
                    p.restore();
                }
            }

            return;
        }

        if (!row.item) {
            return;
        }

        if (index == d->hoverRowIndex && d->hoverLevel > 0.0) {
            QColor hover = colors.hover;
            hover.setAlphaF(0.3 * d->hoverLevel);
            p.setPen(Qt::NoPen);
            p.setBrush(hover);
            p.drawRoundedRect(rect.adjusted(4.0, 2.0, -4.0, -2.0), 4.0, 4.0);
        }

        const int indent = row.depth * kDepthIndent;
        const QRectF iconRect(rect.left() + indent + kItemPaddingX,
                              rect.center().y() - kIconSize / 2.0,
                              kIconSize,
                              kIconSize);
        drawItemIcon(iconRect, *row.item);

        if (labelOpacity > 0.0) {
            const int textX = static_cast<int>(rect.left()) + indent + kItemPaddingX + kIconSize + 12;
            const int textWidth = static_cast<int>(rect.width()) - textX - 32;
            if (textWidth > 0) {
                QFont textFont = font();
                textFont.setPixelSize(14);
                const QFontMetrics fm(textFont);
                const QString text = fm.elidedText(row.item->text, Qt::ElideRight, textWidth);
                p.save();
                p.setOpacity(labelOpacity);
                p.setPen(colors.text);
                p.setFont(textFont);
                p.drawText(QRectF(textX, rect.top(), textWidth, rect.height()),
                           Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine,
                           text);
                p.restore();
            }

            if (row.isExpander) {
                p.save();
                p.setOpacity(labelOpacity);
                drawChevron(expanderHitRect(rect).center(), !row.expanded);
                p.restore();
            }
        }
    };

    int y = d->headerHeight();
    for (int i = 0; i < footerStartRow && i < static_cast<int>(d->rows.size()); ++i) {
        const int rowHeightValue = d->rowHeight(i);
        paintRow(i, QRectF(0, y, W, rowHeightValue));
        y += rowHeightValue;
    }

    y = d->footerBaseY(H);
    for (int i = footerStartRow; i < static_cast<int>(d->rows.size()); ++i) {
        const int rowHeightValue = d->rowHeight(i);
        paintRow(i, QRectF(0, y, W, rowHeightValue));
        y += rowHeightValue;
    }
}

void FluentNavigationView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    const auto refreshRows = [this]() {
        d->rebuildRows();
        d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
        d->selStartRect = d->selRect;
        d->selTargetRect = d->selRect;
        updateGeometry();
        update();
    };

    if (d->displayMode == Top) {
        const QRectF backRect = d->topBackButtonRect();
        if (d->backButtonVisible && backRect.contains(event->pos())) {
            if (d->backButtonEnabled) {
                emit backRequested();
            }
            return;
        }

        const auto layouts = d->topLayouts(width());
        for (const auto &layout : layouts) {
            if (!layout.item || !layout.rect.contains(event->pos())) {
                continue;
            }

            const bool onChevron = !layout.item->children.empty() && topExpanderHitRect(layout.rect).contains(event->pos());
            const QPoint popupPos = mapToGlobal(QPoint(qRound(layout.rect.left()), qRound(layout.rect.bottom()) + 6));

            if (!layout.item->children.empty()) {
                if (!onChevron && layout.item->selectsOnInvoked && !layout.item->key.isEmpty()) {
                    setSelectedKey(layout.item->key);
                    emit itemInvoked(layout.item->key);
                }
                d->showItemFlyout(this, layout.item, popupPos);
                return;
            }

            d->closeItemFlyout();
            if (layout.item->selectsOnInvoked && !layout.item->key.isEmpty()) {
                setSelectedKey(layout.item->key);
            }
            if (!layout.item->key.isEmpty()) {
                emit itemInvoked(layout.item->key);
            }
            return;
        }

        d->closeItemFlyout();
        return;
    }

    const int hit = d->hitTest(event->pos(), height());
    if (hit < 0) {
        d->closeItemFlyout();
        return;
    }

    const auto &row = d->rows[static_cast<size_t>(hit)];
    if (row.kind == FlatRow::Control) {
        const QRectF rowRect = d->rowRect(hit, width(), height());
        const QRectF backRect = d->controlBackRect(rowRect);
        if (d->backButtonVisible && backRect.contains(event->pos())) {
            if (d->backButtonEnabled) {
                emit backRequested();
            }
            return;
        }

        if (d->controlHamburgerRect(rowRect).contains(event->pos())) {
            d->closeItemFlyout();
            toggleExpanded();
        }
        return;
    }

    if (row.kind == FlatRow::Separator || !row.item) {
        d->closeItemFlyout();
        return;
    }

    if (row.isExpander && d->showLabels()) {
        const QRectF rowRect = d->rowRect(hit, width(), height());
        if (expanderHitRect(rowRect).contains(event->pos())) {
            d->closeItemFlyout();
            d->setGroupExpanded(row.item->key, !row.expanded, true);
            refreshRows();
            return;
        }
    }

    if (row.isExpander) {
        if (!row.item->selectsOnInvoked) {
            if (d->isCompactMode()) {
                const QRectF rowRect = d->rowRect(hit, width(), height());
                d->showItemFlyout(this, row.item, mapToGlobal(QPoint(width() + 8, qRound(rowRect.top()))));
            } else {
                d->closeItemFlyout();
                d->setGroupExpanded(row.item->key, !row.expanded, true);
                refreshRows();
            }
            return;
        }

        if (d->isCompactMode()) {
            const QRectF rowRect = d->rowRect(hit, width(), height());
            if (!row.item->key.isEmpty()) {
                setSelectedKey(row.item->key);
                emit itemInvoked(row.item->key);
            }
            d->showItemFlyout(this, row.item, mapToGlobal(QPoint(width() + 8, qRound(rowRect.top()))));
            return;
        }

        d->closeItemFlyout();
        if (!row.item->key.isEmpty()) {
            setSelectedKey(row.item->key);
            emit itemInvoked(row.item->key);
        }
        return;
    }

    d->closeItemFlyout();
    if (row.item->selectsOnInvoked && !row.item->key.isEmpty()) {
        setSelectedKey(row.item->key);
    }
    if (!row.item->key.isEmpty()) {
        emit itemInvoked(row.item->key);
    }
}

void FluentNavigationView::mouseMoveEvent(QMouseEvent *event)
{
    const int hit = (d->displayMode == Top) ? d->topHitTest(event->pos(), width()) : d->hitTest(event->pos(), height());
    if (hit != d->hoverRowIndex) {
        d->hoverRowIndex = hit;
        d->hoverAnim->stop();
        d->hoverAnim->setStartValue(d->hoverLevel);
        d->hoverAnim->setEndValue(hit >= 0 ? 1.0 : 0.0);
        d->hoverAnim->start();
    }

    QWidget::mouseMoveEvent(event);
}

void FluentNavigationView::leaveEvent(QEvent *event)
{
    d->hoverRowIndex = -1;
    d->hoverAnim->stop();
    d->hoverAnim->setStartValue(d->hoverLevel);
    d->hoverAnim->setEndValue(0.0);
    d->hoverAnim->start();
    QWidget::leaveEvent(event);
}

void FluentNavigationView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (d->headerWidget) {
        d->headerWidget->setGeometry(0, 0, width(), d->headerWidget->sizeHint().height());
        d->headerWidget->setVisible(d->showHeader());
    }

    d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
    d->selStartRect = d->selRect;
    d->selTargetRect = d->selRect;
}

bool FluentNavigationView::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == d->observedParent && event->type() == QEvent::Resize) {
        syncAutoCollapseState();
    }

    return QWidget::eventFilter(watched, event);
}

bool FluentNavigationView::event(QEvent *event)
{
    if (event->type() == QEvent::ParentChange || event->type() == QEvent::Show) {
        updateParentEventFilter();
        syncAutoCollapseState();
        d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
        d->selStartRect = d->selRect;
        d->selTargetRect = d->selRect;
    }

    return QWidget::event(event);
}

} // namespace Fluent