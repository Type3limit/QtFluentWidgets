#pragma once

#include <QRadioButton>

class QPaintEvent;
class QVariantAnimation;

namespace Fluent {

class FluentRadioButton final : public QRadioButton
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(qreal focusLevel READ focusLevel WRITE setFocusLevel)
public:
    explicit FluentRadioButton(QWidget *parent = nullptr);
    explicit FluentRadioButton(const QString &text, QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    qreal focusLevel() const;
    void setFocusLevel(qreal value);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    void applyTheme();
    void startHoverAnimation(qreal endValue);
    void startFocusAnimation(qreal endValue);

    qreal m_hoverLevel = 0.0;
    qreal m_focusLevel = 0.0;
    qreal m_checkLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_focusAnim = nullptr;
    QVariantAnimation *m_checkAnim = nullptr;
};

} // namespace Fluent
