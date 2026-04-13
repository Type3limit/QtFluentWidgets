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
#include <QPainterPath>
#include <QPointer>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QVariantAnimation>
#include <QWindow>

#include <algorithm>
#include <cmath>

namespace Fluent {

// ---------------------------------------------------------------------------
// Internal "flat row" – each visible row in the navigation pane
// ---------------------------------------------------------------------------
namespace {

struct FlatRow {
    enum Kind { Hamburger, Header, Item, Separator };
    Kind    kind = Item;
    int     groupIndex  = -1; // index into items/footerItems vector
    int     childIndex  = -1; // -1 = top-level, >=0 = child
    bool    isFooter    = false;
    bool    isExpander  = false; // top-level item that has children
    bool    expanded    = false; // currently showing children?
    int     depth       = 0;    // 0 = top-level, 1 = child

    const FluentNavigationItem *item = nullptr; // pointer into source data (not owned)

    QString key() const { return item ? item->key : QString(); }
};

// Geometry constants
constexpr int kRowHeight       = 40;
constexpr int kSeparatorHeight = 9;
constexpr int kIconSize        = 20;
constexpr int kIndicatorWidth  = 3;
constexpr int kIndicatorHeight = 16;
constexpr int kHamburgerHeight = 40;
constexpr int kDefaultExpandedWidth  = 280;
constexpr int kDefaultCompactWidth   = 48;
constexpr int kDepthIndent           = 28;
constexpr int kItemPaddingX          = 14;

QString defaultGlyphFontFamily()
{
    return QStringLiteral("Segoe Fluent Icons");
}

QRectF expanderHitRect(const QRectF &rowRect)
{
    return QRectF(rowRect.right() - 36.0, rowRect.top(), 36.0, rowRect.height());
}

} // namespace

// ---------------------------------------------------------------------------
// Private data
// ---------------------------------------------------------------------------
struct FluentNavigationView::Private
{
    std::vector<FluentNavigationItem> items;
    std::vector<FluentNavigationItem> footerItems;

    // flattened row list (rebuilt on data / expand change)
    std::vector<FlatRow> rows;

    QString selectedKey;
    int     hoverRowIndex = -1;

    bool expanded = true;
    int  expandedWidth = kDefaultExpandedWidth;
    int  compactWidth  = kDefaultCompactWidth;
    int  currentWidth  = kDefaultExpandedWidth;

    int autoCollapseThresh = 0; // 0 = disabled

    QWidget *headerWidget = nullptr;
    QWidget *observedParent = nullptr;
    QPointer<FluentMenu> compactFlyout;
    QString compactFlyoutParentKey;

    // expand / collapse state for sub-groups (keyed by group key)
    std::vector<QString> expandedGroups;

    // Animations
    QVariantAnimation *widthAnim  = nullptr;
    QVariantAnimation *hoverAnim  = nullptr;
    qreal hoverLevel = 0.0;

    // Selection indicator animation
    QVariantAnimation *selAnim = nullptr;
    QRectF selRect;
    QRectF selStartRect;
    QRectF selTargetRect;
    qreal  selOpacity = 0.0;

    // ---- helpers ----
    bool isGroupExpanded(const QString &key) const
    {
        return std::find(expandedGroups.begin(), expandedGroups.end(), key) != expandedGroups.end();
    }

