#include "Fluent/FluentTreeView.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentTheme.h"

#include <QAbstractItemModel>
#include <QEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVariantAnimation>
#include <QWindow>

namespace Fluent {

namespace {

class FluentHeaderView final : public QHeaderView
{
public:
    explicit FluentHeaderView(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QHeaderView(orientation, parent)
    {
        setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    }

protected:
    bool viewportEvent(QEvent *event) override
    {
        const bool baseResult = QHeaderView::viewportEvent(event);

        if (!event || event->type() != QEvent::Paint) {
            return baseResult;
        }

        if (orientation() != Qt::Horizontal) {
            return baseResult;
        }

        if (viewport() && !viewport()->testAttribute(Qt::WA_WState_InPaintEvent)) {
            return baseResult;
        }

        // Avoid creating a QPainter for the header viewport before the window is exposed.
        // This prevents sporadic "Paint device returned engine == 0" warnings on startup.
        if (QWidget *vp = viewport()) {
            if (QWidget *w = vp->window()) {
                if (w->windowHandle() && !w->windowHandle()->isExposed()) {
                    return baseResult;
                }
            }
        }

        const auto &colors = ThemeManager::instance().colors();
        QColor sep = colors.border;
        sep.setAlpha(140);

        QPainter painter(viewport());
        if (!painter.isActive()) {
            return baseResult;
        }
        painter.setRenderHint(QPainter::Antialiasing, false);
        painter.setPen(QPen(sep, 1));

        const int inset = 6;
        const int y1 = inset;
        const int y2 = viewport()->height() - 1 - inset;
        if (y2 <= y1) {
            return baseResult;
        }

        for (int logical = 0; logical < count(); ++logical) {
            if (isSectionHidden(logical)) {
                continue;
            }
            const int w = sectionSize(logical);
            if (w <= 0) {
                continue;
            }
            const int left = sectionViewportPosition(logical);
            const int right = left + w - 1;
            if (logical == count() - 1) {
                continue;
            }

            if (right < 0 || right >= viewport()->width() - 1) {
                continue;
            }
            painter.drawLine(QPointF(right + 0.5, y1), QPointF(right + 0.5, y2));
        }

        return baseResult;
    }
};

class FluentTreeItemDelegate final : public QStyledItemDelegate
{
public:
    explicit FluentTreeItemDelegate(FluentTreeView *view)
        : QStyledItemDelegate(view)
        , m_view(view)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        opt.state &= ~QStyle::State_HasFocus; 

        const auto &colors = ThemeManager::instance().colors();
        
        // Layout info
        const QAbstractItemModel *model = index.model();
        bool isFirst = (index.column() == 0);
        bool isLast = (model && index.column() == model->columnCount(index.parent()) - 1);
        
        // TreeView row selection logic:
        // Selection behavior usually is SelectRows.
        bool isRowSelection = (m_view && m_view->selectionBehavior() == QAbstractItemView::SelectRows);

        QRectF bgRect = QRectF(opt.rect);
        // Vertical breathing room
        bgRect.adjust(0, 1, 0, -1);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QColor bgColor = Qt::transparent;
        
        const bool isSelected = opt.state.testFlag(QStyle::State_Selected);
        bool isHovered = false;

        bool isCurrentRow = false;
        if (m_view) {
            const QModelIndex cur = m_view->currentIndex();
            if (cur.isValid()) {
                isCurrentRow = (cur.row() == index.row() && cur.parent() == index.parent());
            }
        }

        if (m_view && index.isValid()) {
             QModelIndex hover = m_view->hoverIndex();
             if (hover.isValid()) {
                 if (isRowSelection) {
                     // Check if same row (row + parent match)
                     if (index.row() == hover.row() && index.parent() == hover.parent()) {
                         isHovered = true;
                     }
                 } else {
                     if (index == hover) isHovered = true;
                 }
             }
        }

