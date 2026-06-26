#pragma once

#include "Fluent/FluentExport.h"

#include <QMetaObject>
#include <QTableWidget>

class QMouseEvent;
class QEvent;
class QItemSelectionModel;
class QVariantAnimation;

namespace Fluent {

class FLUENT_EXPORT FluentTableWidget final : public QTableWidget
{
    Q_OBJECT
public:
    explicit FluentTableWidget(QWidget *parent = nullptr);
    FluentTableWidget(int rows, int columns, QWidget *parent = nullptr);

    QModelIndex hoverIndex() const;
    qreal hoverLevel() const;
    void setSelectionModel(QItemSelectionModel *selectionModel) override;

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void initFluent();
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


