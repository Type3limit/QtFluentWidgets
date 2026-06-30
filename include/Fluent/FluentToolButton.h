#pragma once

#include "Fluent/FluentExport.h"

#include <QToolButton>
#include "Fluent/FluentQtCompat.h"

class QMouseEvent;
class QMenu;
class QRectF;
class QSize;
class QVariantAnimation;

namespace Fluent {

class FLUENT_EXPORT FluentToolButton final : public QToolButton
{
    Q_OBJECT
public:
    enum class Shape {
        Rounded,
        Pill,
        Circular
    };
    Q_ENUM(Shape)

    Q_PROPERTY(bool primary READ isPrimary WRITE setPrimary)
    Q_PROPERTY(Shape shape READ shape WRITE setShape)
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(qreal pressLevel READ pressLevel WRITE setPressLevel)

    explicit FluentToolButton(QWidget *parent = nullptr);
    explicit FluentToolButton(const QString &text, QWidget *parent = nullptr);

    bool isPrimary() const;
    void setPrimary(bool primary);

    Shape shape() const;
    void setShape(Shape shape);

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    qreal pressLevel() const;
    void setPressLevel(qreal value);

    void setMenu(QMenu *menu);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(FluentEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void applyTheme();
    void showFluentMenu();
    void startHoverAnimation(qreal endValue);
    void startPressAnimation(qreal endValue);
    qreal controlRadiusForRect(const QRectF &rect) const;

    bool m_primary = false;
    Shape m_shape = Shape::Rounded;
    bool m_menuPressArmed = false;
    qreal m_hoverLevel = 0.0;
    qreal m_pressLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_pressAnim = nullptr;
};

} // namespace Fluent
