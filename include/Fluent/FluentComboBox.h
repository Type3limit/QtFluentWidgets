#pragma once

#include <QComboBox>

class QPropertyAnimation;

namespace Fluent {

class FluentComboBox final : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
public:
    explicit FluentComboBox(QWidget *parent = nullptr);

    QSize sizeHint() const override;

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

protected:
    void changeEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void showPopup() override;

private:
    void applyTheme();

    qreal m_hoverLevel = 0.0;
    QPropertyAnimation *m_hoverAnim = nullptr;
};

} // namespace Fluent