    void closeCompactFlyout()
    {
        if (compactFlyout) {
            compactFlyout->close();
            compactFlyout->deleteLater();
            compactFlyout = nullptr;
        }
        compactFlyoutParentKey.clear();
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

        // Hamburger button row
        FlatRow hamburger;
        hamburger.kind = FlatRow::Hamburger;
        rows.push_back(hamburger);

        auto addItems = [&](const std::vector<FluentNavigationItem> &src, bool footer) {
            for (int i = 0; i < static_cast<int>(src.size()); ++i) {
                const auto &item = src[static_cast<size_t>(i)];

                if (item.separator) {
                    FlatRow sep;
                    sep.kind = FlatRow::Separator;
                    sep.isFooter = footer;
                    rows.push_back(sep);
                    continue;
                }

                FlatRow row;
                row.kind       = FlatRow::Item;
                row.groupIndex = i;
                row.childIndex = -1;
                row.isFooter   = footer;
                row.depth      = 0;
                row.item       = &item;

                if (!item.children.empty()) {
                    row.isExpander = true;
                    row.expanded   = isGroupExpanded(item.key);
                }

                rows.push_back(row);

                if (expanded && row.isExpander && row.expanded) {
                    for (int c = 0; c < static_cast<int>(item.children.size()); ++c) {
                        FlatRow child;
                        child.kind       = FlatRow::Item;
                        child.groupIndex = i;
                        child.childIndex = c;
                        child.isFooter   = footer;
                        child.depth      = 1;
                        child.item       = &item.children[static_cast<size_t>(c)];
                        rows.push_back(child);
                    }
                }
            }
        };

        addItems(items, false);

        // Footer items (Settings, etc.) go at the end
        if (!footerItems.empty()) {
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
        return expanded && currentWidth >= labelRevealWidth();
    }

    bool showHeader() const
    {
        return headerWidget && expanded && currentWidth >= headerRevealWidth();
    }

    qreal labelOpacity() const
    {
        if (!expanded) {
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

    // Y offset for each row
    int headerHeight() const
    {
        if (showHeader()) {
            return headerWidget->sizeHint().height();
        }
        return 0;
    }

    int rowY(int index) const
    {
        int y = headerHeight();
        for (int i = 0; i < index && i < static_cast<int>(rows.size()); ++i) {
            y += rowHeight(i);
        }
        return y;
    }

    int rowHeight(int index) const
    {
        if (index < 0 || index >= static_cast<int>(rows.size()))
            return 0;
        const auto &r = rows[static_cast<size_t>(index)];
        if (r.kind == FlatRow::Separator)
            return kSeparatorHeight;
        return kRowHeight;
    }

    int totalHeight() const
    {
        int h = headerHeight();
        for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
            h += rowHeight(i);
        }
        return h;
    }

    // Find the "footer start" Y to pin footer items at the bottom
    int footerStartRow() const
    {
        for (int i = static_cast<int>(rows.size()) - 1; i >= 0; --i) {
            const auto &r = rows[static_cast<size_t>(i)];
            if (!r.isFooter && r.kind != FlatRow::Separator) {
                return i + 1;
            }
            // also check separator that precedes footer
            if (r.kind == FlatRow::Separator && i > 0 && !rows[static_cast<size_t>(i) - 1].isFooter) {
                return i;
            }
        }
        return static_cast<int>(rows.size());
    }

    int footerHeightFromRow(int fsRow) const
    {
        int footerH = 0;
        for (int i = fsRow; i < static_cast<int>(rows.size()); ++i) {
            footerH += rowHeight(i);
        }
        return footerH;
    }

    int mainHeightUntilRow(int fsRow) const
    {
        int y = headerHeight();
        for (int i = 0; i < fsRow && i < static_cast<int>(rows.size()); ++i) {
            y += rowHeight(i);
        }
        return y;
    }

    int footerBaseY(int widgetHeight) const
    {
        const int fsRow = footerStartRow();
        const int footerH = footerHeightFromRow(fsRow);
        const int pinnedY = widgetHeight - footerH;
        const int stackedY = mainHeightUntilRow(fsRow);
        return qMax(pinnedY, stackedY);
    }

    int hitTest(const QPoint &pos, int widgetHeight) const
    {
        // Main items (non-footer)
        int fsRow = footerStartRow();
        {
            int y = headerHeight();
            for (int i = 0; i < fsRow && i < static_cast<int>(rows.size()); ++i) {
                int rh = rowHeight(i);
                if (pos.y() >= y && pos.y() < y + rh) {
                    return i;
                }
                y += rh;
            }
        }

        // Footer items rendered from the bottom
        {
            int y = footerBaseY(widgetHeight);
            for (int i = fsRow; i < static_cast<int>(rows.size()); ++i) {
                int rh = rowHeight(i);
                if (pos.y() >= y && pos.y() < y + rh) {
                    return i;
                }
                y += rh;
            }
        }
        return -1;
    }

    QRectF rowRect(int index, int widgetWidth, int widgetHeight) const
    {
        int fsRow = footerStartRow();
        if (index < fsRow) {
            int y = rowY(index);
            return QRectF(0, y, widgetWidth, rowHeight(index));
        }
        // footer
        int y = footerBaseY(widgetHeight);
        for (int i = fsRow; i < index; ++i) {
            y += rowHeight(i);
        }
        return QRectF(0, y, widgetWidth, rowHeight(index));
    }

    QRectF selectionRectForKey(const QString &key, int widgetWidth, int widgetHeight) const
    {
        auto rectForVisibleKey = [&](const QString &visibleKey) {
            for (int i = 0; i < static_cast<int>(rows.size()); ++i) {
                if (rows[static_cast<size_t>(i)].key() == visibleKey) {
                    QRectF r = rowRect(i, widgetWidth, widgetHeight);
                    return r.adjusted(4, 2, -4, -2);
                }
            }
            return QRectF();
        };

        const QRectF directRect = rectForVisibleKey(key);
        if (directRect.isValid()) {
            return directRect;
        }

        if (!expanded) {
            auto fallbackKeyInItems = [&key](const std::vector<FluentNavigationItem> &source) {
                for (const auto &item : source) {
                    for (const auto &child : item.children) {
                        if (child.key == key) {
                            return item.key;
                        }
                    }
                }
                return QString();
            };

            const QString fallbackKey = [&]() {
                const QString top = fallbackKeyInItems(items);
                if (!top.isEmpty()) {
                    return top;
                }
                return fallbackKeyInItems(footerItems);
            }();

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
                        if (!item.key.isEmpty()) {
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

    void showCompactFlyout(FluentNavigationView *owner, const FlatRow &row, const QRectF &rowRect)
    {
        if (!owner || !row.item || row.item->children.empty()) {
            return;
        }

        if (compactFlyout && compactFlyoutParentKey == row.item->key && compactFlyout->isVisible()) {
            closeCompactFlyout();
            return;
        }

        closeCompactFlyout();

        auto *menu = new FluentMenu(owner);
        menu->setAttribute(Qt::WA_DeleteOnClose, true);
        compactFlyout = menu;
        compactFlyoutParentKey = row.item->key;

        QObject::connect(menu, &QObject::destroyed, owner, [this]() {
            compactFlyout = nullptr;
            compactFlyoutParentKey.clear();
        });

        QAction *activeAction = nullptr;
        for (const auto &child : row.item->children) {
            QAction *action = menu->addAction(child.text);
            if (!child.icon.isNull()) {
                action->setIcon(child.icon);
            }

            QObject::connect(action, &QAction::triggered, owner, [owner, this, childKey = child.key]() {
                owner->setSelectedKey(childKey);
                closeCompactFlyout();
            });

            if (child.key == selectedKey) {
                activeAction = action;
            }
        }

        const QPoint popupPos = owner->mapToGlobal(QPoint(owner->width() + 8, qRound(rowRect.top())));
        menu->popup(popupPos, activeAction);
    }
};

// ---------------------------------------------------------------------------
// Construction / Destruction
// ---------------------------------------------------------------------------
FluentNavigationView::FluentNavigationView(QWidget *parent)
    : QWidget(parent)
    , d(std::make_unique<Private>())
{
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    setFixedWidth(d->currentWidth);

    // Width animation (expand / collapse)
    d->widthAnim = new QVariantAnimation(this);
    d->widthAnim->setDuration(200);
    d->widthAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->widthAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        d->currentWidth = v.toInt();
        setFixedWidth(d->currentWidth);
        if (d->headerWidget) {
            d->headerWidget->setVisible(d->showHeader());
        }
        update();
    });

    // Hover animation
    d->hoverAnim = new QVariantAnimation(this);
    d->hoverAnim->setDuration(120);
    connect(d->hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        d->hoverLevel = v.toReal();
        update();
    });

    // Selection animation
    d->selAnim = new QVariantAnimation(this);
    d->selAnim->setDuration(180);
    d->selAnim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(d->selAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) {
        const qreal t = qBound<qreal>(0.0, v.toReal(), 1.0);
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
}

FluentNavigationView::~FluentNavigationView() = default;

// ---------------------------------------------------------------------------
// Item management
// ---------------------------------------------------------------------------
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

void FluentNavigationView::setFooterItems(const std::vector<FluentNavigationItem> &items)
{
    d->footerItems = items;
    d->ensureKeyVisible(d->selectedKey);
    d->rebuildRows();
    d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
    d->selStartRect = d->selRect;
    d->selTargetRect = d->selRect;
    updateGeometry();
    update();
}

// ---------------------------------------------------------------------------
// Selection
// ---------------------------------------------------------------------------
QString FluentNavigationView::selectedKey() const
{
    return d->selectedKey;
}

void FluentNavigationView::setSelectedKey(const QString &key)
{
    if (d->selectedKey == key)
        return;

    const QRectF oldRect = d->selectionRectForKey(d->selectedKey, width(), height());
    d->ensureKeyVisible(key);
    d->rebuildRows();
    d->selectedKey = key;
    const QRectF newRect = d->selectionRectForKey(key, width(), height());

    // Animate indicator
    const bool animRunning = d->selAnim->state() == QAbstractAnimation::Running;
    d->selStartRect   = animRunning ? d->selRect : oldRect;
    d->selTargetRect  = newRect;
    d->selAnim->stop();
    d->selAnim->setStartValue(0.0);
    d->selAnim->setEndValue(1.0);
    d->selAnim->start();

    updateGeometry();
    emit selectedKeyChanged(key);
    update();
}

// ---------------------------------------------------------------------------
// Expand / Collapse
// ---------------------------------------------------------------------------
bool FluentNavigationView::isExpanded() const
{
    return d->expanded;
}

void FluentNavigationView::setExpanded(bool expanded)
{
    if (d->expanded == expanded)
        return;
    d->closeCompactFlyout();
    d->expanded = expanded;

    d->rebuildRows();
    d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
    d->selStartRect = d->selRect;
    d->selTargetRect = d->selRect;
    updateGeometry();

    int target = expanded ? d->expandedWidth : d->compactWidth;
    d->widthAnim->stop();
    d->widthAnim->setStartValue(d->currentWidth);
    d->widthAnim->setEndValue(target);
    d->widthAnim->start();

    emit expandedChanged(expanded);
}

void FluentNavigationView::toggleExpanded()
{
    setExpanded(!d->expanded);
}

int FluentNavigationView::expandedWidth() const { return d->expandedWidth; }
void FluentNavigationView::setExpandedWidth(int w) { d->expandedWidth = w; if (d->expanded) setFixedWidth(w); }

int FluentNavigationView::compactWidth() const { return d->compactWidth; }
void FluentNavigationView::setCompactWidth(int w) { d->compactWidth = w; if (!d->expanded) setFixedWidth(w); }

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

void FluentNavigationView::setAutoCollapseWidth(int threshold)
{
    d->autoCollapseThresh = threshold;
    syncAutoCollapseState();
}

int FluentNavigationView::autoCollapseWidth() const { return d->autoCollapseThresh; }

QSize FluentNavigationView::sizeHint() const
{
    return QSize(d->expanded ? d->expandedWidth : d->compactWidth, d->totalHeight());
}

QSize FluentNavigationView::minimumSizeHint() const
{
    return QSize(d->compactWidth, d->totalHeight());
}

void FluentNavigationView::syncAutoCollapseState()
{
    if (d->autoCollapseThresh <= 0 || !d->observedParent) {
        return;
    }

    const int parentWidth = d->observedParent->width();
    if (d->expanded && parentWidth < d->autoCollapseThresh) {
        setExpanded(false);
    } else if (!d->expanded && parentWidth >= d->autoCollapseThresh + 100) {
        setExpanded(true);
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

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------
void FluentNavigationView::paintEvent(QPaintEvent * /*event*/)
{
    QPainter p(this);
    if (!p.isActive())
        return;
    p.setRenderHint(QPainter::Antialiasing, true);

    const auto &colors = ThemeManager::instance().colors();
    const int W = width();
    const int H = height();
    const bool showLabels = d->showLabels();
    const qreal labelOpacity = d->labelOpacity();

    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(colors.surface);
    p.drawRoundedRect(QRectF(0, 0, W, H), 8, 8);

    // ---- helper lambdas ----
    auto drawHamburger = [&](const QRectF &rect) {
        const QPointF center = rect.center();
        const qreal barW = 16.0;
        const qreal barH = 2.0;
        const qreal gap = 5.0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors.text);
        for (int i = -1; i <= 1; ++i) {
            QRectF bar(center.x() - barW / 2.0, center.y() + i * gap - barH / 2.0, barW, barH);
            p.drawRoundedRect(bar, 1, 1);
        }
    };

    auto drawChevron = [&](const QPointF &center, bool down) {
        if (down)
            Style::drawChevronDown(p, center, colors.subText, 6.0, 1.4);
        else
            Style::drawChevronUp(p, center, colors.subText, 6.0, 1.4);
    };

    // ---- selection indicator (animated) ----
    {
        const bool animRunning = d->selAnim->state() == QAbstractAnimation::Running;
        const QRectF sr = animRunning ? d->selRect : d->selectionRectForKey(d->selectedKey, W, H);
        const qreal opacity = animRunning ? qBound<qreal>(0.0, d->selOpacity, 1.0) : (sr.isValid() ? 1.0 : 0.0);

        if (sr.isValid() && opacity > 0.0) {
            // Selection background
            QColor fill = colors.accent;
            fill.setAlpha(qBound(0, int(std::lround(30.0 * opacity)), 30));
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawRoundedRect(sr, 4, 4);

            // Left accent indicator
            QColor ind = colors.accent;
            ind.setAlpha(qBound(0, int(std::lround(255.0 * opacity)), 255));
            p.setBrush(ind);
            const qreal indH = kIndicatorHeight;
            QRectF indRect(sr.left(), sr.center().y() - indH / 2.0, kIndicatorWidth, indH);
            p.drawRoundedRect(indRect, 1.5, 1.5);
        }
    }

    // ---- rows ----
    const int fsRow = d->footerStartRow();

    auto paintRow = [&](int i, const QRectF &rect) {
        const auto &row = d->rows[static_cast<size_t>(i)];

        // separator
        if (row.kind == FlatRow::Separator) {
            p.setPen(QPen(colors.border, 1));
            const qreal cy = rect.center().y();
            p.drawLine(QPointF(rect.left() + 12, cy), QPointF(rect.right() - 12, cy));
            return;
        }

        // hover highlight
        if (i == d->hoverRowIndex && d->hoverLevel > 0.0 && row.kind != FlatRow::Hamburger) {
            QColor hc = colors.hover;
            hc.setAlphaF(0.3 * d->hoverLevel);
            p.setPen(Qt::NoPen);
            p.setBrush(hc);
            p.drawRoundedRect(rect.adjusted(4, 2, -4, -2), 4, 4);
        }

        // Hamburger button
        if (row.kind == FlatRow::Hamburger) {
            if (i == d->hoverRowIndex && d->hoverLevel > 0.0) {
                QColor hc = colors.hover;
                hc.setAlphaF(0.25 * d->hoverLevel);
                p.setPen(Qt::NoPen);
                p.setBrush(hc);
                p.drawRoundedRect(rect.adjusted(4, 4, -4, -4), 4, 4);
            }
            drawHamburger(QRectF(rect.left(), rect.top(), d->compactWidth, rect.height()));
            return;
        }

        if (!row.item)
            return;

        const int indent = row.depth * kDepthIndent;

        const QRectF iconRect(rect.left() + indent + kItemPaddingX,
                              rect.center().y() - kIconSize / 2.0,
                              kIconSize,
                              kIconSize);

        if (!row.item->iconGlyph.isEmpty()) {
            QFont iconFont = font();
            iconFont.setFamily(row.item->iconFontFamily.isEmpty() ? defaultGlyphFontFamily() : row.item->iconFontFamily);
            iconFont.setPixelSize(kIconSize);
            iconFont.setStyleStrategy(QFont::PreferAntialias);

            p.save();
            p.setFont(iconFont);
            p.setPen(colors.text);
            p.drawText(iconRect, Qt::AlignCenter, row.item->iconGlyph);
            p.restore();
        } else if (!row.item->icon.isNull()) {
            row.item->icon.paint(&p,
                                 QRect(static_cast<int>(iconRect.left()),
                                       static_cast<int>(iconRect.top()),
                                       static_cast<int>(iconRect.width()),
                                       static_cast<int>(iconRect.height())));
        }

        // Text (only when wide enough)
        if (labelOpacity > 0.0) {
            const int textX = static_cast<int>(rect.left()) + indent + kItemPaddingX + kIconSize + 12;
            const int textW = static_cast<int>(rect.width()) - textX - 32;
            if (textW > 0) {
                QFont f = font();
                f.setPixelSize(14);
                const QFontMetrics fm(f);
                const QString text = fm.elidedText(row.item->text, Qt::ElideRight, textW);
                QRectF textRect(textX, rect.top(), textW, rect.height());

                p.save();
                p.setOpacity(labelOpacity);
                p.setPen(colors.text);
                p.setFont(f);
                p.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft | Qt::TextSingleLine, text);
                p.restore();
            }

            // Expander chevron
            if (row.isExpander && showLabels) {
                QPointF chevCenter(expanderHitRect(rect).center().x(), rect.center().y());
                p.save();
                p.setOpacity(labelOpacity);
                drawChevron(chevCenter, !row.expanded);
                p.restore();
            }
        }
    };

    // Paint main rows
    {
        int y = d->headerHeight();
        for (int i = 0; i < fsRow && i < static_cast<int>(d->rows.size()); ++i) {
            int rh = d->rowHeight(i);
            QRectF rect(0, y, W, rh);
            paintRow(i, rect);
            y += rh;
        }
    }

    // Paint footer rows. If space is insufficient, they stack after main rows
    // instead of overlapping with expanded items.
    {
        int y = d->footerBaseY(H);
        for (int i = fsRow; i < static_cast<int>(d->rows.size()); ++i) {
            int rh = d->rowHeight(i);
            QRectF rect(0, y, W, rh);
            paintRow(i, rect);
            y += rh;
        }
    }
}

// ---------------------------------------------------------------------------
// Mouse interaction
// ---------------------------------------------------------------------------
void FluentNavigationView::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    const int hit = d->hitTest(event->pos(), height());
    if (hit < 0)
        return;

    const auto &row = d->rows[static_cast<size_t>(hit)];

    if (row.kind == FlatRow::Hamburger) {
        d->closeCompactFlyout();
        toggleExpanded();
        return;
    }

    if (row.kind == FlatRow::Separator)
        return;

    if (row.isExpander && d->showLabels()) {
        const QRectF rowRect = d->rowRect(hit, width(), height());
        if (expanderHitRect(rowRect).contains(event->pos())) {
            d->setGroupExpanded(row.item->key, !row.expanded, true);

            d->rebuildRows();
            d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
            d->selStartRect = d->selRect;
            d->selTargetRect = d->selRect;
            updateGeometry();
            update();
            return;
        }
    }

    // Parent items remain clickable and navigate independently from the expander.
    if (row.isExpander && row.item && !row.item->key.isEmpty()) {
        if (!d->expanded) {
            const QRectF rowRect = d->rowRect(hit, width(), height());
            setSelectedKey(row.item->key);
            d->showCompactFlyout(this, row, rowRect);
            return;
        }

        d->closeCompactFlyout();
        setSelectedKey(row.item->key);
        return;
    }

    // Normal item selection
    if (row.item && !row.item->key.isEmpty()) {
        d->closeCompactFlyout();
        setSelectedKey(row.item->key);
    }
}

void FluentNavigationView::mouseMoveEvent(QMouseEvent *event)
{
    const int hit = d->hitTest(event->pos(), height());
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

    // Reposition header widget
    if (d->headerWidget) {
        d->headerWidget->setGeometry(0, 0, width(), d->headerWidget->sizeHint().height());
    }

    // Update selection rect (widget height changed)
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
        // Recalculate selection rect
        d->selRect = d->selectionRectForKey(d->selectedKey, width(), height());
        d->selStartRect = d->selRect;
        d->selTargetRect = d->selRect;
    }
    return QWidget::event(event);
}

} // namespace Fluent