        if (isSelected && !isCurrentRow) {
            bgColor = colors.accent;
            bgColor.setAlpha(40);
        } else if (isHovered) {
             QColor hoverColor = colors.hover;
             hoverColor.setAlphaF(0.3 * m_view->hoverLevel()); // Global anim level
             bgColor = hoverColor;
        }

        // Keep selected text color stable (Qt would otherwise use HighlightedText).
        if (isSelected) {
            opt.palette.setColor(QPalette::HighlightedText, opt.palette.color(QPalette::Text));
            opt.palette.setColor(QPalette::Highlight, Qt::transparent);
        }

        if (bgColor.alpha() > 0) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(bgColor);

            if (isRowSelection) {
                // Unified row logic
                qreal r = 4.0;
                QPainterPath path;
                path.setFillRule(Qt::WindingFill);

                if (isFirst && isLast) {
                    path.addRoundedRect(bgRect.adjusted(2,0,-2,0), r, r);
                } 
                else if (isFirst) {
                    path.addRoundedRect(bgRect.adjusted(2,0,0,0), r, r);
                    QRectF fixRight = bgRect.adjusted(2,0,0,0);
                    fixRight.setLeft(fixRight.right() - r);
                    path.addRect(fixRight);
                } 
                else if (isLast) {
                    path.addRoundedRect(bgRect.adjusted(0,0,-2,0), r, r);
                    QRectF fixLeft = bgRect.adjusted(0,0,-2,0);
                    fixLeft.setWidth(r);
                    path.addRect(fixLeft);
                } 
                else {
                    path.addRect(bgRect);
                }
                
                painter->drawPath(path);
                
            } else {
                // Single cell
                painter->drawRoundedRect(bgRect.adjusted(2,0,-2,0), 4, 4);
            }
            
        }

        painter->restore();

        QStyledItemDelegate::paint(painter, opt, index);
    }

private:
    FluentTreeView *m_view = nullptr;
};

} // namespace

FluentTreeView::FluentTreeView(QWidget *parent)
    : QTreeView(parent)
{
    setHeader(new FluentHeaderView(Qt::Horizontal, this));
    header()->setStretchLastSection(true);
    setFrameShape(QFrame::NoFrame);
    setAlternatingRowColors(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setIndentation(18);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    setItemDelegate(new FluentTreeItemDelegate(this));

    setVerticalScrollBar(new FluentScrollBar(Qt::Vertical, this));
    setHorizontalScrollBar(new FluentScrollBar(Qt::Horizontal, this));

    m_hoverAnim = new QVariantAnimation(this);
    m_hoverAnim->setDuration(120);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        viewport()->update();
    });

    m_selAnim = new QVariantAnimation(this);
    m_selAnim->setDuration(180);
    m_selAnim->setEasingCurve(QEasingCurve::InOutCubic);
    connect(m_selAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        const qreal t = value.toReal();
        const qreal tt = qBound<qreal>(0.0, t, 1.0);
        m_selRect = QRectF(
            m_selStartRect.x() + (m_selTargetRect.x() - m_selStartRect.x()) * tt,
            m_selStartRect.y() + (m_selTargetRect.y() - m_selStartRect.y()) * tt,
            m_selStartRect.width() + (m_selTargetRect.width() - m_selStartRect.width()) * tt,
            m_selStartRect.height() + (m_selTargetRect.height() - m_selStartRect.height()) * tt);
        m_selOpacity = m_selStartOpacity + (m_selTargetOpacity - m_selStartOpacity) * tt;
        viewport()->update();
    });
    connect(m_selAnim, &QVariantAnimation::finished, this, [this]() {
        if (m_selTargetOpacity <= 0.0) {
            m_selRect = QRectF();
            m_selOpacity = 0.0;
        } else {
            m_selOpacity = 1.0;
        }
        viewport()->update();
    });
    applyTheme();
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentTreeView::applyTheme);

    QTimer::singleShot(0, this, [this]() { hookSelectionModel(); });
}

