#include "Fluent/FluentTableWidget.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentTheme.h"
#include "FluentItemViewPaintSupport.h"
#include "FluentPaintSupport.h"
#include "FluentTableSupport.h"
#include "FluentViewPaletteSupport.h"

#include <QAbstractItemModel>
#include <QEvent>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <QVariantAnimation>

namespace Fluent {

using Detail::FluentHeaderView;
using Detail::FluentTableItemDelegate;

FluentTableWidget::FluentTableWidget(QWidget *parent)
    : QTableWidget(parent)
{
    initFluent();
}

FluentTableWidget::FluentTableWidget(int rows, int columns, QWidget *parent)
    : QTableWidget(rows, columns, parent)
{
    initFluent();
}

void FluentTableWidget::initFluent()
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
    setItemDelegate(new FluentTableItemDelegate(
        this,
        [this]() { return hoverIndex(); },
        [this]() { return hoverLevel(); }));

    setVerticalScrollBar(new FluentScrollBar(Qt::Vertical, this));
    setHorizontalScrollBar(new FluentScrollBar(Qt::Horizontal, this));

    m_hoverAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    connect(m_hoverAnim, &QVariantAnimation::valueChanged, this, [this](const QVariant &value) {
        m_hoverLevel = value.toReal();
        viewport()->update();
    });

    m_selAnim = new QVariantAnimation(this);
    FluentMotion::configure(m_selAnim, FluentMotionRole::Selection);
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
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentTableWidget::applyTheme);

    QTimer::singleShot(0, this, [this]() { hookSelectionModel(); });
}

QModelIndex FluentTableWidget::hoverIndex() const
{
    return m_hoverIndex;
}

qreal FluentTableWidget::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentTableWidget::changeEvent(QEvent *event)
{
    QTableWidget::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentTableWidget::paintEvent(QPaintEvent *event)
{
    {
        const QModelIndex cur = currentIndex();
        const bool paintAnim = (m_selAnim && m_selAnim->state() == QAbstractAnimation::Running && m_selRect.isValid() && m_selOpacity > 0.0);
        const bool paintStatic = (cur.isValid() && selectionModel() && selectionModel()->isSelected(cur));
        if (paintAnim || paintStatic) {
            const auto &colors = ThemeManager::instance().colors();
            const QRectF r = paintAnim ? m_selRect : selectionRectForIndex(cur);
            const qreal opacity = paintAnim ? qBound<qreal>(0.0, m_selOpacity, 1.0) : 1.0;
            if (Detail::canBeginWidgetPainter(viewport())) {
                QPainter p(viewport());
                p.setRenderHint(QPainter::Antialiasing, true);

                p.setPen(Qt::NoPen);
                p.setBrush(Detail::fluentItemSelectionFill(colors, opacity));
                p.drawRoundedRect(r, 4.0, 4.0);

                Detail::paintFluentItemSelectionIndicator(p, r, colors, opacity, 3.0);
            }
        }
    }

    QTableWidget::paintEvent(event);
}

void FluentTableWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QModelIndex index = indexAt(event->pos());
    if (index != m_hoverIndex) {
        m_hoverIndex = index;
        startHoverAnimation(index.isValid() ? 1.0 : 0.0);
    }
    QTableWidget::mouseMoveEvent(event);
}

void FluentTableWidget::leaveEvent(QEvent *event)
{
    startHoverAnimation(0.0);
    m_hoverIndex = QModelIndex();
    QTableWidget::leaveEvent(event);
}

void FluentTableWidget::applyTheme()
{
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    FluentMotion::configure(m_selAnim, FluentMotionRole::Selection);

    const auto &colors = ThemeManager::instance().colors();
    const QString next = Theme::tableViewStyle(colors);
    if (styleSheet() != next) {
        setStyleSheet(next);
    }
    Detail::applyFluentViewPalette(this, viewport(), colors);
    Detail::applyFluentViewPalette(horizontalHeader(), horizontalHeader() ? horizontalHeader()->viewport() : nullptr, colors);
}

void FluentTableWidget::hookSelectionModel()
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

QRectF FluentTableWidget::selectionRectForIndex(const QModelIndex &index) const
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

void FluentTableWidget::startSelectionAnimation(const QModelIndex &from, const QModelIndex &to)
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

void FluentTableWidget::startHoverAnimation(qreal endValue)
{
    m_hoverAnim->stop();
    if (m_hoverAnim->duration() <= 0) {
        m_hoverLevel = qBound<qreal>(0.0, endValue, 1.0);
        viewport()->update();
        return;
    }
    m_hoverAnim->setStartValue(m_hoverLevel);
    m_hoverAnim->setEndValue(endValue);
    m_hoverAnim->start();
}

} // namespace Fluent


