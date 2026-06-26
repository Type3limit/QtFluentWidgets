#pragma once

#include "Fluent/FluentExport.h"

#include <QMetaObject>
#include <QTreeView>

class QMouseEvent;
class QEvent;
class QItemSelectionModel;
class QVariantAnimation;

namespace Fluent {

class FLUENT_EXPORT FluentTreeView final : public QTreeView
{
    Q_OBJECT
public:
    explicit FluentTreeView(QWidget *parent = nullptr);

    QModelIndex hoverIndex() const;
    qreal hoverLevel() const;
    void setModel(QAbstractItemModel *model) override;
    void setSelectionModel(QItemSelectionModel *selectionModel) override;
protected:
    void changeEvent(QEvent *event) override;

    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const override;

private:
    void applyTheme();
    void startHoverAnimation(qreal endValue);

    void hookSelectionModel();
    QRectF selectionRectForIndex(const QModelIndex &index) const;
    void startSelectionAnimation(const QModelIndex &from, const QModelIndex &to);

    QModelIndex m_hoverIndex;
    qreal m_hoverLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;

    QRectF m_selRect;
    QRectF m_selStartRect;
    QRectF m_selTargetRect;
    qreal m_selOpacity = 0.0;
    qreal m_selStartOpacity = 0.0;
    qreal m_selTargetOpacity = 0.0;
    QVariantAnimation *m_selAnim = nullptr;
    QMetaObject::Connection m_selectionConnection;
};

} // namespace Fluent
