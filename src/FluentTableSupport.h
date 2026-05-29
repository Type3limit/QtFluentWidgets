#pragma once

#include "Fluent/FluentTheme.h"
#include "FluentItemEditorSupport.h"
#include "FluentItemViewPaintSupport.h"
#include "FluentPaintSupport.h"

#include <QAbstractItemModel>
#include <QEvent>
#include <QHeaderView>
#include <QModelIndex>
#include <QPainter>
#include <QPainterPath>
#include <QStyledItemDelegate>
#include <QTableView>
#include <QWidget>
#include <QWindow>

#include <functional>

namespace Fluent::Detail {

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
        if (!canBeginWidgetPainter(viewport())) {
            return baseResult;
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

class FluentTableItemDelegate final : public QStyledItemDelegate
{
public:
    using HoverIndexGetter = std::function<QModelIndex()>;
    using HoverLevelGetter = std::function<qreal()>;

    FluentTableItemDelegate(QTableView *view,
                            HoverIndexGetter hoverIndexGetter,
                            HoverLevelGetter hoverLevelGetter)
        : QStyledItemDelegate(view)
        , m_view(view)
        , m_hoverIndexGetter(std::move(hoverIndexGetter))
        , m_hoverLevelGetter(std::move(hoverLevelGetter))
    {
    }

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        QStyleOptionViewItem opt(option);
        initStyleOption(&opt, index);
        opt.state &= ~QStyle::State_HasFocus;

        const auto &colors = ThemeManager::instance().colors();

        const QAbstractItemModel *model = index.model();
        const bool isFirst = (index.column() == 0);
        const bool isLast = (model && index.column() == model->columnCount(index.parent()) - 1);
        const bool isRowSelection = (m_view && m_view->selectionBehavior() == QAbstractItemView::SelectRows);

        QRectF bgRect = QRectF(opt.rect);
        bgRect.adjust(0, 1, 0, -1);

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing, true);

        QColor bgColor = Qt::transparent;
        const bool selected = opt.state.testFlag(QStyle::State_Selected);
        bool isCurrentRow = false;
        if (m_view) {
            const QModelIndex cur = m_view->currentIndex();
            if (cur.isValid()) {
                isCurrentRow = (cur.row() == index.row() && cur.parent() == index.parent());
            }
        }

        const QModelIndex hoverIdx = m_hoverIndexGetter ? m_hoverIndexGetter() : QModelIndex();
        const qreal hoverLvl = m_hoverLevelGetter ? m_hoverLevelGetter() : 0.0;

        if (selected && !isCurrentRow) {
            bgColor = fluentItemSelectionFill(colors, 0.86);
        } else if (isRowSelection && hoverIdx.isValid() && index.row() == hoverIdx.row()) {
            bgColor = fluentItemHoverFill(colors, hoverLvl);
        } else if (!isRowSelection && index == hoverIdx) {
            bgColor = fluentItemHoverFill(colors, hoverLvl);
        }

        if (bgColor.alpha() > 0) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(bgColor);

            if (isRowSelection) {
                const qreal r = 4.0;
                QPainterPath path;

                if (isFirst && isLast) {
                    path.addRoundedRect(bgRect.adjusted(2, 0, -2, 0), r, r);
                } else if (isFirst) {
                    QRectF lRect = bgRect.adjusted(2, 0, 0, 0);
                    path.setFillRule(Qt::WindingFill);
                    path.addRoundedRect(lRect, r, r);
                    QRectF fixRight = lRect;
                    fixRight.setLeft(fixRight.right() - r);
                    path.addRect(fixRight);
                } else if (isLast) {
                    QRectF rRect = bgRect.adjusted(0, 0, -2, 0);
                    path.setFillRule(Qt::WindingFill);
                    path.addRoundedRect(rRect, r, r);
                    QRectF fixLeft = rRect;
                    fixLeft.setWidth(r);
                    path.addRect(fixLeft);
                } else {
                    path.addRect(bgRect);
                }
                painter->drawPath(path);

            } else {
                painter->drawRoundedRect(bgRect.adjusted(2, 0, -2, 0), 4, 4);
            }

            opt.palette.setColor(QPalette::HighlightedText, opt.palette.color(QPalette::Text));
            opt.palette.setColor(QPalette::Highlight, Qt::transparent);
        }

        if (selected) {
            opt.palette.setColor(QPalette::HighlightedText, opt.palette.color(QPalette::Text));
            opt.palette.setColor(QPalette::Highlight, Qt::transparent);
        }

        painter->restore();

        QStyledItemDelegate::paint(painter, opt, index);
    }

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        return fluentizeTextEditor(QStyledItemDelegate::createEditor(parent, option, index), parent);
    }

    void setEditorData(QWidget *editor, const QModelIndex &index) const override
    {
        if (setFluentEditorData(editor, index)) {
            return;
        }
        QStyledItemDelegate::setEditorData(editor, index);
    }

    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const override
    {
        if (setFluentModelData(editor, model, index)) {
            return;
        }
        QStyledItemDelegate::setModelData(editor, model, index);
    }

    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const override
    {
        if (isFluentTextEditor(editor)) {
            editor->setGeometry(fluentEditorRect(option, 2, 1));
            return;
        }
        QStyledItemDelegate::updateEditorGeometry(editor, option, index);
    }

private:
    QTableView *m_view = nullptr;
    HoverIndexGetter m_hoverIndexGetter;
    HoverLevelGetter m_hoverLevelGetter;
};

} // namespace Fluent::Detail

