#pragma once

#include <QTimeEdit>

class QFocusEvent;
class QPaintEvent;
class QResizeEvent;
class QVariantAnimation;
class QLineEdit;

namespace Fluent {

class FluentTimePicker final : public QTimeEdit
{
    Q_OBJECT
    Q_PROPERTY(qreal hoverLevel READ hoverLevel WRITE setHoverLevel)
    Q_PROPERTY(qreal focusLevel READ focusLevel WRITE setFocusLevel)
public:
    explicit FluentTimePicker(QWidget *parent = nullptr);

    qreal hoverLevel() const;
    void setHoverLevel(qreal value);

    qreal focusLevel() const;
    void setFocusLevel(qreal value);

protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void applyTheme();
    void startHoverAnimation(qreal endValue);
    void startFocusAnimation(qreal endValue);
    void startStepperAnimation(qreal endValue);
    void ensureEditor();

    enum class ButtonPart {
        None,
        Up,
        Down
    };

    ButtonPart hitTestButton(const QPoint &pos) const;

    qreal m_hoverLevel = 0.0;
    qreal m_focusLevel = 0.0;
    QVariantAnimation *m_hoverAnim = nullptr;
    QVariantAnimation *m_focusAnim = nullptr;

    qreal m_stepperLevel = 0.0;
    QVariantAnimation *m_stepperAnim = nullptr;

    ButtonPart m_hoverButton = ButtonPart::None;
    ButtonPart m_pressedButton = ButtonPart::None;

    QLineEdit *m_editor = nullptr;
};

} // namespace Fluent
