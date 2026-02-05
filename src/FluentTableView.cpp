#include "Fluent/FluentTableView.h"
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

        // Draw after all sections are painted so it can't be overwritten.
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
            // Skip the last visible section to avoid a line at the far right edge.
            if (logical == count() - 1) {
                continue;
            }

            // Only draw if inside viewport.
            if (right < 0 || right >= viewport()->width() - 1) {
                continue;
            }
            painter.drawLine(QPointF(right + 0.5, y1), QPointF(right + 0.5, y2));
        }

        return baseResult;
    }
};

class FluentTableItemDelegate final : public QStyledItemDelegate
{
public:
    explicit FluentTableItemDelegate(FluentTableView *view)
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
        
        // Determine layout context
        const QAbstractItemModel *model = index.model();
        bool isFirst = (index.column() == 0);
        bool isLast = (model && index.column() == model->columnCount(index.parent()) - 1);
        bool isRowSelection = (m_view && m_view->selectionBehavior() == QAbstractItemView::SelectRows);
        
        // Define the geometry for the background
        QRectF bgRect = QRectF(opt.rect);
        // We want a unified row look, so we might need to expand slightly to overlap gaps if any?
        // Typically QTableView packs cells tight. 
        // We'll adjust vertical padding to give breathing room between rows (Fluent Style 2px gap?)
        // If we do that, we must ensure the 'hover/selection' doesn't look fragmented.
        // Let's contract vertically by 2px (1 top, 1 bottom) for the "floating row" look.
        bgRect.adjust(0, 1, 0, -1);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        // Determine Background Color
        QColor bgColor = Qt::transparent;

        const bool selected = opt.state.testFlag(QStyle::State_Selected);
        bool isCurrentRow = false;
        if (m_view) {
            const QModelIndex cur = m_view->currentIndex();
            if (cur.isValid()) {
                isCurrentRow = (cur.row() == index.row() && cur.parent() == index.parent());
            }
        }

        if (selected && !isCurrentRow) {
            bgColor = colors.accent;
            bgColor.setAlpha(40);
        } else if (m_view && isRowSelection && index.row() == m_view->hoverIndex().row()) {
             QColor hover = colors.hover;
             hover.setAlphaF(0.3 * m_view->hoverLevel()); 
             bgColor = hover;
        } else if (m_view && !isRowSelection && index == m_view->hoverIndex()) {
             // Cell hover
             QColor hover = colors.hover;
             hover.setAlphaF(0.3 * m_view->hoverLevel());
             bgColor = hover;
        }

        if (bgColor.alpha() > 0) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(bgColor);

            if (isRowSelection) {
                qreal r = 4.0;
                QPainterPath path;
                
                if (isFirst && isLast) {
                    // Single column
                    path.addRoundedRect(bgRect.adjusted(2,0,-2,0), r, r);
                } else if (isFirst) {
                    // Left rounded, Right square
                    // We draw a rect that extends bit to right, then clip? width is small.
                    // Let's build path manually or use the intersection trick
                    QRectF lRect = bgRect.adjusted(2, 0, 0, 0); 
                    // To get perfectly straight right edge, we can map:
                    path.setFillRule(Qt::WindingFill);
                    path.addRoundedRect(lRect, r, r);
                    // Add a rect to "fill" the right corners if they got rounded
                    QRectF fixRight = lRect;
                    fixRight.setLeft(fixRight.right() - r);
                    path.addRect(fixRight); 
                } else if (isLast) {
                    // Left square, Right rounded
                    QRectF rRect = bgRect.adjusted(0, 0, -2, 0);
                    path.setFillRule(Qt::WindingFill);
                    path.addRoundedRect(rRect, r, r);
                    // Add a rect to fill left corners
                    QRectF fixLeft = rRect;
                    fixLeft.setWidth(r);
                    path.addRect(fixLeft);
                } else {
                    // Middle - pure rect
                    path.addRect(bgRect);
                }
                painter->drawPath(path);

            } else {
                // Cell selection/hover
                painter->drawRoundedRect(bgRect.adjusted(2,0,-2,0), 4, 4);
            }
            
            // Fix text color being white on light background
            opt.palette.setColor(QPalette::HighlightedText, opt.palette.color(QPalette::Text));
            opt.palette.setColor(QPalette::Highlight, Qt::transparent); 
        }

        if (selected) {
            // Ensure Qt doesn't paint its own highlight over our custom selection.
            opt.palette.setColor(QPalette::HighlightedText, opt.palette.color(QPalette::Text));
            opt.palette.setColor(QPalette::Highlight, Qt::transparent);
        }

        painter->restore();

        QStyledItemDelegate::paint(painter, opt, index);
    }

private:
    FluentTableView *m_view = nullptr;
};

} // namespace

FluentTableView::FluentTableView(QWidget *parent)
    : QTableView(parent)
{
    setHorizontalHeader(new FluentHeaderView(Qt::Horizontal, this));
    horizontalHeader()->setStretchLastSection(true);
    verticalHeader()->setVisible(false);
    setFrameShape(QFrame::NoFrame);
    setShowGrid(false);
    setAlternatingRowColors(false);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    setItemDelegate(new FluentTableItemDelegate(this));

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
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentTableView::applyTheme);

    QTimer::singleShot(0, this, [this]() { hookSelectionModel(); });
}

QModelIndex FluentTableView::hoverIndex() const
{
    return m_hoverIndex;
}

qreal FluentTableView::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentTableView::changeEvent(QEvent *event)
{
    QTableView::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentTableView::setModel(QAbstractItemModel *model)
{
    QTableView::setModel(model);
    hookSelectionModel();
}

void FluentTableView::paintEvent(QPaintEvent *event)
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

    QTableView::paintEvent(event);
}

void FluentTableView::mouseMoveEvent(QMouseEvent *event)
{
    const QModelIndex index = indexAt(event->pos());
    if (index != m_hoverIndex) {
        m_hoverIndex = index;
        startHoverAnimation(index.isValid() ? 1.0 : 0.0);
    }
    QTableView::mouseMoveEvent(event);
}

void FluentTableView::leaveEvent(QEvent *event)
{
    startHoverAnimation(0.0);
    m_hoverIndex = QModelIndex();
    QTableView::leaveEvent(event);
}

void FluentTableView::applyTheme()
{
    const QString next = Theme::tableViewStyle(ThemeManager::instance().colors());
    if (styleSheet() != next) {
        setStyleSheet(next);
    }
}

void FluentTableView::hookSelectionModel()
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

QRectF FluentTableView::selectionRectForIndex(const QModelIndex &index) const
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

void FluentTableView::startSelectionAnimation(const QModelIndex &from, const QModelIndex &to)
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

void FluentTableView::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

} // namespace Fluent
