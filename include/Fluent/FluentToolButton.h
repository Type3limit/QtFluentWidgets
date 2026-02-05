#pragma once

#include <QToolButton>

class QMouseEvent;
class QVariantAnimation;

namespace Fluent {

class FluentToolButton final : public QToolButton
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(qreal pressLevel READ pressLevel WRITE setPressLevel)
public:
    explicit FluentToolButton(QWidget *parent = nullptr);
    explicit FluentToolButton(const QString &text, QWidget *parent = nullptr);

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    qreal pressLevel() const;
    void setPressLevel(qreal value);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void applyTheme();
    void startHoverAnimation(qreal endValue);
    void startPressAnimation(qreal endValue);

    qreal m_hoverLevel = 0.0;
    qreal m_pressLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_pressAnim = nullptr;
};

} // namespace Fluent