QModelIndex FluentTreeView::hoverIndex() const
{
    return m_hoverIndex;
}

qreal FluentTreeView::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentTreeView::changeEvent(QEvent *event)
{
    QTreeView::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentTreeView::setModel(QAbstractItemModel *model)
{
    QTreeView::setModel(model);
    hookSelectionModel();
}

void FluentTreeView::paintEvent(QPaintEvent *event)
{
    {
        const QModelIndex cur = currentIndex();
        const bool paintAnim = (m_selAnim && m_selAnim->state() == QAbstractAnimation::Running && m_selRect.isValid() && m_selOpacity > 0.0);
        const bool paintStatic = (cur.isValid() && selectionModel() && selectionModel()->isSelected(cur));
        if (paintAnim || paintStatic) {
            const auto &colors = ThemeManager::instance().colors();
            const QRectF r = paintAnim ? m_selRect : selectionRectForIndex(cur);
            const qreal opacity = paintAnim ? qBound<qreal>(0.0, m_selOpacity, 1.0) : 1.0;
            QPainter p(viewport());
            if (p.isActive()) {
                p.setRenderHint(QPainter::Antialiasing, true);

                QColor fill = colors.accent;
                fill.setAlpha(qBound(0, int(std::lround(40.0 * opacity)), 40));
                p.setPen(Qt::NoPen);
                p.setBrush(fill);
                p.drawRoundedRect(r, 4.0, 4.0);

                QColor indicator = colors.accent;
                indicator.setAlpha(qBound(0, int(std::lround(255.0 * opacity)), 255));
                p.setBrush(indicator);
                const qreal indicatorHeight = 16.0;
                QRectF indRect(r.left() + 3.0, r.center().y() - indicatorHeight / 2.0, 3.0, indicatorHeight);
                p.drawRoundedRect(indRect, 1.5, 1.5);
            }
        }
    }

    QTreeView::paintEvent(event);
}

void FluentTreeView::mouseMoveEvent(QMouseEvent *event)
{
    const QModelIndex index = indexAt(event->pos());
    if (index != m_hoverIndex) {
        m_hoverIndex = index;
        startHoverAnimation(index.isValid() ? 1.0 : 0.0);
    }
    QTreeView::mouseMoveEvent(event);
}

void FluentTreeView::leaveEvent(QEvent *event)
{
    startHoverAnimation(0.0);
    m_hoverIndex = QModelIndex();
    QTreeView::leaveEvent(event);
}

void FluentTreeView::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    // Draw default branch lines first (optional)
    // QTreeView::drawBranches(painter, rect, index);
    
    if (!index.isValid()) {
        return;
    }
    
    // Only draw chevron for items with children in first column
    if (index.column() == 0 && model() && model()->hasChildren(index)) {
        const auto &colors = ThemeManager::instance().colors();
        
        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);
        
        // Use same pen style as ComboBox arrow
        QPen pen(colors.subText, 1.5);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter->setPen(pen);
        painter->setBrush(Qt::NoBrush);
        
        // Calculate center position in the branch rect
        int centerX = rect.center().x();
        int centerY = rect.center().y();
        
        QPainterPath path;
        if (isExpanded(index)) {
            // Down chevron (expanded): ∨ shape
            // M centerX-4,centerY-2 L centerX,centerY+2 L centerX+4,centerY-2
            path.moveTo(centerX - 4, centerY - 2);
            path.lineTo(centerX, centerY + 2);
            path.lineTo(centerX + 4, centerY - 2);
        } else {
            // Right chevron (collapsed): > shape  
            // M centerX-2,centerY-4 L centerX+2,centerY L centerX-2,centerY+4
            path.moveTo(centerX - 2, centerY - 4);
            path.lineTo(centerX + 2, centerY);
            path.lineTo(centerX - 2, centerY + 4);
        }
        
        painter->drawPath(path);
        painter->restore();
    }
}

