#include "Fluent/FluentListView.h"
#include "Fluent/FluentMotion.h"
#include "Fluent/FluentScrollBar.h"
#include "Fluent/FluentTheme.h"
#include "FluentPaintSupport.h"
#include "FluentItemEditorSupport.h"
#include "FluentItemViewPaintSupport.h"
#include "FluentViewPaletteSupport.h"

#include <QAbstractItemModel>
#include <QEvent>
#include <QItemSelectionModel>
#include <QMouseEvent>
#include <QPainter>
#include <QStyledItemDelegate>
#include <QTimer>
#include <QVariantAnimation>

namespace Fluent {

namespace {

class FluentListItemDelegate final : public QStyledItemDelegate
{
public:
    explicit FluentListItemDelegate(FluentListView *view)
        : QStyledItemDelegate(view)
        , m_view(view)
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        opt.state &= ~QStyle::State_HasFocus; // Remove dotted line

        const auto &colors = ThemeManager::instance().colors();
        // Adjust rect for list item padding - Fluent Style: Inner Padding 2px, Height 36-40px usually
        // We use the rect provided by layout.
        const QRectF rect = QRectF(opt.rect).adjusted(4, 2, -4, -2); // Give it some breathing room

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        // Background Painting
        QColor bgColor = Qt::transparent;

        const bool selected = opt.state.testFlag(QStyle::State_Selected);
        const bool isCurrent = (m_view && m_view->currentIndex().isValid() && index == m_view->currentIndex());

        if (selected && !isCurrent) {
            bgColor = Detail::fluentItemSelectionFill(colors, 0.86);
        } else if (m_view && index == m_view->hoverIndex()) {
             bgColor = Detail::fluentItemHoverFill(colors, m_view->hoverLevel());
        }

        if (bgColor.alpha() > 0) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(bgColor);
            painter->drawRoundedRect(rect, 4, 4);
        }
        
        // Keep text color stable for our light custom selection background.
        if (selected) {
            opt.palette.setColor(QPalette::HighlightedText, opt.palette.color(QPalette::Text));
            opt.palette.setColor(QPalette::Highlight, Qt::transparent);
        }

        painter->restore();
        
        // Ensure text color is correct for selection? 
        // Standard text color is fine for light selection background.
        // If selection was dark solid color, we'd need white text. 
        // Current design is "Subtle/Light" selection, so Text color remains standard.

        QStyledItemDelegate::paint(painter, opt, index);
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        return Detail::fluentizeTextEditor(QStyledItemDelegate::createEditor(parent, option, index), parent);
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        if (Detail::setFluentEditorData(editor, index)) {
            return;
        }
        QStyledItemDelegate::setEditorData(editor, index);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        if (Detail::setFluentModelData(editor, model, index)) {
            return;
        }
        QStyledItemDelegate::setModelData(editor, model, index);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (Detail::isFluentTextEditor(editor)) {
            editor->setGeometry(Detail::fluentEditorRect(option, 4, 2));
            return;
        }
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }

private:
    FluentListView *m_view = nullptr;
};

} // namespace

FluentListView::FluentListView(QWidget *parent)
    : QListView(parent)
{
    setMouseTracking(true);
    viewport()->setMouseTracking(true);
    setFrameShape(QFrame::NoFrame);
    setUniformItemSizes(true);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setItemDelegate(new FluentListItemDelegate(this));

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
    connect(&ThemeManager::instance(), &ThemeManager::themeChanged, this, &FluentListView::applyTheme);

    // Model may be set after construction.
    QTimer::singleShot(0, this, [this]() { hookSelectionModel(); });
}

QModelIndex FluentListView::hoverIndex() const
{
    return m_hoverIndex;
}

qreal FluentListView::hoverLevel() const
{
    return m_hoverLevel;
}

void FluentListView::changeEvent(QEvent *event)
{
    QListView::changeEvent(event);
    if (event->type() == QEvent::EnabledChange) {
        applyTheme();
    }
}

void FluentListView::setModel(QAbstractItemModel *model)
{
    QListView::setModel(model);
    hookSelectionModel();
}

void FluentListView::paintEvent(QPaintEvent *event)
{
    // Paint animated selection background first (so text stays crisp).
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

                Detail::paintFluentItemSelectionIndicator(p, r, colors, opacity);
            }
        }
    }

    QListView::paintEvent(event);
}

void FluentListView::mouseMoveEvent(QMouseEvent *event)
{
    const QModelIndex index = indexAt(event->pos());
    if (index != m_hoverIndex) {
        m_hoverIndex = index;
        startHoverAnimation(index.isValid() ? 1.0 : 0.0);
    }
    QListView::mouseMoveEvent(event);
}

void FluentListView::leaveEvent(QEvent *event)
{
    startHoverAnimation(0.0);
    m_hoverIndex = QModelIndex();
    QListView::leaveEvent(event);
}

void FluentListView::applyTheme()
{
    FluentMotion::configure(m_hoverAnim, FluentMotionRole::Hover);
    FluentMotion::configure(m_selAnim, FluentMotionRole::Selection);

    const auto &colors = ThemeManager::instance().colors();
    const QString next = Theme::listViewStyle(colors);
    if (styleSheet() != next) {
        setStyleSheet(next);
    }
    Detail::applyFluentViewPalette(this, viewport(), colors);
}

void FluentListView::hookSelectionModel()
{
    if (!selectionModel()) {
        return;
    }

    // Avoid duplicate connections.
    disconnect(selectionModel(), nullptr, this, nullptr);

    connect(selectionModel(), &QItemSelectionModel::currentChanged, this, [this](const QModelIndex &current, const QModelIndex &previous) {
        startSelectionAnimation(previous, current);
    });

    // Keep initial position in sync.
    const QModelIndex cur = currentIndex();
    m_selRect = selectionRectForIndex(cur);
    m_selStartRect = m_selRect;
    m_selTargetRect = m_selRect;
}

QRectF FluentListView::selectionRectForIndex(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QRectF();
    }
    QRect r = visualRect(index);
    if (!r.isValid()) {
        return QRectF();
    }
    // Match delegate padding.
    return QRectF(r).adjusted(4, 2, -4, -2);
}

void FluentListView::startSelectionAnimation(const QModelIndex &from, const QModelIndex &to)
{
    const bool animRunning = (m_selAnim && m_selAnim->state() == QAbstractAnimation::Running);
    const QRectF startRect = animRunning ? m_selRect : selectionRectForIndex(from);
    const qreal startOpacity = animRunning ? m_selOpacity : (startRect.isValid() ? 1.0 : 0.0);
    const QRectF targetRect = selectionRectForIndex(to);
    const bool targetValid = targetRect.isValid();

    // Fade out when selection clears.
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

    // Fade in when this is the first visible selection.
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

    // Move + subtle fade in during travel.
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

void FluentListView::startHoverAnimation(qreal endValue)
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
