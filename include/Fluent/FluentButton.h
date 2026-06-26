#pragma once

#include "Fluent/FluentExport.h"

#include <QPushButton>
#include "Fluent/FluentQtCompat.h"

class QIcon;
class QMouseEvent;
class QVariantAnimation;

namespace Fluent {

class FLUENT_EXPORT FluentButton : public QPushButton
{
    Q_OBJECT
public:
    enum class IconPosition {
        Left,
        Right,
        Top,
        Bottom
    };
    Q_ENUM(IconPosition)

    Q_PROPERTY(IconPosition iconPosition READ iconPosition WRITE setIconPosition)
    Q_PROPERTY(int iconSpacing READ iconSpacing WRITE setIconSpacing)
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(qreal pressLevel READ pressLevel WRITE setPressLevel)

    explicit FluentButton(QWidget *parent = nullptr);
    explicit FluentButton(const QString &text, QWidget *parent = nullptr);
    FluentButton(const QIcon &icon, const QString &text, QWidget *parent = nullptr);

    bool isPrimary() const;
    void setPrimary(bool primary);

    IconPosition iconPosition() const;
    void setIconPosition(IconPosition position);

    int iconSpacing() const;
    void setIconSpacing(int spacing);

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    qreal pressLevel() const;
    void setPressLevel(qreal value);

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
    void initialize();
    void applyTheme();
    void startHoverAnimation(qreal endValue);
    void startPressAnimation(qreal endValue);

    bool m_primary = false;
    IconPosition m_iconPosition = IconPosition::Left;
    int m_iconSpacing = 8;
    qreal m_hoverLevel = 0.0;
    qreal m_pressLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_pressAnim = nullptr;
};

} // namespace Fluent