void FluentTreeView::applyTheme()
{
    const QString next = Theme::treeViewStyle(ThemeManager::instance().colors());
    if (styleSheet() != next) {
        setStyleSheet(next);
    }
}

void FluentTreeView::hookSelectionModel()
{
    if (!selectionModel()) {
        return;
    }

    disconnect(selectionModel(), nullptr, this, nullptr);

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current, const QModelIndex &previous) {
        startSelectionAnimation(previous, current);
    });

    const QModelIndex cur = currentIndex();
    m_selRect = selectionRectForIndex(cur);
    m_selStartRect = m_selRect;
    m_selTargetRect = m_selRect;
    m_selOpacity = m_selRect.isValid() ? 1.0 : 0.0;
    m_selStartOpacity = m_selOpacity;
    m_selTargetOpacity = m_selOpacity;
}

QRectF FluentTreeView::selectionRectForIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QRectF();
    }

    if (selectionBehavior() == QAbstractItemView::SelectRows) {
        QRect rowRect;
        const QAbstractItemModel *m = model();
        if (!m) {
            return QRectF();
        }
        const int cols = m->columnCount(index.parent());
        for (int c = 0; c < cols; ++c) {
            if (isColumnHidden(c)) {
                continue;
            }
            const QRect r = visualRect(index.sibling(index.row(), c));
            if (!r.isValid()) {
                continue;
            }
            rowRect = rowRect.isNull() ? r : rowRect.united(r);
        }
        if (!rowRect.isValid()) {
            rowRect = visualRect(index);
        }
        return QRectF(rowRect).adjusted(2, 1, -2, -1);
    }

    const QRect r = visualRect(index);
    return r.isValid() ? QRectF(r).adjusted(2, 1, -2, -1) : QRectF();
}

void FluentTreeView::startSelectionAnimation(const QModelIndex &from, const QModelIndex &to)
{
    const bool animRunning = (m_selAnim && m_selAnim->state() == QAbstractAnimation::Running);
    const QRectF startRect = animRunning ? m_selRect : selectionRectForIndex(from);
    const qreal startOpacity = animRunning ? m_selOpacity : (startRect.isValid() ? 1.0 : 0.0);
    const QRectF targetRect = selectionRectForIndex(to);
    const bool targetValid = targetRect.isValid();

    if (!targetValid) {
        if (!startRect.isValid()) {
            m_selRect = QRectF();
            m_selOpacity = 0.0;
            viewport()->update();
            return;
        }
        m_selStartRect = startRect;
        m_selTargetRect = startRect;
        m_selStartOpacity = startOpacity;
        m_selTargetOpacity = 0.0;
        m_selRect = startRect;
        m_selOpacity = startOpacity;
        m_selAnim->stop();
        m_selAnim->setStartValue(0.0);
        m_selAnim->setEndValue(1.0);
        m_selAnim->start();
        return;
    }

    if (!startRect.isValid()) {
        m_selStartRect = targetRect;
        m_selTargetRect = targetRect;
        m_selStartOpacity = 0.0;
        m_selTargetOpacity = 1.0;
        m_selRect = targetRect;
        m_selOpacity = 0.0;
        m_selAnim->stop();
        m_selAnim->setStartValue(0.0);
        m_selAnim->setEndValue(1.0);
        m_selAnim->start();
        return;
    }

    if (startRect == targetRect) {
        m_selRect = targetRect;
        m_selOpacity = 1.0;
        viewport()->update();
        return;
    }

    m_selStartRect = startRect;
    m_selTargetRect = targetRect;
    m_selStartOpacity = 0.75;
    m_selTargetOpacity = 1.0;
    m_selRect = startRect;
    m_selOpacity = m_selStartOpacity;

    m_selAnim->stop();
    m_selAnim->setStartValue(0.0);
    m_selAnim->setEndValue(1.0);
    m_selAnim->start();
}

void FluentTreeView::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

} // namespace Fluent
